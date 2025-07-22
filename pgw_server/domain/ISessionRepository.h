#pragma once

#include "Session.h"
#include <string>
#include <vector>
#include <memory>

/**
 * @brief Интерфейс репозитория сессий
 *
 * Определяет методы для работы с хранилищем сессий абонентов.
 */
class ISessionRepository {
public:
    virtual ~ISessionRepository() = default;

    /**
     * @brief Добавляет сессию в репозиторий
     * @param session Сессия для добавления
     * @return true если сессия успешно добавлена, иначе false
     */
    virtual bool addSession(const Session& session) = 0;
    
    /**
     * @brief Удаляет сессию из репозитория
     * @param imsi IMSI абонента
     * @return true если сессия успешно удалена, иначе false
     */
    virtual bool removeSession(const std::string& imsi) = 0;
    
    /**
     * @brief Проверяет существование сессии
     * @param imsi IMSI абонента
     * @return true если сессия существует, иначе false
     */
    [[nodiscard]] virtual bool sessionExists(const std::string& imsi) const = 0;
    
    /**
     * @brief Возвращает все IMSI
     * @return Вектор всех IMSI
     */
    [[nodiscard]] virtual std::vector<std::string> getAllImsis() const = 0;
    
    /**
     * @brief Возвращает количество сессий
     * @return Количество сессий
     */
    [[nodiscard]] virtual size_t getSessionCount() const = 0;
    
    /**
     * @brief Очищает репозиторий
     */
    virtual void clear() = 0;
    
    /**
     * @brief Возвращает истекшие сессии
     * @param timeoutSeconds Таймаут в секундах
     * @return Вектор истекших сессий
     */
    [[nodiscard]] virtual std::vector<Session> getExpiredSessions(uint32_t timeoutSeconds) const = 0;
};