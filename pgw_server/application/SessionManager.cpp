#include "SessionManager.h"
#include <utility>
#include <random>

SessionManager::SessionManager(std::shared_ptr<ISessionRepository> sessionRepo,
                             std::shared_ptr<ICdrRepository> cdrRepo,
                             std::shared_ptr<Blacklist> blacklist,
                             std::shared_ptr<RateLimiter> rateLimiter,
                             std::shared_ptr<Logger> logger)
    : _sessionRepo(std::move(sessionRepo)),
      _cdrRepo(std::move(cdrRepo)),
      _blacklist(std::move(blacklist)),
      _rateLimiter(std::move(rateLimiter)),
      _logger(std::move(logger)) {
    
    if (!_sessionRepo) throw std::invalid_argument("sessionRepo cannot be null");
    if (!_cdrRepo) throw std::invalid_argument("cdrRepo cannot be null");
    if (!_blacklist) throw std::invalid_argument("blacklist cannot be null");
    if (!_rateLimiter) throw std::invalid_argument("rateLimiter cannot be null");
    if (!_logger) throw std::invalid_argument("logger cannot be null");
    
    _logger->info("Session manager service initialized");
}

SessionResult SessionManager::createSession(const std::string& imsi) const {
    _logger->debug("Processing session creation request for IMSI: " + imsi);
    
    // Проверка черного списка
    if (isImsiBlacklisted(imsi)) {
        _logger->info("Session rejected: IMSI " + imsi + " is blacklisted");
        logCdr(imsi, "rejected_blacklist");
        return SessionResult::REJECTED;
    }
    
    // Проверка ограничения скорости
    if (!_rateLimiter->allowRequest(imsi)) {
        _logger->warn("Session rejected: Rate limit exceeded for IMSI " + imsi);
        logCdr(imsi, "rejected_rate_limit");
        return SessionResult::REJECTED;
    }
    
    // Проверка существования сессии
    if (_sessionRepo->sessionExists(imsi)) {
        _logger->debug("Session already exists for IMSI: " + imsi + ", returning CREATED");
        return SessionResult::CREATED; // Сессия уже существует, считаем что создали
    }
    
    try {
        // Создание новой сессии с логгером
        Session session(imsi, _logger);
        
        // Сохранение сессии в репозитории
        if (_sessionRepo->addSession(session)) {
            _logger->info("New session successfully created for IMSI: " + imsi);
            logCdr(imsi, "create");
            return SessionResult::CREATED;
        } else {
            _logger->error("Repository error: Failed to add session for IMSI: " + imsi);
            return SessionResult::ERROR;
        }
    } catch (const std::exception& e) {
        _logger->error("Session creation failed for IMSI " + imsi + ": " + e.what());
        return SessionResult::ERROR;
    }
}

bool SessionManager::isSessionActive(const std::string& imsi) const {
    bool active = _sessionRepo->sessionExists(imsi);
    _logger->debug("Session status check for IMSI " + imsi + ": " + (active ? "active" : "not active"));
    return active;
}

bool SessionManager::removeSession(const std::string& imsi, const std::string& action) const {
    _logger->debug("Removing session for IMSI: " + imsi + " (reason: " + action + ")");
    
    if (!_sessionRepo->sessionExists(imsi)) {
        _logger->debug("Session not found for IMSI: " + imsi + ", nothing to remove");
        return false;
    }
    
    if (_sessionRepo->removeSession(imsi)) {
        logCdr(imsi, action);
        _logger->info("Session for IMSI: " + imsi + " successfully removed (" + action + ")");
        return true;
    } else {
        _logger->error("Repository error: Failed to remove session for IMSI: " + imsi);
        return false;
    }
}

size_t SessionManager::cleanExpiredSessions(std::chrono::seconds timeout) const {
    _logger->debug("Starting expired sessions cleanup (timeout: " + std::to_string(timeout.count()) + "s)");
    
    // Получаем все истекшие сессии
    auto expiredSessions = _sessionRepo->getExpiredSessions(timeout.count());
    
    if (expiredSessions.empty()) {
        _logger->debug("No expired sessions found");
        return 0;
    }
    
    _logger->debug("Found " + std::to_string(expiredSessions.size()) + " expired sessions to clean");
    
    size_t removedCount = 0;
    for (const auto& session : expiredSessions) {
        const auto& imsi = session.getImsi();
        if (_sessionRepo->removeSession(imsi)) {
            logCdr(imsi, "timeout");
            removedCount++;
        } else {
            _logger->warn("Failed to remove expired session for IMSI: " + imsi);
        }
    }
    
    if (removedCount > 0) {
        _logger->info("Cleaned " + std::to_string(removedCount) + "/" + 
                     std::to_string(expiredSessions.size()) + " expired sessions");
    }
    
    return removedCount;
}

size_t SessionManager::getActiveSessionsCount() const {
    size_t count = _sessionRepo->getSessionCount();
    _logger->debug("Current active sessions count: " + std::to_string(count));
    return count;
}

std::vector<std::string> SessionManager::getAllActiveImsis() const {
    auto imsis = _sessionRepo->getAllImsis();
    _logger->debug("Retrieved " + std::to_string(imsis.size()) + " active IMSIs");
    return imsis;
}

void SessionManager::logCdr(const std::string& imsi, const std::string& action) const {
    try {
        _logger->debug("Writing CDR record: IMSI=" + imsi + ", action=" + action);
        if (!_cdrRepo->writeCdr(imsi, action)) {
            _logger->error("CDR write failed for IMSI " + imsi + ": repository error");
        }
    } catch (const std::exception& e) {
        _logger->critical("CDR system error for IMSI " + imsi + ": " + e.what());
    }
}

bool SessionManager::isImsiBlacklisted(const std::string& imsi) const {
    bool result = _blacklist->isBlacklisted(imsi);
    _logger->debug("Blacklist check for IMSI " + imsi + ": " + (result ? "blacklisted" : "not blacklisted"));
    return result;
} 