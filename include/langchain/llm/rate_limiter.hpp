#pragma once

#include <chrono>
#include <mutex>

namespace langchain::llm {

// A requests-per-second rate limiter using a token-bucket algorithm:
// tokens refill continuously at `requests_per_second`, up to
// `max_bucket_size`; acquire() blocks (sleeping) until a token is
// available. Mirrors LangChain's InMemoryRateLimiter -- this limits
// request *frequency*, not LLM token/cost usage (that needs
// model-specific tokenization, out of scope here).
//
// Thread-safe: a single RateLimiter can be shared (via shared_ptr) across
// multiple RateLimitedChatModel instances -- even wrapping different
// underlying models -- to throttle them all against one combined quota.
class RateLimiter {
public:
    explicit RateLimiter(double requests_per_second, double max_bucket_size = 1.0);

    void acquire();

private:
    double requests_per_second_;
    double max_bucket_size_;
    double available_tokens_;
    std::chrono::steady_clock::time_point last_refill_;
    std::mutex mutex_;
};

} // namespace langchain::llm
