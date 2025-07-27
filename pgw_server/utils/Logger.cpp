#include <Logger.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <iostream>

Logger::Logger(const std::string& logFile, LogLevel level)
    : _logFile(logFile), _logLevel(level), _logToFile(!logFile.empty()), _isHealthy(true) {
    try {
        // Настраиваем логирование в консоль
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        
        // Если указан файл, настраиваем логирование в файл
        if (_logToFile) {
            try {
                auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(_logFile, true);
                
                // Создаем логгер с двумя sink'ами - консоль и файл
                std::vector<spdlog::sink_ptr> sinks {console_sink, file_sink};
                auto logger = std::make_shared<spdlog::logger>("pgw_logger", sinks.begin(), sinks.end());
                
                // Устанавливаем этот логгер как логгер по умолчанию
                spdlog::set_default_logger(logger);
            } catch (const spdlog::spdlog_ex& ex) {
                std::cerr << "Logger initialization failed: " << ex.what() << std::endl;
                _isHealthy = false;
                _logToFile = false;
                
                // Если не удалось создать файловый логгер, используем только консольный
                spdlog::set_default_logger(std::make_shared<spdlog::logger>("pgw_console_logger", console_sink));
            }
        } else {
            // Если файл не указан, используем только консольный логгер
            spdlog::set_default_logger(std::make_shared<spdlog::logger>("pgw_console_logger", console_sink));
        }
        
        // Устанавливаем уровень логирования
        spdlog::level::level_enum spdlog_level;
        switch (_logLevel) {
            case LogLevel::LOG_DEBUG:
                spdlog_level = spdlog::level::debug;
                break;
            case LogLevel::INFO:
                spdlog_level = spdlog::level::info;
                break;
            case LogLevel::WARN:
                spdlog_level = spdlog::level::warn;
                break;
            case LogLevel::ERROR:
                spdlog_level = spdlog::level::err;
                break;
            case LogLevel::CRITICAL:
                spdlog_level = spdlog::level::critical;
                break;
            default:
                spdlog_level = spdlog::level::info; // По умолчанию INFO
                break;
        }
        spdlog::set_level(spdlog_level);
        
        // Устанавливаем формат сообщений
        spdlog::set_pattern("%Y-%m-%d %H:%M:%S.%e [%l] [%t] %v");
        
        // Логируем информацию о запуске логгера
        spdlog::info("Logger initialized. Log level: {}", levelToString(_logLevel));
        if (_logToFile) {
            spdlog::info("Logging to file: {}", _logFile);
        }
    } catch (const std::exception& e) {
        std::cerr << "Logger initialization failed: " << e.what() << std::endl;
        _isHealthy = false;
    }
}

Logger::~Logger() {
    try {
        if (_isHealthy) {
            spdlog::info("Logger shutting down");
            spdlog::shutdown();
        }
    } catch (...) {
        // Игнорируем исключения в деструкторе
    }
}

void Logger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(_mutex);
    _logLevel = level;
    
    // Исправляем преобразование уровней
    spdlog::level::level_enum spdlog_level;
    switch (level) {
        case LogLevel::LOG_DEBUG:
            spdlog_level = spdlog::level::debug;
            break;
        case LogLevel::INFO:
            spdlog_level = spdlog::level::info;
            break;
        case LogLevel::WARN:
            spdlog_level = spdlog::level::warn;
            break;
        case LogLevel::ERROR:
            spdlog_level = spdlog::level::err;
            break;
        case LogLevel::CRITICAL:
            spdlog_level = spdlog::level::critical;
            break;
        default:
            spdlog_level = spdlog::level::info;
            break;
    }
    
    spdlog::set_level(spdlog_level);
    spdlog::info("Log level changed to: {}", levelToString(level));
}

LogLevel Logger::getLogLevel() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _logLevel;
}

void Logger::debug(const std::string& message) {
    log(LogLevel::LOG_DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warn(const std::string& message) {
    log(LogLevel::WARN, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void Logger::critical(const std::string& message) {
    log(LogLevel::CRITICAL, message);
}

void Logger::log(LogLevel level, const std::string& message) {
    if (!_isHealthy) {
        return;
    }
    
    try {
        switch (level) {
            case LogLevel::LOG_DEBUG:
                spdlog::debug(message);
                break;
            case LogLevel::INFO:
                spdlog::info(message);
                break;
            case LogLevel::WARN:
                spdlog::warn(message);
                break;
            case LogLevel::ERROR:
                spdlog::error(message);
                break;
            case LogLevel::CRITICAL:
                spdlog::critical(message);
                break;
            default:
                spdlog::info(message);
                break;
        }
    } catch (const std::exception& e) {
        std::cerr << "Logging error: " << e.what() << std::endl;
        _isHealthy = false;
    }
}

bool Logger::isHealthy() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _isHealthy;
}

void Logger::flush() {
    if (!_isHealthy) {
        return;
    }
    
    try {
        auto logger = spdlog::default_logger();
        if (logger) {
            logger->flush();
        }
    } catch (const std::exception& e) {
        std::cerr << "Flush error: " << e.what() << std::endl;
        _isHealthy = false;
    }
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::LOG_DEBUG:
            return "DEBUG";
        case LogLevel::INFO:
            return "INFO";
        case LogLevel::WARN:
            return "WARN";
        case LogLevel::ERROR:
            return "ERROR";
        case LogLevel::CRITICAL:
            return "CRITICAL";
        default:
            return "UNKNOWN";
    }
}

LogLevel Logger::stringToLevel(const std::string& levelStr) {
    if (levelStr == "DEBUG") {
        return LogLevel::LOG_DEBUG;
    } else if (levelStr == "INFO") {
        return LogLevel::INFO;
    } else if (levelStr == "WARN") {
        return LogLevel::WARN;
    } else if (levelStr == "ERROR") {
        return LogLevel::ERROR;
    } else if (levelStr == "CRITICAL") {
        return LogLevel::CRITICAL;
    } else {
        return LogLevel::INFO;
    }
} 