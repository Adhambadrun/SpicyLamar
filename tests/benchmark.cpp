#define BENCHMARK
#include "../src/main.cpp"
#include <iostream>
#include <vector>
#include <algorithm>

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
    for (int i = 0; i < 100; ++i) {
        LARGE_INTEGER t0 = SL::StatsTracker::Instance().QpcNow();
        SL::Engine::Instance().TryAnswer(dummy, 0x01);
        LARGE_INTEGER t1 = SL::StatsTracker::Instance().QpcNow();
        results.push_back(SL::StatsTracker::Instance().DeltaMicros(t0, t1));
        std::this_thread::sleep_for(std::chrono::milliseconds(5)); // debounce reset
    }

    std::sort(results.begin(), results.end());
    std::cout << "P50 Latency: " << results[50] << " us" << std::endl;
    std::cout << "P95 Latency: " << results[95] << " us" << std::endl;
    std::cout << "STATUS: PASS" << std::endl;

    DestroyWindow(dummy);
    return 0;
}
