#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <Logger.h>
#include <memory>

/**
 * @brief Структура для хранения состояния token bucket
 * 
 * Реализует алгоритм Token Bucket для ограничения скорости запросов.
 * Токены пополняются со временем с заданной скоростью до максимального значения.
 */
struct TokenBucket {
    double tokens{};                     // Текущее количество токенов
    double tokenRate{};                  // Скорость пополнения токенов (токенов в секунду)
    double maxTokens{};                  // Максимальное количество токенов
    std::chrono::steady_clock::time_point lastRefillTime;  // Время последнего пополнения
    std::chrono::steady_clock::time_point lastUseTime;     // Время последнего использования
};

/**
 * @brief Класс для ограничения скорости запросов
 * 
 * Использует алгоритм Token Bucket для ограничения скорости запросов.
 * Каждый IMSI имеет свой bucket, который пополняется со временем.
 */
class RateLimiter {
public:
    /**
     * @brief Создает ограничитель скорости запросов
     * @param maxRequestsPerMinute Максимальное количество запросов в минуту
     */
    explicit RateLimiter(uint32_t maxRequestsPerMinute);

    /**
     * @brief Создает ограничитель скорости запросов с логированием
     * @param maxRequestsPerMinute Максимальное количество запросов в минуту
     * @param logger Указатель на логгер
     */
    RateLimiter(uint32_t maxRequestsPerMinute, std::shared_ptr<Logger> logger);
    
    ~RateLimiter() = default;

    // Запрещаем копирование и перемещение
    RateLimiter(const RateLimiter&) = delete;
    RateLimiter& operator=(const RateLimiter&) = delete;
    RateLimiter(RateLimiter&&) = delete;
    RateLimiter& operator=(RateLimiter&&) = delete;

    /**
     * @brief Проверяет, можно ли обработать запрос
     * @param imsi IMSI абонента
     * @return true если запрос можно обработать, иначе false
     */
    [[nodiscard]] bool allowRequest(const std::string& imsi);

private:
    /**
     * @brief Обновляет состояние bucket для указанного IMSI
     * @param bucket Bucket для обновления
     */
    void refillTokens(TokenBucket& bucket) const;
    
    /**
     * @brief Инициализирует новый bucket или обновляет существующий
     * @param bucket Bucket для инициализации
     * @return Ссылка на инициализированный bucket
     */
    TokenBucket& initializeOrUpdateBucket(TokenBucket& bucket) const;
    
    std::unordered_map<std::string, TokenBucket> _buckets;  // Хранилище bucket'ов
    mutable std::mutex _mutex;                              // Мьютекс для потокобезопасности
    double _tokenRate;                                      // Скорость пополнения токенов (токенов в секунду)
    double _maxTokens;                                      // Максимальное количество токенов
    std::shared_ptr<Logger> _logger;                        // Логгер
};