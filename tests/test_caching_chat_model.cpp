#include "langchain/llm/caching_chat_model.hpp"
#include "langchain/llm/chat_model.hpp"
#include "langchain/llm/in_memory_chat_model_cache.hpp"
#include "langchain/tools/tool_registry.hpp"

#include <gtest/gtest.h>

using namespace langchain::core;
using namespace langchain::llm;
using namespace langchain::tools;

namespace {

// Returns a different response each time it's actually called (embedding
// the call count), so a cache hit vs. miss is directly observable: a hit
// returns the same "response #N" as before; a miss bumps N.
class CountingChatModel : public ChatModel {
public:
    int calls = 0;

    Message invoke(const std::vector<Message>& messages) override {
        ++calls;
        return Message::assistant("response #" + std::to_string(calls) + " to " + messages.back().content);
    }

    std::string model_name() const override { return "counting-model"; }

    std::shared_ptr<ChatModel> bind_tools(std::shared_ptr<ToolRegistry>) override {
        return std::make_shared<CountingChatModel>(*this);
    }
};

} // namespace

TEST(InMemoryChatModelCache, GetOnMissingKeyReturnsNullopt) {
    InMemoryChatModelCache cache;
    EXPECT_EQ(cache.get("missing"), std::nullopt);
}

TEST(InMemoryChatModelCache, PutThenGetRoundTrips) {
    InMemoryChatModelCache cache;
    cache.put("key", Message::assistant("value"));

    auto result = cache.get("key");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->content, "value");
}

TEST(CachingChatModel, SecondIdenticalCallHitsCacheAndSkipsInnerModel) {
    auto inner = std::make_shared<CountingChatModel>();
    CachingChatModel model(inner, std::make_shared<InMemoryChatModelCache>());

    Message first = model.invoke({Message::user("what is RAII?")});
    Message second = model.invoke({Message::user("what is RAII?")});

    EXPECT_EQ(inner->calls, 1);
    EXPECT_EQ(first.content, second.content);
}

TEST(CachingChatModel, DifferentMessagesMissCacheAndCallInnerAgain) {
    auto inner = std::make_shared<CountingChatModel>();
    CachingChatModel model(inner, std::make_shared<InMemoryChatModelCache>());

    model.invoke({Message::user("what is RAII?")});
    model.invoke({Message::user("what is a vector database?")});

    EXPECT_EQ(inner->calls, 2);
}

TEST(CachingChatModel, BindToolsChangesCacheKeySoToolBoundAndPlainCallsDontCollide) {
    // Checked via cache entry count rather than inner->calls: CountingChatModel::bind_tools
    // returns a *copy* of itself, so the tool-bound path's inner model is a different
    // object from `inner` -- its own call counter wouldn't be observable through `inner`
    // even if the cache behaved correctly. Cache size is the direct, unambiguous check.
    auto inner = std::make_shared<CountingChatModel>();
    auto cache = std::make_shared<InMemoryChatModelCache>();
    CachingChatModel plain(inner, cache);

    plain.invoke({Message::user("what is RAII?")});

    auto tool_bound = plain.bind_tools(std::make_shared<ToolRegistry>());
    // Same messages, but a (trivially empty) tool registry is now bound --
    // still expected to be treated as a distinct request from the plain one.
    tool_bound->invoke({Message::user("what is RAII?")});

    EXPECT_EQ(cache->size(), 2u);
}

TEST(CachingChatModel, ImagesAttachedToOtherwiseIdenticalMessageChangeCacheKey) {
    auto inner = std::make_shared<CountingChatModel>();
    CachingChatModel model(inner, std::make_shared<InMemoryChatModelCache>());

    model.invoke({Message::user("what's in this?")});
    model.invoke({Message::user_with_images("what's in this?", {ImageContent::from_url("https://example.com/cat.png")})});

    EXPECT_EQ(inner->calls, 2);
}

TEST(CachingChatModel, StreamOnCacheMissDelegatesToInnerAndPopulatesCache) {
    class WordStreamingModel : public ChatModel {
    public:
        Message invoke(const std::vector<Message>&) override { return Message::assistant("hello there"); }
        std::string model_name() const override { return "word-streaming-model"; }
        void stream(const std::vector<Message>& messages, const StreamCallback& on_chunk) override {
            Message result = invoke(messages);
            on_chunk(StreamChunk{"hello", false, {}});
            on_chunk(StreamChunk{" there", false, {}});
            on_chunk(StreamChunk{"", true, result});
        }
    };

    auto inner = std::make_shared<WordStreamingModel>();
    CachingChatModel model(inner, std::make_shared<InMemoryChatModelCache>());

    std::vector<StreamChunk> chunks;
    model.stream({Message::user("hi")}, [&](const StreamChunk& chunk) { chunks.push_back(chunk); });

    ASSERT_EQ(chunks.size(), 3u);
    EXPECT_FALSE(chunks[0].is_final);
    EXPECT_TRUE(chunks.back().is_final);
    EXPECT_EQ(chunks.back().message.content, "hello there");
}

TEST(CachingChatModel, StreamOnCacheHitDeliversSingleFinalChunkWithoutCallingInner) {
    auto inner = std::make_shared<CountingChatModel>();
    CachingChatModel model(inner, std::make_shared<InMemoryChatModelCache>());

    model.invoke({Message::user("hi")}); // populates the cache
    ASSERT_EQ(inner->calls, 1);

    std::vector<StreamChunk> chunks;
    model.stream({Message::user("hi")}, [&](const StreamChunk& chunk) { chunks.push_back(chunk); });

    EXPECT_EQ(inner->calls, 1); // still 1 -- the inner model was not called again
    ASSERT_EQ(chunks.size(), 1u);
    EXPECT_TRUE(chunks[0].is_final);
    EXPECT_EQ(chunks[0].message.content, "response #1 to hi");
}

TEST(CachingChatModel, ModelNameDelegatesToInner) {
    auto inner = std::make_shared<CountingChatModel>();
    CachingChatModel model(inner, std::make_shared<InMemoryChatModelCache>());
    EXPECT_EQ(model.model_name(), "counting-model");
}
