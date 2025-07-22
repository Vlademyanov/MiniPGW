#include "ClientLogger.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <iostream>

ClientLogger::ClientLogger(const std::string& logFile, ClientLogLevel level)
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
                _logger = std::make_shared<spdlog::logger>("pgw_client_logger", sinks.begin(), sinks.end());
            } catch (const spdlog::spdlog_ex& ex) {
                std::cerr << "Logger initialization failed: " << ex.what() << std::endl;
                _isHealthy = false;
                _logToFile = false;
                
                // Если не удалось создать файловый логгер, используем только консольный
                _logger = std::make_shared<spdlog::logger>("pgw_client_console_logger", console_sink);
            }
        } else {
            // Если файл не указан, используем только консольный логгер
            _logger = std::make_shared<spdlog::logger>("pgw_client_console_logger", console_sink);
        }
        
        // Устанавливаем уровень логирования
        spdlog::level::level_enum spdlog_level;
        switch (_logLevel) {
            case ClientLogLevel::DEBUG:
                spdlog_level = spdlog::level::debug;
                break;
            case ClientLogLevel::INFO:
                spdlog_level = spdlog::level::info;
                break;
            case ClientLogLevel::WARN:
                spdlog_level = spdlog::level::warn;
                break;
            case ClientLogLevel::ERROR:
                spdlog_level = spdlog::level::err;
                break;
            case ClientLogLevel::CRITICAL:
                spdlog_level = spdlog::level::critical;
                break;
            default:
                spdlog_level = spdlog::level::info; // По умолчанию INFO
                break;
        }
        _logger->set_level(spdlog_level);
        
        // Устанавливаем формат сообщений
        _logger->set_pattern("%Y-%m-%d %H:%M:%S.%e [%l] [%t] %v");
        
        // Логируем информацию о запуске логгера
        _logger->info("Logger initialized with level: {}", levelToString(_logLevel));
        if (_logToFile) {
            _logger->info("Logging to file: {}", _logFile);
        }
    } catch (const std::exception& e) {
        std::cerr << "Logger initialization failed: " << e.what() << std::endl;
        _isHealthy = false;
    }
}

ClientLogger::~ClientLogger() {
    try {
        if (_isHealthy && _logger) {
            _logger->info("Logger shutting down");
            _logger->flush();
        }
    } catch (...) {
        // Игнорируем исключения в деструкторе
    }
}

void ClientLogger::setLogLevel(ClientLogLevel level) {
    std::lock_guard<std::mutex> lock(_mutex);
    _logLevel = level;
    
    if (!_logger) {
        return;
    }
    
    // Преобразуем уровень логирования в spdlog
    spdlog::level::level_enum spdlog_level;
    switch (level) {
        case ClientLogLevel::DEBUG:
            spdlog_level = spdlog::level::debug;
            break;
        case ClientLogLevel::INFO:
            spdlog_level = spdlog::level::info;
            break;
        case ClientLogLevel::WARN:
            spdlog_level = spdlog::level::warn;
            break;
        case ClientLogLevel::ERROR:
            spdlog_level = spdlog::level::err;
            break;
        case ClientLogLevel::CRITICAL:
            spdlog_level = spdlog::level::critical;
            break;
        default:
            spdlog_level = spdlog::level::info;
            break;
    }
    
    _logger->set_level(spdlog_level);
    _logger->info("Log level changed to: {}", levelToString(level));
}

void ClientLogger::debug(const std::string& message) {
    log(ClientLogLevel::DEBUG, message);
}

void ClientLogger::info(const std::string& message) {
    log(ClientLogLevel::INFO, message);
}

void ClientLogger::warn(const std::string& message) {
    log(ClientLogLevel::WARN, message);
}

void ClientLogger::error(const std::string& message) {
    log(ClientLogLevel::ERROR, message);
}

void ClientLogger::critical(const std::string& message) {
    log(ClientLogLevel::CRITICAL, message);
}

void ClientLogger::log(ClientLogLevel level, const std::string& message) {
    if (!_isHealthy || !_logger) {
        return;
    }
    
    // Проверяем уровень логирования
    if (level < _logLevel) {
        return;
    }
    
    // Логируем сообщение с соответствующим уровнем
    switch (level) {
        case ClientLogLevel::DEBUG:
            _logger->debug(message);
            break;
        case ClientLogLevel::INFO:
            _logger->info(message);
            break;
        case ClientLogLevel::WARN:
            _logger->warn(message);
            break;
        case ClientLogLevel::ERROR:
            _logger->error(message);
            break;
        case ClientLogLevel::CRITICAL:
            _logger->critical(message);
            break;
        default:
            _logger->info(message);
            break;
    }
}

std::string ClientLogger::levelToString(ClientLogLevel level) {
    switch (level) {
        case ClientLogLevel::DEBUG:
            return "DEBUG";
        case ClientLogLevel::INFO:
            return "INFO";
        case ClientLogLevel::WARN:
            return "WARN";
        case ClientLogLevel::ERROR:
            return "ERROR";
        case ClientLogLevel::CRITICAL:
            return "CRITICAL";
        default:
            return "UNKNOWN";
    }
}