#include "langchain/core/dotenv.hpp"

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>

using namespace langchain::core;

namespace {

std::filesystem::path write_temp_env(const std::string& name, const std::string& content) {
    auto path = std::filesystem::temp_directory_path() / name;
    std::ofstream file(path);
    file << content;
    return path;
}

} // namespace

TEST(LoadDotenv, SetsUnsetVariables) {
    unsetenv("LANGCHAIN_CPP_TEST_DOTENV_A");
    auto path = write_temp_env("langchain_cpp_dotenv_test_a.env", "LANGCHAIN_CPP_TEST_DOTENV_A=hello world\n");

    load_dotenv(path.string());

    ASSERT_NE(std::getenv("LANGCHAIN_CPP_TEST_DOTENV_A"), nullptr);
    EXPECT_STREQ(std::getenv("LANGCHAIN_CPP_TEST_DOTENV_A"), "hello world");

    std::filesystem::remove(path);
    unsetenv("LANGCHAIN_CPP_TEST_DOTENV_A");
}

TEST(LoadDotenv, DoesNotOverwriteAlreadySetVariables) {
    setenv("LANGCHAIN_CPP_TEST_DOTENV_B", "real value", /*overwrite=*/1);
    auto path = write_temp_env("langchain_cpp_dotenv_test_b.env", "LANGCHAIN_CPP_TEST_DOTENV_B=from file\n");

    load_dotenv(path.string());

    EXPECT_STREQ(std::getenv("LANGCHAIN_CPP_TEST_DOTENV_B"), "real value");

    std::filesystem::remove(path);
    unsetenv("LANGCHAIN_CPP_TEST_DOTENV_B");
}

TEST(LoadDotenv, SkipsCommentsAndBlankLinesAndStripsQuotes) {
    unsetenv("LANGCHAIN_CPP_TEST_DOTENV_C");
    unsetenv("LANGCHAIN_CPP_TEST_DOTENV_D");
    auto path = write_temp_env("langchain_cpp_dotenv_test_c.env",
                                "# a comment\n\nLANGCHAIN_CPP_TEST_DOTENV_C=\"quoted value\"\n"
                                "LANGCHAIN_CPP_TEST_DOTENV_D='single quoted'\n");

    load_dotenv(path.string());

    EXPECT_STREQ(std::getenv("LANGCHAIN_CPP_TEST_DOTENV_C"), "quoted value");
    EXPECT_STREQ(std::getenv("LANGCHAIN_CPP_TEST_DOTENV_D"), "single quoted");

    std::filesystem::remove(path);
    unsetenv("LANGCHAIN_CPP_TEST_DOTENV_C");
    unsetenv("LANGCHAIN_CPP_TEST_DOTENV_D");
}

TEST(LoadDotenv, MissingFileIsNotAnError) {
    EXPECT_NO_THROW(load_dotenv("/no/such/.env"));
}
