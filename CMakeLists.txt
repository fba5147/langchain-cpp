cmake_minimum_required(VERSION 3.21)
project(langchain-cpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(LANGCHAIN_BUILD_EXAMPLES "Build example programs" ON)
option(LANGCHAIN_BUILD_TESTS "Build unit tests" ON)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(cpr CONFIG REQUIRED)

add_library(langchain_core
    src/core/message.cpp
    src/prompts/prompt_template.cpp
    src/prompts/chat_prompt_template.cpp
    src/providers/mock/mock_chat.cpp
    src/providers/openai/openai_chat.cpp
    src/providers/anthropic/anthropic_chat.cpp
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
