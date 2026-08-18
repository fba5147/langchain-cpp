#include "mcp/http_client_transport.hpp"

#include <gtest/gtest.h>

using namespace langchain::mcp::detail;

TEST(ExtractLastSseEventData, SingleEventWithTrailingBlankLine) {
    std::string body = "event: message\nid: abc\ndata: {\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{}}\n\n";

    auto data = extract_last_sse_event_data(body);

    ASSERT_TRUE(data.has_value());
    EXPECT_EQ(*data, "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{}}");
}

TEST(ExtractLastSseEventData, NoTrailingBlankLineStillExtractsLastEvent) {
    std::string body = "event: message\ndata: {\"result\":42}";

    auto data = extract_last_sse_event_data(body);

    ASSERT_TRUE(data.has_value());
    EXPECT_EQ(*data, "{\"result\":42}");
}

TEST(ExtractLastSseEventData, MultipleEventsReturnsTheLastOne) {
    std::string body = "data: {\"jsonrpc\":\"2.0\",\"method\":\"notifications/progress\"}\n\n"
                        "data: {\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{\"ok\":true}}\n\n";

    auto data = extract_last_sse_event_data(body);

    ASSERT_TRUE(data.has_value());
    EXPECT_EQ(*data, "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{\"ok\":true}}");
}

TEST(ExtractLastSseEventData, MultiLineDataFieldIsJoinedWithNewline) {
    std::string body = "data: line one\ndata: line two\n\n";

    auto data = extract_last_sse_event_data(body);

    ASSERT_TRUE(data.has_value());
    EXPECT_EQ(*data, "line one\nline two");
}

TEST(ExtractLastSseEventData, IgnoresNonDataFieldsLikeEventAndId) {
    std::string body = ": this is a comment\nevent: message\nid: xyz\nretry: 1000\ndata: {\"result\":1}\n\n";

    auto data = extract_last_sse_event_data(body);

    ASSERT_TRUE(data.has_value());
    EXPECT_EQ(*data, "{\"result\":1}");
}

TEST(ExtractLastSseEventData, EmptyBodyReturnsNullopt) { EXPECT_FALSE(extract_last_sse_event_data("").has_value()); }

TEST(ExtractLastSseEventData, BodyWithNoDataFieldReturnsNullopt) {
    EXPECT_FALSE(extract_last_sse_event_data("event: message\nid: abc\n\n").has_value());
}
