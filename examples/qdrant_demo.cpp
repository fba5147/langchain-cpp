// Demonstrates QdrantVectorStore: the same load/split/index/retrieve/
// answer pipeline as rag_demo.cpp, but backed by a real Qdrant server
// instead of an in-process store -- data survives this process exiting.
// Run this example twice: the second run detects the collection already
// has data (via a probe search) and skips re-indexing, rather than
// inserting duplicate points -- QdrantVectorStore itself doesn't dedupe
// on add_documents(), so doing that check is this demo's job, not the
// library's (see the header comment on QdrantVectorStore for why).
//
// Requires a Qdrant server reachable at QDRANT_URL (default
// http://localhost:6333):
//
//   docker run -p 6333:6333 qdrant/qdrant
//
// Uses MockEmbeddings (deterministic, no network) by default; set
// OPENAI_API_KEY (or create a .env file, see .env.example) to embed with
// OpenAIEmbeddings instead.

#include "langchain/langchain.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>

using namespace langchain;

#ifndef EXAMPLES_DATA_DIR
#define EXAMPLES_DATA_DIR "."
#endif

int main() {
    core::load_dotenv();

    std::shared_ptr<rag::Embeddings> embeddings;
    if (std::getenv("OPENAI_API_KEY") != nullptr) {
        embeddings = std::make_shared<rag::OpenAIEmbeddings>();
    } else {
        std::cout << "No OPENAI_API_KEY set; using MockEmbeddings instead.\n\n";
        embeddings = std::make_shared<rag::MockEmbeddings>();
    }

    rag::QdrantConfig config;
    if (const char* url = std::getenv("QDRANT_URL")) {
        config.url = url;
    }
    config.collection_name = "langchain_cpp_qdrant_demo";
    std::cout << "Qdrant collection: " << config.collection_name << " at " << config.url << "\n\n";

    auto store = std::make_shared<rag::QdrantVectorStore>(embeddings, config);

    std::string question = "What is RAII?";

    if (store->similarity_search(question, 1).empty()) {
        std::cout << "Collection is empty -- loading, splitting, and indexing example documents.\n\n";

        rag::TextLoader raii_loader(std::string(EXAMPLES_DATA_DIR) + "/raii.txt");
        rag::TextLoader vector_db_loader(std::string(EXAMPLES_DATA_DIR) + "/vector_databases.txt");

        std::vector<core::Document> documents = raii_loader.load();
        auto more = vector_db_loader.load();
        documents.insert(documents.end(), more.begin(), more.end());

        rag::RecursiveCharacterTextSplitter splitter(
            rag::RecursiveCharacterTextSplitter::Config{300, 50, {"\n\n", "\n", " ", ""}});
        std::vector<core::Document> chunks = splitter.split_documents(documents);

        store->add_documents(chunks);
    } else {
        std::cout << "Collection already has data from a previous run -- skipping re-indexing.\n\n";
    }

    auto retriever = store->as_retriever(/*k=*/1);
    std::vector<core::Document> retrieved = retriever->invoke(question);

    std::string source = retrieved.front().metadata.value("source", "?");
    std::cout << "Retrieved chunk (source: " << std::filesystem::path(source).filename().string() << "):\n"
              << retrieved.front().content << "\n\n";

    std::cout << "Run this example again -- it'll detect the existing data in Qdrant and skip re-indexing.\n";
    return 0;
}
