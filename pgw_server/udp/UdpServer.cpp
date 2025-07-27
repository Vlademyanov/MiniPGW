#include <UdpServer.h>
#include <utility>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <sys/epoll.h>
#include <cerrno>

UdpServer::UdpServer(std::string  ip, uint16_t port,
                   std::shared_ptr<SessionManager> sessionManager,
                   std::shared_ptr<Logger> logger)
    : _ip(std::move(ip)), _port(port),
      _sessionManager(std::move(sessionManager)),
      _logger(std::move(logger)) {
    
    if (!_sessionManager) throw std::invalid_argument("sessionManager cannot be null");
    if (!_logger) throw std::invalid_argument("logger cannot be null");
    if (_port == 0) throw std::invalid_argument("port cannot be 0");
    if (_ip.empty()) throw std::invalid_argument("ip cannot be empty");
    
    _logger->info("UDP server initialized on " + _ip + ":" + std::to_string(_port));
}

UdpServer::~UdpServer() {
    stop();
}

bool UdpServer::start() {
    if (_running) {
        _logger->warn("UDP server already running");
        return false;
    }
    
    // Создаем сокет
    _socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (_socket < 0) {
        _logger->error("Failed to create socket: " + std::string(strerror(errno)));
        return false;
    }
    
    // Настраиваем адрес сервера
    struct sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(_port);
    
    // Преобразуем IP-адрес из строки в бинарный формат
    if (inet_pton(AF_INET, _ip.c_str(), &serverAddr.sin_addr) <= 0) {
        _logger->error("Invalid address: " + _ip + ", error: " + std::string(strerror(errno)));
        close(_socket);
        _socket = -1;
        return false;
    }
    
    // Включаем повторное использование адреса
    int reuse = 1;
    if (setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        _logger->warn("Failed to set SO_REUSEADDR: " + std::string(strerror(errno)));
    }
    
    // Привязываем сокет к адресу
    if (bind(_socket, reinterpret_cast<struct sockaddr *>(&serverAddr), sizeof(serverAddr)) < 0) {
        _logger->error("Bind failed for " + _ip + ":" + std::to_string(_port) + 
                      ", error: " + std::string(strerror(errno)));
        close(_socket);
        _socket = -1;
        return false;
    }
    
    
    // Настраиваем epoll
    if (!setupEpollSocket()) {
        close(_socket);
        _socket = -1;
        return false;
    }
    
    // Запускаем серверный поток
    _running = true;
    _serverThread = std::thread(&UdpServer::serverLoop, this);
    
    _logger->info("UDP server started on " + _ip + ":" + std::to_string(_port));
    return true;
}

void UdpServer::stop() {
    if (!_running) {
        return;
    }
    
    _running = false;
    
    if (_serverThread.joinable()) {
        _serverThread.join();
    }
    
    cleanupResources();
    
    _logger->info("UDP server stopped");
}

bool UdpServer::isRunning() const {
    return _running;
}

void UdpServer::serverLoop() {
    constexpr int MAX_EVENTS = 512; // для высоконагруженных систем 128-1024
    struct epoll_event events[MAX_EVENTS];
    char buffer[8 * 1024];
    
    while (_running) {
        // Ждем события с таймаутом 30 мс для высоконагруженных систем 10-50
        int nfds = epoll_wait(_epollFd, events, MAX_EVENTS, 30);
        
        if (nfds == -1) {
            if (errno == EINTR) {
                // Прерывание системным вызовом, продолжаем
                continue;
            }
            
            _logger->error("epoll_wait error: " + std::string(strerror(errno)));
            break;
        }
        
        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == _socket) {
                struct sockaddr_in clientAddr{};
                socklen_t clientLen = sizeof(clientAddr);
                
                // Получаем данные от клиента
                ssize_t bytesReceived = recvfrom(_socket, buffer, sizeof(buffer) - 1, 0,
                                               reinterpret_cast<struct sockaddr *>(&clientAddr), &clientLen);
                
                if (bytesReceived < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        // Нет данных, продолжаем
                        continue;
                    }
                    
                    _logger->error("Error receiving data: " + std::string(strerror(errno)));
                    continue;
                }
                
                if (bytesReceived == 0) {
                    continue;
                }
                
                // Обрабатываем полученный пакет
                handleIncomingPacket(buffer, bytesReceived, clientAddr);
            }
        }
    }
}

void UdpServer::handleIncomingPacket(const char* buffer, size_t length,
                                   const struct sockaddr_in& clientAddr) const {
    try {
        // Получаем IP-адрес клиента для логирования
        char clientIp[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIp, INET_ADDRSTRLEN);
        
        // Извлекаем IMSI из пакета
        std::string imsi = extractImsiFromBcd(buffer, length);
        
        if (imsi.empty()) {
            _logger->warn("Received packet with invalid IMSI format from " + std::string(clientIp));
            sendResponse("rejected", clientAddr);
            return;
        }
        
        _logger->info("Received request for IMSI: " + imsi + " from " + std::string(clientIp));
        
        // Создаем сессию через SessionManager
        SessionResult result = _sessionManager->createSession(imsi);
        
        // Отправляем ответ клиенту
        if (result == SessionResult::CREATED) {
            sendResponse("created", clientAddr);
                _logger->info("Session created for IMSI: " + imsi);
        } else {
                sendResponse("rejected", clientAddr);
            _logger->info("Session rejected for IMSI: " + imsi + ", result: " + 
                          (result == SessionResult::REJECTED ? "REJECTED" : "ERROR"));
        }
    } catch (const std::exception& e) {
        _logger->error("Error handling packet: " + std::string(e.what()));
        sendResponse("rejected", clientAddr);
    }
}

std::string UdpServer::extractImsiFromBcd(const char* buffer, size_t length) const {
    // Проверяем минимальную длину пакета
    if (length < 8) {
        _logger->warn("Packet too short for IMSI: " + std::to_string(length) + " bytes");
        return "";
    }
    
    // Пропускаем первые 4 байта заголовка
    const size_t headerSize = 4;
    
    // Декодируем IMSI из BCD формата
    // BCD формат: каждый байт содержит две цифры кроме последнего
    std::string imsi;
    imsi.reserve(15); 
    
    // Отладочный вывод для анализа байтов
    std::string hexDump;
    for (size_t i = 0; i < length; i++) {
        char hex[8];
        snprintf(hex, sizeof(hex), "%02x ", static_cast<unsigned char>(buffer[i]));
        hexDump += hex;
    }
    _logger->debug("Raw packet bytes: " + hexDump);
    
    // Декодируем IMSI из BCD формата
    for (size_t i = headerSize; i < length && imsi.length() < 15; i++) {
        auto byte = static_cast<uint8_t>(buffer[i]);
        
        // Извлекаем младшую цифру (4 младших бита)
        uint8_t digit1 = byte & 0x0F;
        if (digit1 <= 9) {
            imsi.push_back('0' + digit1);
        } else {
            _logger->warn("Invalid BCD digit in IMSI: " + std::to_string(digit1));
            return "";
        }
        
        // Если уже набрали 15 цифр, выходим
        if (imsi.length() >= 15) {
            break;
        }
        
        // Извлекаем старшую цифру (4 старших бита)
        uint8_t digit2 = (byte >> 4) & 0x0F;
        if (digit2 <= 9) {
            imsi.push_back('0' + digit2);
        } else if (digit2 == 0x0F && i == length - 1) {
            // Последний полубайт может быть заполнителем F
            break;
        } else {
            _logger->warn("Invalid BCD digit in IMSI: " + std::to_string(digit2));
            return "";
        }
    }
    
    // Проверяем, что IMSI имеет правильную длину (15 цифр)
    if (imsi.length() != 15) {
        _logger->warn("Invalid IMSI length: " + std::to_string(imsi.length()));
        return "";
    }
    
    _logger->debug("Decoded IMSI from BCD: " + imsi);
    return imsi;
}

void UdpServer::sendResponse(const std::string& response,
                           const struct sockaddr_in& clientAddr) const {
    ssize_t bytesSent = sendto(_socket, response.c_str(), response.length(), 0,
                             reinterpret_cast<const struct sockaddr*>(&clientAddr), sizeof(clientAddr));
    
    if (bytesSent < 0) {
        char clientIp[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIp, INET_ADDRSTRLEN);
        _logger->error("Error sending response to " + std::string(clientIp) + ": " + std::string(strerror(errno)));
    } else {
        _logger->debug("Sent response: " + response);
    }
}

bool UdpServer::setupEpollSocket() {
    // Делаем сокет неблокирующим
    int flags = fcntl(_socket, F_GETFL, 0);
    if (flags == -1) {
        _logger->error("Failed to get socket flags: " + std::string(strerror(errno)));
        return false;
    }
    
    if (fcntl(_socket, F_SETFL, flags | O_NONBLOCK) == -1) {
        _logger->error("Failed to set non-blocking mode: " + std::string(strerror(errno)));
        return false;
    }
    
    // Создаем epoll инстанс
    _epollFd = epoll_create1(0);
    if (_epollFd == -1) {
        _logger->error("Failed to create epoll instance: " + std::string(strerror(errno)));
        return false;
    }
    
    // Добавляем сокет в epoll
    struct epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = _socket;
    
    if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, _socket, &ev) == -1) {
        _logger->error("Failed to add socket to epoll: " + std::string(strerror(errno)));
        close(_epollFd);
        _epollFd = -1;
        return false;
    }
    
    _logger->debug("Socket configured for epoll");
    return true;
}

void UdpServer::cleanupResources() {
    // Закрываем epoll
    if (_epollFd >= 0) {
        close(_epollFd);
        _epollFd = -1;
    }
    
    // Закрываем сокет
    if (_socket >= 0) {
        close(_socket);
        _socket = -1;
    }
} 