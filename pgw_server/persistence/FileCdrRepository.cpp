#include <FileCdrRepository.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <utility>

FileCdrRepository::FileCdrRepository(std::string filePath)
    : _filePath(std::move(filePath)), _isHealthy(true), _logger(nullptr) {
    // Пытаемся открыть файл при создании объекта
    std::lock_guard<std::mutex> lock(_mutex);
    openFileIfNeeded();
}

FileCdrRepository::FileCdrRepository(std::string filePath, std::shared_ptr<Logger> logger)
    : _filePath(std::move(filePath)), _isHealthy(true), _logger(std::move(logger)) {
    // Пытаемся открыть файл при создании объекта
    std::lock_guard<std::mutex> lock(_mutex);
    if (openFileIfNeeded()) {
        _logger->info("CDR repository initialized with file: " + _filePath);
    } else {
        _logger->critical("Failed to initialize CDR repository: cannot open file " + _filePath);
    }
}

FileCdrRepository::~FileCdrRepository() {
    // Закрываем файл при уничтожении объекта
    std::lock_guard<std::mutex> lock(_mutex);
    if (_file.is_open()) {
        _file.close();
        if (_logger) {
            _logger->debug("CDR file closed: " + _filePath);
        }
    }
}

bool FileCdrRepository::writeCdr(const std::string& imsi, const std::string& action) {
    // Используем текущее время
    return writeCdr(imsi, action, getCurrentTimestamp());
}

bool FileCdrRepository::writeCdr(const std::string& imsi, const std::string& action, const std::string& timestamp) {
    std::lock_guard<std::mutex> lock(_mutex);
    
    if (!_isHealthy) {
        if (_logger) {
            _logger->error("CDR write failed: repository is in unhealthy state");
        }
        return false;
    }
    
    // Открываем файл, если он еще не открыт
    if (!openFileIfNeeded()) {
        if (_logger) {
            _logger->error("CDR write failed: cannot open file " + _filePath);
        }
        return false;
    }

    _file << timestamp << "," << imsi << "," << action << std::endl;
    
    // Проверяем, успешно ли записалось
    if (_file.fail()) {
        _isHealthy = false;
        if (_logger) {
            _logger->critical("CDR system failure: write operation failed on file " + _filePath);
        }
        return false;
    }
    
    if (_logger) {
        _logger->debug("CDR record written: " + timestamp + "," + imsi + "," + action);
    }
    
    return true;
}

std::string FileCdrRepository::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    
    return ss.str();
}

bool FileCdrRepository::openFileIfNeeded() {
    if (_file.is_open()) {
        return true;
    }
    
    // Открываем файл для append
    _file.open(_filePath, std::ios::app);
    
    if (!_file.is_open()) {
        _isHealthy = false;
        if (_logger) {
            _logger->error("Failed to open CDR file: " + _filePath + " (check permissions and path)");
        }
        return false;
    }

    return true;
} 