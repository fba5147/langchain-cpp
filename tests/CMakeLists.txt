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
)
target_link_libraries(langchain_tests PRIVATE langchain::core GTest::gtest GTest::gtest_main)

# Lets tests reach internal (non-public-API) provider wire-format headers
# under src/, e.g. "providers/google/gemini_wire_format.hpp" -- these
# aren't installed/exposed under include/, but unit-testing the pure
# conversion logic is valuable where there's no live endpoint to
# smoke-test against (see test_gemini_wire_format.cpp).
target_include_directories(langchain_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)

include(GoogleTest)
gtest_discover_tests(langchain_tests)
