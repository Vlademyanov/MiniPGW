#include <AppBootstrap.h>
#include <JsonConfigAdapter.h>
#include <UdpServer.h>
#include <HttpServer.h>
#include <SessionManager.h>
#include <GracefulShutdownManager.h>
#include <SessionCleaner.h>
#include <RateLimiter.h>
#include <InMemorySessionRepository.h>
#include <FileCdrRepository.h>
#include <Logger.h>
#include <Blacklist.h>
#include <iostream>
#include <csignal>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <unistd.h>
#include <stdexcept>
#include <filesystem>

#include <ServerMetrics.h>

// Глобальный указатель для обработчика сигналов
static AppBootstrap* g_appBootstrap = nullptr;

// Обработчик сигналов
static void appBootstrapSignalHandler(int signal [[maybe_unused]]) {
    if (g_appBootstrap) {
        g_appBootstrap->initiateShutdown();
    }
}

AppBootstrap::AppBootstrap() {
    // Устанавливаем глобальный указатель для обработчика сигналов
    g_appBootstrap = this;
}

AppBootstrap::~AppBootstrap() {
    // Останавливаем все сервисы, если они еще работают
    if (_running.exchange(false)) {
        stopServices();
    }
    
    // Очищаем глобальный указатель
    if (g_appBootstrap == this) {
        g_appBootstrap = nullptr;
    }
}

void AppBootstrap::initialize() {
    try {
        // Инициализируем компоненты
        setupComponents();
        
        // Регистрируем обработчики сигналов
        std::signal(SIGINT, appBootstrapSignalHandler);
        std::signal(SIGTERM, appBootstrapSignalHandler);
        
        if (_logger) {
            _logger->info("Application initialized");
        }
    } catch (const std::exception& e) {
        if (_logger) {
            _logger->error("Initialization error: " + std::string(e.what()));
        }
        std::cerr << "Initialization error: " << e.what() << std::endl;
        throw;
    }
}

void AppBootstrap::run() {
    if (_running.exchange(true)) {
        if (_logger) {
            _logger->warn("Application already running");
        }
        return;
    }
    
    try {
        // Запускаем сервисы
        startServices();
        
        if (_logger) {
            _logger->info("Application running");
        }
        
        // Ожидаем сигнала завершения
        while (_running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } catch (const std::exception& e) {
        if (_logger) {
            _logger->error("Runtime error: " + std::string(e.what()));
        } else {
            std::cerr << "Runtime error: " << e.what() << std::endl;
        }
        _running = false;
    }
    
    // Останавливаем сервисы
    stopServices();
    
    if (_logger) {
        _logger->info("Application stopped");
    }
}

void AppBootstrap::initiateShutdown() {
    // Проверяем, не запущен ли уже процесс завершения
    if (!_running.load()) {
        if (_logger) {
            _logger->debug("Shutdown already initiated, ignoring repeated call");
        }
        return;
    }
    
    if (_logger) {
        _logger->info("Graceful shutdown initiated");
    }
    
    // Запускаем процесс плавного завершения
    if (_shutdownManager) {
        if (_shutdownManager->initiateShutdown()) {
            if (_logger) {
                _logger->info("Waiting for graceful shutdown to complete...");
            }

            if (_shutdownManager->waitForCompletion()) {
                if (_logger) {
                    _logger->info("All sessions successfully offloaded");
                }
            } else {
                if (_logger) {
                    _logger->warn("Graceful shutdown did not complete as expected");
                }
            }
        } else {
            if (_logger) {
                _logger->warn("Failed to initiate graceful shutdown");
            }
        }
    }
    
    // Устанавливаем флаг завершения работы
    _running = false;
}

std::string AppBootstrap::findConfigFile() {
    // Список возможных путей к конфигурационному файлу
    std::vector<std::string> possiblePaths = {
        "../pgw_server/config/server_config.json",
        "config/server_config.json",
        "pgw_server/config/server_config.json",
        "../config/server_config.json",
        "server_config.json"
    };
    
    // Проверяем каждый путь
    for (const auto& path : possiblePaths) {
        std::ifstream configFile(path);
        if (configFile.is_open()) {
            configFile.close();
            return path;
        }
    }
    
    // Если не нашли файл, выбрасываем исключение
    throw std::runtime_error("Cannot find configuration file");
}

void AppBootstrap::setupComponents() {

    // Находим конфигурационный файл
    std::string configPath = findConfigFile();

    // Создаем конфигурацию
    _config = std::make_unique<JsonConfigAdapter>(configPath);
    _config->load();

    int metrics_port = _config->getUint("metrics_port", 9101);
    ServerMetrics::init(metrics_port);

    // Создаем логгер
    std::string logFile = _config->getString("log_file", "pgw.log");
    std::string logLevelStr = _config->getString("log_level", "INFO");
    _logger = std::make_unique<Logger>(logFile, Logger::stringToLevel(logLevelStr));
    _logger->info("Configuration loaded from: " + configPath);
    
    // Создаем shared_ptr для логгера
    auto logger = createSharedFromUnique(_logger.get());
    
    // Создаем репозитории
    _sessionRepo = std::make_unique<InMemorySessionRepository>(logger);
    _logger->info("Session repository initialized");
    
    std::string cdrFile = _config->getString("cdr_file", "cdr.log");
    _cdrRepo = std::make_unique<FileCdrRepository>(cdrFile, logger);
    
    // Создаем shared_ptr для репозиториев
    auto sessionRepo = createSharedFromUnique(_sessionRepo.get());
    auto cdrRepo = createSharedFromUnique(_cdrRepo.get());
    
    // Создаем черный список
    auto blacklistItems = _config->getStringArray("blacklist");
    _blacklist = std::make_unique<Blacklist>(blacklistItems);
    _logger->info("Blacklist initialized with " + std::to_string(blacklistItems.size()) + " items");
    
    // Создаем shared_ptr для черного списка
    auto blacklist = createSharedFromUnique(_blacklist.get());
    
    // Создаем ограничитель скорости запросов
    uint32_t maxRequestsPerMinute = _config->getUint("max_requests_per_minute", 100);
    _rateLimiter = std::make_unique<RateLimiter>(maxRequestsPerMinute, logger);
    
    // Создаем shared_ptr для ограничителя скорости
    auto rateLimiter = createSharedFromUnique(_rateLimiter.get());
    
    // Создаем менеджер сессий
    _sessionManager = std::make_unique<SessionManager>(
        sessionRepo,
        cdrRepo,
        blacklist,
        rateLimiter,
        logger
    );
    
    // Создаем shared_ptr для менеджера сессий
    auto sessionManager = createSharedFromUnique(_sessionManager.get());
    
    // Создаем очиститель сессий
    uint32_t sessionTimeoutSec = _config->getUint("session_timeout_sec", 30);
    uint32_t cleanupIntervalSec = _config->getUint("cleanup_interval_sec", 5);
    _sessionCleaner = std::make_unique<SessionCleaner>(
        sessionManager,
        std::chrono::seconds(sessionTimeoutSec),
        logger,
        std::chrono::seconds(cleanupIntervalSec)
    );
    
    // Создаем менеджер плавного завершения
    uint32_t gracefulShutdownRate = _config->getUint("graceful_shutdown_rate", 10);
    _shutdownManager = std::make_unique<GracefulShutdownManager>(
        sessionManager,
        gracefulShutdownRate,
        logger
    );

    // Создаем UDP сервер
    std::string serverIp = _config->getString("udp_ip", "0.0.0.0");
    uint16_t udpPort = static_cast<uint16_t>(_config->getUint("udp_port", 9000));
    _udpServer = std::make_unique<UdpServer>(
        serverIp,
        udpPort,
        sessionManager,
        logger
    );
    
    // Создаем HTTP сервер
    uint16_t httpPort = static_cast<uint16_t>(_config->getUint("http_port", 8080));
    auto shutdownManager = createSharedFromUnique(_shutdownManager.get());
    _httpServer = std::make_unique<HttpServer>(
        serverIp,
        httpPort,
        sessionManager,
        shutdownManager,
        logger,
        [this]() { this->initiateShutdown(); }
    );
}

void AppBootstrap::startServices() const {
    // Запускаем очиститель сессий
    if (_sessionCleaner) {
        _sessionCleaner->start();
    }
    
    // Запускаем UDP сервер
    if (_udpServer) {
        if (!_udpServer->start()) {
            throw std::runtime_error("Failed to start UDP server");
        }
    }
    
    // Запускаем HTTP сервер
    if (_httpServer) {
        if (!_httpServer->start()) {
            throw std::runtime_error("Failed to start HTTP server");
        }
    }
}

void AppBootstrap::stopServices() const {
    // Останавливаем HTTP сервер
    if (_httpServer) {
        _httpServer->stop();
    }
    
    // Останавливаем UDP сервер
    if (_udpServer) {
        _udpServer->stop();
    }
    
    // Останавливаем очиститель сессий
    if (_sessionCleaner) {
        _sessionCleaner->stop();
        
    }
    // Останавливаем менеджер плавного завершения
    if (_shutdownManager) {
        _shutdownManager->stop();
    }
}
