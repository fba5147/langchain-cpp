#include "langchain/llm/rate_limiter.hpp"

#include <algorithm>
#include <thread>

namespace langchain::llm {

RateLimiter::RateLimiter(double requests_per_second, double max_bucket_size)
    : requests_per_second_(requests_per_second),
      max_bucket_size_(max_bucket_size),
      available_tokens_(max_bucket_size),
      last_refill_(std::chrono::steady_clock::now()) {}

void RateLimiter::acquire() {
    std::unique_lock<std::mutex> lock(mutex_);

    while (true) {
        auto now = std::chrono::steady_clock::now();
        double elapsed_seconds = std::chrono::duration<double>(now - last_refill_).count();
        available_tokens_ = std::min(max_bucket_size_, available_tokens_ + elapsed_seconds * requests_per_second_);
        last_refill_ = now;

        if (available_tokens_ >= 1.0) {
            available_tokens_ -= 1.0;
            return;
        }

        double deficit = 1.0 - available_tokens_;
        double wait_seconds = deficit / requests_per_second_;

        lock.unlock();
        std::this_thread::sleep_for(std::chrono::duration<double>(wait_seconds));
        lock.lock();
    }
}

} // namespace langchain::llm
