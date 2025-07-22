#pragma once

#include <string>
#include <mutex>
#include <memory>

// Forward declarations для spdlog
namespace spdlog {
    class logger;
}

/**
 * @brief Уровни логирования
 */
enum class ClientLogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    CRITICAL = 4
};

/**
 * @brief Класс для логирования сообщений клиента
 */
class ClientLogger {
public:
    /**
     * @brief Создает логгер с указанным файлом и уровнем логирования
     * @param logFile Путь к файлу логов (если пустой, логирование только в консоль)
     * @param level Уровень логирования
     */
    explicit ClientLogger(const std::string& logFile = "", 
                         ClientLogLevel level = ClientLogLevel::INFO);
    
    /**
     * @brief Деструктор, закрывает логгер
     */
    ~ClientLogger();

    // Запрещаем копирование и перемещение
    ClientLogger(const ClientLogger&) = delete;
    ClientLogger& operator=(const ClientLogger&) = delete;
    ClientLogger(ClientLogger&&) = delete;
    ClientLogger& operator=(ClientLogger&&) = delete;

    /**
     * @brief Устанавливает уровень логирования
     * @param level Новый уровень логирования
     */
    void setLogLevel(ClientLogLevel level);
    
    /**
     * @brief Логирует сообщение с уровнем DEBUG
     * @param message Сообщение для логирования
     */
    void debug(const std::string& message);
    
    /**
     * @brief Логирует сообщение с уровнем INFO
     * @param message Сообщение для логирования
     */
    void info(const std::string& message);
    
    /**
     * @brief Логирует сообщение с уровнем WARN
     * @param message Сообщение для логирования
     */
    void warn(const std::string& message);
    
    /**
     * @brief Логирует сообщение с уровнем ERROR
     * @param message Сообщение для логирования
     */
    void error(const std::string& message);
    
    /**
     * @brief Логирует сообщение с уровнем CRITICAL
     * @param message Сообщение для логирования
     */
    void critical(const std::string& message);
    
    /**
     * @brief Логирует сообщение с указанным уровнем
     * @param level Уровень логирования
     * @param message Сообщение для логирования
     */
    void log(ClientLogLevel level, const std::string& message);

private:
    /**
     * @brief Преобразует уровень логирования в строку
     * @param level Уровень логирования
     * @return Строковое представление уровня логирования
     */
    static std::string levelToString(ClientLogLevel level) ;

    std::string _logFile;                        // Путь к файлу логов
    ClientLogLevel _logLevel;                    // Текущий уровень логирования
    mutable std::mutex _mutex;                   // Мьютекс для потокобезопасности
    bool _logToFile = false;                     // Флаг логирования в файл
    bool _isHealthy = true;                      // Флаг работоспособности логгера
    std::shared_ptr<spdlog::logger> _logger;     // Указатель на логгер spdlog
};