#include <FloodManager.h>
#include <iostream>
#include <Metrics.h>
#include <atomic>
#include <csignal>

std::atomic<bool> running{true};

void signal_handler(int) {
    running = false;
}

int main() {

    std::signal(SIGTERM, signal_handler);
    std::signal(SIGINT, signal_handler);

    const int threadCount = 1;

    Metrics::init(9100);
    std::cout << "Starting IMSI flooder with " << threadCount << " threads...\n";
    FloodManager manager(threadCount);
    manager.start();

    std::cout << "Flooder running. Send SIGTERM or SIGINT to stop.\n";
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    manager.stop();

    std::cout << "Flooding finished.\n";
    return 0;
}