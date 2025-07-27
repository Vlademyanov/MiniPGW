#include <Metrics.h>
using namespace prometheus;

std::shared_ptr<Registry> Metrics::registry_;
Counter* Metrics::requests_counter_ = nullptr;

void Metrics::init(int port) {
    static Exposer exposer{"0.0.0.0:" + std::to_string(port)};
    registry_ = std::make_shared<Registry>();

    auto& fam = BuildCounter()
        .Name("pgw_imsi_requests_total")
        .Help("Total number of IMSI requests sent")
        .Register(*registry_);
    requests_counter_ = &fam.Add({});

    exposer.RegisterCollectable(registry_);
}

void Metrics::incRequests() {
    if (requests_counter_) {
        requests_counter_->Increment();
    }
}