#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <chrono>
#include "../../application/SessionManager.h"
#include "../../utils/Logger.h"
#include "../../domain/Session.h"
#include "../../domain/Blacklist.h"
#include "../../persistence/InMemorySessionRepository.h"
#include "../../persistence/FileCdrRepository.h"
#include "../../application/RateLimiter.h"

class SessionManagerTest : public ::testing::Test {
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
        rateLimiter = std::make_shared<RateLimiter>(6000, logger);
        
        // Создаем менеджер сессий
        sessionManager = std::make_unique<SessionManager>(
            sessionRepo, cdrRepo, blacklist, rateLimiter, logger);
        
        // Тестовые IMSI
        validImsi = "123456789012345";
        blacklistedImsi = "987654321098765";
        
        // Добавляем IMSI в черный список
        std::vector<std::string> blacklistImsis = {blacklistedImsi};
        blacklist->setBlacklist(blacklistImsis);
    }

    void TearDown() override {
        // Очищаем репозиторий
        sessionRepo->clear();
    }

    std::shared_ptr<Logger> logger;
    std::shared_ptr<InMemorySessionRepository> sessionRepo;
    std::shared_ptr<FileCdrRepository> cdrRepo;
    std::shared_ptr<Blacklist> blacklist;
    std::shared_ptr<RateLimiter> rateLimiter;
    std::unique_ptr<SessionManager> sessionManager;
    std::string validImsi;
    std::string blacklistedImsi;
};

TEST_F(SessionManagerTest, Constructor) {
    // Проверяем, что конструктор не выбрасывает исключений с корректными параметрами
    EXPECT_NO_THROW({
        SessionManager manager(sessionRepo, cdrRepo, blacklist, rateLimiter, logger);
    });
}

TEST_F(SessionManagerTest, ConstructorWithNullSessionRepo) {
    // Проверяем, что конструктор выбрасывает исключение с nullptr для sessionRepo
    EXPECT_THROW({
        SessionManager manager(nullptr, cdrRepo, blacklist, rateLimiter, logger);
    }, std::invalid_argument);
}

TEST_F(SessionManagerTest, ConstructorWithNullCdrRepo) {
    // Проверяем, что конструктор выбрасывает исключение с nullptr для cdrRepo
    EXPECT_THROW({
        SessionManager manager(sessionRepo, nullptr, blacklist, rateLimiter, logger);
    }, std::invalid_argument);
}

TEST_F(SessionManagerTest, ConstructorWithNullBlacklist) {
    // Проверяем, что конструктор выбрасывает исключение с nullptr для blacklist
    EXPECT_THROW({
        SessionManager manager(sessionRepo, cdrRepo, nullptr, rateLimiter, logger);
    }, std::invalid_argument);
}

TEST_F(SessionManagerTest, ConstructorWithNullRateLimiter) {
    // Проверяем, что конструктор выбрасывает исключение с nullptr для rateLimiter
    EXPECT_THROW({
        SessionManager manager(sessionRepo, cdrRepo, blacklist, nullptr, logger);
    }, std::invalid_argument);
}

TEST_F(SessionManagerTest, ConstructorWithNullLogger) {
    // Проверяем, что конструктор выбрасывает исключение с nullptr для logger
    EXPECT_THROW({
        SessionManager manager(sessionRepo, cdrRepo, blacklist, rateLimiter, nullptr);
    }, std::invalid_argument);
}

TEST_F(SessionManagerTest, CreateSession) {
    // Создаем сессию с валидным IMSI
    SessionResult result = sessionManager->createSession(validImsi);
    
    // Проверяем, что сессия успешно создана
    EXPECT_EQ(result, SessionResult::CREATED);
    EXPECT_TRUE(sessionManager->isSessionActive(validImsi));
    EXPECT_EQ(sessionManager->getActiveSessionsCount(), 1);
}

TEST_F(SessionManagerTest, CreateSessionWithBlacklistedImsi) {
    // Пытаемся создать сессию с IMSI из черного списка
    SessionResult result = sessionManager->createSession(blacklistedImsi);
    
    // Проверяем, что сессия отклонена
    EXPECT_EQ(result, SessionResult::REJECTED);
    EXPECT_FALSE(sessionManager->isSessionActive(blacklistedImsi));
    EXPECT_EQ(sessionManager->getActiveSessionsCount(), 0);
}

TEST_F(SessionManagerTest, CreateDuplicateSession) {
    // Создаем сессию
    SessionResult result1 = sessionManager->createSession(validImsi);
    
    // Пытаемся создать сессию с тем же IMSI
    SessionResult result2 = sessionManager->createSession(validImsi);
    
    // Проверяем, что обе операции успешны (вторая считается успешной, т.к. сессия уже существует)
    EXPECT_EQ(result1, SessionResult::CREATED);
    EXPECT_EQ(result2, SessionResult::CREATED);
    EXPECT_TRUE(sessionManager->isSessionActive(validImsi));
    EXPECT_EQ(sessionManager->getActiveSessionsCount(), 1);
}

TEST_F(SessionManagerTest, IsSessionActive) {
    // Проверяем, что сессия изначально не активна
    EXPECT_FALSE(sessionManager->isSessionActive(validImsi));
    
    // Создаем сессию
    sessionManager->createSession(validImsi);
    
    // Проверяем, что сессия стала активной
    EXPECT_TRUE(sessionManager->isSessionActive(validImsi));
}

TEST_F(SessionManagerTest, RemoveSession) {
    // Создаем сессию
    sessionManager->createSession(validImsi);
    
    // Проверяем, что сессия активна
    EXPECT_TRUE(sessionManager->isSessionActive(validImsi));
    
    // Удаляем сессию
    bool result = sessionManager->removeSession(validImsi, "test_remove");
    
    // Проверяем, что сессия успешно удалена
    EXPECT_TRUE(result);
    EXPECT_FALSE(sessionManager->isSessionActive(validImsi));
    EXPECT_EQ(sessionManager->getActiveSessionsCount(), 0);
}

TEST_F(SessionManagerTest, RemoveNonExistentSession) {
    // Пытаемся удалить несуществующую сессию
    bool result = sessionManager->removeSession(validImsi, "test_remove");
    
    // Проверяем, что операция не выполнена
    EXPECT_FALSE(result);
}

TEST_F(SessionManagerTest, CleanExpiredSessions) {
    // Создаем сессию
    SessionResult result = sessionManager->createSession(validImsi);
    EXPECT_EQ(result, SessionResult::CREATED);
    
    // Проверяем, что сессия активна
    EXPECT_TRUE(sessionManager->isSessionActive(validImsi));
    
    // Ждем, чтобы сессия истекла 
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Очищаем истекшие сессии с таймаутом 1 секунда
    size_t removedCount = sessionManager->cleanExpiredSessions(std::chrono::seconds(1));
    
    // Проверяем, что сессия удалена
    EXPECT_EQ(removedCount, 1);
    EXPECT_FALSE(sessionManager->isSessionActive(validImsi));
    EXPECT_EQ(sessionManager->getActiveSessionsCount(), 0);
}

TEST_F(SessionManagerTest, CleanExpiredSessionsWithNonExpired) {
    // Создаем сессию
    sessionManager->createSession(validImsi);
    
    // Проверяем, что сессия активна
    EXPECT_TRUE(sessionManager->isSessionActive(validImsi));
    
    // Очищаем истекшие сессии с большим таймаутом
    size_t removedCount = sessionManager->cleanExpiredSessions(std::chrono::seconds(30));
    
    // Проверяем, что сессия не удалена
    EXPECT_EQ(removedCount, 0);
    EXPECT_TRUE(sessionManager->isSessionActive(validImsi));
    EXPECT_EQ(sessionManager->getActiveSessionsCount(), 1);
}

TEST_F(SessionManagerTest, GetActiveSessionsCount) {
    // Проверяем, что изначально нет активных сессий
    EXPECT_EQ(sessionManager->getActiveSessionsCount(), 0);
    
    // Создаем сессию
    sessionManager->createSession(validImsi);
    
    // Проверяем, что количество активных сессий увеличилось
    EXPECT_EQ(sessionManager->getActiveSessionsCount(), 1);
    
    // Создаем еще одну сессию
    std::string anotherImsi = "234567890123456";
    sessionManager->createSession(anotherImsi);
    
    // Проверяем, что количество активных сессий увеличилось
    EXPECT_EQ(sessionManager->getActiveSessionsCount(), 2);
    
    // Удаляем сессию
    sessionManager->removeSession(validImsi, "test_remove");
    
    // Проверяем, что количество активных сессий уменьшилось
    EXPECT_EQ(sessionManager->getActiveSessionsCount(), 1);
}

TEST_F(SessionManagerTest, GetAllActiveImsis) {
    // Проверяем, что изначально нет активных IMSI
    EXPECT_TRUE(sessionManager->getAllActiveImsis().empty());
    
    // Создаем сессии
    std::string imsi1 = validImsi;
    std::string imsi2 = "234567890123456";
    sessionManager->createSession(imsi1);
    sessionManager->createSession(imsi2);
    
    // Получаем список активных IMSI
    auto activeImsis = sessionManager->getAllActiveImsis();
    
    // Проверяем, что список содержит оба IMSI
    EXPECT_EQ(activeImsis.size(), 2);
    EXPECT_TRUE(std::find(activeImsis.begin(), activeImsis.end(), imsi1) != activeImsis.end());
    EXPECT_TRUE(std::find(activeImsis.begin(), activeImsis.end(), imsi2) != activeImsis.end());
}

TEST_F(SessionManagerTest, RateLimiting) {
    // Создаем менеджер сессий с очень низким лимитом запросов
    // 6 запросов в минуту = 0.1 запроса в секунду = 1 токен в 10 секунд
    // При этом максимальный размер бакета = 1 токен
    auto lowRateLimiter = std::make_shared<RateLimiter>(6, logger);
    auto limitedSessionManager = std::make_unique<SessionManager>(
        sessionRepo, cdrRepo, blacklist, lowRateLimiter, logger);
    
    // Создаем сессию (используя 1 токен)
    SessionResult result1 = limitedSessionManager->createSession(validImsi);
    
    // Проверяем, что сессия создана
    EXPECT_EQ(result1, SessionResult::CREATED);
    
    // Удаляем сессию, чтобы можно было создать её снова
    bool removeResult = limitedSessionManager->removeSession(validImsi, "test_remove");
    EXPECT_TRUE(removeResult);
    
    // Пытаемся создать сессию снова сразу после удаления (токенов больше нет)
    SessionResult result2 = limitedSessionManager->createSession(validImsi);
    
    // Проверяем, что вторая сессия отклонена из-за ограничения скорости
    EXPECT_EQ(result2, SessionResult::REJECTED);
}
