#include <PgwClient.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <iostream>
#include <poll.h>
#include <algorithm>

PgwClient::PgwClient(const std::string& configPath)
    : _config(std::make_unique<ClientConfig>(configPath)),
      _logger(std::make_unique<ClientLogger>()),
      _socket(-1) {
}

PgwClient::~PgwClient() {
    closeSocket();
}

bool PgwClient::initialize() {
    if (!_config->loadConfig()) {
        std::cerr << "Failed to load configuration" << std::endl;
        return false;
    }
    
    const auto& config = _config->getConfig();
    
    ClientLogLevel logLevel = ClientLogLevel::INFO;
    if (config.log_level == "DEBUG") {
        logLevel = ClientLogLevel::DEBUG;
    } else if (config.log_level == "INFO") {
        logLevel = ClientLogLevel::INFO;
    } else if (config.log_level == "WARN") {
        logLevel = ClientLogLevel::WARN;
    } else if (config.log_level == "ERROR") {
        logLevel = ClientLogLevel::ERROR;
    } else if (config.log_level == "CRITICAL") {
        logLevel = ClientLogLevel::CRITICAL;
    }
    
    _logger = std::make_unique<ClientLogger>(config.log_file, logLevel);
    
    _logger->info("Client initialized with server: " + config.server_ip + ":" + 
                  std::to_string(config.server_port));
    
    return setupUdpSocket();
}

std::pair<bool, std::string> PgwClient::sendRequest(const std::string& imsi) {
    _logger->info("Sending request for IMSI: " + imsi);
    
    // Проверяем, что IMSI имеет правильный формат (15 цифр)
    if (imsi.length() != 15 || !std::all_of(imsi.begin(), imsi.end(), ::isdigit)) {
        _logger->error("Invalid IMSI format: " + imsi);
        return {false, "Invalid IMSI format"};
    }
    
    // Кодируем IMSI в BCD формат
    auto [encodeSuccess, bcdImsi] = encodeImsiToBcd(imsi);
    if (!encodeSuccess) {
        _logger->error("Failed to encode IMSI to BCD");
        return {false, "Failed to encode IMSI to BCD"};
    }
    
    // Отправляем UDP пакет
    if (!sendUdpPacket(bcdImsi)) {
        _logger->error("Failed to send UDP packet");
        return {false, "Failed to send UDP packet"};
    }
    
    // Получаем ответ
    auto [receiveSuccess, response] = receiveResponse();
    if (!receiveSuccess) {
        _logger->error("Failed to receive response");
        return {false, "Failed to receive response"};
    }
    
    _logger->info("Received response: " + response);
    return {true, response};
}

bool PgwClient::setupUdpSocket() {
    // Закрываем сокет, если он уже был открыт
    closeSocket();
    
    _socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (_socket < 0) {
        _logger->error("Failed to create UDP socket: " + std::string(strerror(errno)));
        return false;
    }
    
    _logger->debug("UDP socket created successfully");
    return true;
}

void PgwClient::closeSocket() {
    if (_socket >= 0) {
        close(_socket);
        _socket = -1;
        _logger->debug("UDP socket closed");
    }
}

std::pair<bool, std::string> PgwClient::encodeImsiToBcd(const std::string& imsi) {
    // Проверяем, что IMSI имеет правильный формат (15 цифр)
    if (imsi.length() != 15 || !std::all_of(imsi.begin(), imsi.end(), ::isdigit)) {
        return {false, ""};
    }
    
    // Преобразуем строку IMSI в BCD формат
    // BCD формат: каждый байт содержит две цифры (кроме последнего, если количество цифр нечетное)
    std::string bcdImsi;
    
    // Добавляем заголовок пакета (4 байта)
    bcdImsi.push_back(0x01);
    bcdImsi.push_back(0x00);
    bcdImsi.push_back(0x00);
    bcdImsi.push_back(0x00);
    
    // Резервируем память для данных IMSI
    bcdImsi.reserve(4 + (imsi.length() + 1) / 2);
    
    for (size_t i = 0; i < imsi.length(); i += 2) {
        uint8_t byte = 0;
        
        // Первая цифра (младшие 4 бита)
        byte |= (imsi[i] - '0') & 0x0F;
        
        // Вторая цифра (старшие 4 бита), если она есть
        if (i + 1 < imsi.length()) {
            byte |= ((imsi[i + 1] - '0') << 4) & 0xF0;
        } else {
            // Если количество цифр нечетное, заполняем старшие 4 бита значением 0xF
            byte |= 0xF0;
        }
        
        bcdImsi.push_back(static_cast<char>(byte));
    }
    
    // Выводим отладочную информацию
    std::string hexDump;
    for (unsigned char c : bcdImsi) {
        char hex[8];
        snprintf(hex, sizeof(hex), "%02x ", c);
        hexDump += hex;
    }
    _logger->debug("BCD encoded IMSI: " + hexDump);
    
    return {true, bcdImsi};
}

bool PgwClient::sendUdpPacket(const std::string& bcdImsi) {
    if (_socket < 0) {
        _logger->error("Socket not initialized");
        return false;
    }
    
    const auto& config = _config->getConfig();
    
    // Настраиваем адрес сервера
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(config.server_port);
    
    // if (inet_pton(AF_INET, config.server_ip.c_str(), &(serverAddr.sin_addr)) <= 0) {
    //     _logger->error("Invalid IP address: " + config.server_ip);
    //     return false;
    // }
    
    // Отправляем пакет
    ssize_t bytesSent = sendto(_socket, bcdImsi.c_str(), bcdImsi.length(), 0,
                             (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    
    if (bytesSent < 0) {
        _logger->error("Failed to send UDP packet: " + std::string(strerror(errno)));
        return false;
    }
    
    if (static_cast<size_t>(bytesSent) != bcdImsi.length()) {
        _logger->warn("Sent only " + std::to_string(bytesSent) + " bytes out of " + 
                      std::to_string(bcdImsi.length()));
    }
    
    _logger->debug("Sent " + std::to_string(bytesSent) + " bytes to " + 
                   config.server_ip + ":" + std::to_string(config.server_port));
    
    return true;
}

std::pair<bool, std::string> PgwClient::receiveResponse() {
    if (_socket < 0) {
        _logger->error("Socket not initialized");
        return {false, ""};
    }
    
    const auto& config = _config->getConfig();
    
    // Используем poll для ожидания ответа с таймаутом
    struct pollfd fds[1];
    fds[0].fd = _socket;
    fds[0].events = POLLIN;
    
    int pollResult = poll(fds, 1, config.receive_timeout_ms);
    
    if (pollResult < 0) {
        _logger->error("Poll error: " + std::string(strerror(errno)));
        return {false, ""};
    }
    
    if (pollResult == 0) {
        _logger->error("Timeout waiting for response");
        return {false, "Timeout"};
    }
    
    // Получаем ответ
    char buffer[MAX_RESPONSE_SIZE];
    struct sockaddr_in serverAddr;
    socklen_t serverAddrLen = sizeof(serverAddr);
    
    ssize_t bytesRead = recvfrom(_socket, buffer, MAX_RESPONSE_SIZE - 1, 0,
                               (struct sockaddr*)&serverAddr, &serverAddrLen);
    
    if (bytesRead < 0) {
        _logger->error("Failed to receive response: " + std::string(strerror(errno)));
        return {false, ""};
    }
    
    // Преобразуем IP адрес в строку для логирования
    char serverIp[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(serverAddr.sin_addr), serverIp, INET_ADDRSTRLEN);
    
    _logger->debug("Received " + std::to_string(bytesRead) + " bytes from " + 
                   std::string(serverIp) + ":" + std::to_string(ntohs(serverAddr.sin_port)));
    
    return {true, std::string(buffer, bytesRead)};
} 