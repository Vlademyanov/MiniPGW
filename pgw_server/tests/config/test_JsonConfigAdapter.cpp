#include <gtest/gtest.h>
#include <memory>
#include <fstream>
#include <filesystem>
#include <string>
#include "../../config/JsonConfigAdapter.h"

class JsonConfigAdapterTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Создаем временный файл конфигурации для тестирования
        tempConfigFile = "test_config.json";
        
        // Удаляем файл, если он существует
        if (std::filesystem::exists(tempConfigFile)) {
            std::filesystem::remove(tempConfigFile);
        }
        
        // Создаем тестовый файл конфигурации
        createTestConfigFile();
    }

    void TearDown() override {
        // Удаляем временный файл после тестов
        if (std::filesystem::exists(tempConfigFile)) {
            std::filesystem::remove(tempConfigFile);
        }
    }

    // Создает тестовый файл конфигурации
    void createTestConfigFile() {
        std::ofstream file(tempConfigFile);
        ASSERT_TRUE(file.is_open());
        
        file << R"({
            "udp_ip": "192.168.1.1",
            "udp_port": 9999,
            "session_timeout_sec": 60,
            "cleanup_interval_sec": 10,
            "cdr_file": "test_cdr.log",
            "http_port": 8888,
            "http_ip": "192.168.1.2",
            "graceful_shutdown_rate": 20,
            "max_requests_per_minute": 1000,
            "log_file": "test_log.log",
            "log_level": "DEBUG",
            "blacklist": ["111111111111111", "222222222222222"]
        })";
        
        file.close();
    }

    // Создает некорректный файл конфигурации
    void createInvalidConfigFile() {
        std::ofstream file(tempConfigFile);
        ASSERT_TRUE(file.is_open());
        
        file << R"({
            "udp_ip": "192.168.1.1",
            "udp_port": "invalid_port", // Порт должен быть числом
            "session_timeout_sec": 60
        })";
        
        file.close();
    }

    std::string tempConfigFile;
};

TEST_F(JsonConfigAdapterTest, Constructor) {
    // Проверяем, что конструктор не выбрасывает исключений
    EXPECT_NO_THROW({
        JsonConfigAdapter adapter(tempConfigFile);
    });
}

TEST_F(JsonConfigAdapterTest, Load) {
    // Создаем адаптер
    JsonConfigAdapter adapter(tempConfigFile);
    
    // Загружаем конфигурацию
    bool result = adapter.load();
    
    // Проверяем, что загрузка успешна
    EXPECT_TRUE(result);
    EXPECT_TRUE(adapter.isValid());
    EXPECT_EQ(adapter.getLastError(), "");
}

TEST_F(JsonConfigAdapterTest, LoadInvalidFile) {
    // Создаем некорректный файл конфигурации
    createInvalidConfigFile();
    
    // Создаем адаптер
    JsonConfigAdapter adapter(tempConfigFile);
    
    // Загружаем конфигурацию
    bool result = adapter.load();
    
    // Проверяем, что загрузка не удалась
    EXPECT_FALSE(result);
    EXPECT_FALSE(adapter.isValid());
    EXPECT_NE(adapter.getLastError(), "");
}

TEST_F(JsonConfigAdapterTest, LoadNonExistentFile) {
    // Создаем адаптер с несуществующим файлом
    JsonConfigAdapter adapter("non_existent_file.json");
    
    // Загружаем конфигурацию
    bool result = adapter.load();
    
    // Проверяем, что загрузка не удалась
    EXPECT_FALSE(result);
    EXPECT_FALSE(adapter.isValid());
    EXPECT_NE(adapter.getLastError(), "");
}

TEST_F(JsonConfigAdapterTest, GetConfig) {
    // Создаем адаптер
    JsonConfigAdapter adapter(tempConfigFile);
    
    // Загружаем конфигурацию
    adapter.load();
    
    // Получаем конфигурацию
    const ServerConfig& config = adapter.getConfig();
    
    // Проверяем значения конфигурации
    EXPECT_EQ(config.udp_ip, "192.168.1.1");
    EXPECT_EQ(config.udp_port, 9999);
    EXPECT_EQ(config.session_timeout_sec, 60);
    EXPECT_EQ(config.cleanup_interval_sec, 10);
    EXPECT_EQ(config.cdr_file, "test_cdr.log");
    EXPECT_EQ(config.http_port, 8888);
    EXPECT_EQ(config.graceful_shutdown_rate, 20);
    EXPECT_EQ(config.max_requests_per_minute, 1000);
    EXPECT_EQ(config.log_file, "test_log.log");
    EXPECT_EQ(config.log_level, "DEBUG");
    EXPECT_EQ(config.blacklist.size(), 2);
    EXPECT_EQ(config.blacklist[0], "111111111111111");
    EXPECT_EQ(config.blacklist[1], "222222222222222");
}

TEST_F(JsonConfigAdapterTest, GetString) {
    // Создаем адаптер
    JsonConfigAdapter adapter(tempConfigFile);
    
    // Загружаем конфигурацию
    adapter.load();
    
    // Проверяем получение строковых значений
    EXPECT_EQ(adapter.getString("udp_ip"), "192.168.1.1");
    EXPECT_EQ(adapter.getString("log_file"), "test_log.log");
    EXPECT_EQ(adapter.getString("non_existent_key", "default"), "default");
}

TEST_F(JsonConfigAdapterTest, GetUint) {
    // Создаем адаптер
    JsonConfigAdapter adapter(tempConfigFile);
    
    // Загружаем конфигурацию
    adapter.load();
    
    // Проверяем получение целочисленных значений
    EXPECT_EQ(adapter.getUint("udp_port"), 9999);
    EXPECT_EQ(adapter.getUint("session_timeout_sec"), 60);
    EXPECT_EQ(adapter.getUint("non_existent_key", 42), 42);
}

TEST_F(JsonConfigAdapterTest, GetStringArray) {
    // Создаем адаптер
    JsonConfigAdapter adapter(tempConfigFile);
    
    // Загружаем конфигурацию
    adapter.load();
    
    // Проверяем получение массива строк
    auto blacklist = adapter.getStringArray("blacklist");
    EXPECT_EQ(blacklist.size(), 2);
    EXPECT_EQ(blacklist[0], "111111111111111");
    EXPECT_EQ(blacklist[1], "222222222222222");
    
    // Проверяем получение несуществующего массива
    auto emptyArray = adapter.getStringArray("non_existent_key");
    EXPECT_TRUE(emptyArray.empty());
}
