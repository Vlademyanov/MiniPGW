#pragma once
#include <memory>
#include <prometheus/registry.h>
#include <prometheus/counter.h>
#include <prometheus/exposer.h>

class Metrics {
public:
    static void init(int port = 9100);
    static void incRequests();
private:
    static std::shared_ptr<prometheus::Registry> registry_;
    static prometheus::Counter* requests_counter_;
};