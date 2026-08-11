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
    src/rag/retrievers/retriever.cpp
    src/rag/format_documents.cpp
)
add_library(langchain::core ALIAS langchain_core)

target_include_directories(langchain_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(langchain_core PUBLIC nlohmann_json::nlohmann_json cpr::cpr)

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
