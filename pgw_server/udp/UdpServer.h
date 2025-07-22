#pragma once

#include "../application/SessionManager.h"
#include "../utils/Logger.h"
#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <netinet/in.h>

/**
 * @brief UDP-сервер для обработки запросов клиентов
 * 
 * Предоставляет UDP интерфейс для взаимодействия с клиентами Mini-PGW.
 * Отвечает за прием UDP-запросов на создание сессий абонентов,
 * их обработку и отправку ответов клиентам.
 * Использует epoll для обработки запросов.
 */
class UdpServer {
public:
    /**
     * @brief Создает новый UDP-сервер
     * @param ip IP-адрес для прослушивания
     * @param port Порт для прослушивания
     * @param sessionManager Указатель на менеджер сессий
     * @param logger Указатель на логгер
     */
    UdpServer(std::string  ip,
              uint16_t port,
              std::shared_ptr<SessionManager> sessionManager,
              std::shared_ptr<Logger> logger);
    
    /**
     * @brief Деструктор, останавливает сервер
     */
    ~UdpServer();
    
    // Запрещаем копирование и перемещение
    UdpServer(const UdpServer&) = delete;
    UdpServer& operator=(const UdpServer&) = delete;
    UdpServer(UdpServer&&) = delete;
    UdpServer& operator=(UdpServer&&) = delete;

    /**
     * @brief Запускает UDP-сервер
     * @return true если сервер успешно запущен, иначе false
     */
    bool start();
    
    /**
     * @brief Останавливает UDP-сервер
     */
    void stop();
    
    /**
     * @brief Проверяет, запущен ли сервер
     * @return true если сервер запущен, иначе false
     */
    [[nodiscard]] bool isRunning() const;

private:
    /**
     * @brief Основной цикл сервера
     */
    void serverLoop();
    
    /**
     * @brief Обрабатывает входящий UDP-пакет
     * @param buffer Буфер с данными
     * @param length Длина данных
     * @param clientAddr Адрес клиента
     */
    void handleIncomingPacket(const char* buffer, size_t length,
                             const struct sockaddr_in& clientAddr) const;
    
    /**
     * @brief Извлекает IMSI из BCD-формата
     * @param buffer Буфер с данными
     * @param length Длина данных
     * @return IMSI в виде строки или пустая строка в случае ошибки
     */
    [[nodiscard]] std::string extractImsiFromBcd(const char* buffer, size_t length) const;
    
    /**
     * @brief Отправляет ответ клиенту
     * @param response Ответ для отправки
     * @param clientAddr Адрес клиента
     */
    void sendResponse(const std::string& response,
                     const struct sockaddr_in& clientAddr) const;
    
    /**
     * @brief Настраивает неблокирующий сокет и epoll
     * @return true если настройка успешна, иначе false
     */
    bool setupEpollSocket();
    
    /**
     * @brief Закрывает сокет и освобождает ресурсы epoll
     */
    void cleanupResources();

    std::string _ip;                // IP-адрес для прослушивания
    uint16_t _port;                 // Порт для прослушивания
    int _socket = -1;               // Дескриптор сокета
    int _epollFd = -1;              // Дескриптор epoll
    std::atomic<bool> _running{false}; // Флаг работы сервера
    std::thread _serverThread;      // Поток сервера

    std::shared_ptr<SessionManager> _sessionManager; // Менеджер сессий
    std::shared_ptr<Logger> _logger;                 // Логгер
};