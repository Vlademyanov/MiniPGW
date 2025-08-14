#include <ServerMetrics.h>
#include <prometheus/exposer.h>

using namespace prometheus;

// Инициализация статических переменных
std::shared_ptr<Registry> ServerMetrics::registry_;
Counter* ServerMetrics::processed_requests_counter_ = nullptr;
Counter* ServerMetrics::rejected_requests_counter_ = nullptr;

void ServerMetrics::init(int port) {
    // HTTP endpoint для Prometheus
    static Exposer exposer{"0.0.0.0:" + std::to_string(port)};
    registry_ = std::make_shared<Registry>();

    // Счетчик обработанных запросов
    auto& processed_requests_family = BuildCounter()
        .Name("pgw_requests_processed_total")
        .Help("Total number of processed requests")
        .Register(*registry_);
    processed_requests_counter_ = &processed_requests_family.Add({});

    // Счетчик отклоненных запросов
    auto& rejected_requests_family = BuildCounter()
        .Name("pgw_requests_rejected_total")
        .Help("Total number of rejected requests")
        .Register(*registry_);
    rejected_requests_counter_ = &rejected_requests_family.Add({});

    // Регистрация коллектора для Prometheus
    exposer.RegisterCollectable(registry_);
}

void ServerMetrics::incProcessedRequests() {
    if (processed_requests_counter_) {
        processed_requests_counter_->Increment();
    }
}

void ServerMetrics::incRejectedRequests() {
    if (rejected_requests_counter_) {
        rejected_requests_counter_->Increment();
    }
}