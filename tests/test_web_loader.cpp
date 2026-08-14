#include "langchain/rag/loaders/web_loader.hpp"

#include "rag/loaders/html_to_text.hpp"
#include "support/mock_http_server.hpp"

#include <gtest/gtest.h>

#include <stdexcept>

using namespace langchain::rag;
using namespace langchain::rag::detail;
using namespace langchain::testing;

// --- html_to_text (pure, no server involved) ---

TEST(HtmlToText, StripsTagsAndKeepsText) {
    EXPECT_EQ(html_to_text("<p>Hello <b>world</b></p>"), "Hello world");
}

TEST(HtmlToText, DropsScriptAndStyleContentEntirely) {
    std::string html = "<html><head><style>body { color: red; }</style></head>"
                        "<body><script>alert('hi');</script><p>Visible text</p></body></html>";
    EXPECT_EQ(html_to_text(html), "Visible text");
}

TEST(HtmlToText, InsertsLineBreaksAtBlockBoundaries) {
    EXPECT_EQ(html_to_text("<p>First</p><p>Second</p>"), "First\nSecond");
}

TEST(HtmlToText, ConvertsBrToNewline) {
    EXPECT_EQ(html_to_text("Line one<br>Line two"), "Line one\nLine two");
}

TEST(HtmlToText, DecodesCommonEntities) {
    EXPECT_EQ(html_to_text("Q&amp;A &lt;tag&gt; &quot;quoted&quot; caf&eacute;-like&nbsp;text"),
              "Q&A <tag> \"quoted\" caf&eacute;-like text");
}

TEST(HtmlToText, CollapsesRepeatedWhitespaceAndTrims) {
    EXPECT_EQ(html_to_text("  <p>  a   b  </p>  "), "a b");
}

TEST(HtmlToText, EmptyInputYieldsEmptyString) {
    EXPECT_EQ(html_to_text(""), "");
}

// --- WebLoader (a real HTTP round trip against a local mock server) ---

TEST(WebLoader, FetchesAndStripsHtmlFromA200Response) {
    MockHttpServer server([](const MockHttpRequest&) {
        return MockHttpResponse{200, "<html><body><h1>Title</h1><p>Some body text.</p></body></html>", "text/html"};
    });

    WebLoader loader(server.base_url() + "/page");
    auto documents = loader.load();

    ASSERT_EQ(documents.size(), 1u);
    EXPECT_EQ(documents[0].content, "Title\nSome body text.");
    EXPECT_EQ(documents[0].metadata["source"], server.base_url() + "/page");
}

TEST(WebLoader, ThrowsOnNonSuccessStatus) {
    MockHttpServer server([](const MockHttpRequest&) { return MockHttpResponse{404, "not found", "text/plain"}; });

    WebLoader loader(server.base_url() + "/missing");
    EXPECT_THROW(loader.load(), std::runtime_error);
}
