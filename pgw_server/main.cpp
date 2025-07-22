#include "AppBootstrap.h"
#include <iostream>
#include <exception>

int main() {
    try {
        
        // Создаем и инициализируем приложение
        std::cout << "Initializing application..." << std::endl;
        AppBootstrap app;
        app.initialize();
        
        // Запускаем приложение
        std::cout << "Running application..." << std::endl;
        app.run();
        
        std::cout << "Application terminated successfully" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error occurred" << std::endl;
        return 2;
    }
}
