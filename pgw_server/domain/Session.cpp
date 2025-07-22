#include "Session.h"
#include <stdexcept>
#include <regex>

Session::Session(std::string imsi)
    : _imsi(std::move(imsi)), _createdAt(std::chrono::system_clock::now()), _logger(nullptr) {
    validateImsi(_imsi);
}

Session::Session(std::string imsi, std::shared_ptr<Logger> logger)
    : _imsi(std::move(imsi)), _createdAt(std::chrono::system_clock::now()), _logger(std::move(logger)) {
    validateImsi(_imsi);
    if (_logger) {
        _logger->debug("Session created for IMSI: " + _imsi);
    }
}

const std::string& Session::getImsi() const {
    return _imsi;
}

const std::chrono::system_clock::time_point& Session::getCreatedAt() const {
    return _createdAt;
}

bool Session::isExpired(std::chrono::seconds timeout) const {
    auto now = std::chrono::system_clock::now();
    auto age = std::chrono::duration_cast<std::chrono::seconds>(now - _createdAt);
    bool expired = age > timeout;
    
    if (_logger && expired) {
        _logger->debug("Session for IMSI " + _imsi + " expired after " + 
                      std::to_string(age.count()) + "s (timeout: " + 
                      std::to_string(timeout.count()) + "s)");
    }
    
    return expired;
}

std::chrono::seconds Session::getAge() const {
    auto now = std::chrono::system_clock::now();
    auto age = std::chrono::duration_cast<std::chrono::seconds>(now - _createdAt);
    
    if (_logger && _logger->getLogLevel() <= LogLevel::LOG_DEBUG) {
        _logger->debug("Session for IMSI " + _imsi + " age: " + std::to_string(age.count()) + "s");
    }
    
    return age;
}

void Session::validateImsi(const std::string& imsi) {
    std::regex imsiPattern("^[0-9]{15}$");
    if (!std::regex_match(imsi, imsiPattern)) {
        std::string errorMsg = "Invalid IMSI format: " + imsi + ". IMSI must be 15 digits.";
        throw std::invalid_argument(errorMsg);
    }
} 