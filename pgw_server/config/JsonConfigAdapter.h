#pragma once

#include <string>
#include <vector>
#include <cstdint>

/**
 * @brief Структура с конфигурацией сервера
 * 
 * Содержит все параметры конфигурации сервера, загружаемые из JSON-файла
 */
struct ServerConfig {
    std::string udp_ip = "0.0.0.0";               // IP-адрес для UDP-сервера
    uint16_t udp_port = 9000;                     // Порт для UDP-сервера
    uint32_t session_timeout_sec = 30;            // Таймаут сессий в секундах
    uint32_t cleanup_interval_sec = 5;            // Интервал очистки сессий в секундах
    std::string cdr_file = "cdr.log";             // Путь к файлу CDR
    uint16_t http_port = 8080;                    // Порт для HTTP-сервера
    uint32_t graceful_shutdown_rate = 10;         // Скорость удаления сессий при завершении (сессий в секунду)
    uint32_t max_requests_per_minute = 100;       // Максимальное количество запросов в минуту
    std::string log_file = "pgw.log";             // Путь к файлу логов
    std::string log_level = "INFO";               // Уровень логирования
    std::vector<std::string> blacklist;           // Черный список IMSI
};

/**
 * @brief Адаптер для загрузки конфигурации из JSON-файла
 */
class JsonConfigAdapter {
public:
    /**
     * @brief Создает адаптер для загрузки конфигурации
     * @param configPath Путь к файлу конфигурации
     */
    explicit JsonConfigAdapter(std::string configPath = "config.json");
    ~JsonConfigAdapter() = default;
    
    // Запрещаем копирование и перемещение
    JsonConfigAdapter(const JsonConfigAdapter&) = delete;
    JsonConfigAdapter& operator=(const JsonConfigAdapter&) = delete;
    JsonConfigAdapter(JsonConfigAdapter&&) = delete;
    JsonConfigAdapter& operator=(JsonConfigAdapter&&) = delete;

    /**
     * @brief Загружает конфигурацию из файла
     * @return true если загрузка успешна, иначе false
     */
    bool load();
    
    /**
     * @brief Возвращает структуру конфигурации
     * @return Ссылка на структуру конфигурации
     */
    [[nodiscard]] const ServerConfig& getConfig() const;
    
    /**
     * @brief Проверяет валидность конфигурации
     * @return true если конфигурация валидна, иначе false
     */
    [[nodiscard]] bool isValid() const;
    
    /**
     * @brief Возвращает последнюю ошибку
     * @return Строка с описанием ошибки
     */
    [[nodiscard]] std::string getLastError() const;
    
    /**
     * @brief Возвращает строковое значение из конфигурации
     * @param key Ключ параметра
     * @param defaultValue Значение по умолчанию
     * @return Значение параметра или значение по умолчанию
     */
    [[nodiscard]] std::string getString(const std::string& key, const std::string& defaultValue = "") const;
    
    /**
     * @brief Возвращает целочисленное значение из конфигурации
     * @param key Ключ параметра
     * @param defaultValue Значение по умолчанию
     * @return Значение параметра или значение по умолчанию
     */
    [[nodiscard]] uint32_t getUint(const std::string& key, uint32_t defaultValue = 0) const;
    
    /**
     * @brief Возвращает массив строк из конфигурации
     * @param key Ключ параметра
     * @return Массив строк
     */
    [[nodiscard]] std::vector<std::string> getStringArray(const std::string& key) const;

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
     * @brief Устанавливает сообщение об ошибке
     * @param error Сообщение об ошибке
     */
    void setError(const std::string& error);

    std::string _configPath;    // Путь к файлу конфигурации
    ServerConfig _config;       // Структура конфигурации
    bool _isValid = false;      // Флаг валидности конфигурации
    std::string _lastError;     // Последнее сообщение об ошибке
};