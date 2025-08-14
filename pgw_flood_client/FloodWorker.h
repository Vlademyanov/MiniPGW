#pragma once
#include <atomic>
#include <thread>
#include <memory>


class PgwClient;

class FloodWorker {
public:
    FloodWorker(int id);
    ~FloodWorker();

    void start();
    void stop();
    void join();

private:
    void run();

    int _id;
    std::atomic<bool> _running;
    std::thread _thread;
    std::unique_ptr<PgwClient> _client;
};
