#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <chrono>
#include "../../application/SessionCleaner.h"
#include "../../application/SessionManager.h"
#include "../../application/RateLimiter.h"
#include "../../utils/Logger.h"
#include "../../domain/Session.h"
#include "../../domain/Blacklist.h"
#include "../../persistence/InMemorySessionRepository.h"
#include "../../persistence/FileCdrRepository.h"

class SessionCleanerTest : public ::testing::Test {
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
        
        // Создаем менеджер сессий с коротким таймаутом для тестирования
        sessionManager = std::make_shared<SessionManager>(
            sessionRepo, cdrRepo, blacklist, rateLimiter, logger);
        
        // Создаем очиститель сессий с коротким интервалом очистки
        sessionCleaner = std::make_unique<SessionCleaner>(
            sessionManager, std::chrono::seconds(2), logger, std::chrono::seconds(1));
    }

    void TearDown() override {
        // Останавливаем очиститель, если он запущен
        sessionCleaner->stop();
    }

    std::shared_ptr<Logger> logger;
    std::shared_ptr<InMemorySessionRepository> sessionRepo;
    std::shared_ptr<FileCdrRepository> cdrRepo;
    std::shared_ptr<Blacklist> blacklist;
    std::shared_ptr<RateLimiter> rateLimiter;
    std::shared_ptr<SessionManager> sessionManager;
    std::unique_ptr<SessionCleaner> sessionCleaner;
};

TEST_F(SessionCleanerTest, Constructor) {
    // Проверяем, что конструктор не выбрасывает исключений
    EXPECT_NO_THROW({
        SessionCleaner cleaner(sessionManager, std::chrono::seconds(5), logger);
    });
}

TEST_F(SessionCleanerTest, StartAndStop) {
    // Запускаем очиститель
    bool result = sessionCleaner->start();
    
    // Проверяем, что запуск успешен
    EXPECT_TRUE(result);
    
    // Останавливаем очиститель
    sessionCleaner->stop();
}

TEST_F(SessionCleanerTest, DoubleStart) {
    // Запускаем очиститель
    bool result1 = sessionCleaner->start();
    
    // Пытаемся запустить очиститель повторно
    bool result2 = sessionCleaner->start();
    
    // Проверяем, что первый запуск успешен, а второй - нет
    EXPECT_TRUE(result1);
    EXPECT_FALSE(result2);
    
    // Останавливаем очиститель
    sessionCleaner->stop();
}

TEST_F(SessionCleanerTest, CleanExpiredSessions) {
    // Создаем сессии
    std::string imsi1 = "123456789012345";
    std::string imsi2 = "234567890123456";
    
    sessionManager->createSession(imsi1);
    sessionManager->createSession(imsi2);
    
    // Проверяем, что сессии созданы
    EXPECT_TRUE(sessionRepo->sessionExists(imsi1));
    EXPECT_TRUE(sessionRepo->sessionExists(imsi2));
    
    // Запускаем очиститель
    sessionCleaner->start();
    
    // Ждем, чтобы сессии истекли и были очищены
    std::this_thread::sleep_for(std::chrono::seconds(4));
    
    // Проверяем, что сессии удалены
    EXPECT_FALSE(sessionRepo->sessionExists(imsi1));
    EXPECT_FALSE(sessionRepo->sessionExists(imsi2));
    
    // Останавливаем очиститель
    sessionCleaner->stop();
}

TEST_F(SessionCleanerTest, CleanExpiredSessionsWithNonExpired) {
    // Создаем сессии
    std::string imsi1 = "123456789012345";
    std::string imsi2 = "234567890123456";
    
    SessionResult result1 = sessionManager->createSession(imsi1);
    EXPECT_EQ(result1, SessionResult::CREATED);
    
    // Проверяем, что первая сессия создана
    EXPECT_TRUE(sessionRepo->sessionExists(imsi1));
    
    // Запускаем очиститель
    sessionCleaner->start();
    
    // Ждем, чтобы первая сессия истекла и была гарантированно очищена
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    // Создаем вторую сессию
    SessionResult result2 = sessionManager->createSession(imsi2);
    EXPECT_EQ(result2, SessionResult::CREATED);
    
    // Проверяем, что первая сессия удалена, а вторая - нет
    EXPECT_FALSE(sessionRepo->sessionExists(imsi1));
    EXPECT_TRUE(sessionRepo->sessionExists(imsi2));
    
    // Останавливаем очиститель
    sessionCleaner->stop();
}

TEST_F(SessionCleanerTest, StopCleaner) {
    // Создаем сессию
    std::string imsi = "123456789012345";
    sessionManager->createSession(imsi);
    
    // Запускаем очиститель
    sessionCleaner->start();
    
    // Останавливаем очиститель до истечения сессии
    sessionCleaner->stop();
    
    // Ждем, чтобы сессия истекла
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // Проверяем, что сессия все еще существует (не была очищена)
    EXPECT_TRUE(sessionRepo->sessionExists(imsi));
}
