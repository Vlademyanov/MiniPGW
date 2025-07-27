#include <FloodManager.h>

FloodManager::FloodManager(int threadCount) {
    for (int i = 0; i < threadCount; ++i)
        _workers.emplace_back(std::make_unique<FloodWorker>(i));
}

void FloodManager::start() {
    for (auto& w : _workers)
        w->start();
}

void FloodManager::stop() {
    for (auto& w : _workers)
        w->stop();
    for (auto& w : _workers)
        w->join();
}
