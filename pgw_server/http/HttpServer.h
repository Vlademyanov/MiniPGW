#pragma once

#include <SessionManager.h>
#include <GracefulShutdownManager.h>
#include <Logger.h>
#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <functional>

// Предварительное объявление для избежания включения заголовка
namespace httplib {
    class Server;
}

/**
 * @brief HTTP-сервер для управления и мониторинга системы
 * 
 * Предоставляет REST API для проверки статуса сессий и управления системой:
 * - GET /check_subscriber?imsi=XXX - проверка статуса абонента
 * - GET /stop - инициирование плавного завершения работы
 * - GET /health - проверка работоспособности сервера
 */
class HttpServer {
public:
    /**
     * @brief Тип функции обратного вызова для обработки команды остановки
     */
    using StopCallback = std::function<void()>;

    /**
     * @brief Создает новый HTTP-сервер
     * @param ip IP-адрес для прослушивания
     * @param port Порт для прослушивания
     * @param sessionManager Указатель на менеджер сессий
     * @param shutdownManager Указатель на менеджер плавного завершения
     * @param logger Указатель на логгер
     * @param onStopRequested Функция обратного вызова для обработки команды остановки
     */
    HttpServer(const std::string& ip,
               uint16_t port,
               std::shared_ptr<SessionManager> sessionManager,
               std::shared_ptr<GracefulShutdownManager> shutdownManager,
               std::shared_ptr<Logger> logger,
               StopCallback onStopRequested = nullptr);
    
    /**
     * @brief Деструктор, останавливает сервер
     */
    ~HttpServer();

    // Запрещаем копирование и перемещение
    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;
    HttpServer(HttpServer&&) = delete;
    HttpServer& operator=(HttpServer&&) = delete;

    /**
     * @brief Запускает HTTP-сервер
     * @return true если сервер успешно запущен, иначе false
     */
    bool start();
    
    /**
     * @brief Останавливает HTTP-сервер
     */
    void stop();
    
    /**
     * @brief Проверяет, запущен ли сервер
     * @return true если сервер запущен, иначе false
     */
    [[nodiscard]] bool isRunning() const;
    
    /**
     * @brief Возвращает порт, на котором запущен сервер
     * @return Номер порта
     */
    [[nodiscard]] uint16_t getPort() const { return _port; }

private:
    /**
     * @brief Настраивает маршруты HTTP-сервера
     */
    void setupRoutes() const;
    
    /**
     * @brief Обрабатывает запрос проверки статуса абонента
     * @param imsi IMSI абонента
     * @param response Строка для записи ответа
     */
    void handleCheckSubscriber(const std::string& imsi, std::string& response) const;
    
    /**
     * @brief Обрабатывает команду остановки
     * @param response Строка для записи ответа
     */
    void handleStopCommand(std::string& response) const;

    std::string _ip;                                 // IP-адрес для прослушивания
    uint16_t _port;                                  // Порт для прослушивания
    std::atomic<bool> _running{false};             // Флаг работы сервера
    std::thread _serverThread;                       // Поток сервера
    StopCallback _onStopRequested;                   // Функция обратного вызова для обработки команды остановки

    std::shared_ptr<SessionManager> _sessionManager;           // Менеджер сессий
    std::shared_ptr<GracefulShutdownManager> _shutdownManager; // Менеджер плавного завершения
    std::shared_ptr<Logger> _logger;                           // Логгер

    // Используем unique_ptr с кастомным делитером для управления экземпляром httplib::Server
    std::unique_ptr<void, std::function<void(void*)>> _httpLibServer;
};
