#pragma once

#include <string>
#include <vector>
#include <unordered_set>

/**
 * @brief Класс Blacklist представляет черный список IMSI абонентов
 */
class Blacklist {
public:
    /**
     * @brief Создает пустой черный список
     */
    Blacklist() = default;
    
    /**
     * @brief Создает черный список с указанными IMSI
     * @param blacklistedImsis Список IMSI для добавления в черный список
     */
    explicit Blacklist(const std::vector<std::string>& blacklistedImsis);
    
    // Поддержка семантики копирования и перемещения
    Blacklist(const Blacklist&) = default;
    Blacklist& operator=(const Blacklist&) = default;
    Blacklist(Blacklist&&) noexcept = default;
    Blacklist& operator=(Blacklist&&) noexcept = default;
    
    ~Blacklist() = default;

    /**
     * @brief Проверяет, находится ли IMSI в черном списке
     * @param imsi IMSI для проверки
     * @return true если IMSI в черном списке, иначе false
     */
    [[nodiscard]] bool isBlacklisted(const std::string& imsi) const;
    
    /**
     * @brief Заменяет текущий черный список новым списком IMSI
     * @param blacklistedImsis Новый список IMSI
     */
    void setBlacklist(const std::vector<std::string>& blacklistedImsis);

private:
    std::unordered_set<std::string> _blacklistedImsis; // Множество IMSI в черном списке
};