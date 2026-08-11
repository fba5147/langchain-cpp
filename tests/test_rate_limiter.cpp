// RateLimiter's behavior is inherently about real wall-clock time, so
// these tests use generous tolerances (checking "did it block for
// roughly the right ballpark," not an exact duration) to stay reliable
// on a loaded CI machine while still exercising real behavior rather than
// a fake clock.

#include "langchain/llm/rate_limited_chat_model.hpp"
#include "langchain/llm/rate_limiter.hpp"
#include "langchain/providers/mock/mock_chat.hpp"

#include <gtest/gtest.h>

#include <chrono>

using namespace langchain::core;
using namespace langchain::llm;
using namespace langchain::providers;

namespace {

double elapsed_seconds(std::chrono::steady_clock::time_point start) {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}

} // namespace

TEST(RateLimiter, AllowsBurstUpToBucketSizeWithoutBlocking) {
    RateLimiter limiter(/*requests_per_second=*/1000.0, /*max_bucket_size=*/3.0);

    auto start = std::chrono::steady_clock::now();
    limiter.acquire();
    limiter.acquire();
    limiter.acquire();

    EXPECT_LT(elapsed_seconds(start), 0.1); // should be effectively instant
}

TEST(RateLimiter, BlocksOnceBucketIsExhausted) {
    RateLimiter limiter(/*requests_per_second=*/20.0, /*max_bucket_size=*/1.0); // ~50ms per token after the first

    limiter.acquire(); // consumes the initial full bucket, returns immediately

    auto start = std::chrono::steady_clock::now();
    limiter.acquire(); // bucket now empty -- must wait for a refill
    double elapsed = elapsed_seconds(start);

    EXPECT_GT(elapsed, 0.015); // some real blocking occurred (lenient vs. the ~50ms expected)
    EXPECT_LT(elapsed, 2.0);   // and it didn't hang
}

TEST(RateLimitedChatModel, DelegatesToInnerModelUnderAnEffectivelyUnlimitedRate) {
    auto limiter = std::make_shared<RateLimiter>(/*requests_per_second=*/1e6, /*max_bucket_size=*/1e6);
    RateLimitedChatModel model(std::make_shared<MockChat>("fixed reply"), limiter);

    EXPECT_EQ(model.model_name(), "mock-chat");
    EXPECT_EQ(model.invoke({Message::user("hi")}).content, "fixed reply");
}

TEST(RateLimitedChatModel, StreamForwardsChunksThroughTheLimiter) {
    auto limiter = std::make_shared<RateLimiter>(1e6, 1e6);
    RateLimitedChatModel model(std::make_shared<MockChat>("two words"), limiter);

    std::vector<StreamChunk> chunks;
    model.stream({Message::user("hi")}, [&](const StreamChunk& chunk) { chunks.push_back(chunk); });

    ASSERT_FALSE(chunks.empty());
    EXPECT_TRUE(chunks.back().is_final);
    EXPECT_EQ(chunks.back().message.content, "two words");
}

TEST(RateLimitedChatModel, BindToolsWrapsToolBoundInnerModel) {
    auto limiter = std::make_shared<RateLimiter>(1e6, 1e6);
    RateLimitedChatModel model(std::make_shared<MockChat>("reply"), limiter);

    auto bound = model.bind_tools(std::make_shared<langchain::tools::ToolRegistry>());

    EXPECT_EQ(bound->model_name(), "mock-chat");
    EXPECT_EQ(bound->invoke({Message::user("hi")}).content, "reply");
}
