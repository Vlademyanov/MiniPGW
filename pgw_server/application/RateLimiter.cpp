#include <RateLimiter.h>
#include <algorithm>

RateLimiter::RateLimiter(uint32_t maxRequestsPerMinute)
    : _tokenRate(maxRequestsPerMinute / 60.0),  // Преобразуем в токенов в секунду
      _maxTokens(std::max(maxRequestsPerMinute / 10.0, 1.0)),   // Максимальный размер бакета - 1/10 от минутного лимита, но не меньше 1
      _logger(nullptr)
{
}

RateLimiter::RateLimiter(uint32_t maxRequestsPerMinute, std::shared_ptr<Logger> logger)
    : _tokenRate(maxRequestsPerMinute / 60.0),  // Преобразуем в токенов в секунду
      _maxTokens(std::max(maxRequestsPerMinute / 10.0, 1.0)),   // Максимальный размер бакета - 1/10 от минутного лимита, но не меньше 1
      _logger(std::move(logger))
{
    if (_logger) {
        _logger->debug("RateLimiter initialized: " + std::to_string(maxRequestsPerMinute) + 
                      " req/min, token rate: " + std::to_string(_tokenRate) + 
                      " tokens/sec, max tokens: " + std::to_string(_maxTokens));
    }
}

bool RateLimiter::allowRequest(const std::string& imsi) {
    std::lock_guard<std::mutex> lock(_mutex);
    
    // Получаем или создаем bucket для данного IMSI
    auto& bucket = _buckets[imsi];
    bool isNewBucket = (bucket.lastRefillTime == std::chrono::steady_clock::time_point());
    
    // Инициализируем или обновляем bucket
    initializeOrUpdateBucket(bucket);
    
    // Проверяем, есть ли доступный токен
    if (bucket.tokens >= 1.0) {
        // Забираем токен
        bucket.tokens -= 1.0;
        // Обновляем время последнего использования
        bucket.lastUseTime = std::chrono::steady_clock::now();
        
        if (_logger && isNewBucket) {
            _logger->debug("Created new rate limit bucket for IMSI: " + imsi);
        }
        
        return true;
    }
    
    // Нет доступных токенов, запрос отклоняется
    if (_logger) {
        _logger->warn("Rate limit exceeded for IMSI: " + imsi + 
                     ", available tokens: " + std::to_string(bucket.tokens));
    }
    return false;
}
    
TokenBucket& RateLimiter::initializeOrUpdateBucket(TokenBucket& bucket) const {
    // Если это первый запрос для данного IMSI, инициализируем bucket
    if (bucket.lastRefillTime == std::chrono::steady_clock::time_point()) {
        bucket.tokens = _maxTokens;
        bucket.tokenRate = _tokenRate;
        bucket.maxTokens = _maxTokens;
        bucket.lastRefillTime = std::chrono::steady_clock::now();
        bucket.lastUseTime = bucket.lastRefillTime;
    } else {
        // Пополняем токены в соответствии с прошедшим временем
        refillTokens(bucket);
    }

    return bucket;
}

void RateLimiter::refillTokens(TokenBucket& bucket) const {
    // Текущее время
    auto now = std::chrono::steady_clock::now();
    
    // Вычисляем прошедшее время с момента последнего пополнения
    auto timeElapsed = std::chrono::duration_cast<std::chrono::duration<double>>(
        now - bucket.lastRefillTime).count();
    
    // Вычисляем количество новых токенов
    double newTokens = timeElapsed * bucket.tokenRate;
    
    // Обновляем состояние bucket
    double oldTokens = bucket.tokens;
    bucket.tokens = std::min(bucket.tokens + newTokens, bucket.maxTokens);
    bucket.lastRefillTime = now;
    
    if (_logger && bucket.tokens > oldTokens + 0.1) { // Логируем только если добавили значимое количество токенов
        _logger->debug("Refilled tokens: " + std::to_string(bucket.tokens - oldTokens) + 
                      ", new total: " + std::to_string(bucket.tokens));
    }
}
