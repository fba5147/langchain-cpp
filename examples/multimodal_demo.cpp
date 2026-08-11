// Demonstrates multi-modal messages: Message::user_with_images() attaches
// images alongside text, ImageContent::from_file() base64-encodes a local
// file, and OpenAIChat (via the shared openai_wire_format module) is the
// only provider that currently understands images -- AnthropicChat and
// GeminiChat throw a clear error rather than silently dropping them.
//
// Uses a scripted MockChat, so this runs fully offline; no image files or
// API keys are needed.

#include "langchain/langchain.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

using namespace langchain;

int main() {
    // A URL image needs no local file at all.
    core::Message url_message = core::Message::user_with_images(
        "What's in this picture?", {core::ImageContent::from_url("https://example.com/cat.png")});

    // A local file gets base64-encoded and its media type guessed from
    // the extension.
    auto image_path = std::filesystem::temp_directory_path() / "langchain_cpp_multimodal_demo.png";
    {
        std::ofstream file(image_path, std::ios::binary);
        file << "not a real png, just enough bytes to demonstrate encoding";
    }
    core::ImageContent local_image = core::ImageContent::from_file(image_path.string());
    core::Message file_message = core::Message::user_with_images("And this one?", {local_image});
    std::filesystem::remove(image_path);

    std::cout << "URL image   -> media_type=\"" << url_message.images[0].media_type << "\" data=\""
              << url_message.images[0].data << "\"\n";
    std::cout << "Local image -> media_type=\"" << local_image.media_type << "\" base64 length="
              << local_image.data.size() << "\n\n";

    // A model that just reports what it was given -- stands in for any
    // real ChatModel, since every provider receives the same Message type.
    auto model = std::make_shared<providers::MockChat>([](const std::vector<core::Message>& messages) {
        const core::Message& last = messages.back();
        return "I received " + std::to_string(last.images.size()) + " image(s) alongside: \"" + last.content + "\"";
    });

    std::cout << "User:      " << url_message.content << " [1 image attached]\n";
    std::cout << "Assistant: " << model->invoke({url_message}).content << "\n\n";

    // OpenAIChat (and AzureOpenAIChat, which shares the same wire format)
    // encode images as OpenAI's content-parts array. AnthropicChat and
    // GeminiChat don't support images yet, so they throw immediately --
    // before any network call is made -- rather than silently dropping
    // the image and confusing the model.
    providers::AnthropicConfig anthropic_config;
    anthropic_config.api_key = "unused-no-network-call-happens";
    providers::AnthropicChat anthropic(anthropic_config);
    try {
        anthropic.invoke({url_message});
        std::cout << "(unexpected: AnthropicChat did not throw)\n";
    } catch (const std::exception& error) {
        std::cout << "AnthropicChat with an image: " << error.what() << "\n";
    }

    providers::GeminiConfig gemini_config;
    gemini_config.api_key = "unused-no-network-call-happens";
    providers::GeminiChat gemini(gemini_config);
    try {
        gemini.invoke({url_message});
        std::cout << "(unexpected: GeminiChat did not throw)\n";
    } catch (const std::exception& error) {
        std::cout << "GeminiChat with an image:    " << error.what() << "\n";
    }

    std::cout << "\nOpenAIChat (and AzureOpenAIChat) are the providers that currently support images.\n";
    return 0;
}
