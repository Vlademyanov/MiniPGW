#pragma once

#include <ISessionRepository.h>
#include <Session.h>
#include <Logger.h>
#include <unordered_map>
#include <mutex>
#include <string>
#include <vector>
#include <memory>

/**
 * @brief Потокобезопасное in-memory хранилище сессий
 * 
 * Реализует интерфейс ISessionRepository для хранения сессий в оперативной памяти
 * Не сохраняет данные между перезапусками приложения
 */
class InMemorySessionRepository : public ISessionRepository {
public:
    /**
     * @brief Создает репозиторий сессий без логирования
     */
    InMemorySessionRepository() = default;
    
    /**
     * @brief Создает репозиторий сессий с логированием
     * @param logger Указатель на логгер
     */
    explicit InMemorySessionRepository(std::shared_ptr<Logger> logger);
    
    ~InMemorySessionRepository() override = default;

    // Запрещаем копирование и перемещение
    InMemorySessionRepository(const InMemorySessionRepository&) = delete;
    InMemorySessionRepository& operator=(const InMemorySessionRepository&) = delete;
    InMemorySessionRepository(InMemorySessionRepository&&) = delete;
    InMemorySessionRepository& operator=(InMemorySessionRepository&&) = delete;

    /**
     * @brief Добавить сессию
     * @param session Сессия для добавления
     * @return true, если сессия добавлена
     */
    bool addSession(const Session& session) override;

    /**
     * @brief Удалить сессию по IMSI
     * @param imsi IMSI абонента
     * @return true, если сессия была удалена
     */
    bool removeSession(const std::string& imsi) override;

    /**
     * @brief Проверить наличие сессии по IMSI
     * @param imsi IMSI абонента
     * @return true если сессия существует, иначе false
     */
    [[nodiscard]] bool sessionExists(const std::string& imsi) const override;

    /**
     * @brief Получить все IMSI
     * @return Вектор всех IMSI
     */
    [[nodiscard]] std::vector<std::string> getAllImsis() const override;

    /**
     * @brief Получить количество сессий
     * @return Количество сессий
     */
    [[nodiscard]] size_t getSessionCount() const override;
    
    /**
     * @brief Очистить все сессии.
     */
    void clear() override;

    /**
     * @brief Получить все истёкшие сессии
     * @param timeoutSeconds Таймаут в секундах
     * @return Вектор истёкших сессий
     */
    [[nodiscard]] std::vector<Session> getExpiredSessions(uint32_t timeoutSeconds) const override;

    bool refreshSession(const std::string& imsi) override;

private:
    mutable std::mutex _mutex; // Мьютекс для потокобезопасности
    std::unordered_map<std::string, Session> _sessions; // Хранилище сессий
    std::shared_ptr<Logger> _logger; // Логгер (может быть nullptr)
};