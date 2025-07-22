#include <gtest/gtest.h>
#include <memory>
#include <fstream>
#include <filesystem>
#include <string>
#include "../../persistence/FileCdrRepository.h"
#include "../../utils/Logger.h"

class FileCdrRepositoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Создаем временный файл для тестирования
        tempCdrFile = "test_cdr_file.log";
        
        // Удаляем файл, если он существует
        if (std::filesystem::exists(tempCdrFile)) {
            std::filesystem::remove(tempCdrFile);
        }
        
        // Создаем логгер для тестов
        logger = std::make_shared<Logger>("", LogLevel::LOG_DEBUG);
        
        // Создаем репозиторий CDR
        cdrRepo = std::make_unique<FileCdrRepository>(tempCdrFile, logger);
    }

    void TearDown() override {
        // Удаляем временный файл после тестов
        if (std::filesystem::exists(tempCdrFile)) {
            std::filesystem::remove(tempCdrFile);
        }
    }

    // Вспомогательный метод для проверки содержимого файла CDR
    bool fileContains(const std::string& substring) {
        std::ifstream file(tempCdrFile);
        if (!file.is_open()) {
            return false;
        }
        
        std::string content;
        std::string line;
        while (std::getline(file, line)) {
            content += line + "\n";
        }
        
        return content.find(substring) != std::string::npos;
    }

    std::string tempCdrFile;
    std::shared_ptr<Logger> logger;
    std::unique_ptr<FileCdrRepository> cdrRepo;
};

TEST_F(FileCdrRepositoryTest, ConstructorWithoutLogger) {
    // Проверяем, что конструктор без логгера работает
    EXPECT_NO_THROW({
        FileCdrRepository repo("test_file.log");
    });
}

TEST_F(FileCdrRepositoryTest, ConstructorWithLogger) {
    // Проверяем, что конструктор с логгером работает
    EXPECT_NO_THROW({
        FileCdrRepository repo("test_file.log", logger);
    });
}

TEST_F(FileCdrRepositoryTest, WriteCdr) {
    // Создаем тестовые данные
    std::string imsi = "123456789012345";
    std::string action = "CREATE";
    
    // Записываем CDR
    bool result = cdrRepo->writeCdr(imsi, action);
    
    // Проверяем, что запись успешна
    EXPECT_TRUE(result);
    
    // Проверяем, что файл создан
    EXPECT_TRUE(std::filesystem::exists(tempCdrFile));
    
    // Проверяем, что файл содержит нужные данные
    EXPECT_TRUE(fileContains(imsi));
    EXPECT_TRUE(fileContains(action));
}

TEST_F(FileCdrRepositoryTest, WriteCdrWithTimestamp) {
    // Создаем тестовые данные
    std::string imsi = "123456789012345";
    std::string action = "DELETE";
    std::string timestamp = "2023-01-01 12:00:00";
    
    // Записываем CDR с указанным временем
    bool result = cdrRepo->writeCdr(imsi, action, timestamp);
    
    // Проверяем, что запись успешна
    EXPECT_TRUE(result);
    
    // Проверяем, что файл создан
    EXPECT_TRUE(std::filesystem::exists(tempCdrFile));
    
    // Проверяем, что файл содержит нужные данные
    EXPECT_TRUE(fileContains(imsi));
    EXPECT_TRUE(fileContains(action));
    EXPECT_TRUE(fileContains(timestamp));
}

TEST_F(FileCdrRepositoryTest, MultipleWrites) {
    // Создаем тестовые данные
    std::string imsi1 = "123456789012345";
    std::string imsi2 = "234567890123456";
    std::string action1 = "CREATE";
    std::string action2 = "DELETE";
    
    // Записываем несколько CDR
    bool result1 = cdrRepo->writeCdr(imsi1, action1);
    bool result2 = cdrRepo->writeCdr(imsi2, action2);
    
    // Проверяем, что записи успешны
    EXPECT_TRUE(result1);
    EXPECT_TRUE(result2);
    
    // Проверяем, что файл содержит нужные данные
    EXPECT_TRUE(fileContains(imsi1));
    EXPECT_TRUE(fileContains(action1));
    EXPECT_TRUE(fileContains(imsi2));
    EXPECT_TRUE(fileContains(action2));
}

TEST_F(FileCdrRepositoryTest, FileCreationFailure) {
    // Создаем репозиторий с недопустимым путем к файлу
    FileCdrRepository invalidRepo("/invalid/path/to/file.log", logger);
    
    // Пытаемся записать CDR
    bool result = invalidRepo.writeCdr("123456789012345", "CREATE");
    
    // Проверяем, что запись не удалась
    EXPECT_FALSE(result);
}

TEST_F(FileCdrRepositoryTest, DestructorClosesFile) {
    // Создаем тестовые данные
    std::string imsi = "123456789012345";
    std::string action = "CREATE";
    
    // Записываем CDR
    cdrRepo->writeCdr(imsi, action);
    
    // Уничтожаем репозиторий
    cdrRepo.reset();
    
    // Проверяем, что файл существует и может быть открыт для чтения
    std::ifstream file(tempCdrFile);
    EXPECT_TRUE(file.is_open());
    file.close();
}
