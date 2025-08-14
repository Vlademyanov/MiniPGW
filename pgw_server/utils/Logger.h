#pragma once

#include <string>
#include <fstream>
#include <mutex>

/**
 * @brief Уровни логирования
 */
enum class LogLevel {
    LOG_DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    CRITICAL = 4
};

/**
 * @brief Класс для логирования сообщений
 * 
 * Logger обеспечивает логирование сообщений разных уровней
 * в консоль и/или файл с использованием библиотеки spdlog.
 */
class Logger {
public:
    /**
     * @brief Создает новый логгер
     * @param logFile Путь к файлу логов (пустая строка для логирования только в консоль)
     * @param level Уровень логирования
     */
    explicit Logger(const std::string& logFile = "", 
                   LogLevel level = LogLevel::INFO);
    
    /**
     * @brief Деструктор, закрывает логгер
     */
    ~Logger();
    
    // Запрещаем копирование и перемещение
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    /**
     * @brief Устанавливает уровень логирования
     * @param level Новый уровень логирования
     */
    void setLogLevel(LogLevel level);
    
    /**
     * @brief Возвращает текущий уровень логирования
     * @return Текущий уровень логирования
     */
    [[nodiscard]] LogLevel getLogLevel() const;
    
    /**
     * @brief Логирует отладочное сообщение (уровень DEBUG)
     * @param message Сообщение для логирования
     */
    void debug(const std::string& message);
    
    /**
     * @brief Логирует информационное сообщение (уровень INFO)
     * @param message Сообщение для логирования
     */
    void info(const std::string& message);
    
    /**
     * @brief Логирует предупреждение (уровень WARN)
     * @param message Сообщение для логирования
     */
    void warn(const std::string& message);
    
    /**
     * @brief Логирует ошибку (уровень ERROR)
     * @param message Сообщение для логирования
     */
    void error(const std::string& message);
    
    /**
     * @brief Логирует критическую ошибку (уровень CRITICAL)
     * @param message Сообщение для логирования
     */
    void critical(const std::string& message);
    
    /**
     * @brief Логирует сообщение с указанным уровнем
     * @param level Уровень логирования
     * @param message Сообщение для логирования
     */
    void log(LogLevel level, const std::string& message);
    
    /**
     * @brief Проверяет работоспособность логгера
     * @return true если логгер работоспособен, иначе false
     */
    [[nodiscard]] bool isHealthy() const;
    
    /**
     * @brief Принудительно записывает буферизованные сообщения
     */
    void flush();
    
    /**
     * @brief Преобразует строковое представление уровня логирования в enum LogLevel
     * @param levelStr Строковое представление уровня логирования
     * @return Соответствующее значение LogLevel или LogLevel::INFO, если строка не распознана
     */
    static LogLevel stringToLevel(const std::string& levelStr);
    
    /**
     * @brief Преобразует enum LogLevel в строковое представление
     * @param level Уровень логирования
     * @return Строковое представление уровня логирования
     */
    static std::string levelToString(LogLevel level);

private:
    std::string _logFile;           // Путь к файлу логов
    LogLevel _logLevel;             // Текущий уровень логирования
    mutable std::mutex _mutex;      // Мьютекс для потокобезопасности
    std::ofstream _file;            // Файловый поток (не используется с spdlog)
    bool _logToFile = false;        // Флаг логирования в файл
    bool _isHealthy = true;         // Флаг работоспособности логгера
};