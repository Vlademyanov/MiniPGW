#include <FloodWorker.h>
#include <ImsiGenerator.h>
#include <iostream>
#include <chrono>
#include <Metrics.h>

#include <PgwClient.h>

using namespace std::chrono;

FloodWorker::FloodWorker(int id)
    : _id(id), _running(false), _client(std::make_unique<PgwClient>()) {}

FloodWorker::~FloodWorker() {
    stop();
    join();
}

void FloodWorker::start() {
    if (!_client->initialize()) {
        std::cerr << "[Worker " << _id << "] Failed to initialize client\n";
        return;
    }
    _running = true;
    _thread = std::thread(&FloodWorker::run, this);
}

void FloodWorker::stop() {
    _running = false;
}

void FloodWorker::join() {
    if (_thread.joinable())
        _thread.join();
}

void FloodWorker::run() {
    size_t sent = 0;
    auto last = steady_clock::now();

    while (_running) {
        std::string imsi = ImsiGenerator::generate();
        auto [ok, bcd] = _client->encodeImsiToBcd(imsi);
        if (ok) {
            _client->sendUdpPacket(bcd);
            Metrics::incRequests();
            ++sent;
        }

        auto now = steady_clock::now();
        if (now - last > seconds(1)) {
            std::cout << "[Worker " << _id << "] Sent: " << sent << " IMSI/s\n";
            sent = 0;
            last = now;
        }
    }
}
