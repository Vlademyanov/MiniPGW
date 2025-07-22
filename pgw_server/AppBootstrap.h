#pragma once
#include <memory>
#include <atomic>
#include <string>

// Предварительные объявления классов для уменьшения зависимостей
class JsonConfigAdapter;
class UdpServer;
class HttpServer;
class SessionManager;
class GracefulShutdownManager;
class SessionCleaner;
class RateLimiter;
class InMemorySessionRepository;
class FileCdrRepository;
class Logger;
class Blacklist;

/**
 * @brief Класс для инициализации и управления жизненным циклом приложения
 * 
 * AppBootstrap отвечает за создание и настройку всех компонентов приложения,
 * их правильный запуск и корректное завершение работы.
 */
class AppBootstrap {
public:
    /**
     * @brief Конструктор
     */
    AppBootstrap();
    
    /**
     * @brief Деструктор
     */
    ~AppBootstrap();
    
    // Запрещаем копирование и перемещение
    AppBootstrap(const AppBootstrap&) = delete;
    AppBootstrap& operator=(const AppBootstrap&) = delete;
    AppBootstrap(AppBootstrap&&) = delete;
    AppBootstrap& operator=(AppBootstrap&&) = delete;

    /**
     * @brief Инициализирует все компоненты приложения
     * @throw std::runtime_error в случае ошибки инициализации
     */
    void initialize();
    
    /**
     * @brief Запускает приложение и блокирует выполнение до завершения
     */
    void run();
    
    /**
     * @brief Инициирует процесс плавного завершения работы
     */
    void initiateShutdown();

private:
    /**
     * @brief Находит доступный конфигурационный файл
     * @return Путь к найденному конфигурационному файлу
     * @throw std::runtime_error если файл не найден
     */
    [[nodiscard]] static std::string findConfigFile() ;
    
    /**
     * @brief Создает и настраивает все компоненты приложения
     */
    void setupComponents();
    
    /**
     * @brief Запускает все сервисы
     * @throw std::runtime_error в случае ошибки запуска
     */
    void startServices() const;
    
    /**
     * @brief Останавливает все сервисы
     */
    void stopServices() const;
    
    /**
     * @brief Создает shared_ptr с корректным deleter
     * @tparam T Тип объекта
     * @param ptr Указатель на объект
     * @return shared_ptr на объект с пустым deleter
     */
    template<typename T>
    std::shared_ptr<T> createSharedFromUnique(T* ptr) const {
        return std::shared_ptr<T>(ptr, [](T*){});
    }

    // Конфигурация
    std::unique_ptr<JsonConfigAdapter> _config;
    
    // Логирование
    std::unique_ptr<Logger> _logger;
    
    // Хранение данных
    std::unique_ptr<InMemorySessionRepository> _sessionRepo;
    std::unique_ptr<FileCdrRepository> _cdrRepo;
    
    // Бизнес-логика
    std::unique_ptr<Blacklist> _blacklist;
    std::unique_ptr<RateLimiter> _rateLimiter;
    std::unique_ptr<SessionManager> _sessionManager;
    std::unique_ptr<GracefulShutdownManager> _shutdownManager;
    std::unique_ptr<SessionCleaner> _sessionCleaner;
    
    // Серверы
    std::unique_ptr<UdpServer> _udpServer;
    std::unique_ptr<HttpServer> _httpServer;

    // Состояние
    std::atomic<bool> _running{false};
};