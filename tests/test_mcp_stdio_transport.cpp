// Exercises the real fork/exec/pipe transport against actual child
// processes -- /bin/cat (echoes stdin back on stdout, line by line) and
// /bin/echo (writes one line then exits) are already present on any
// POSIX system, so this validates the transport for real without needing
// an actual MCP server (see examples/mcp_client_demo.cpp for that).

#include "mcp/stdio_transport.hpp"

#include <gtest/gtest.h>

using namespace langchain::mcp::detail;

TEST(StdioTransport, EchoesASingleLineWrittenToStdin) {
    StdioTransport transport({"/bin/cat"});

    transport.write_line("hello mcp");
    auto line = transport.read_line();

    ASSERT_TRUE(line.has_value());
    EXPECT_EQ(*line, "hello mcp");
}

TEST(StdioTransport, EchoesMultipleLinesInOrder) {
    StdioTransport transport({"/bin/cat"});

    transport.write_line("first");
    transport.write_line("second");
    transport.write_line("third");

    EXPECT_EQ(transport.read_line(), "first");
    EXPECT_EQ(transport.read_line(), "second");
    EXPECT_EQ(transport.read_line(), "third");
}

TEST(StdioTransport, ReadLineReturnsNulloptAfterChildExits) {
    // /bin/echo writes one line to stdout and exits on its own -- no
    // write_line() needed, its argv is the message.
    StdioTransport transport({"/bin/echo", "one line then exit"});

    EXPECT_EQ(transport.read_line(), "one line then exit");
    EXPECT_EQ(transport.read_line(), std::nullopt);
}

TEST(StdioTransport, ThrowsWhenCommandIsEmpty) {
    EXPECT_THROW(StdioTransport({}), std::runtime_error);
}
