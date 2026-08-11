// Demonstrates the full RAG pipeline offline: load a couple of text files,
// split them into chunks, embed and index them in an in-memory vector
// store, retrieve the most relevant chunk for a question, and hand it to
// a chat model as context.
//
// Uses MockEmbeddings (deterministic, no network) and MockChat by default;
// set OPENAI_API_KEY (or create a .env file, see .env.example) to embed
// with OpenAIEmbeddings and answer with OpenAIChat instead.
//
// Note: today's Runnable doesn't yet have a RunnableParallel/passthrough
// combinator, so `retriever | prompt | model | parser` as one pipe chain
// isn't expressible yet -- the retrieved context and the original
// question both need to flow into the prompt. Until that lands, this
// calls the retriever explicitly and feeds its output into the prompt's
// `context` variable.

#include "langchain/langchain.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>

using namespace langchain;

#ifndef EXAMPLES_DATA_DIR
#define EXAMPLES_DATA_DIR "."
#endif

namespace {

std::string join_contents(const std::vector<core::Document>& documents) {
    std::string context;
    for (const auto& document : documents) {
        if (!context.empty()) {
            context += "\n\n";
        }
        context += document.content;
    }
    return context;
}

} // namespace

int main() {
    core::load_dotenv();

    std::shared_ptr<rag::Embeddings> embeddings;
    std::shared_ptr<llm::ChatModel> model;

    if (std::getenv("OPENAI_API_KEY") != nullptr) {
        embeddings = std::make_shared<rag::OpenAIEmbeddings>();
        model = std::make_shared<providers::OpenAIChat>();
    } else {
        std::cout << "No OPENAI_API_KEY set; using MockEmbeddings + MockChat instead.\n\n";
        embeddings = std::make_shared<rag::MockEmbeddings>();
        model = std::make_shared<providers::MockChat>([](const std::vector<core::Message>& messages) {
            return "Based on the context: " + messages.back().content.substr(0, 60) + "...";
        });
    }

    rag::TextLoader raii_loader(std::string(EXAMPLES_DATA_DIR) + "/raii.txt");
    rag::TextLoader vector_db_loader(std::string(EXAMPLES_DATA_DIR) + "/vector_databases.txt");

    std::vector<core::Document> documents = raii_loader.load();
    auto more = vector_db_loader.load();
    documents.insert(documents.end(), more.begin(), more.end());

    rag::RecursiveCharacterTextSplitter splitter(rag::RecursiveCharacterTextSplitter::Config{300, 50, {"\n\n", "\n", " ", ""}});
    std::vector<core::Document> chunks = splitter.split_documents(documents);

    auto store = std::make_shared<rag::InMemoryVectorStore>(embeddings);
    store->add_documents(chunks);

    auto retriever = store->as_retriever(/*k=*/1);

    std::string question = "What is RAII?";
    std::vector<core::Document> retrieved = retriever->invoke(question);

    std::string source = retrieved.front().metadata.value("source", "?");
    std::cout << "Retrieved chunk (source: " << std::filesystem::path(source).filename().string() << "):\n"
              << retrieved.front().content << "\n\n";

    auto prompt = std::make_shared<prompts::ChatPromptTemplate>(
        std::vector<std::pair<core::MessageRole, std::string>>{
            {core::MessageRole::System, "Answer using only the provided context."},
            {core::MessageRole::User, "Context:\n{context}\n\nQuestion: {question}"},
        });
    auto parser = std::make_shared<parsers::StrOutputParser>();
    auto chain = prompt | model | parser;

    std::string answer =
        chain->invoke({{"context", join_contents(retrieved)}, {"question", question}});

    std::cout << "Answer: " << answer << '\n';
    return 0;
}
