#include <GracefulShutdownManager.h>
#include <utility>
#include <thread>
#include <algorithm>

GracefulShutdownManager::GracefulShutdownManager(std::shared_ptr<SessionManager> sessionManager,
                                               uint32_t shutdownRate,
    std::shared_ptr<Logger> logger)
    : _sessionManager(std::move(sessionManager)),
      _shutdownRate(shutdownRate),
      _logger(std::move(logger)) {
    
    if (!_sessionManager) throw std::invalid_argument("sessionManager cannot be null");
    if (!_logger) throw std::invalid_argument("logger cannot be null");
    if (_shutdownRate == 0) throw std::invalid_argument("shutdownRate must be positive");
    
    _logger->info("GracefulShutdownManager initialized with rate: " + std::to_string(_shutdownRate) + " sessions/sec");
}

GracefulShutdownManager::~GracefulShutdownManager() {
    // Ждем завершения потока, если он запущен
    if (_shutdownThread.joinable()) {
        _logger->debug("~GracefulShutdownManager: Waiting for shutdown thread to complete");
        _shutdownThread.join();
    }
}

bool GracefulShutdownManager::initiateShutdown() {
    // Если процесс уже запущен, возвращаем false
    if (_shutdownInProgress.exchange(true)) {
        _logger->debug("initiateShutdown: Shutdown already in progress, ignoring request");
        return false;
    }
    
    _logger->info("Initiating graceful shutdown process");
    
    // Запускаем поток для плавного завершения
    _shutdownThread = std::thread(&GracefulShutdownManager::shutdownWorker, this);
    
    return true;
}

bool GracefulShutdownManager::isShutdownInProgress() const {
    return _shutdownInProgress;
}

bool GracefulShutdownManager::isShutdownComplete() const {
    return _shutdownComplete;
}

void GracefulShutdownManager::stop() {
    _stopRequested = true;
    _shutdownCondition.notify_all(); // мгновенно пробудить поток
    if (_shutdownThread.joinable()) {
        _logger->debug("GracefulShutdownManager::stop: Waiting for shutdown thread to complete");
        _shutdownThread.join();
    }
}

bool GracefulShutdownManager::waitForCompletion() {
    // Если процесс не запущен или уже завершен, сразу возвращаемся
    if (!_shutdownInProgress || _shutdownComplete) {
        _logger->debug("waitForCompletion: No shutdown in progress or already complete");
        return true;
    }

    std::unique_lock<std::mutex> lock(_shutdownMutex);
        // Ждем без таймаута
        _shutdownCondition.wait(lock, [this] { return _shutdownComplete.load(); });
        _logger->debug("Shutdown completion wait finished (no timeout)");
        return true;
}

void GracefulShutdownManager::shutdownWorker() {
    _logger->debug("Shutdown worker thread started");
    
    try {
        // Получаем список всех активных IMSI
        auto imsis = _sessionManager->getAllActiveImsis();
        size_t totalSessions = imsis.size();
        
        if (totalSessions == 0) {
            _logger->info("No active sessions to shutdown, process complete");
            _shutdownComplete.store(true);
            _shutdownCondition.notify_all();
            return;
        }
        
        _logger->info("Beginning graceful shutdown: " + std::to_string(totalSessions) + 
                      " active sessions at rate " + std::to_string(_shutdownRate) + " sessions/sec");
        
        // Рассчитываем интервал между удалениями сессий
        std::chrono::milliseconds interval(1000 / _shutdownRate);
        
        // Удаляем сессии с заданной скоростью
        size_t removedCount = 0;
        auto startTime = std::chrono::steady_clock::now();
        std::mutex localMutex;
        
        for (const auto& imsi : imsis) {
            if (_stopRequested) {
                _logger->info("Graceful shutdown interrupted by stop request");
                break;
            }
            // Проверяем, что сессия все еще существует
            if (_sessionManager->isSessionActive(imsi)) {
                // Удаляем сессию
                if (_sessionManager->removeSession(imsi, "graceful_shutdown")) {
                    removedCount++;
            
                    // Логируем прогресс на основе скорости удаления и общего количества
                    size_t logInterval = std::max<size_t>(totalSessions / 10, _shutdownRate);
                    if (removedCount % logInterval == 0 || removedCount == totalSessions) {
                        size_t percent = (removedCount * 100) / totalSessions;
                        _logger->debug("Shutdown progress: " + std::to_string(removedCount) + "/" + 
                                      std::to_string(totalSessions) + " (" + std::to_string(percent) + "%)");
                    }
                } else {
                    _logger->warn("Failed to remove session for IMSI: " + imsi + " during shutdown");
                }
            } else {
                _logger->debug("Session for IMSI: " + imsi + " no longer active, skipping");
            }
            // Если сессий больше не осталось — завершить shutdown немедленно
            if (_sessionManager->getActiveSessionsCount() == 0) {
                _logger->info("All sessions removed, shutdown complete early");
                break;
            }
            std::unique_lock<std::mutex> lock(localMutex);
            _shutdownCondition.wait_for(lock, interval, [this]() { return _stopRequested.load(); });
        }
        
        // Проверяем, остались ли еще активные сессии
        size_t remainingSessions = _sessionManager->getActiveSessionsCount();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime);
        
        if (remainingSessions > 0) {
            _logger->warn("Shutdown incomplete: " + std::to_string(remainingSessions) + 
                         " sessions remain active after shutdown attempt");
        } else {
            _logger->info("Shutdown complete: All " + std::to_string(removedCount) + 
                         " sessions successfully removed in " + std::to_string(duration.count()) + "ms");
        }
        
    } catch (const std::exception& e) {
        _logger->critical("Critical error during graceful shutdown: " + std::string(e.what()));
    }
    
    // Устанавливаем флаг завершения и уведомляем ожидающие потоки
    _shutdownComplete.store(true);
    _shutdownCondition.notify_all();
    _logger->info("Graceful shutdown process completed");
} 