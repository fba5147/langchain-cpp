find_package(GTest CONFIG REQUIRED)

add_executable(langchain_tests
    test_message.cpp
    test_prompt_template.cpp
    test_runnable.cpp
    test_mock_chat.cpp
    test_json_parser.cpp
    test_tools.cpp
)
target_link_libraries(langchain_tests PRIVATE langchain::core GTest::gtest GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(langchain_tests)
