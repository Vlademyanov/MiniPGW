#pragma once
#include <ISessionRepository.h>
#include <ICdrRepository.h>
#include <Blacklist.h>
#include <RateLimiter.h>
#include <Logger.h>
#include <string>
#include <memory>
#include <chrono>
#include <vector>
#include <atomic>

/**
 * @brief Результат создания сессии
 */
enum class SessionResult {
    CREATED,
    REJECTED,
    ERROR
};

/**
 * @brief Управляет сессиями абонентов,
 * 
 * Объединяет функциональность управления сессиями:
 * - Создание и удаление сессий
 * - Проверка существования сессий
 * - Работа с черным списком
 * - Запись CDR
 * - Ограничение скорости запросов
 */
class SessionManager {
public:
    /**
     * @brief Создает новый менеджер сессий
     * @param sessionRepo Репозиторий сессий
     * @param cdrRepo Репозиторий CDR
     * @param blacklist Черный список IMSI
     * @param rateLimiter Ограничитель скорости запросов
     * @param logger Логгер
     */
    SessionManager(std::shared_ptr<ISessionRepository> sessionRepo,
                  std::shared_ptr<ICdrRepository> cdrRepo,
                  std::shared_ptr<Blacklist> blacklist,
                  std::shared_ptr<RateLimiter> rateLimiter,
                  std::shared_ptr<Logger> logger);
                  
    // Запрещаем копирование и перемещение
    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;
    SessionManager(SessionManager&&) = delete;
    SessionManager& operator=(SessionManager&&) = delete;

    /**
     * @brief Создает новую сессию для абонента
     * @param imsi IMSI абонента
     * @return Результат создания сессии
     */
    SessionResult createSession(const std::string& imsi) const;
    
    /**
     * @brief Проверяет, активна ли сессия для указанного IMSI
     * @param imsi IMSI абонента
     * @return true если сессия активна, иначе false
     */
    [[nodiscard]] bool isSessionActive(const std::string& imsi) const;
    
    /**
     * @brief Удаляет сессию для указанного IMSI
     * @param imsi IMSI абонента
     * @param action Действие для записи в CDR
     * @return true если сессия успешно удалена, иначе false
     */
    bool removeSession(const std::string& imsi, const std::string& action) const;
    
    /**
     * @brief Очищает истекшие сессии
     * @param timeout Таймаут в секундах
     * @return Количество удаленных сессий
     */
    [[nodiscard]] size_t cleanExpiredSessions(std::chrono::seconds timeout, const std::atomic<bool>* stopFlag = nullptr) const;
    
    /**
     * @brief Возвращает количество активных сессий
     * @return Количество активных сессий
     */
    [[nodiscard]] size_t getActiveSessionsCount() const;
    
    /**
     * @brief Возвращает список всех активных IMSI
     * @return Вектор IMSI
     */
    [[nodiscard]] std::vector<std::string> getAllActiveImsis() const;

private:
    /**
     * @brief Записывает CDR для указанного IMSI и действия
     * @param imsi IMSI абонента
     * @param action Действие
     */
    void logCdr(const std::string& imsi, const std::string& action) const;
    
    /**
     * @brief Проверяет, находится ли IMSI в черном списке
     * @param imsi IMSI для проверки
     * @return true если IMSI в черном списке, иначе false
     */
    [[nodiscard]] bool isImsiBlacklisted(const std::string& imsi) const;

    std::shared_ptr<ISessionRepository> _sessionRepo;  // Репозиторий сессий
    std::shared_ptr<ICdrRepository> _cdrRepo;          // Репозиторий CDR
    std::shared_ptr<Blacklist> _blacklist;             // Черный список IMSI
    std::shared_ptr<RateLimiter> _rateLimiter;         // Ограничитель скорости запросов
    std::shared_ptr<Logger> _logger;                   // Логгер
}; 
 
 