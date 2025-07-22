#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <chrono>
#include "../../application/RateLimiter.h"
#include "../../utils/Logger.h"

class RateLimiterTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Создаем тестовые IMSI
        imsi1 = "123456789012345";
        imsi2 = "234567890123456";
        
        // Создаем логгер для тестов
        logger = std::make_shared<Logger>("", LogLevel::LOG_DEBUG);
    }

    std::string imsi1;
    std::string imsi2;
    std::shared_ptr<Logger> logger;
};

TEST_F(RateLimiterTest, ConstructorWithoutLogger) {
    // Создаем ограничитель скорости запросов без логгера
    EXPECT_NO_THROW({
        RateLimiter limiter(100);
    });
}

TEST_F(RateLimiterTest, ConstructorWithLogger) {
    // Создаем ограничитель скорости запросов с логгером
    EXPECT_NO_THROW({
        RateLimiter limiter(100, logger);
    });
}

TEST_F(RateLimiterTest, AllowRequestWithinLimit) {
    // Создаем ограничитель скорости запросов с лимитом 60 запросов в минуту
    RateLimiter limiter(60, logger);
    
    // Проверяем, что первый запрос разрешен
    EXPECT_TRUE(limiter.allowRequest(imsi1));
}

TEST_F(RateLimiterTest, AllowRequestExceedingLimit) {
    // Создаем ограничитель скорости запросов с лимитом 60 запросов в минуту
    // Максимальный размер бакета = 6 токенов
    RateLimiter limiter(60, logger);
    
    // Используем все токены
    for (int i = 0; i < 6; i++) {
        EXPECT_TRUE(limiter.allowRequest(imsi1));
    }
    
    // Проверяем, что следующий запрос отклоняется (токены закончились)
    EXPECT_FALSE(limiter.allowRequest(imsi1));
    
    // Ждем 1.1 секунды (чуть больше, чем время пополнения одного токена)
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    
    // Проверяем, что после ожидания запрос снова разрешен (пополнился 1 токен)
    EXPECT_TRUE(limiter.allowRequest(imsi1));
}

TEST_F(RateLimiterTest, AllowRequestMultipleImsis) {
    // Создаем ограничитель скорости запросов с лимитом 60 запросов в минуту
    // Максимальный размер бакета = 6 токенов
    RateLimiter limiter(60, logger);
    
    // Используем все токены для первого IMSI
    for (int i = 0; i < 6; i++) {
        EXPECT_TRUE(limiter.allowRequest(imsi1));
    }
    
    // Используем все токены для второго IMSI
    for (int i = 0; i < 6; i++) {
        EXPECT_TRUE(limiter.allowRequest(imsi2));
    }
    
    // Проверяем, что следующий запрос для первого IMSI отклоняется (токены закончились)
    EXPECT_FALSE(limiter.allowRequest(imsi1));
    
    // Проверяем, что следующий запрос для второго IMSI также отклоняется (токены закончились)
    EXPECT_FALSE(limiter.allowRequest(imsi2));
}

TEST_F(RateLimiterTest, TokenRefill) {
    // Создаем ограничитель скорости запросов с лимитом 300 запросов в минуту
    // Максимальный размер бакета = 30 токенов
    RateLimiter limiter(300, logger);
    
    // Используем все токены (максимум 30 для лимита 300 запросов в минуту)
    for (int i = 0; i < 30; i++) {
        EXPECT_TRUE(limiter.allowRequest(imsi1));
    }
    
    // Проверяем, что следующий запрос отклоняется (токены закончились)
    EXPECT_FALSE(limiter.allowRequest(imsi1));
    
    // Ждем 0.2 секунды
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Проверяем, что запрос разрешен (пополнился 1 токен)
    EXPECT_TRUE(limiter.allowRequest(imsi1));
    
    // Проверяем, что следующий запрос отклоняется (токен использован)
    EXPECT_FALSE(limiter.allowRequest(imsi1));
    
    // Ждем 0.2 секунды (должен пополниться 1 токен)
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Проверяем, что запрос разрешен (пополнился 1 токен)
    EXPECT_TRUE(limiter.allowRequest(imsi1));
    
    // Проверяем, что следующий запрос отклоняется (токен использован)
    EXPECT_FALSE(limiter.allowRequest(imsi1));
}

TEST_F(RateLimiterTest, HighRateLimit) {
    // Создаем ограничитель скорости запросов с высоким лимитом
    RateLimiter limiter(6000, logger);
    
    // Проверяем, что можно сделать несколько запросов подряд
    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(limiter.allowRequest(imsi1));
    }
    
    // Проверяем, что можно сделать еще несколько запросов для другого IMSI
    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(limiter.allowRequest(imsi2));
    }
}

TEST_F(RateLimiterTest, LowRateLimit) {
    // Создаем ограничитель скорости запросов с низким лимитом
    RateLimiter limiter(6, logger);
    
    // Проверяем, что первый запрос разрешен
    EXPECT_TRUE(limiter.allowRequest(imsi1));
    
    // Проверяем, что второй запрос сразу после первого отклоняется
    EXPECT_FALSE(limiter.allowRequest(imsi1));
    
    // Ждем 10.1 секунды (должен пополниться 1 токен)
    std::this_thread::sleep_for(std::chrono::milliseconds(10100));
    
    // Проверяем, что запрос разрешен
    EXPECT_TRUE(limiter.allowRequest(imsi1));
}
