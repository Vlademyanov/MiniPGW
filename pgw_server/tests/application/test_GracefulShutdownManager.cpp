#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <chrono>
#include "../../application/GracefulShutdownManager.h"
#include "../../application/SessionManager.h"
#include "../../application/RateLimiter.h"
#include "../../utils/Logger.h"
#include "../../domain/Session.h"
#include "../../domain/Blacklist.h"
#include "../../persistence/InMemorySessionRepository.h"
#include "../../persistence/FileCdrRepository.h"

class GracefulShutdownManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Создаем логгер для тестов
        logger = std::make_shared<Logger>("", LogLevel::LOG_DEBUG);
        
        // Создаем репозитории
        sessionRepo = std::make_shared<InMemorySessionRepository>(logger);
        cdrRepo = std::make_shared<FileCdrRepository>("test_cdr.log", logger);
        
        // Создаем черный список
        blacklist = std::make_shared<Blacklist>();
        
        // Создаем ограничитель скорости запросов с высоким лимитом для тестов
        rateLimiter = std::make_shared<RateLimiter>(10000, logger);
        
        // Создаем менеджер сессий
        sessionManager = std::make_shared<SessionManager>(
            sessionRepo, cdrRepo, blacklist, rateLimiter, logger);
        
        // Создаем менеджер плавного завершения с высокой скоростью удаления для тестов
        shutdownManager = std::make_unique<GracefulShutdownManager>(
            sessionManager, 100, logger);
    }

    void TearDown() override {
        // Ожидаем завершения процесса плавного завершения, если он запущен
        shutdownManager->waitForCompletion(std::chrono::seconds(1));
    }

    std::shared_ptr<Logger> logger;
    std::shared_ptr<InMemorySessionRepository> sessionRepo;
    std::shared_ptr<FileCdrRepository> cdrRepo;
    std::shared_ptr<Blacklist> blacklist;
    std::shared_ptr<RateLimiter> rateLimiter;
    std::shared_ptr<SessionManager> sessionManager;
    std::unique_ptr<GracefulShutdownManager> shutdownManager;
};

TEST_F(GracefulShutdownManagerTest, Constructor) {
    // Проверяем, что конструктор не выбрасывает исключений
    EXPECT_NO_THROW({
        GracefulShutdownManager manager(sessionManager, 10, logger);
    });
}

TEST_F(GracefulShutdownManagerTest, InitiateShutdown) {
    // Инициируем процесс плавного завершения
    bool result = shutdownManager->initiateShutdown();
    
    // Проверяем, что процесс успешно инициирован
    EXPECT_TRUE(result);
    
    // Ожидаем завершения процесса
    bool completed = shutdownManager->waitForCompletion(std::chrono::seconds(1));
    
    // Проверяем, что процесс завершен
    EXPECT_TRUE(completed);
}

TEST_F(GracefulShutdownManagerTest, DoubleInitiateShutdown) {
    // Инициируем процесс плавного завершения
    bool result1 = shutdownManager->initiateShutdown();
    
    // Пытаемся инициировать процесс повторно
    bool result2 = shutdownManager->initiateShutdown();
    
    // Проверяем, что первая инициация успешна, а вторая - нет
    EXPECT_TRUE(result1);
    EXPECT_FALSE(result2);
    
    // Ожидаем завершения процесса
    shutdownManager->waitForCompletion(std::chrono::seconds(1));
}

TEST_F(GracefulShutdownManagerTest, WaitForCompletion) {
    // Создаем несколько сессий
    for (int i = 0; i < 10; ++i) {
        std::string imsi = "12345678901234" + std::to_string(i);
        SessionResult result = sessionManager->createSession(imsi);
        EXPECT_EQ(result, SessionResult::CREATED);
    }
    
    // Проверяем, что сессии созданы
    EXPECT_EQ(sessionRepo->getSessionCount(), 10);
    
    // Инициируем процесс плавного завершения
    shutdownManager->initiateShutdown();
    
    // Ожидаем завершения процесса с таймаутом
    bool completed = shutdownManager->waitForCompletion(std::chrono::seconds(2));
    
    // Проверяем, что процесс завершен и все сессии удалены
    EXPECT_TRUE(completed);
    EXPECT_EQ(sessionRepo->getSessionCount(), 0);
}

TEST_F(GracefulShutdownManagerTest, WaitForCompletionWithTimeout) {
    // Создаем много сессий (больше, чем может быть удалено за короткий таймаут)
    for (int i = 0; i < 1000; ++i) {
        std::string imsi = "12345678901234" + std::to_string(i % 10);
        SessionResult result = sessionManager->createSession(imsi);
        EXPECT_EQ(result, SessionResult::CREATED);
    }
    
    // Создаем менеджер плавного завершения с низкой скоростью удаления
    auto slowShutdownManager = std::make_unique<GracefulShutdownManager>(
        sessionManager, 1, logger);
    
    // Инициируем процесс плавного завершения
    slowShutdownManager->initiateShutdown();
    
    // Ожидаем завершения процесса с очень коротким таймаутом
    bool completed = slowShutdownManager->waitForCompletion(std::chrono::seconds(1));
    
    // Проверяем, что процесс не завершен за указанное время
    EXPECT_FALSE(completed);
}

TEST_F(GracefulShutdownManagerTest, EmptyRepository) {
    // Проверяем, что репозиторий пуст
    EXPECT_EQ(sessionRepo->getSessionCount(), 0);
    
    // Инициируем процесс плавного завершения
    bool result = shutdownManager->initiateShutdown();
    
    // Проверяем, что процесс успешно инициирован
    EXPECT_TRUE(result);
    
    // Ожидаем завершения процесса
    bool completed = shutdownManager->waitForCompletion(std::chrono::seconds(1));
    
    // Проверяем, что процесс завершен
    EXPECT_TRUE(completed);
}
