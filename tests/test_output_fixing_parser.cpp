#include "langchain/parsers/output_fixing_parser.hpp"
#include "langchain/parsers/structured_parser.hpp"
#include "langchain/providers/mock/mock_chat.hpp"

#include <gtest/gtest.h>

#include <stdexcept>

using namespace langchain::core;
using namespace langchain::llm;
using namespace langchain::parsers;
using namespace langchain::providers;

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

TEST(OutputFixingParser, FirstAttemptSucceedsWithoutCallingModel) {
    auto inner = std::make_shared<StructuredOutputParser<Person>>();
    auto model = std::make_shared<MockChat>([](const std::vector<Message>&) -> std::string {
        ADD_FAILURE() << "model should not have been called when the first parse succeeds";
        return "{}";
    });

    OutputFixingParser<Person> parser(inner, model);
    Person person = parser.invoke(Message::assistant(R"({"name": "Ada", "age": 36})"));

    EXPECT_EQ(person.name, "Ada");
    EXPECT_EQ(person.age, 36);
}

TEST(OutputFixingParser, RetriesOnceThenSucceeds) {
    auto inner = std::make_shared<StructuredOutputParser<Person>>();
    auto model = std::make_shared<MockChat>(std::vector<Message>{
        Message::assistant(R"({"name": "Ada", "age": 36})"),
    });

    OutputFixingParser<Person> parser(inner, model, /*max_retries=*/1);
    Person person = parser.invoke(Message::assistant("this is not valid JSON at all"));

    EXPECT_EQ(person.name, "Ada");
    EXPECT_EQ(person.age, 36);
}

TEST(OutputFixingParser, GivesUpAndRethrowsAfterMaxRetries) {
    auto inner = std::make_shared<StructuredOutputParser<Person>>();
    auto model = std::make_shared<MockChat>("still not valid JSON");

    OutputFixingParser<Person> parser(inner, model, /*max_retries=*/2);

    EXPECT_THROW(parser.invoke(Message::assistant("not valid JSON")), std::exception);
}
