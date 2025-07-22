#pragma once

#include "SessionManager.h"
#include "../utils/Logger.h"
#include <memory>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>
#include <condition_variable>

/**
 * @brief Класс для периодической очистки истекших сессий
 * 
 * SessionCleaner запускает отдельный поток, который с заданной периодичностью
 * проверяет и удаляет истекшие сессии через SessionManager.
 */
class SessionCleaner {
public:
    /**
     * @brief Создает новый очиститель сессий
     * @param sessionManager Указатель на менеджер сессий
     * @param sessionTimeout Таймаут сессий
     * @param logger Указатель на логгер
     * @param cleanupInterval Интервал между очистками (по умолчанию 5 секунд)
     */
    SessionCleaner(std::shared_ptr<SessionManager> sessionManager,
                  std::chrono::seconds sessionTimeout,
                  std::shared_ptr<Logger> logger,
                  std::chrono::seconds cleanupInterval = std::chrono::seconds(5));
    
    /**
     * @brief Деструктор, останавливает поток очистки
     */
    ~SessionCleaner();
    
    // Запрещаем копирование и перемещение
    SessionCleaner(const SessionCleaner&) = delete;
    SessionCleaner& operator=(const SessionCleaner&) = delete;
    SessionCleaner(SessionCleaner&&) = delete;
    SessionCleaner& operator=(SessionCleaner&&) = delete;
    
    /**
     * @brief Запускает очиститель сессий
     * @return true если очиститель успешно запущен, иначе false
     */
    bool start();
    
    /**
     * @brief Останавливает очиститель сессий
     */
    void stop();

private:
    /**
     * @brief Рабочий метод для потока очистки
     */
    void cleanerWorker();

    std::shared_ptr<SessionManager> _sessionManager;  // Менеджер сессий
    std::chrono::seconds _sessionTimeout;             // Таймаут сессий
    std::shared_ptr<Logger> _logger;                  // Логгер
    std::chrono::seconds _cleanupInterval;            // Интервал между очистками
    
    std::atomic<bool> _running{false};              // Флаг работы потока
    std::thread _cleanerThread;                       // Поток очистки
    
    std::mutex _mutex;                                // Мьютекс для условной переменной
    std::condition_variable _cv;                      // Условная переменная для быстрой остановки
};