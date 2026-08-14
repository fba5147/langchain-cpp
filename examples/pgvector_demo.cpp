// Demonstrates PgVectorStore: the same load/split/index/retrieve/answer
// pipeline as rag_demo.cpp/qdrant_demo.cpp, but backed by a real Postgres
// database with the pgvector extension. Run this example twice: the
// second run detects the table already has data (via a probe search) and
// skips re-indexing, rather than inserting duplicate rows --
// PgVectorStore itself doesn't dedupe on add_documents(), so doing that
// check is this demo's job, not the library's (see qdrant_demo.cpp for
// the same pattern).
//
// Requires a Postgres server with the pgvector extension reachable via
// PGVECTOR_CONNECTION_STRING (default below):
//
//   docker run -p 5432:5432 -e POSTGRES_PASSWORD=postgres pgvector/pgvector:pg16
//   psql -h localhost -U postgres -c "CREATE EXTENSION IF NOT EXISTS vector;"
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

    rag::PgVectorConfig config;
    if (const char* connection_string = std::getenv("PGVECTOR_CONNECTION_STRING")) {
        config.connection_string = connection_string;
    }
    config.table_name = "langchain_cpp_pgvector_demo";
    std::cout << "pgvector table: " << config.table_name << "\n\n";

    std::shared_ptr<rag::PgVectorStore> store;
    try {
        store = std::make_shared<rag::PgVectorStore>(embeddings, config);
    } catch (const std::exception& error) {
        std::cerr << "Could not connect to Postgres: " << error.what() << "\n\n"
                  << "Start one with:\n"
                  << "  docker run -p 5432:5432 -e POSTGRES_PASSWORD=postgres pgvector/pgvector:pg16\n"
                  << "  psql -h localhost -U postgres -c \"CREATE EXTENSION IF NOT EXISTS vector;\"\n";
        return 1;
    }

    std::string question = "What is RAII?";

    if (store->similarity_search(question, 1).empty()) {
        std::cout << "Table is empty -- loading, splitting, and indexing example documents.\n\n";

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
        std::cout << "Table already has data from a previous run -- skipping re-indexing.\n\n";
    }

    auto retriever = store->as_retriever(/*k=*/1);
    std::vector<core::Document> retrieved = retriever->invoke(question);

    std::string source = retrieved.front().metadata.value("source", "?");
    std::cout << "Retrieved chunk (source: " << std::filesystem::path(source).filename().string() << "):\n"
              << retrieved.front().content << "\n\n";

    std::cout << "Run this example again -- it'll detect the existing data in Postgres and skip re-indexing.\n";
    return 0;
}
