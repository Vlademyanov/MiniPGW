// domain/Session.hpp
#pragma once
#include <string>
#include <chrono>
#include "../utils/Logger.h"
#include <memory>


/**
 * @brief Сессия абонента в PGW
 * 
 * Сессия содержит информацию об абоненте (IMSI) и времени создания.
 */
class Session {
public:
    /**
     * @brief Создает новую сессию с текущим временем
     * @param imsi IMSI абонента (15 цифр)
     * @throws std::invalid_argument если IMSI имеет неверный формат
     */
    explicit Session(std::string imsi);


    /**
     * @brief Создает новую сессию с текущим временем и логгером
     * @param imsi IMSI абонента (15 цифр)
     * @param logger Указатель на логгер
     * @throws std::invalid_argument если IMSI имеет неверный формат
     */
    Session(std::string imsi, std::shared_ptr<Logger> logger);
    

    // Поддержка семантики копирования и перемещения
    Session(const Session& other) = default;
    Session(Session&& other) noexcept = default;
    Session& operator=(const Session& other) = default;
    Session& operator=(Session&& other) noexcept = default;

    ~Session() = default;

    // Геттеры
    [[nodiscard]] const std::string& getImsi() const;
    [[nodiscard]] const std::chrono::system_clock::time_point& getCreatedAt() const;
    
    /**
     * @brief Проверяет, истекла ли сессия
     * @param timeout Таймаут в секундах
     * @return true если сессия истекла, иначе false
     */
    [[nodiscard]] bool isExpired(std::chrono::seconds timeout) const;
    
    /**
     * @brief Возвращает возраст сессии
     * @return Возраст сессии в секундах
     */
    [[nodiscard]] std::chrono::seconds getAge() const;

private:
    /**
     * @brief Проверяет формат IMSI
     * @param imsi IMSI для проверки
     * @throws std::invalid_argument если IMSI имеет неверный формат
     */
    void validateImsi(const std::string& imsi);
    
    std::string _imsi;                                  // IMSI абонента
    std::chrono::system_clock::time_point _createdAt;   // Время создания сессии
    std::shared_ptr<Logger> _logger;                    // Логгер (может быть nullptr)
};