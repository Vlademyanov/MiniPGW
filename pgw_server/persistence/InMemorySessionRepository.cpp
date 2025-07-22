#include "InMemorySessionRepository.h"

#include <chrono>
#include <ranges>
#include <utility>

InMemorySessionRepository::InMemorySessionRepository(std::shared_ptr<Logger> logger)
    : _logger(std::move(logger))
{
    if (_logger) {
        _logger->debug("InMemorySessionRepository initialized");
    }
}

bool InMemorySessionRepository::addSession(const Session& session) {
    std::lock_guard<std::mutex> lock(_mutex);
    
    const std::string& imsi = session.getImsi();
    
    // Проверяем, существует ли уже сессия с таким IMSI
    if (_sessions.contains(imsi)) {
        if (_logger) {
            _logger->debug("Session add failed: IMSI " + imsi + " already exists");
        }
        return false;
    }
    
    // Используем emplace для эффективного добавления
    auto [it, inserted] = _sessions.emplace(imsi, session);
    
    if (_logger) {
        if (inserted) {
            _logger->debug("Session added for IMSI: " + imsi + 
                          " (total sessions: " + std::to_string(_sessions.size()) + ")");
        } else {
            _logger->warn("Failed to add session for IMSI: " + imsi);
        }
    }
    
    return inserted;
}

bool InMemorySessionRepository::removeSession(const std::string& imsi) {
    std::lock_guard<std::mutex> lock(_mutex);
    
    auto count = _sessions.erase(imsi);
    
    if (_logger) {
        if (count > 0) {
            _logger->debug("Session removed for IMSI: " + imsi + 
                          " (remaining sessions: " + std::to_string(_sessions.size()) + ")");
        } else {
            _logger->debug("Session removal failed: IMSI " + imsi + " not found");
        }
    }
    
    return count > 0;
}

bool InMemorySessionRepository::sessionExists(const std::string& imsi) const {
    std::lock_guard<std::mutex> lock(_mutex);
    bool exists = _sessions.contains(imsi);
    
    if (_logger && _logger->getLogLevel() <= LogLevel::LOG_DEBUG) {
        _logger->debug("Session existence check for IMSI " + imsi + ": " + 
                      (exists ? "exists" : "not found"));
    }
    
    return exists;
}

std::vector<std::string> InMemorySessionRepository::getAllImsis() const {
    std::lock_guard<std::mutex> lock(_mutex);
    
    std::vector<std::string> imsis;
    imsis.reserve(_sessions.size());
    
    for (const auto &key: _sessions | std::views::keys) {
        imsis.push_back(key);
    }

    if (_logger) {
        _logger->debug("Retrieved " + std::to_string(imsis.size()) + " IMSIs from repository");
    }
    
    return imsis;
}

size_t InMemorySessionRepository::getSessionCount() const {
    std::lock_guard<std::mutex> lock(_mutex);
    auto count = _sessions.size();
    
    if (_logger && _logger->getLogLevel() <= LogLevel::LOG_DEBUG) {
        _logger->debug("Current session count: " + std::to_string(count));
    }
    
    return count;
}

void InMemorySessionRepository::clear() {
    std::lock_guard<std::mutex> lock(_mutex);
    
    size_t count = _sessions.size();
    _sessions.clear();
    
    if (_logger) {
        _logger->info("Repository cleared, removed " + std::to_string(count) + " sessions");
    }
}

std::vector<Session> InMemorySessionRepository::getExpiredSessions(uint32_t timeoutSeconds) const {
    std::lock_guard<std::mutex> lock(_mutex);
    
    std::vector<Session> expiredSessions;
    const auto timeout = std::chrono::seconds(timeoutSeconds);
    
    for (const auto &val: _sessions | std::views::values) {
        if (const auto &session = val; session.isExpired(timeout)) {
            expiredSessions.push_back(session);
        }
    }
    
    if (_logger) {
        if (!expiredSessions.empty()) {
            _logger->debug("Found " + std::to_string(expiredSessions.size()) + 
                          " expired sessions (timeout: " + std::to_string(timeoutSeconds) + "s)");
        } else {
            _logger->debug("No expired sessions found (timeout: " + 
                          std::to_string(timeoutSeconds) + "s)");
        }
    }
    
    return expiredSessions;
}