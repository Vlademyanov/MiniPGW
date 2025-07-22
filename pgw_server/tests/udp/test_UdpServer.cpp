#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include "../../udp/UdpServer.h"
#include "../../application/SessionManager.h"
#include "../../application/RateLimiter.h"
#include "../../utils/Logger.h"
#include "../../domain/Session.h"
#include "../../domain/Blacklist.h"
#include "../../persistence/InMemorySessionRepository.h"
#include "../../persistence/FileCdrRepository.h"

class UdpServerTest : public ::testing::Test {
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
        
        // Создаем UDP-сервер на localhost с нестандартным портом для тестов
        udpServer = std::make_unique<UdpServer>(
            "127.0.0.1", 9001, sessionManager, logger);
    }

    void TearDown() override {
        // Останавливаем сервер, если он запущен
        if (udpServer && udpServer->isRunning()) {
            udpServer->stop();
        }
    }

    // Вспомогательный метод для создания BCD-кодированного IMSI
    static std::vector<uint8_t> createBcdImsi(const std::string& imsi) {
        // Проверяем длину IMSI
        if (imsi.length() != 15) {
            throw std::invalid_argument("IMSI must contain 15 digits");
        }
        
        // Создаем вектор для результата
        std::vector<uint8_t> bcd;
        
        // Добавляем заголовок пакета
        bcd.push_back(0x01);
        bcd.push_back(0x00);
        bcd.push_back(0x00);
        bcd.push_back(0x00);
        
        // Преобразуем IMSI в BCD формат
        for (size_t i = 0; i < imsi.length(); i += 2) {
            uint8_t byte = 0;
            
            // Первая цифра (младшие 4 бита)
            byte |= (imsi[i] - '0') & 0x0F;
            
            // Вторая цифра (старшие 4 бита), если она есть
            if (i + 1 < imsi.length()) {
                byte |= ((imsi[i + 1] - '0') << 4) & 0xF0;
            } else {
                // Если количество цифр нечетное, заполняем старшие 4 бита значением 0xF
                byte |= 0xF0;
            }
            
            bcd.push_back(byte);
        }
        
        // Выводим отладочную информацию
        std::cout << "Encoding IMSI: " << imsi << " to BCD" << std::endl;
        std::cout << "BCD bytes: ";
        for (auto b : bcd) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b) << " ";
        }
        std::cout << std::dec << std::endl;
        
        // Декодируем BCD обратно в IMSI для проверки
        std::string decodedImsi;
        for (size_t i = 4; i < bcd.size() && decodedImsi.length() < 15; i++) {
            uint8_t byte = bcd[i];
            
            // Извлекаем младшую цифру (4 младших бита)
            uint8_t digit1 = byte & 0x0F;
            if (digit1 <= 9) {
                decodedImsi.push_back('0' + digit1);
            }
            
            // Если уже набрали 15 цифр, выходим
            if (decodedImsi.length() >= 15) {
                break;
            }
            
            // Извлекаем старшую цифру (4 старших бита)
            uint8_t digit2 = (byte >> 4) & 0x0F;
            if (digit2 <= 9) {
                decodedImsi.push_back('0' + digit2);
            } else if (digit2 == 0x0F && i == bcd.size() - 1) {
                // Последний полубайт может быть заполнителем F
                break;
            }
        }
        std::cout << "Decoded back: " << decodedImsi << std::endl;
        
        return bcd;
    }

    std::shared_ptr<Logger> logger;
    std::shared_ptr<InMemorySessionRepository> sessionRepo;
    std::shared_ptr<FileCdrRepository> cdrRepo;
    std::shared_ptr<Blacklist> blacklist;
    std::shared_ptr<RateLimiter> rateLimiter;
    std::shared_ptr<SessionManager> sessionManager;
    std::unique_ptr<UdpServer> udpServer;
};

TEST_F(UdpServerTest, Constructor) {
    // Проверяем, что конструктор не выбрасывает исключений
    EXPECT_NO_THROW({
        UdpServer server("127.0.0.1", 9002, sessionManager, logger);
    });
}

TEST_F(UdpServerTest, StartAndStop) {
    // Проверяем, что сервер изначально не запущен
    EXPECT_FALSE(udpServer->isRunning());
    
    // Запускаем сервер
    EXPECT_TRUE(udpServer->start());
    
    // Проверяем, что сервер запущен
    EXPECT_TRUE(udpServer->isRunning());
    
    // Останавливаем сервер
    udpServer->stop();
    
    // Проверяем, что сервер остановлен
    EXPECT_FALSE(udpServer->isRunning());
}

TEST_F(UdpServerTest, DoubleStart) {
    // Запускаем сервер
    EXPECT_TRUE(udpServer->start());
    
    // Пытаемся запустить сервер повторно
    EXPECT_FALSE(udpServer->start());
    
    // Останавливаем сервер
    udpServer->stop();
}

// Этот тест требует отправки UDP-пакетов и проверки создания сессий
TEST_F(UdpServerTest, HandleIncomingPacket) {
    // Запускаем сервер
    EXPECT_TRUE(udpServer->start());
    
    // Создаем тестовый IMSI
    std::string imsi = "123456789012345";
    
    // Создаем BCD-кодированный IMSI
    auto bcdData = createBcdImsi(imsi);
    
    // Проверяем размер BCD данных
    std::cout << "BCD data size: " << bcdData.size() << " bytes" << std::endl;
    std::cout << "BCD data: ";
    for (auto b : bcdData) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b) << " ";
    }
    std::cout << std::dec << std::endl;
    
    // Создаем UDP-сокет для отправки пакета
    int clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
    ASSERT_GE(clientSocket, 0);
    
    // Настраиваем адрес сервера
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(9001);
    inet_pton(AF_INET, "127.0.0.1", &(serverAddr.sin_addr));
    
    // Отправляем пакет
    ssize_t sent = sendto(clientSocket, bcdData.data(), bcdData.size(), 0,
           (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    
    std::cout << "Sent " << sent << " bytes to UDP server" << std::endl;
    
    // Ждем некоторое время для обработки пакета
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Проверяем, что сессия была создана с ожидаемым IMSI
    bool exists = sessionRepo->sessionExists(imsi);
    std::cout << "Session exists for IMSI " << imsi << ": " << (exists ? "yes" : "no") << std::endl;
    EXPECT_TRUE(exists) << "Сессия для IMSI " << imsi << " не создана";
    
    // Закрываем сокет
    close(clientSocket);
    
    // Останавливаем сервер
    udpServer->stop();
}

// Этот тест проверяет обработку некорректных пакетов
TEST_F(UdpServerTest, HandleInvalidPacket) {
    // Запускаем сервер
    EXPECT_TRUE(udpServer->start());
    
    // Создаем некорректный пакет (слишком короткий)
    std::vector<uint8_t> invalidPacket = {0x01, 0x02, 0x03};
    
    // Создаем UDP-сокет для отправки пакета
    int clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
    ASSERT_GE(clientSocket, 0);
    
    // Настраиваем адрес сервера
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(9001);
    inet_pton(AF_INET, "127.0.0.1", &(serverAddr.sin_addr));
    
    // Отправляем пакет
    ssize_t bytesSent = sendto(clientSocket, invalidPacket.data(), invalidPacket.size(), 0,
           (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    
    // Проверяем, что пакет был отправлен
    ASSERT_GT(bytesSent, 0);
    std::cout << "Sent " << bytesSent << " bytes of invalid data" << std::endl;
    
    // Ждем некоторое время для обработки пакета
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Проверяем, что количество сессий не изменилось
    EXPECT_EQ(sessionRepo->getSessionCount(), 0);
    
    // Закрываем сокет
    close(clientSocket);
    
    // Останавливаем сервер
    udpServer->stop();
}

// Этот тест проверяет создание нескольких сессий через UDP
TEST_F(UdpServerTest, HandleMultipleSessions) {
    // Запускаем сервер
    EXPECT_TRUE(udpServer->start());
    
    // Создаем несколько тестовых IMSI
    std::vector<std::string> imsis = {
        "123456789012345",
        "234567890123456",
        "345678901234567"
    };
    
    // Создаем UDP-сокет для отправки пакетов
    int clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
    ASSERT_GE(clientSocket, 0);
    
    // Настраиваем адрес сервера
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(9001);
    inet_pton(AF_INET, "127.0.0.1", &(serverAddr.sin_addr));
    
    // Отправляем пакеты для каждого IMSI
    for (size_t i = 0; i < imsis.size(); ++i) {
        // Создаем BCD-кодированный IMSI
        auto bcdData = createBcdImsi(imsis[i]);
        
        // Отправляем пакет
        ssize_t bytesSent = sendto(clientSocket, bcdData.data(), bcdData.size(), 0,
               (struct sockaddr*)&serverAddr, sizeof(serverAddr));
        
        // Проверяем, что пакет был отправлен
        ASSERT_GT(bytesSent, 0);
        std::cout << "Sent " << bytesSent << " bytes for IMSI: " << imsis[i] << std::endl;
        
        // Ждем некоторое время для обработки пакета
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Даем дополнительное время для обработки всех пакетов
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Проверяем, что все сессии были созданы
    for (const auto& imsi : imsis) {
        bool sessionExists = sessionRepo->sessionExists(imsi);
        std::cout << "Session exists for IMSI " << imsi << ": " << (sessionExists ? "true" : "false") << std::endl;
        EXPECT_TRUE(sessionExists) << "Сессия для IMSI " << imsi << " не создана";
    }
    
    // Проверяем общее количество сессий
    size_t sessionCount = sessionRepo->getSessionCount();
    std::cout << "Total session count: " << sessionCount << ", expected: " << imsis.size() << std::endl;
    EXPECT_EQ(sessionRepo->getSessionCount(), imsis.size());
    
    // Закрываем сокет
    close(clientSocket);
    
    // Останавливаем сервер
    udpServer->stop();
}
