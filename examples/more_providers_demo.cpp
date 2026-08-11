// Demonstrates the newer providers: GeminiChat, AzureOpenAIChat, and the
// OpenAI-compatible presets (GroqChat/MistralChat/DeepSeekChat). Each
// section only actually calls out if its credentials are configured
// (via real env vars or a .env file, see .env.example); otherwise it
// prints what it would have done and moves on, so this always runs to
// completion with no configuration at all.

#include "langchain/langchain.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>

using namespace langchain;

namespace {

// A bad/expired key in one section (a real request failure, not a missing
// env var) shouldn't take down the rest of the demo.
void run_section(void (*section)()) {
    try {
        section();
    } catch (const std::exception& e) {
        std::cout << "failed: " << e.what() << "\n\n";
    }
}

void try_gemini() {
    std::cout << "--- Gemini ---\n";
    if (std::getenv("GOOGLE_API_KEY") == nullptr) {
        std::cout << "GOOGLE_API_KEY not set; skipping.\n\n";
        return;
    }
    providers::GeminiChat model;
    core::Message reply = model.invoke({core::Message::user("Say hello in exactly 3 words.")});
    std::cout << "[" << model.model_name() << "] " << reply.content << "\n\n";
}

void try_azure_openai() {
    std::cout << "--- Azure OpenAI ---\n";
    const char* deployment = std::getenv("AZURE_OPENAI_DEPLOYMENT");
    if (std::getenv("AZURE_OPENAI_API_KEY") == nullptr || std::getenv("AZURE_OPENAI_ENDPOINT") == nullptr ||
        deployment == nullptr) {
        std::cout << "AZURE_OPENAI_API_KEY / AZURE_OPENAI_ENDPOINT / AZURE_OPENAI_DEPLOYMENT not all set; "
                     "skipping.\n\n";
        return;
    }
    providers::AzureOpenAIChat model(providers::AzureOpenAIConfig{.deployment = deployment});
    core::Message reply = model.invoke({core::Message::user("Say hello in exactly 3 words.")});
    std::cout << "[" << model.model_name() << "] " << reply.content << "\n\n";
}

void try_groq() {
    std::cout << "--- Groq (OpenAI-compatible preset) ---\n";
    if (std::getenv("GROQ_API_KEY") == nullptr) {
        std::cout << "GROQ_API_KEY not set; skipping.\n\n";
        return;
    }
    auto model = providers::GroqChat();
    core::Message reply = model->invoke({core::Message::user("Say hello in exactly 3 words.")});
    std::cout << "[" << model->model_name() << "] " << reply.content << "\n\n";
}

} // namespace

int main() {
    core::load_dotenv();

    run_section(try_gemini);
    run_section(try_azure_openai);
    run_section(try_groq);

    return 0;
}
