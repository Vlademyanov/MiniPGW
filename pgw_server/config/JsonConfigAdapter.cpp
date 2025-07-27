#include <JsonConfigAdapter.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

JsonConfigAdapter::JsonConfigAdapter(std::string configPath)
    : _configPath(std::move(configPath)) {
    setDefaults();
}

bool JsonConfigAdapter::load() {
    try {
        // Открываем файл конфигурации

        std::ifstream configFile(_configPath);
        if (!configFile.is_open()) {
            std::string error = "Failed to open config file: " + _configPath;
            std::cerr << error << std::endl;
            setError(error);
            return false;
        }
        
        
        // Парсим JSON
        nlohmann::json jsonConfig;
        try {
            configFile >> jsonConfig;

        } catch (const std::exception& e) {
            std::cerr << "Error parsing JSON: " << e.what() << std::endl;
            setError("JSON parsing error: " + std::string(e.what()));
            return false;
        }
        
        // Заполняем структуру конфигурации
        if (jsonConfig.contains("udp_ip")) {
            _config.udp_ip = jsonConfig["udp_ip"].get<std::string>();
        }
        
        if (jsonConfig.contains("udp_port")) {
            _config.udp_port = jsonConfig["udp_port"].get<uint16_t>();
        }
        
        if (jsonConfig.contains("session_timeout_sec")) {
            _config.session_timeout_sec = jsonConfig["session_timeout_sec"].get<uint32_t>();
        }
        
        if (jsonConfig.contains("cleanup_interval_sec")) {
            _config.cleanup_interval_sec = jsonConfig["cleanup_interval_sec"].get<uint32_t>();
        }
        
        if (jsonConfig.contains("cdr_file")) {
            _config.cdr_file = jsonConfig["cdr_file"].get<std::string>();
        }
        
        if (jsonConfig.contains("http_port")) {
            _config.http_port = jsonConfig["http_port"].get<uint16_t>();
        }
        
        if (jsonConfig.contains("graceful_shutdown_rate")) {
            _config.graceful_shutdown_rate = jsonConfig["graceful_shutdown_rate"].get<uint32_t>();
        }
        
        if (jsonConfig.contains("max_requests_per_minute")) {
            _config.max_requests_per_minute = jsonConfig["max_requests_per_minute"].get<uint32_t>();
        }
        
        if (jsonConfig.contains("log_file")) {
            _config.log_file = jsonConfig["log_file"].get<std::string>();
        }
        
        if (jsonConfig.contains("log_level")) {
            _config.log_level = jsonConfig["log_level"].get<std::string>();
        }
        
        // Загружаем черный список
        if (jsonConfig.contains("blacklist") && jsonConfig["blacklist"].is_array()) {
            _config.blacklist.clear();
            for (const auto& item : jsonConfig["blacklist"]) {
                if (item.is_string()) {
                    _config.blacklist.push_back(item.get<std::string>());
                }
            }
        }
        
        // Проверяем валидность конфигурации
        if (!validateConfig()) {
            return false;
        }
        
        _isValid = true;
        return true;
    } catch (const std::exception& e) {
        setError("Failed to parse config file: " + std::string(e.what()));
        return false;
    }
}

const ServerConfig& JsonConfigAdapter::getConfig() const {
    return _config;
}

bool JsonConfigAdapter::isValid() const {
    return _isValid;
}

std::string JsonConfigAdapter::getLastError() const {
    return _lastError;
}

std::string JsonConfigAdapter::getString(const std::string& key, const std::string& defaultValue) const {
    if (key == "udp_ip") return _config.udp_ip;
    if (key == "cdr_file") return _config.cdr_file;
    if (key == "log_file") return _config.log_file;
    if (key == "log_level") return _config.log_level;
    return defaultValue;
}

uint32_t JsonConfigAdapter::getUint(const std::string& key, uint32_t defaultValue) const {
    if (key == "udp_port") return _config.udp_port;
    if (key == "http_port") return _config.http_port;
    if (key == "session_timeout_sec") return _config.session_timeout_sec;
    if (key == "cleanup_interval_sec") return _config.cleanup_interval_sec;
    if (key == "graceful_shutdown_rate") return _config.graceful_shutdown_rate;
    if (key == "max_requests_per_minute") return _config.max_requests_per_minute;
    return defaultValue;
}

std::vector<std::string> JsonConfigAdapter::getStringArray(const std::string& key) const {
    if (key == "blacklist") return _config.blacklist;
    return {};
}

void JsonConfigAdapter::setDefaults() {
    _config.udp_ip = "0.0.0.0";
    _config.udp_port = 9000;
    _config.session_timeout_sec = 30;
    _config.cleanup_interval_sec = 5;
    _config.cdr_file = "cdr.log";
    _config.http_port = 8080;
    _config.graceful_shutdown_rate = 10;
    _config.max_requests_per_minute = 100;
    _config.log_file = "pgw.log";
    _config.log_level = "INFO";
    _config.blacklist.clear();
}

bool JsonConfigAdapter::validateConfig() {
    // Проверяем UDP порт
    if (_config.udp_port == 0) {
        setError("Invalid UDP port: 0");
        return false;
    }
    
    // Проверяем HTTP порт
    if (_config.http_port == 0) {
        setError("Invalid HTTP port: 0");
        return false;
    }
    
    // Проверяем таймаут сессии
    if (_config.session_timeout_sec == 0) {
        setError("Invalid session timeout: 0");
        return false;
    }
    
    // Проверяем интервал очистки
    if (_config.cleanup_interval_sec == 0) {
        setError("Invalid cleanup interval: 0");
        return false;
    }
    
    // Проверяем скорость плавного завершения
    if (_config.graceful_shutdown_rate == 0) {
        setError("Invalid graceful shutdown rate: 0");
        return false;
    }
    
    // Проверяем максимальное количество запросов в минуту
    if (_config.max_requests_per_minute == 0) {
        setError("Invalid max requests per minute: 0");
        return false;
    }
    
    return true;
}

void JsonConfigAdapter::setError(const std::string& error) {
    _lastError = error;
    _isValid = false;
} 