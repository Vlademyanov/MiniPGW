#include "PgwClient.h"
#include <iostream>
#include <string>
#include <algorithm>

/**
 * @brief Выводит справку по использованию программы
 * @param programName Имя программы
 */
void printUsage(const std::string& programName) {
    std::cout << "Usage: " << programName << " <IMSI>" << std::endl;
    std::cout << "  IMSI must be a 15-digit number" << std::endl;
    std::cout << "Example: " << programName << " 123456789012345" << std::endl;
}

int main(int argc, char* argv[]) {
    // Проверяем количество аргументов
    if (argc != 2) {
        std::cerr << "Error: Invalid number of arguments" << std::endl;
        printUsage(argv[0]);
        return 1;
    }
    
    // Получаем IMSI из аргументов
    std::string imsi = argv[1];
    
    // Проверяем, что IMSI состоит только из цифр и имеет длину 15
    if (imsi.length() != 15 || !std::all_of(imsi.begin(), imsi.end(), ::isdigit)) {
        std::cerr << "Error: IMSI must be a 15-digit number" << std::endl;
        printUsage(argv[0]);
        return 1;
    }
    
    try {
        // Создаем клиент
        PgwClient client;
        
        // Инициализируем клиент
        if (!client.initialize()) {
            std::cerr << "Error: Failed to initialize client" << std::endl;
            return 1;
        }
        
        // Отправляем запрос
        std::cout << "Sending request for IMSI: " << imsi << std::endl;
        auto [success, response] = client.sendRequest(imsi);
        
        // Обрабатываем результат
        if (success) {
            std::cout << "Response: " << response << std::endl;
            return 0;
        } else {
            std::cerr << "Error: " << response << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}