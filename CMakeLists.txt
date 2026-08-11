cmake_minimum_required(VERSION 3.21)
project(langchain-cpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(LANGCHAIN_BUILD_EXAMPLES "Build example programs" ON)
option(LANGCHAIN_BUILD_TESTS "Build unit tests" ON)
option(LANGCHAIN_BUILD_BENCHMARKS "Build benchmark executables" ON)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(cpr CONFIG REQUIRED)

# FAISS's CMake config target name isn't consistent: vcpkg's port exports
# the namespaced faiss::faiss, Homebrew's bottle exports a bare `faiss`
# (no cmake config at all on some Homebrew versions) -- try each in turn,
# falling back to a manual find_path/find_library as a last resort.
find_package(faiss CONFIG QUIET)
if(TARGET faiss::faiss)
    set(LANGCHAIN_FAISS_TARGET faiss::faiss)
elseif(TARGET faiss)
    set(LANGCHAIN_FAISS_TARGET faiss)
else()
    find_path(FAISS_INCLUDE_DIR faiss/IndexFlat.h)
    find_library(FAISS_LIBRARY faiss)
    if(NOT FAISS_INCLUDE_DIR OR NOT FAISS_LIBRARY)
        message(FATAL_ERROR "faiss not found (no CMake config, and no faiss/IndexFlat.h + libfaiss via "
                             "find_path/find_library). Install it (e.g. `brew install faiss`) or via vcpkg.")
    endif()
    add_library(faiss::faiss UNKNOWN IMPORTED)
    set_target_properties(faiss::faiss PROPERTIES
        IMPORTED_LOCATION "${FAISS_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${FAISS_INCLUDE_DIR}"
    )
    set(LANGCHAIN_FAISS_TARGET faiss::faiss)
endif()

# poppler-cpp has no consistent CMake config across vcpkg/Homebrew either;
# pkg-config is what poppler's own vcpkg port documents using.
find_package(PkgConfig REQUIRED)
pkg_check_modules(POPPLER_CPP REQUIRED IMPORTED_TARGET poppler-cpp)

add_library(langchain_core
    src/core/message.cpp
    src/core/dotenv.cpp
    src/core/callbacks.cpp
    src/core/similarity.cpp
    src/callbacks/console_callback_handler.cpp
    src/callbacks/callbacking_chat_model.cpp
    src/callbacks/callbacking_tool.cpp
    src/llm/in_memory_chat_model_cache.cpp
    src/llm/caching_chat_model.cpp
    src/llm/in_memory_chat_message_history.cpp
    src/llm/file_chat_message_history.cpp
    src/llm/chat_model_with_history.cpp
    src/llm/rate_limiter.cpp
    src/llm/rate_limited_chat_model.cpp
    src/prompts/prompt_template.cpp
    src/prompts/chat_prompt_template.cpp
    src/prompts/few_shot_prompt_template.cpp
    src/prompts/semantic_similarity_example_selector.cpp
    src/providers/mock/mock_chat.cpp
    src/providers/openai/openai_wire_format.cpp
    src/providers/openai/openai_chat.cpp
    src/providers/openai/openai_compatible_presets.cpp
    src/providers/anthropic/anthropic_wire_format.cpp
    src/providers/anthropic/anthropic_chat.cpp
    src/providers/azure/azure_openai_chat.cpp
    src/providers/google/gemini_wire_format.cpp
    src/providers/google/gemini_chat.cpp
    src/mcp/jsonrpc.cpp
    src/mcp/mcp_protocol.cpp
    src/mcp/stdio_transport.cpp
    src/mcp/mcp_client.cpp
    src/mcp/mcp_server.cpp
    src/mcp/mcp_tools.cpp
    src/parsers/json_parser.cpp
    src/tools/function_tool.cpp
    src/tools/tool_registry.cpp
    src/agents/agent_executor.cpp
    src/rag/loaders/text_loader.cpp
    src/rag/loaders/markdown_loader.cpp
    src/rag/splitters/recursive_character_text_splitter.cpp
    src/rag/embeddings/mock_embeddings.cpp
    src/rag/embeddings/openai_embeddings.cpp
    src/rag/vectorstores/vector_store.cpp
    src/rag/vectorstores/in_memory_vector_store.cpp
    src/rag/vectorstores/faiss_vector_store.cpp
    src/rag/loaders/pdf_loader.cpp
    src/rag/retrievers/retriever.cpp
    src/rag/format_documents.cpp
)
add_library(langchain::core ALIAS langchain_core)

target_include_directories(langchain_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(langchain_core PUBLIC nlohmann_json::nlohmann_json cpr::cpr ${LANGCHAIN_FAISS_TARGET} PkgConfig::POPPLER_CPP)

if(LANGCHAIN_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(LANGCHAIN_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(LANGCHAIN_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks/cpp)
endif()
