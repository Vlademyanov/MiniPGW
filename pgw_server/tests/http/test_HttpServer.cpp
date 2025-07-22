#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <chrono>
#include <httplib.h>
#include "../../http/HttpServer.h"
#include "../../application/SessionManager.h"
#include "../../application/RateLimiter.h"
#include "../../application/GracefulShutdownManager.h"
#include "../../utils/Logger.h"
#include "../../domain/Session.h"
#include "../../domain/Blacklist.h"
#include "../../persistence/InMemorySessionRepository.h"
#include "../../persistence/FileCdrRepository.h"

class HttpServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Создаем логгер для тестов
        logger = std::make_shared<Logger>("", LogLevel::LOG_DEBUG);
        
        // Создаем репозитории
        sessionRepo = std::make_shared<InMemorySessionRepository>(logger);
        cdrRepo = std::make_shared<FileCdrRepository>("test_cdr.log", logger);
        
        // Создаем черный список
        blacklist = std::make_shared<Blacklist>();
        
        // Создаем ограничитель скорости запросов
        rateLimiter = std::make_shared<RateLimiter>(100, logger);
        
        // Создаем менеджер сессий
        sessionManager = std::make_shared<SessionManager>(
            sessionRepo, cdrRepo, blacklist, rateLimiter, logger);
        
        // Создаем менеджер плавного завершения
        shutdownManager = std::make_shared<GracefulShutdownManager>(
            sessionManager, 10, logger);
        
        // Флаг для проверки вызова callback при остановке
        stopCallbackCalled = false;
        
        // Создаем HTTP-сервер
        httpServer = std::make_unique<HttpServer>(
            "127.0.0.1", 8081, sessionManager, shutdownManager, logger,
            [this]() { stopCallbackCalled = true; });
    }

    void TearDown() override {
        // Останавливаем сервер, если он запущен
        if (httpServer && httpServer->isRunning()) {
            httpServer->stop();
        }
    }

    std::shared_ptr<Logger> logger;
    std::shared_ptr<InMemorySessionRepository> sessionRepo;
    std::shared_ptr<FileCdrRepository> cdrRepo;
    std::shared_ptr<Blacklist> blacklist;
    std::shared_ptr<RateLimiter> rateLimiter;
    std::shared_ptr<SessionManager> sessionManager;
    std::shared_ptr<GracefulShutdownManager> shutdownManager;
    std::unique_ptr<HttpServer> httpServer;
    bool stopCallbackCalled;
};

TEST_F(HttpServerTest, Constructor) {
    // Проверяем, что конструктор не выбрасывает исключений
    EXPECT_NO_THROW({
        HttpServer server("127.0.0.1", 8082, sessionManager, shutdownManager, logger);
    });
}

TEST_F(HttpServerTest, StartAndStop) {
    // Проверяем, что сервер изначально не запущен
    EXPECT_FALSE(httpServer->isRunning());
    
    // Запускаем сервер
    EXPECT_TRUE(httpServer->start());
    
    // Проверяем, что сервер запущен
    EXPECT_TRUE(httpServer->isRunning());
    
    // Останавливаем сервер
    httpServer->stop();
    
    // Проверяем, что сервер остановлен
    EXPECT_FALSE(httpServer->isRunning());
}

TEST_F(HttpServerTest, DoubleStart) {
    // Запускаем сервер
    EXPECT_TRUE(httpServer->start());
    
    // Пытаемся запустить сервер повторно
    EXPECT_FALSE(httpServer->start());
    
    // Останавливаем сервер
    httpServer->stop();
}

TEST_F(HttpServerTest, StopCallback) {
    // Запускаем сервер
    EXPECT_TRUE(httpServer->start());
    
    // Проверяем, что callback еще не вызван
    EXPECT_FALSE(stopCallbackCalled);
    
    httpServer->stop();
    

}

TEST_F(HttpServerTest, CheckSubscriberEndpoint) {
    // Запускаем сервер
    EXPECT_TRUE(httpServer->start());
    
    // Создаем сессию
    std::string imsi = "123456789012345";
    SessionResult result = sessionManager->createSession(imsi);
    EXPECT_EQ(result, SessionResult::CREATED);
    
    // Отправляем HTTP-запрос
    httplib::Client client("127.0.0.1", httpServer->getPort());
    auto response = client.Get("/check_subscriber?imsi=" + imsi);
    
    // Проверяем ответ
    ASSERT_TRUE(response != nullptr);
    EXPECT_EQ(response->status, 200);
    EXPECT_EQ(response->body, "active");
    
    // Останавливаем сервер
    httpServer->stop();
}

TEST_F(HttpServerTest, StopEndpoint) {
    // Запускаем сервер
    EXPECT_TRUE(httpServer->start());
    
    // Проверяем, что callback еще не вызван
    EXPECT_FALSE(stopCallbackCalled);
    
    // Отправляем HTTP-запрос к /stop
    httplib::Client client("127.0.0.1", httpServer->getPort());
    auto response = client.Get("/stop");
    
    // Проверяем ответ
    ASSERT_TRUE(response != nullptr);
    EXPECT_EQ(response->status, 200);
    EXPECT_EQ(response->body, "Graceful shutdown initiated");
    
    // Проверяем, что callback был вызван
    EXPECT_TRUE(stopCallbackCalled);
    
    // Останавливаем сервер
    httpServer->stop();
}

TEST_F(HttpServerTest, HealthEndpoint) {
    // Запускаем сервер
    EXPECT_TRUE(httpServer->start());
    
    // Отправляем HTTP-запрос к /health
    httplib::Client client("127.0.0.1", httpServer->getPort());
    auto response = client.Get("/health");
    
    // Проверяем ответ
    ASSERT_TRUE(response != nullptr);
    EXPECT_EQ(response->status, 200);
    EXPECT_EQ(response->body, "OK");
    
    // Останавливаем сервер
    httpServer->stop();
}

TEST_F(HttpServerTest, NotFoundEndpoint) {
    // Запускаем сервер
    EXPECT_TRUE(httpServer->start());
    
    // Отправляем HTTP-запрос к несуществующему маршруту
    httplib::Client client("127.0.0.1", httpServer->getPort());
    auto response = client.Get("/non_existent_route");
    
    // Проверяем ответ
    ASSERT_TRUE(response != nullptr);
    EXPECT_EQ(response->status, 404);
    EXPECT_EQ(response->body, "Not Found");
    
    // Останавливаем сервер
    httpServer->stop();
}

TEST_F(HttpServerTest, RootEndpoint) {
    // Запускаем сервер
    EXPECT_TRUE(httpServer->start());
    
    // Отправляем HTTP-запрос к корневому маршруту
    httplib::Client client("127.0.0.1", httpServer->getPort());
    auto response = client.Get("/");
    
    // Проверяем ответ
    ASSERT_TRUE(response != nullptr);
    EXPECT_EQ(response->status, 200);
    EXPECT_EQ(response->body, "Mini-PGW API Server");
    
    // Останавливаем сервер
    httpServer->stop();
}
