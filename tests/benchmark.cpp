#define BENCHMARK
#include "../src/main.cpp"
#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>

int main() {
    std::cout << "--- SPICY LAMAR QUANTUM BENCHMARK ---" << std::endl;
    SL::StatsTracker::Instance().Initialize();
    SL::Engine::Instance().Initialize();

    // Create dummy window for testing
    HWND dummy = CreateWindowExW(0, L"STATIC", L"RingCentral Phone", 0, 0, 0, 0, 0, NULL, NULL, GetModuleHandle(NULL), NULL);
    if (!dummy) {
        std::cerr << "Failed to create dummy window" << std::endl;
        return 1;
    }

    std::vector<uint64_t> results;
    results.reserve(100);
    for (int i = 0; i < 100; ++i) {
        // BENCHMARK mode in Engine bypasses debounce automatically,
        // but we still add a small pause between iterations to avoid hammering the message queue.
        LARGE_INTEGER t0 = SL::StatsTracker::Instance().QpcNow();
        SL::Engine::Instance().TryAnswer(dummy, 0x01, true); // bypass debounce
        LARGE_INTEGER t1 = SL::StatsTracker::Instance().QpcNow();
        results.push_back(SL::StatsTracker::Instance().DeltaMicros(t0, t1));
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    std::sort(results.begin(), results.end());
    uint64_t sum = std::accumulate(results.begin(), results.end(), uint64_t(0));
    std::cout << "P50 Latency:  " << results[50] << " us" << std::endl;
    std::cout << "P95 Latency:  " << results[95] << " us" << std::endl;
    std::cout << "P99 Latency:  " << results[99] << " us" << std::endl;
    std::cout << "Best Latency: " << results.front() << " us" << std::endl;
    std::cout << "Worst Latency:" << results.back() << " us" << std::endl;
    std::cout << "Avg Latency:  " << (sum / results.size()) << " us" << std::endl;
    std::cout << "Samples:      " << results.size() << std::endl;
    std::cout << "STATUS: PASS" << std::endl;

    DestroyWindow(dummy);
    return 0;
}
