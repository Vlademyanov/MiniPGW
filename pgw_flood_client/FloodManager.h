#pragma once
#include <vector>
#include <memory>
#include <FloodWorker.h>

class FloodManager {
public:
    explicit FloodManager(int threadCount);
    void start();
    void stop();

private:
    std::vector<std::unique_ptr<FloodWorker>> _workers;
};
