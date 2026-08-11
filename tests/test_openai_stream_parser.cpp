// Unit tests for the SSE stream parser shared by OpenAIChat and
// AzureOpenAIChat. Verified against live Ollama streaming output (see
// examples/streaming_demo.cpp) for the real end-to-end wire format; these
// tests pin down the parsing/accumulation logic itself, including edge
// cases (lines split across feed() calls) that are awkward to provoke
// reliably against a live server.

#include "providers/openai/openai_wire_format.hpp"

#include <gtest/gtest.h>

using namespace langchain::providers::detail;

namespace {

std::string sse_line(const std::string& json_payload) { return "data: " + json_payload + "\n\n"; }

} // namespace

TEST(OpenAiStreamParser, AccumulatesTextDeltasAndReportsThemAsTheyArrive) {
    OpenAiStreamParser parser;
    std::vector<std::string> deltas;
    auto on_delta = [&](const std::string& delta) { deltas.push_back(delta); };

    parser.feed(sse_line(R"({"choices":[{"delta":{"content":"Hello"}}]})"), on_delta);
    parser.feed(sse_line(R"({"choices":[{"delta":{"content":", world"}}]})"), on_delta);
    parser.feed("data: [DONE]\n\n", on_delta);

    ASSERT_EQ(deltas.size(), 2u);
    EXPECT_EQ(deltas[0], "Hello");
    EXPECT_EQ(deltas[1], ", world");

    auto message = parser.finish();
    EXPECT_EQ(message.content, "Hello, world");
    EXPECT_FALSE(message.has_tool_calls());
}

TEST(OpenAiStreamParser, HandlesLinesSplitAcrossMultipleFeedCalls) {
    OpenAiStreamParser parser;
    std::vector<std::string> deltas;
    auto on_delta = [&](const std::string& delta) { deltas.push_back(delta); };

    std::string line = sse_line(R"({"choices":[{"delta":{"content":"partial"}}]})");
    parser.feed(line.substr(0, 20), on_delta);
    EXPECT_TRUE(deltas.empty()); // nothing yet -- the line isn't complete

    parser.feed(line.substr(20), on_delta);
    ASSERT_EQ(deltas.size(), 1u);
    EXPECT_EQ(deltas[0], "partial");
}

TEST(OpenAiStreamParser, AccumulatesToolCallArgumentFragmentsAcrossChunks) {
    // Each fed line must stay on a single line -- feed() splits on '\n',
    // and the real protocol never embeds a raw newline inside one SSE
    // "data: " event, so a literal newline here would (correctly) get
    // treated as ending the JSON early.
    OpenAiStreamParser parser;
    auto on_delta = [](const std::string&) {};

    parser.feed(sse_line(R"({"choices":[{"delta":{"tool_calls":[{"index":0,"id":"call_1","function":{"name":"calculator","arguments":""}}]}}]})"),
                on_delta);
    parser.feed(sse_line(R"({"choices":[{"delta":{"tool_calls":[{"index":0,"function":{"arguments":"{\"a\":1,"}}]}}]})"),
                on_delta);
    parser.feed(sse_line(R"({"choices":[{"delta":{"tool_calls":[{"index":0,"function":{"arguments":"\"b\":2}"}}]}}]})"),
                on_delta);
    parser.feed("data: [DONE]\n\n", on_delta);

    auto message = parser.finish();
    ASSERT_TRUE(message.has_tool_calls());
    ASSERT_EQ(message.tool_calls.size(), 1u);
    EXPECT_EQ(message.tool_calls[0].id, "call_1");
    EXPECT_EQ(message.tool_calls[0].tool_name, "calculator");
    EXPECT_EQ(message.tool_calls[0].arguments["a"], 1);
    EXPECT_EQ(message.tool_calls[0].arguments["b"], 2);
}

TEST(OpenAiStreamParser, WholeToolCallInASingleDeltaWorksToo) {
    // Matches what Ollama has been observed sending: the full tool call,
    // arguments included, in one delta rather than fragmented.
    OpenAiStreamParser parser;
    auto on_delta = [](const std::string&) {};

    parser.feed(sse_line(R"({"choices":[{"delta":{"tool_calls":[{"index":0,"id":"call_1","function":{"name":"calculator","arguments":"{\"a\":1,\"b\":2}"}}]}}]})"),
                on_delta);
    parser.feed("data: [DONE]\n\n", on_delta);

    auto message = parser.finish();
    ASSERT_TRUE(message.has_tool_calls());
    EXPECT_EQ(message.tool_calls[0].arguments["a"], 1);
    EXPECT_EQ(message.tool_calls[0].arguments["b"], 2);
}

TEST(OpenAiStreamParser, FeedReturnsFalseOnceDoneMarkerSeen) {
    OpenAiStreamParser parser;
    auto on_delta = [](const std::string&) {};

    EXPECT_TRUE(parser.feed(sse_line(R"({"choices":[{"delta":{"content":"hi"}}]})"), on_delta));
    EXPECT_FALSE(parser.feed("data: [DONE]\n\n", on_delta));
}

TEST(OpenAiStreamParser, NoContentOrToolCallsProducesEmptyAssistantMessage) {
    OpenAiStreamParser parser;
    auto message = parser.finish();
    EXPECT_EQ(message.content, "");
    EXPECT_FALSE(message.has_tool_calls());
}
