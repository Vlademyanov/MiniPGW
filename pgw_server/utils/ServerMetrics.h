#pragma once
#include <memory>
#include <prometheus/registry.h>
#include <prometheus/counter.h>

class ServerMetrics {
public:
    static void init(int port = 9101);
    
    // Счетчики запросов
    static void incProcessedRequests();
    static void incRejectedRequests();
    
private:
    static std::shared_ptr<prometheus::Registry> registry_;
    
    // Счетчики
    static prometheus::Counter* processed_requests_counter_;
    static prometheus::Counter* rejected_requests_counter_;
};