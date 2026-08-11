#include "langchain/core/message.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

using namespace langchain::core;

TEST(Message, FactoriesSetRoleAndContent) {
    EXPECT_EQ(Message::system("s").role, MessageRole::System);
    EXPECT_EQ(Message::user("u").role, MessageRole::User);
    EXPECT_EQ(Message::assistant("a").role, MessageRole::Assistant);
    EXPECT_EQ(Message::tool_result("call_1", "t").role, MessageRole::Tool);
    EXPECT_EQ(Message::user("hello").content, "hello");
}

TEST(Message, ToApiRoleMapsToWireStrings) {
    EXPECT_EQ(to_api_role(MessageRole::System), "system");
    EXPECT_EQ(to_api_role(MessageRole::User), "user");
    EXPECT_EQ(to_api_role(MessageRole::Assistant), "assistant");
    EXPECT_EQ(to_api_role(MessageRole::Tool), "tool");
}

TEST(Message, AssistantToolCallsCarriesCallsAndOptionalContent) {
    Message message = Message::assistant_tool_calls({ToolCall{"call_1", "add", {{"a", 1}, {"b", 2}}}}, "thinking...");
    EXPECT_EQ(message.role, MessageRole::Assistant);
    EXPECT_TRUE(message.has_tool_calls());
    ASSERT_EQ(message.tool_calls.size(), 1u);
    EXPECT_EQ(message.tool_calls[0].id, "call_1");
    EXPECT_EQ(message.tool_calls[0].tool_name, "add");
    EXPECT_EQ(message.tool_calls[0].arguments["a"], 1);
    EXPECT_EQ(message.content, "thinking...");
}

TEST(Message, ToolResultCarriesCallId) {
    Message message = Message::tool_result("call_1", "3");
    EXPECT_EQ(message.role, MessageRole::Tool);
    EXPECT_EQ(message.tool_call_id, "call_1");
    EXPECT_EQ(message.content, "3");
    EXPECT_FALSE(message.has_tool_calls());
}

TEST(Message, MessagesWithoutImagesReportHasImagesFalse) {
    EXPECT_FALSE(Message::user("hi").has_images());
}

TEST(Message, UserWithImagesCarriesTextAndImages) {
    Message message =
        Message::user_with_images("what's in this?", {ImageContent::from_url("https://example.com/cat.png")});

    EXPECT_EQ(message.role, MessageRole::User);
    EXPECT_EQ(message.content, "what's in this?");
    EXPECT_TRUE(message.has_images());
    ASSERT_EQ(message.images.size(), 1u);
    EXPECT_EQ(message.images[0].source_type, ImageSourceType::Url);
    EXPECT_EQ(message.images[0].data, "https://example.com/cat.png");
}

TEST(ImageContent, FromUrlDefaultsMediaTypeToEmpty) {
    auto image = ImageContent::from_url("https://example.com/cat.png");
    EXPECT_EQ(image.source_type, ImageSourceType::Url);
    EXPECT_TRUE(image.media_type.empty());
}

TEST(ImageContent, FromFileBase64EncodesBytesAndGuessesMediaType) {
    auto path = std::filesystem::temp_directory_path() / "langchain_cpp_image_content_test.png";
    {
        std::ofstream file(path, std::ios::binary);
        file << "not a real png, just some bytes";
    }

    auto image = ImageContent::from_file(path.string());

    EXPECT_EQ(image.source_type, ImageSourceType::Base64);
    EXPECT_EQ(image.media_type, "image/png");
    EXPECT_FALSE(image.data.empty());
    // Standard base64 alphabet only.
    EXPECT_EQ(image.data.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/="),
              std::string::npos);

    std::filesystem::remove(path);
}

TEST(ImageContent, FromFileThrowsWhenFileMissing) {
    EXPECT_THROW(ImageContent::from_file("/no/such/image.png"), std::runtime_error);
}

TEST(ImageContent, FromFileThrowsWhenExtensionUnknownAndNoMediaTypeGiven) {
    auto path = std::filesystem::temp_directory_path() / "langchain_cpp_image_content_test.unknownext";
    {
        std::ofstream file(path, std::ios::binary);
        file << "bytes";
    }

    EXPECT_THROW(ImageContent::from_file(path.string()), std::runtime_error);

    std::filesystem::remove(path);
}

TEST(ImageContent, FromFileAcceptsExplicitMediaTypeOverridingGuess) {
    auto path = std::filesystem::temp_directory_path() / "langchain_cpp_image_content_test.png";
    {
        std::ofstream file(path, std::ios::binary);
        file << "bytes";
    }

    auto image = ImageContent::from_file(path.string(), "image/custom");
    EXPECT_EQ(image.media_type, "image/custom");

    std::filesystem::remove(path);
}
