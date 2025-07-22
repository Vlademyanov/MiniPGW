#pragma once

#include "../domain/ICdrRepository.h"
#include "../utils/Logger.h"
#include <string>
#include <fstream>
#include <mutex>
#include <memory>

/**
 * @brief Репозиторий CDR с сохранением в файл
 *
 * Реализует интерфейс ICdrRepository для записи CDR (Call Detail Record)
 * в текстовый файл : timestamp,IMSI,action
 */
class FileCdrRepository : public ICdrRepository {
public:
    /**
     * @brief Создает репозиторий CDR с записью в файл
     * @param filePath Путь к файлу для записи CDR
     */
    explicit FileCdrRepository(std::string filePath);
    
    /**
     * @brief Создает репозиторий CDR с записью в файл и логированием
     * @param filePath Путь к файлу для записи CDR
     * @param logger Указатель на логгер
     */
    FileCdrRepository(std::string filePath, std::shared_ptr<Logger> logger);
    
    ~FileCdrRepository() override;

    // Запрещаем копирование и перемещение
    FileCdrRepository(const FileCdrRepository&) = delete;
    FileCdrRepository& operator=(const FileCdrRepository&) = delete;
    FileCdrRepository(FileCdrRepository&&) = delete;
    FileCdrRepository& operator=(FileCdrRepository&&) = delete;

    /**
     * @brief Записывает CDR с текущим временем
     * @param imsi IMSI абонента
     * @param action Действие
     * @return true если запись успешно создана, иначе false
     */
    bool writeCdr(const std::string& imsi, const std::string& action) override;
    
    /**
     * @brief Записывает CDR с указанным временем
     * @param imsi IMSI абонента
     * @param action Действие
     * @param timestamp Временная метка в строковом формате
     * @return true если запись успешно создана, иначе false
     */
    bool writeCdr(const std::string& imsi, const std::string& action,
                 const std::string& timestamp) override;

private:
    /**
     * @brief Возвращает текущую временную метку в формате YYYY-MM-DD HH:MM:SS
     */
    [[nodiscard]] static std::string getCurrentTimestamp();
    
    /**
     * @brief Открывает файл, если он еще не открыт
     * @return true если файл успешно открыт или уже был открыт, иначе false
     * @note Вызывающая функция должна захватить мьютекс перед вызовом
     */
    bool openFileIfNeeded();

    std::string _filePath;         // Путь к файлу CDR
    mutable std::mutex _mutex;     // Мьютекс для потокобезопасности
    std::ofstream _file;           // Файловый поток
    bool _isHealthy = true;        // Флаг работоспособности
    std::shared_ptr<Logger> _logger; // Логгер (может быть nullptr)
};