// Demonstrates RateLimitedChatModel actually throttling calls: with 2
// requests/second and a bucket size of 1, the first call goes through
// immediately (the bucket starts full) and each call after that is
// spaced roughly 0.5s apart. Fully offline -- MockChat, no API key.

#include "langchain/langchain.hpp"

#include <chrono>
#include <iostream>

using namespace langchain;

int main() {
    auto limiter = std::make_shared<llm::RateLimiter>(/*requests_per_second=*/2.0, /*max_bucket_size=*/1.0);
    auto model = std::make_shared<llm::RateLimitedChatModel>(std::make_shared<providers::MockChat>("ok"), limiter);

    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < 4; ++i) {
        model->invoke({core::Message::user("hi")});
        double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        std::cout << "call " << i + 1 << " completed at t=" << elapsed << "s\n";
    }

    std::cout << "\nWith 2 requests/sec and a bucket size of 1, calls after the first should land roughly\n"
                 "0.5s apart -- not all four in a burst.\n";
    return 0;
}
