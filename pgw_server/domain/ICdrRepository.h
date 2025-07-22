#pragma once

#include <string>

/**
 * @brief Интерфейс репозитория CDR (Charging Data Records)
 * 
 * Определяет методы для работы с хранилищем записей о тарификации.
 * Реализации могут использовать различные способы хранения (файл, БД и т.д.).
 */
class ICdrRepository {
public:
    virtual ~ICdrRepository() = default;

    /**
     * @brief Записывает CDR с текущим временем
     * @param imsi IMSI абонента
     * @param action Действие (например, "CREATE", "DELETE")
     * @return true если запись успешно создана, иначе false
     */
    virtual bool writeCdr(const std::string& imsi, const std::string& action) = 0;
    
    /**
     * @brief Записывает CDR с указанным временем
     * @param imsi IMSI абонента
     * @param action Действие (например, "CREATE", "DELETE")
     * @param timestamp Временная метка в строковом формате
     * @return true если запись успешно создана, иначе false
     */
    virtual bool writeCdr(const std::string& imsi, const std::string& action,
                         const std::string& timestamp) = 0;
};