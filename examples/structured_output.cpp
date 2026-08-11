// Demonstrates parsing a chat model's reply straight into a C++ struct via
// StructuredOutputParser<T>, using MockChat so it runs with no API key.

#include "langchain/langchain.hpp"

#include <iostream>

using namespace langchain;

struct Person {
    std::string name;
    int age;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Person, name, age)
};

int main() {
    auto prompt = std::make_shared<prompts::ChatPromptTemplate>(
        "Return a JSON object with \"name\" and \"age\" fields for {name}, who is {age} years old.");

    auto model = std::make_shared<providers::MockChat>(
        "Sure, here you go:\n```json\n{\"name\": \"Ada Lovelace\", \"age\": 36}\n```");

    auto parser = std::make_shared<parsers::StructuredOutputParser<Person>>();

    auto chain = prompt | model | parser;

    Person person = chain->invoke({{"name", "Ada Lovelace"}, {"age", "36"}});
    std::cout << person.name << " is " << person.age << " years old.\n";

    return 0;
}
