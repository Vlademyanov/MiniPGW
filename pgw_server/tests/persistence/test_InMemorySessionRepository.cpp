#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <chrono>
#include "../../persistence/InMemorySessionRepository.h"
#include "../../domain/Session.h"
#include "../../utils/Logger.h"

class InMemorySessionRepositoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Создаем тестовые IMSI
        imsi1 = "123456789012345";
        imsi2 = "234567890123456";
        
        // Создаем логгер для тестов
        logger = std::make_shared<Logger>("", LogLevel::LOG_DEBUG);
        
        // Создаем репозиторий
        repository = std::make_unique<InMemorySessionRepository>(logger);
    }

    void TearDown() override {
        // Очищаем репозиторий после каждого теста
        repository->clear();
    }

    std::string imsi1;
    std::string imsi2;
    std::shared_ptr<Logger> logger;
    std::unique_ptr<InMemorySessionRepository> repository;
};

TEST_F(InMemorySessionRepositoryTest, ConstructorWithoutLogger) {
    // Проверяем, что конструктор без логгера работает
    EXPECT_NO_THROW({
        InMemorySessionRepository repo;
    });
}

TEST_F(InMemorySessionRepositoryTest, ConstructorWithLogger) {
    // Проверяем, что конструктор с логгером работает
    EXPECT_NO_THROW({
        InMemorySessionRepository repo(logger);
    });
}

TEST_F(InMemorySessionRepositoryTest, AddSession) {
    // Создаем сессию
    Session session(imsi1);
    
    // Добавляем сессию в репозиторий
    bool result = repository->addSession(session);
    
    // Проверяем, что сессия добавлена успешно
    EXPECT_TRUE(result);
    EXPECT_TRUE(repository->sessionExists(imsi1));
    EXPECT_EQ(repository->getSessionCount(), 1);
}

TEST_F(InMemorySessionRepositoryTest, AddDuplicateSession) {
    // Создаем сессию
    Session session(imsi1);
    
    // Добавляем сессию в репозиторий
    bool result1 = repository->addSession(session);
    
    // Пытаемся добавить сессию с тем же IMSI
    bool result2 = repository->addSession(session);
    
    // Проверяем, что первое добавление успешно, а второе - нет
    EXPECT_TRUE(result1);
    EXPECT_FALSE(result2);
    EXPECT_EQ(repository->getSessionCount(), 1);
}

TEST_F(InMemorySessionRepositoryTest, RemoveSession) {
    // Создаем и добавляем сессию
    Session session(imsi1);
    repository->addSession(session);
    
    // Удаляем сессию
    bool result = repository->removeSession(imsi1);
    
    // Проверяем, что сессия удалена успешно
    EXPECT_TRUE(result);
    EXPECT_FALSE(repository->sessionExists(imsi1));
    EXPECT_EQ(repository->getSessionCount(), 0);
}

TEST_F(InMemorySessionRepositoryTest, RemoveNonExistentSession) {
    // Пытаемся удалить несуществующую сессию
    bool result = repository->removeSession(imsi1);
    
    // Проверяем, что удаление не выполнено
    EXPECT_FALSE(result);
    EXPECT_EQ(repository->getSessionCount(), 0);
}

TEST_F(InMemorySessionRepositoryTest, SessionExists) {
    // Создаем и добавляем сессию
    Session session(imsi1);
    repository->addSession(session);
    
    // Проверяем наличие сессии
    EXPECT_TRUE(repository->sessionExists(imsi1));
    EXPECT_FALSE(repository->sessionExists(imsi2));
}

TEST_F(InMemorySessionRepositoryTest, GetAllImsis) {
    // Создаем и добавляем две сессии
    Session session1(imsi1);
    Session session2(imsi2);
    repository->addSession(session1);
    repository->addSession(session2);
    
    // Получаем все IMSI
    auto imsis = repository->getAllImsis();
    
    // Проверяем, что получены правильные IMSI
    EXPECT_EQ(imsis.size(), 2);
    EXPECT_TRUE(std::find(imsis.begin(), imsis.end(), imsi1) != imsis.end());
    EXPECT_TRUE(std::find(imsis.begin(), imsis.end(), imsi2) != imsis.end());
}

TEST_F(InMemorySessionRepositoryTest, GetSessionCount) {
    // Проверяем, что изначально репозиторий пуст
    EXPECT_EQ(repository->getSessionCount(), 0);
    
    // Добавляем сессию
    Session session(imsi1);
    repository->addSession(session);
    
    // Проверяем, что количество сессий увеличилось
    EXPECT_EQ(repository->getSessionCount(), 1);
    
    // Добавляем еще одну сессию
    Session session2(imsi2);
    repository->addSession(session2);
    
    // Проверяем, что количество сессий увеличилось
    EXPECT_EQ(repository->getSessionCount(), 2);
    
    // Удаляем сессию
    repository->removeSession(imsi1);
    
    // Проверяем, что количество сессий уменьшилось
    EXPECT_EQ(repository->getSessionCount(), 1);
    
    // Очищаем репозиторий
    repository->clear();
    
    // Проверяем, что репозиторий пуст
    EXPECT_EQ(repository->getSessionCount(), 0);
}

TEST_F(InMemorySessionRepositoryTest, Clear) {
    // Добавляем несколько сессий
    Session session1(imsi1);
    Session session2(imsi2);
    repository->addSession(session1);
    repository->addSession(session2);
    
    // Проверяем, что сессии добавлены
    EXPECT_EQ(repository->getSessionCount(), 2);
    
    // Очищаем репозиторий
    repository->clear();
    
    // Проверяем, что репозиторий пуст
    EXPECT_EQ(repository->getSessionCount(), 0);
    EXPECT_FALSE(repository->sessionExists(imsi1));
    EXPECT_FALSE(repository->sessionExists(imsi2));
}

TEST_F(InMemorySessionRepositoryTest, GetExpiredSessions) {
    // Создаем и добавляем сессию
    Session session(imsi1);
    repository->addSession(session);
    
    // Проверяем, что сразу после создания сессия не истекла
    auto expiredSessions = repository->getExpiredSessions(10);
    EXPECT_EQ(expiredSessions.size(), 0);
    
    // Ждем 2 секунды
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Проверяем, что сессия истекла с таймаутом 1 секунда
    expiredSessions = repository->getExpiredSessions(1);
    EXPECT_EQ(expiredSessions.size(), 1);
    EXPECT_EQ(expiredSessions[0].getImsi(), imsi1);
    
    // Проверяем, что сессия не истекла с таймаутом 10 секунд
    expiredSessions = repository->getExpiredSessions(10);
    EXPECT_EQ(expiredSessions.size(), 0);
}
