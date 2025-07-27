#include <ImsiGenerator.h>

std::string ImsiGenerator::generate() {
    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, 9);

    std::string imsi;
    for (int i = 0; i < 15; ++i)
        imsi += static_cast<char>('0' + dist(rng));
    return imsi;
}
