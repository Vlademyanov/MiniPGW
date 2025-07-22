#include "ClientConfig.h"
#include <fstream>
#include <iostream>
#include <sstream>

ClientConfig::ClientConfig(const std::string& configPath)
    : _configPath(configPath) {
    setDefaults();
}

bool ClientConfig::loadConfig() {
    try {
        // Открываем файл конфигурации
        std::ifstream configFile(_configPath);
        if (!configFile.is_open()) {
            _lastError = "Failed to open config file: " + _configPath;
            std::cerr << _lastError << std::endl;
            return false;
        }
        
        // Читаем содержимое файла
        std::stringstream buffer;
        buffer << configFile.rdbuf();
        std::string content = buffer.str();
        
        // Парсим JSON конфигурацию
        parseJsonValue("server_ip", content, _config.server_ip);
        parseJsonUint16("server_port", content, _config.server_port);
        parseJsonValue("log_file", content, _config.log_file);
        parseJsonValue("log_level", content, _config.log_level);
        parseJsonUint32("receive_timeout_ms", content, _config.receive_timeout_ms);
        
        // Проверяем валидность конфигурации
        _isValid = validateConfig();
        
        return _isValid;
    } catch (const std::exception& e) {
        _lastError = "Error loading config: " + std::string(e.what());
        std::cerr << _lastError << std::endl;
        return false;
    }
}

void ClientConfig::parseJsonValue(const std::string& key, const std::string& json, std::string& value) {
    // Ищем ключ в формате "key": "value"
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = json.find(searchKey);
    
    if (keyPos != std::string::npos) {
        // Находим начало значения (после ":")
        size_t colonPos = json.find(':', keyPos);
        if (colonPos != std::string::npos) {
            // Находим начало строкового значения (после '"')
            size_t valueStartPos = json.find('\"', colonPos);
            if (valueStartPos != std::string::npos) {
                valueStartPos++; // Пропускаем открывающую кавычку
                
                // Находим конец строкового значения (закрывающую кавычку)
                size_t valueEndPos = json.find('\"', valueStartPos);
                if (valueEndPos != std::string::npos) {
                    // Извлекаем значение
                    value = json.substr(valueStartPos, valueEndPos - valueStartPos);
                }
            }
        }
    }
}

void ClientConfig::parseJsonUint16(const std::string& key, const std::string& json, uint16_t& value) {
    // Ищем ключ в формате "key": value
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = json.find(searchKey);
    
    if (keyPos != std::string::npos) {
        // Находим начало значения (после ":")
        size_t colonPos = json.find(':', keyPos);
        if (colonPos != std::string::npos) {
            // Находим начало числового значения (пропускаем пробелы)
            size_t valueStartPos = json.find_first_not_of(" \t\n\r", colonPos + 1);
            if (valueStartPos != std::string::npos) {
                // Находим конец числового значения (до запятой, закрывающей скобки или конца строки)
                size_t valueEndPos = json.find_first_of(",}\n", valueStartPos);
                if (valueEndPos != std::string::npos) {
                    // Извлекаем значение
                    std::string valueStr = json.substr(valueStartPos, valueEndPos - valueStartPos);
                    try {
                        value = static_cast<uint16_t>(std::stoi(valueStr));
                    } catch (const std::exception& e) {
                        std::cerr << "Failed to parse uint16_t value for key '" << key 
                                  << "': " << e.what() << std::endl;
                    }
                }
            }
        }
    }
}

void ClientConfig::parseJsonUint32(const std::string& key, const std::string& json, uint32_t& value) {
    // Ищем ключ в формате "key": value
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = json.find(searchKey);
    
    if (keyPos != std::string::npos) {
        // Находим начало значения (после ":")
        size_t colonPos = json.find(':', keyPos);
        if (colonPos != std::string::npos) {
            // Находим начало числового значения (пропускаем пробелы)
            size_t valueStartPos = json.find_first_not_of(" \t\n\r", colonPos + 1);
            if (valueStartPos != std::string::npos) {
                // Находим конец числового значения (до запятой, закрывающей скобки или конца строки)
                size_t valueEndPos = json.find_first_of(",}\n", valueStartPos);
                if (valueEndPos != std::string::npos) {
                    // Извлекаем значение
                    std::string valueStr = json.substr(valueStartPos, valueEndPos - valueStartPos);
                    try {
                        value = static_cast<uint32_t>(std::stoul(valueStr));
                    } catch (const std::exception& e) {
                        std::cerr << "Failed to parse uint32_t value for key '" << key 
                                  << "': " << e.what() << std::endl;
                    }
                }
            }
        }
    }
}

const ClientConfiguration& ClientConfig::getConfig() const {
    return _config;
}

void ClientConfig::setDefaults() {
    _config.server_ip = "127.0.0.1";
    _config.server_port = 9000;
    _config.log_file = "client.log";
    _config.log_level = "INFO";
    _config.receive_timeout_ms = 5000;
}

bool ClientConfig::validateConfig() {
    // Проверяем IP адрес (простая проверка)
    if (_config.server_ip.empty()) {
        _lastError = "Server IP cannot be empty";
        return false;
    }
    
    // Проверяем порт
    if (_config.server_port == 0) {
        _lastError = "Server port cannot be 0";
        return false;
    }
    
    // Проверяем уровень логирования
    if (_config.log_level != "DEBUG" && 
        _config.log_level != "INFO" && 
        _config.log_level != "WARN" && 
        _config.log_level != "ERROR" && 
        _config.log_level != "CRITICAL") {
        _lastError = "Invalid log level: " + _config.log_level;
        return false;
    }
    
    // Проверяем таймаут
    if (_config.receive_timeout_ms == 0) {
        _lastError = "Receive timeout cannot be 0";
        return false;
    }
    
    return true;
} 