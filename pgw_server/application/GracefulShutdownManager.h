#pragma once

#include <SessionManager.h>
#include <Logger.h>
#include <atomic>
#include <memory>
#include <thread>
#include <chrono>
#include <condition_variable>
#include <mutex>

/**
 * @brief Класс для плавного завершения работы приложения
 * 
 * Отвечает за контролируемое удаление сессий
 * с заданной скоростью при завершении работы приложения.
 */
class GracefulShutdownManager {
public:
    /**
     * @brief Создает менеджер плавного завершения работы
     * @param sessionManager Указатель на менеджер сессий
     * @param shutdownRate Скорость удаления сессий (сессий в секунду)
     * @param logger Указатель на логгер
     */
    GracefulShutdownManager(std::shared_ptr<SessionManager> sessionManager,
                           uint32_t shutdownRate,
                           std::shared_ptr<Logger> logger);
    
    /**
     * @brief Деструктор, ожидает завершения потока
     */
    ~GracefulShutdownManager();
    
    // Запрещаем копирование и перемещение
    GracefulShutdownManager(const GracefulShutdownManager&) = delete;
    GracefulShutdownManager& operator=(const GracefulShutdownManager&) = delete;
    GracefulShutdownManager(GracefulShutdownManager&&) = delete;
    GracefulShutdownManager& operator=(GracefulShutdownManager&&) = delete;

    /**
     * @brief Инициирует процесс плавного завершения работы
     * @return true если процесс успешно инициирован, false если уже запущен
     */
    bool initiateShutdown();
    
    /**
     * @brief Ожидает завершения процесса плавного завершения
     * @param timeout Максимальное время ожидания (0 - ждать бесконечно)
     * @return true если завершение произошло, false если истек таймаут
     */
    bool waitForCompletion();

    bool isShutdownInProgress() const;
    bool isShutdownComplete() const;
    void stop();

private:
    /**
     * @brief Рабочий метод для потока завершения
     */
    void shutdownWorker();

    std::shared_ptr<SessionManager> _sessionManager;  // Менеджер сессий
    uint32_t _shutdownRate;                           // Скорость удаления сессий (сессий в секунду)
    std::shared_ptr<Logger> _logger;                  // Логгер
    
    std::atomic<bool> _shutdownInProgress{false};   // Флаг процесса завершения
    std::atomic<bool> _shutdownComplete{false};     // Флаг завершения процесса
    std::atomic<bool> _stopRequested{false};
    std::thread _shutdownThread;                      // Поток завершения
    
    std::mutex _shutdownMutex;                        // Мьютекс для условной переменной
    std::condition_variable _shutdownCondition;       // Условная переменная для ожидания завершения
};