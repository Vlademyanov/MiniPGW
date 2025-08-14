#include <HttpServer.h>
#include <httplib.h>
#include <utility>
#include <stdexcept>

HttpServer::HttpServer(const std::string& ip,
                     uint16_t port,
                     std::shared_ptr<SessionManager> sessionManager,
                     std::shared_ptr<GracefulShutdownManager> shutdownManager,
                     std::shared_ptr<Logger> logger,
                     StopCallback onStopRequested)
    : _ip(ip),
      _port(port),
      _onStopRequested(std::move(onStopRequested)),
      _sessionManager(std::move(sessionManager)),
      _shutdownManager(std::move(shutdownManager)),
      _logger(std::move(logger)),
      _httpLibServer(nullptr, [](void* ptr) {
          if (auto* server = static_cast<httplib::Server*>(ptr)) {
              server->stop();
              delete server;
          }
      }) {
    
    if (!_sessionManager) throw std::invalid_argument("sessionManager cannot be null");
    if (!_shutdownManager) throw std::invalid_argument("shutdownManager cannot be null");
    if (!_logger) throw std::invalid_argument("logger cannot be null");
    if (_port == 0) throw std::invalid_argument("port cannot be 0");
    if (_ip.empty()) throw std::invalid_argument("ip cannot be empty");
    
    _logger->info("HTTP server initialized on " + _ip + ":" + std::to_string(_port));
}

HttpServer::~HttpServer() {
    stop();
}

bool HttpServer::start() {
    if (_running) {
        _logger->warn("HTTP server already running");
        return false;
    }

    try {
        // Создаем новый HTTP сервер
        auto server = new httplib::Server();
        _httpLibServer.reset(server);
    
    // Настраиваем маршруты
    setupRoutes();
    
    // Запускаем сервер в отдельном потоке
        _running = true;
        _serverThread = std::thread([this, server]() {
            _logger->info("HTTP server started on " + _ip + ":" + std::to_string(_port));
        if (!server->listen(_ip, _port)) {
                _logger->error("Failed to start HTTP server on " + _ip + ":" + std::to_string(_port));
            _running = false;
            }
        });

        // Даем серверу немного времени на запуск
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return _running;
    } catch (const std::exception& e) {
        _logger->error("Failed to start HTTP server: " + std::string(e.what()));
        _running = false;
        return false;
    }
}

void HttpServer::stop() {
    if (!_running) {
        return;
    }
    
    _running = false;
    
    // Останавливаем HTTP сервер
    if (_httpLibServer) {
        if (auto* server = static_cast<httplib::Server*>(_httpLibServer.get())) {
    server->stop();
        }
    }
    
    // Ждем завершения потока
    if (_serverThread.joinable()) {
        _serverThread.join();
    }
    
    _logger->info("HTTP server stopped");
}

bool HttpServer::isRunning() const {
    return _running;
}

void HttpServer::setupRoutes() const {
    auto* server = static_cast<httplib::Server*>(_httpLibServer.get());
    if (!server) {
        _logger->error("HTTP server not initialized");
        return;
    }
    
    // Маршрут для проверки статуса абонента
    server->Get("/check_subscriber", [this](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("imsi")) {
            _logger->warn("Missing IMSI parameter in check_subscriber request");
            res.status = 400;
            res.set_content("Missing IMSI parameter", "text/plain");
            return;
        }
        
        std::string imsi = req.get_param_value("imsi");
        std::string response;
        handleCheckSubscriber(imsi, response);
        res.set_content(response, "text/plain");
    });
    
    // Маршрут для остановки сервера
    server->Get("/stop", [this](const httplib::Request& req, httplib::Response& res) {
        _logger->info("Received stop request from " + req.remote_addr);
        std::string response;
        handleStopCommand(response);
        res.set_content(response, "text/plain");
    });
    
    // Маршрут для проверки работоспособности сервера
    server->Get("/health", [this](const httplib::Request& req, httplib::Response& res) {
        _logger->debug("Received health check from " + req.remote_addr);
        res.set_content("OK", "text/plain");
    });
    
    // Обработчик корневого маршрута
    server->Get("/", [this](const httplib::Request& req, httplib::Response& res) {
        _logger->debug("Received request to root endpoint from " + req.remote_addr);
        res.set_content("Mini-PGW API Server", "text/plain");
    });
    
    // Обработчик для всех остальных маршрутов
    server->set_error_handler([this](const httplib::Request& req, httplib::Response& res) {
        _logger->warn("Invalid request to " + req.path + " from " + req.remote_addr);
        res.status = 404;
        res.set_content("Not Found", "text/plain");
    });

}

void HttpServer::handleCheckSubscriber(const std::string& imsi, std::string& response) const {
    _logger->info("Checking subscriber status for IMSI: " + imsi);
    
    if (_sessionManager->isSessionActive(imsi)) {
        response = "active";
    } else {
        response = "not active";
    }
        
        _logger->info("Subscriber status for IMSI " + imsi + ": " + response);
}

void HttpServer::handleStopCommand(std::string& response) const {
    _logger->info("Received stop command");
    
    // Вызываем callback для остановки приложения
    // Callback сам инициирует graceful shutdown
    if (_onStopRequested) {
        _onStopRequested();
        response = "Graceful shutdown initiated";
    } else {
        // Если callback не установлен, инициируем shutdown напрямую
        if (_shutdownManager->initiateShutdown()) {
            response = "Graceful shutdown initiated";
        } else {
            response = "Shutdown already in progress";
        }
    }
} 