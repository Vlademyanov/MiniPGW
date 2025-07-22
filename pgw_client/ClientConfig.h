#pragma once

#include <string>
#include <cstdint>

/**
 * @brief Структура с конфигурацией клиента
 */
struct ClientConfiguration {
    std::string server_ip = "127.0.0.1";         // IP-адрес сервера
    uint16_t server_port = 9000;                 // Порт сервера
    std::string log_file = "client.log";         // Путь к файлу логов
    std::string log_level = "INFO";              // Уровень логирования
    uint32_t receive_timeout_ms = 5000;          // Таймаут ожидания ответа в миллисекундах
};

/**
 * @brief Класс для работы с конфигурацией клиента
 */
class ClientConfig {
public:
    /**
     * @brief Создает объект конфигурации
     * @param configPath Путь к файлу конфигурации
     */
    explicit ClientConfig(const std::string& configPath);
    
    /**
     * @brief Деструктор
     */
    ~ClientConfig() = default;

    // Запрещаем копирование и перемещение
    ClientConfig(const ClientConfig&) = delete;
    ClientConfig& operator=(const ClientConfig&) = delete;
    ClientConfig(ClientConfig&&) = delete;
    ClientConfig& operator=(ClientConfig&&) = delete;

    /**
     * @brief Загружает конфигурацию из файла
     * @return true если загрузка успешна, иначе false
     */
    bool loadConfig();
    
    /**
     * @brief Возвращает текущую конфигурацию
     * @return Структура с конфигурацией
     */
    const ClientConfiguration& getConfig() const;

private:
    /**
     * @brief Устанавливает значения по умолчанию
     */
    void setDefaults();
    
    /**
     * @brief Проверяет валидность конфигурации
     * @return true если конфигурация валидна, иначе false
     */
    bool validateConfig();
    
    /**
     * @brief Парсит строковое значение из JSON
     * @param key Ключ
     * @param json JSON строка
     * @param value Переменная для записи результата
     */
    void parseJsonValue(const std::string& key, const std::string& json, std::string& value);
    
    /**
     * @brief Парсит uint16_t значение из JSON
     * @param key Ключ
     * @param json JSON строка
     * @param value Переменная для записи результата
     */
    void parseJsonUint16(const std::string& key, const std::string& json, uint16_t& value);
    
    /**
     * @brief Парсит uint32_t значение из JSON
     * @param key Ключ
     * @param json JSON строка
     * @param value Переменная для записи результата
     */
    void parseJsonUint32(const std::string& key, const std::string& json, uint32_t& value);

    std::string _configPath;                     // Путь к файлу конфигурации
    ClientConfiguration _config;                 // Текущая конфигурация
    bool _isValid = false;                       // Флаг валидности конфигурации
    std::string _lastError;                      // Текст последней ошибки
};