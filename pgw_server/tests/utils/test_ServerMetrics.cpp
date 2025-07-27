#include <gtest/gtest.h>
#include "utils/ServerMetrics.h"
#include <thread>
#include <chrono>

class ServerMetricsTest : public ::testing::Test {
protected:
    void SetUp() override {
        ServerMetrics::init(9102);
    }

    void TearDown() override {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
};

TEST_F(ServerMetricsTest, IncRequestsProcessed) {
    ASSERT_NO_THROW(ServerMetrics::incRequestsProcessed());
}

TEST_F(ServerMetricsTest, IncRequestsRejected) {
    ASSERT_NO_THROW(ServerMetrics::incRequestsRejected());
}

TEST_F(ServerMetricsTest, SetActiveSessions) {
    // Проверяем различные значения
    ASSERT_NO_THROW(ServerMetrics::setActiveSessions(0));
    ASSERT_NO_THROW(ServerMetrics::setActiveSessions(100));
    ASSERT_NO_THROW(ServerMetrics::setActiveSessions(1000000));
}