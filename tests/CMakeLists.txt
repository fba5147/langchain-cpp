find_package(GTest CONFIG REQUIRED)

add_executable(langchain_tests
    test_message.cpp
    test_prompt_template.cpp
    test_runnable.cpp
    test_mock_chat.cpp
    test_json_parser.cpp
    test_tools.cpp
    test_agent_executor.cpp
    test_loaders.cpp
    test_text_splitter.cpp
    test_embeddings.cpp
    test_vector_store.cpp
    test_dotenv.cpp
    test_gemini_wire_format.cpp
    test_openai_message_wire_format.cpp
    test_anthropic_wire_format.cpp
    test_azure_url.cpp
    test_format_documents.cpp
    test_openai_stream_parser.cpp
    test_callbacks.cpp
    test_caching_chat_model.cpp
    test_chat_message_history.cpp
    test_similarity.cpp
    test_few_shot_prompt_template.cpp
    test_output_fixing_parser.cpp
    test_rate_limiter.cpp
    test_mcp_jsonrpc.cpp
    test_mcp_stdio_transport.cpp
    test_faiss_vector_store.cpp
    test_pdf_loader.cpp
    support/mock_http_server.cpp
    test_anthropic_chat_live_contract.cpp
    test_gemini_chat_live_contract.cpp
    test_qdrant_wire_format.cpp
    test_qdrant_vector_store_live.cpp
    test_pgvector_sql.cpp
    test_pgvector_store_live.cpp
    test_csv_loader.cpp
    test_web_loader.cpp
)
# test_mcp_server_roundtrip.cpp needs the built mcp_server_demo binary --
# only meaningful (and only buildable, since it needs the real path) when
# examples are also being built.
if(TARGET mcp_server_demo)
    target_sources(langchain_tests PRIVATE test_mcp_server_roundtrip.cpp)
    target_compile_definitions(langchain_tests PRIVATE MCP_SERVER_DEMO_PATH="$<TARGET_FILE:mcp_server_demo>")
endif()

# PostgreSQL::PostgreSQL is linked PRIVATE by langchain_core, so its
# include directories (needed by test_pgvector_store_live.cpp's direct
# `#include <libpq-fe.h>`) don't propagate here automatically -- link it
# directly too (same PRIVATE-doesn't-propagate-includes reasoning as the
# top-level CMakeLists.txt comment on faiss/poppler-cpp).
target_link_libraries(langchain_tests PRIVATE langchain::core PostgreSQL::PostgreSQL GTest::gtest GTest::gtest_main)

# Lets tests reach internal (non-public-API) provider wire-format headers
# under src/, e.g. "providers/google/gemini_wire_format.hpp" -- these
# aren't installed/exposed under include/, but unit-testing the pure
# conversion logic is valuable where there's no live endpoint to
# smoke-test against (see test_gemini_wire_format.cpp).
target_include_directories(langchain_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})

include(GoogleTest)
gtest_discover_tests(langchain_tests)
