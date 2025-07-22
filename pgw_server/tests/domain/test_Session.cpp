#include <gtest/gtest.h>
#include <memory>
#include <chrono>
#include <thread>
#include "../../domain/Session.h"
#include "../../utils/Logger.h"

class SessionTest : public ::testing::Test {
protected:
    void SetUp() override {
        validImsi = "123456789012345"; // 15 цифр
        invalidImsi = "12345"; // Неверная длина
        logger = std::make_shared<Logger>("", LogLevel::LOG_DEBUG);
    }

    std::string validImsi;
    std::string invalidImsi;
    std::shared_ptr<Logger> logger;
};

TEST_F(SessionTest, ConstructorWithValidImsi) {
    // Проверка, что конструктор не выбрасывает исключений с корректным IMSI
    EXPECT_NO_THROW({
        Session session(validImsi);
    });
    
    // Проверка, что конструктор с логгером также работает
    EXPECT_NO_THROW({
        Session session(validImsi, logger);
    });
}

TEST_F(SessionTest, ConstructorWithInvalidImsi) {
    // Проверка, что конструктор выбрасывает исключение с некорректным IMSI
    EXPECT_THROW({
        Session session(invalidImsi);
    }, std::invalid_argument);
    
    // Проверка, что конструктор с логгером также выбрасывает исключение
    EXPECT_THROW({
        Session session(invalidImsi, logger);
    }, std::invalid_argument);
}

TEST_F(SessionTest, GetImsi) {
    Session session(validImsi);
    EXPECT_EQ(session.getImsi(), validImsi);
}

TEST_F(SessionTest, GetCreatedAt) {
    // Запоминаем текущее время
    auto now = std::chrono::system_clock::now();
    
    // Создаем сессию
    Session session(validImsi);
    
    // Проверяем, что время создания близко к текущему времени
    auto sessionTime = session.getCreatedAt();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(sessionTime - now).count();

    EXPECT_LT(std::abs(diff), 100); // Допускаем разницу до 100 мс
}

TEST_F(SessionTest, IsExpired) {
    Session session(validImsi);
    
    // Сразу после создания сессия не должна быть истекшей
    EXPECT_FALSE(session.isExpired(std::chrono::seconds(10)));
    
    // Ждем 2 секунды
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Сессия должна истечь с таймаутом 1 секунда
    EXPECT_TRUE(session.isExpired(std::chrono::seconds(1)));
    
    // Сессия не должна истечь с таймаутом 10 секунд
    EXPECT_FALSE(session.isExpired(std::chrono::seconds(10)));
}

TEST_F(SessionTest, GetAge) {
    Session session(validImsi);
    
    // Ждем 2 секунды
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Проверяем возраст сессии
    auto age = session.getAge();
    
    // Возраст должен быть около 2 секунд
    EXPECT_GE(age.count(), 1);
    EXPECT_LE(age.count(), 3); // Погрешность
}

TEST_F(SessionTest, CopyConstructor) {
    Session original(validImsi);
    
    // Ждем 1 секунду, чтобы убедиться, что время создания отличается от текущего
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // Копируем сессию
    Session copy = original;
    
    // Проверяем, что IMSI и время создания скопированы правильно
    EXPECT_EQ(copy.getImsi(), original.getImsi());
    EXPECT_EQ(copy.getCreatedAt(), original.getCreatedAt());
}

TEST_F(SessionTest, MoveConstructor) {
    Session original(validImsi);
    auto originalImsi = original.getImsi();
    auto originalCreatedAt = original.getCreatedAt();
    
    // Перемещаем сессию
    Session moved = std::move(original);
    
    // Проверяем, что IMSI и время создания перемещены правильно
    EXPECT_EQ(moved.getImsi(), originalImsi);
    EXPECT_EQ(moved.getCreatedAt(), originalCreatedAt);
}

TEST_F(SessionTest, CopyAssignment) {
    Session original(validImsi);
    Session copy("987654321098765"); // Другой IMSI
    
    // Ждем 1 секунду, чтобы убедиться, что время создания отличается
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // Копируем сессию через оператор присваивания
    copy = original;
    
    // Проверяем, что IMSI и время создания скопированы правильно
    EXPECT_EQ(copy.getImsi(), original.getImsi());
    EXPECT_EQ(copy.getCreatedAt(), original.getCreatedAt());
}

TEST_F(SessionTest, MoveAssignment) {
    Session original(validImsi);
    auto originalImsi = original.getImsi();
    auto originalCreatedAt = original.getCreatedAt();
    
    Session moved("987654321098765");
    
    // Перемещаем сессию через оператор присваивания
    moved = std::move(original);
    
    // Проверяем, что IMSI и время создания перемещены правильно
    EXPECT_EQ(moved.getImsi(), originalImsi);
    EXPECT_EQ(moved.getCreatedAt(), originalCreatedAt);
}
