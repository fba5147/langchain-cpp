#include "langchain/parsers/json_parser.hpp"
#include "langchain/parsers/structured_parser.hpp"

#include <gtest/gtest.h>

using namespace langchain::core;
using namespace langchain::parsers;

TEST(ExtractJson, ParsesBareJsonObject) {
    auto parsed = extract_json(R"({"a": 1, "b": "two"})");
    EXPECT_EQ(parsed["a"], 1);
    EXPECT_EQ(parsed["b"], "two");
}

TEST(ExtractJson, StripsMarkdownCodeFence) {
    auto parsed = extract_json("Sure, here you go:\n```json\n{\"a\": 1}\n```\nHope that helps!");
    EXPECT_EQ(parsed["a"], 1);
}

TEST(ExtractJson, ThrowsWhenNoJsonPresent) {
    EXPECT_THROW(extract_json("no json here"), std::runtime_error);
}

TEST(JsonOutputParser, ParsesMessageContent) {
    JsonOutputParser parser;
    auto parsed = parser.invoke(Message::assistant(R"({"ok": true})"));
    EXPECT_TRUE(parsed["ok"].get<bool>());
}

namespace {
struct Person {
    std::string name;
    int age;
};
void to_json(nlohmann::json& j, const Person& p) { j = {{"name", p.name}, {"age", p.age}}; }
void from_json(const nlohmann::json& j, Person& p) {
    j.at("name").get_to(p.name);
    j.at("age").get_to(p.age);
}
} // namespace

TEST(StructuredOutputParser, ParsesMessageIntoStruct) {
    StructuredOutputParser<Person> parser;
    Person person = parser.invoke(Message::assistant(R"({"name": "Ada", "age": 36})"));
    EXPECT_EQ(person.name, "Ada");
    EXPECT_EQ(person.age, 36);
}
