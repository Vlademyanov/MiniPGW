#pragma once

#include "ClientConfig.h"
#include "ClientLogger.h"
#include <string>
#include <memory>
#include <cstdint>
#include <utility>

/**
 * @brief Основной класс клиента для взаимодействия с PGW сервером
 */
class PgwClient {
public:
    /**
     * @brief Создает клиент с указанным путем к конфигурации
     * @param configPath Путь к файлу конфигурации
     */
    explicit PgwClient(const std::string& configPath = "client_config.json");
    
    /**
     * @brief Деструктор, закрывает сокет
     */
    ~PgwClient();

    // Запрещаем копирование и перемещение
    PgwClient(const PgwClient&) = delete;
    PgwClient& operator=(const PgwClient&) = delete;
    PgwClient(PgwClient&&) = delete;
    PgwClient& operator=(PgwClient&&) = delete;

    /**
     * @brief Инициализирует клиент
     * @return true если инициализация успешна, иначе false
     */
    bool initialize();
    
    /**
     * @brief Отправляет запрос на создание сессии с указанным IMSI
     * @param imsi IMSI абонента (15 цифр)
     * @return Пара (успех операции, ответ сервера)
     */
    std::pair<bool, std::string> sendRequest(const std::string& imsi);

private:
    /**
     * @brief Настраивает UDP сокет
     * @return true если сокет успешно создан, иначе false
     */
    bool setupUdpSocket();
    
    /**
     * @brief Закрывает UDP сокет
     */
    void closeSocket();
    
    /**
     * @brief Кодирует IMSI в BCD формат
     * @param imsi IMSI абонента (15 цифр)
     * @return Пара (успех кодирования, закодированная строка)
     */
    std::pair<bool, std::string> encodeImsiToBcd(const std::string& imsi);
    
    /**
     * @brief Отправляет UDP пакет с закодированным IMSI
     * @param bcdImsi IMSI в BCD формате
     * @return true если пакет успешно отправлен, иначе false
     */
    bool sendUdpPacket(const std::string& bcdImsi);
    
    /**
     * @brief Получает ответ от сервера
     * @return Пара (успех получения, ответ)
     */
    std::pair<bool, std::string> receiveResponse();

    std::unique_ptr<ClientConfig> _config;       // Конфигурация клиента
    std::unique_ptr<ClientLogger> _logger;       // Логгер
    int _socket = -1;                            // UDP сокет

    static constexpr size_t MAX_RESPONSE_SIZE = 256;  // Максимальный размер ответа
};