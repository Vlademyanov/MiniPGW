#include <gtest/gtest.h>
#include "../../domain/Blacklist.h"
#include <vector>
#include <string>

class BlacklistTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Создаем тестовые IMSI
        imsi1 = "123456789012345";
        imsi2 = "234567890123456";
        imsi3 = "345678901234567";
        
        // Создаем тестовый список IMSI
        testImsis = {imsi1, imsi2};
    }

    std::string imsi1;
    std::string imsi2;
    std::string imsi3;
    std::vector<std::string> testImsis;
};

TEST_F(BlacklistTest, DefaultConstructor) {
    Blacklist blacklist;
    
    // Проверяем, что черный список пуст
    EXPECT_FALSE(blacklist.isBlacklisted(imsi1));
    EXPECT_FALSE(blacklist.isBlacklisted(imsi2));
    EXPECT_FALSE(blacklist.isBlacklisted(imsi3));
}

TEST_F(BlacklistTest, ConstructorWithList) {
    Blacklist blacklist(testImsis);
    
    // Проверяем, что IMSI из списка находятся в черном списке
    EXPECT_TRUE(blacklist.isBlacklisted(imsi1));
    EXPECT_TRUE(blacklist.isBlacklisted(imsi2));
    
    // Проверяем, что IMSI не из списка отсутствует в черном списке
    EXPECT_FALSE(blacklist.isBlacklisted(imsi3));
}

TEST_F(BlacklistTest, SetBlacklist) {
    Blacklist blacklist;
    
    // Проверяем, что изначально черный список пуст
    EXPECT_FALSE(blacklist.isBlacklisted(imsi1));
    EXPECT_FALSE(blacklist.isBlacklisted(imsi2));
    
    // Устанавливаем черный список
    blacklist.setBlacklist(testImsis);
    
    // Проверяем, что IMSI из списка находятся в черном списке
    EXPECT_TRUE(blacklist.isBlacklisted(imsi1));
    EXPECT_TRUE(blacklist.isBlacklisted(imsi2));
    
    // Проверяем, что IMSI не из списка отсутствует в черном списке
    EXPECT_FALSE(blacklist.isBlacklisted(imsi3));
    
    // Устанавливаем новый черный список
    std::vector<std::string> newList = {imsi3};
    blacklist.setBlacklist(newList);
    
    // Проверяем, что старые IMSI больше не в черном списке
    EXPECT_FALSE(blacklist.isBlacklisted(imsi1));
    EXPECT_FALSE(blacklist.isBlacklisted(imsi2));
    
    // Проверяем, что новый IMSI в черном списке
    EXPECT_TRUE(blacklist.isBlacklisted(imsi3));
}

TEST_F(BlacklistTest, IsBlacklisted) {
    Blacklist blacklist(testImsis);
    
    // Проверяем существующие IMSI
    EXPECT_TRUE(blacklist.isBlacklisted(imsi1));
    EXPECT_TRUE(blacklist.isBlacklisted(imsi2));
    
    // Проверяем отсутствующий IMSI
    EXPECT_FALSE(blacklist.isBlacklisted(imsi3));
    
    // Проверяем пустую строку
    EXPECT_FALSE(blacklist.isBlacklisted(""));
    
    // Проверяем произвольную строку
    EXPECT_FALSE(blacklist.isBlacklisted("not_an_imsi"));
}

TEST_F(BlacklistTest, CopyConstructor) {
    Blacklist original(testImsis);
    
    // Создаем копию через конструктор копирования
    Blacklist copy(original);
    
    // Проверяем, что копия имеет те же IMSI
    EXPECT_TRUE(copy.isBlacklisted(imsi1));
    EXPECT_TRUE(copy.isBlacklisted(imsi2));
    EXPECT_FALSE(copy.isBlacklisted(imsi3));
}

TEST_F(BlacklistTest, MoveConstructor) {
    Blacklist original(testImsis);
    
    // Создаем копию через конструктор перемещения
    Blacklist moved(std::move(original));
    
    // Проверяем, что перемещенный объект имеет те же IMSI
    EXPECT_TRUE(moved.isBlacklisted(imsi1));
    EXPECT_TRUE(moved.isBlacklisted(imsi2));
    EXPECT_FALSE(moved.isBlacklisted(imsi3));
}

TEST_F(BlacklistTest, CopyAssignment) {
    Blacklist original(testImsis);
    Blacklist copy;
    
    // Копируем через оператор присваивания
    copy = original;
    
    // Проверяем, что копия имеет те же IMSI
    EXPECT_TRUE(copy.isBlacklisted(imsi1));
    EXPECT_TRUE(copy.isBlacklisted(imsi2));
    EXPECT_FALSE(copy.isBlacklisted(imsi3));
}

TEST_F(BlacklistTest, MoveAssignment) {
    Blacklist original(testImsis);
    Blacklist moved;
    
    // Перемещаем через оператор присваивания
    moved = std::move(original);
    
    // Проверяем, что перемещенный объект имеет те же IMSI
    EXPECT_TRUE(moved.isBlacklisted(imsi1));
    EXPECT_TRUE(moved.isBlacklisted(imsi2));
    EXPECT_FALSE(moved.isBlacklisted(imsi3));
}
