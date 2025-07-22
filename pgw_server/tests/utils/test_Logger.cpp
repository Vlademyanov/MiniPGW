#include <gtest/gtest.h>
#include "../../utils/Logger.h"
#include <fstream>
#include <filesystem>
#include <string>

class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Создаем временный файл для тестирования
        tempLogFile = "test_log_file.log";
        
        // Удаляем файл, если он существует
        if (std::filesystem::exists(tempLogFile)) {
            std::filesystem::remove(tempLogFile);
        }
    }

    void TearDown() override {
        // Удаляем временный файл после тестов
        if (std::filesystem::exists(tempLogFile)) {
            std::filesystem::remove(tempLogFile);
        }
    }

    std::string tempLogFile;
};

TEST_F(LoggerTest, DefaultConstructor) {
    Logger logger;
    EXPECT_EQ(logger.getLogLevel(), LogLevel::INFO);
    EXPECT_TRUE(logger.isHealthy());
}

TEST_F(LoggerTest, ConstructorWithParameters) {
    Logger logger(tempLogFile, LogLevel::WARN);
    EXPECT_EQ(logger.getLogLevel(), LogLevel::WARN);
    EXPECT_TRUE(logger.isHealthy());
}

TEST_F(LoggerTest, SetAndGetLogLevel) {
    Logger logger;
    
    logger.setLogLevel(LogLevel::ERROR);
    EXPECT_EQ(logger.getLogLevel(), LogLevel::ERROR);
    
    logger.setLogLevel(LogLevel::LOG_DEBUG);
    EXPECT_EQ(logger.getLogLevel(), LogLevel::LOG_DEBUG);
}

TEST_F(LoggerTest, LevelToStringConversion) {
    EXPECT_EQ(Logger::levelToString(LogLevel::LOG_DEBUG), "DEBUG");
    EXPECT_EQ(Logger::levelToString(LogLevel::INFO), "INFO");
    EXPECT_EQ(Logger::levelToString(LogLevel::WARN), "WARN");
    EXPECT_EQ(Logger::levelToString(LogLevel::ERROR), "ERROR");
    EXPECT_EQ(Logger::levelToString(LogLevel::CRITICAL), "CRITICAL");
    
    // Проверка для недопустимого значения
    LogLevel invalidLevel = static_cast<LogLevel>(99);
    EXPECT_EQ(Logger::levelToString(invalidLevel), "UNKNOWN");
}

TEST_F(LoggerTest, StringToLevelConversion) {
    EXPECT_EQ(Logger::stringToLevel("DEBUG"), LogLevel::LOG_DEBUG);
    EXPECT_EQ(Logger::stringToLevel("INFO"), LogLevel::INFO);
    EXPECT_EQ(Logger::stringToLevel("WARN"), LogLevel::WARN);
    EXPECT_EQ(Logger::stringToLevel("ERROR"), LogLevel::ERROR);
    EXPECT_EQ(Logger::stringToLevel("CRITICAL"), LogLevel::CRITICAL);
    
    // Проверка для недопустимого значения
    EXPECT_EQ(Logger::stringToLevel("INVALID"), LogLevel::INFO);
}

TEST_F(LoggerTest, LoggingMethods) {
    Logger logger(tempLogFile, LogLevel::LOG_DEBUG);
    
    // Проверяем, что все методы логирования не вызывают исключений
    EXPECT_NO_THROW(logger.debug("Debug message"));
    EXPECT_NO_THROW(logger.info("Info message"));
    EXPECT_NO_THROW(logger.warn("Warning message"));
    EXPECT_NO_THROW(logger.error("Error message"));
    EXPECT_NO_THROW(logger.critical("Critical message"));
    EXPECT_NO_THROW(logger.log(LogLevel::INFO, "Log message"));
    
    // Проверяем, что логгер остается в рабочем состоянии
    EXPECT_TRUE(logger.isHealthy());
}

TEST_F(LoggerTest, FlushMethod) {
    Logger logger(tempLogFile);
    
    // Проверяем, что метод flush не вызывает исключений
    EXPECT_NO_THROW(logger.flush());
    
    // Проверяем, что логгер остается в рабочем состоянии
    EXPECT_TRUE(logger.isHealthy());
}
