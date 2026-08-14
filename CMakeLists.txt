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

# vcpkg's faiss port links OpenMP::OpenMP_CXX into its exported CMake
# config, so that target must exist in this project's own scope before
# find_package(faiss ...) resolves it below -- otherwise: "the link
# interface of target faiss contains OpenMP::OpenMP_CXX but the target
# was not found." AppleClang doesn't support OpenMP out of the box, and
# CMake's FindOpenMP can't auto-detect it even with libomp installed --
# it needs the exact compiler flags and library path spelled out (see
# triplets/arm64-osx.cmake for the vcpkg-side half of this same fix).
if(APPLE AND CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" AND NOT OpenMP_CXX_FOUND)
    find_program(LANGCHAIN_BREW_EXECUTABLE brew)
    if(LANGCHAIN_BREW_EXECUTABLE)
        execute_process(
            COMMAND "${LANGCHAIN_BREW_EXECUTABLE}" --prefix libomp
            OUTPUT_VARIABLE LANGCHAIN_LIBOMP_PREFIX
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
    endif()
    if(LANGCHAIN_LIBOMP_PREFIX AND EXISTS "${LANGCHAIN_LIBOMP_PREFIX}/lib/libomp.dylib")
        set(OpenMP_CXX_FLAGS "-Xpreprocessor -fopenmp -I${LANGCHAIN_LIBOMP_PREFIX}/include" CACHE STRING "")
        set(OpenMP_CXX_LIB_NAMES "omp" CACHE STRING "")
        set(OpenMP_omp_LIBRARY "${LANGCHAIN_LIBOMP_PREFIX}/lib/libomp.dylib" CACHE STRING "")
    endif()
endif()
find_package(OpenMP QUIET)

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
# faiss/poppler-cpp are PRIVATE, unlike nlohmann_json/cpr above: neither
# type leaks into a public header (FaissVectorStore/PdfLoader forward-
# declare/hide them). NOTE this only keeps their include directories off
# consumers' compile lines -- for a STATIC library, CMake always
# propagates *link* libraries to consumers regardless of PUBLIC/PRIVATE
# (it has to: the static archive's own object files still reference
# faiss::/poppler:: symbols, and nothing strips faiss_vector_store.cpp.o/
# pdf_loader.cpp.o from the archive just because a given consumer never
# calls them). So every consumer -- every example, bench_cpp -- still
# links libfaiss/libpoppler-cpp and pays their dynamic-linker load cost at
# startup even if it never touches FaissVectorStore/PdfLoader; confirmed
# via `otool -L` and measured in benchmarks/RESULTS.md. Actually avoiding
# that would mean splitting FaissVectorStore/PdfLoader into their own
# library target that examples/tests opt into -- a real restructuring,
# not a target_link_libraries keyword change; left as a follow-up.
target_link_libraries(langchain_core
    PUBLIC nlohmann_json::nlohmann_json cpr::cpr
    PRIVATE ${LANGCHAIN_FAISS_TARGET} PkgConfig::POPPLER_CPP
)

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
