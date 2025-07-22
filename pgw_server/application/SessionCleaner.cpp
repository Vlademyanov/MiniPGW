#include "SessionCleaner.h"
#include <utility>

SessionCleaner::SessionCleaner(std::shared_ptr<SessionManager> sessionManager,
                             std::chrono::seconds sessionTimeout,
                             std::shared_ptr<Logger> logger,
                             std::chrono::seconds cleanupInterval)
    : _sessionManager(std::move(sessionManager)),
      _sessionTimeout(sessionTimeout),
      _logger(std::move(logger)),
      _cleanupInterval(cleanupInterval) {
    
    if (!_sessionManager) throw std::invalid_argument("sessionManager cannot be null");
    if (!_logger) throw std::invalid_argument("logger cannot be null");
    if (_sessionTimeout.count() <= 0) throw std::invalid_argument("sessionTimeout must be positive");
    if (_cleanupInterval.count() <= 0) throw std::invalid_argument("cleanupInterval must be positive");
    
    _logger->info("SessionCleaner initialized with timeout: " + std::to_string(_sessionTimeout.count()) + 
                  "s, interval: " + std::to_string(_cleanupInterval.count()) + "s");
}

SessionCleaner::~SessionCleaner() {
    stop();
}

bool SessionCleaner::start() {
    if (_running.exchange(true)) {
        _logger->debug("SessionCleaner.start: Already running, ignoring request");
        return false;
    }
    
    _logger->info("Starting session cleanup service");
    _cleanerThread = std::thread(&SessionCleaner::cleanerWorker, this);
    return true;
}

void SessionCleaner::stop() {
    if (!_running.exchange(false)) {
        _logger->debug("SessionCleaner.stop: Not running, nothing to do");
        return;
    }
    
    _logger->info("Stopping session cleanup service");
    
    // Уведомляем поток очистки о необходимости завершения
    _cv.notify_all();
    
    if (_cleanerThread.joinable()) {
        _logger->debug("Waiting for session cleaner thread to join");
        _cleanerThread.join();
    }
    
    _logger->info("Session cleanup service stopped");
}

void SessionCleaner::cleanerWorker() {
    _logger->debug("Session cleaner thread started");
    
    while (_running) {
        try {
            // Очищаем истекшие сессии
            auto startTime = std::chrono::steady_clock::now();
            size_t removedCount = _sessionManager->cleanExpiredSessions(_sessionTimeout);
            
            if (removedCount > 0) {
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - startTime);
                
                _logger->info("Removed " + std::to_string(removedCount) + " expired sessions in " + 
                             std::to_string(duration.count()) + "ms");
            } else {
                _logger->debug("No expired sessions found during cleanup cycle");
            }
        } catch (const std::exception& e) {
            _logger->error("Session cleanup error: " + std::string(e.what()));
        }
        
        // Ждем до следующего цикла очистки или до сигнала остановки
        std::unique_lock<std::mutex> lock(_mutex);
        _logger->debug("Session cleaner waiting " + std::to_string(_cleanupInterval.count()) + 
                      "s until next cleanup cycle");
        _cv.wait_for(lock, _cleanupInterval, [this]() { return !_running; });
    }
    
    _logger->debug("Session cleaner thread terminated");
} 