#include "langchain/prompts/few_shot_prompt_template.hpp"
#include "langchain/prompts/semantic_similarity_example_selector.hpp"
#include "langchain/rag/embeddings/mock_embeddings.hpp"

#include <gtest/gtest.h>

using namespace langchain::prompts;
using namespace langchain::rag;

TEST(FewShotPromptTemplate, RendersPrefixExamplesAndSuffixJoinedBySeparator) {
    FewShotPromptTemplate prompt(FewShotPromptTemplate::Config{
        .prefix = "Convert to uppercase:",
        .example_template = "{in} -> {out}",
        .suffix = "{in} ->",
        .examples = {
            {{"in", "hi"}, {"out", "HI"}},
            {{"in", "bye"}, {"out", "BYE"}},
        },
        .example_separator = "\n",
    });

    std::string result = prompt.format({{"in", "ok"}});

    EXPECT_EQ(result, "Convert to uppercase:\nhi -> HI\nbye -> BYE\nok ->");
}

TEST(FewShotPromptTemplate, EmptyPrefixLeavesNoLeadingSeparator) {
    FewShotPromptTemplate prompt(FewShotPromptTemplate::Config{
        .example_template = "{in} -> {out}",
        .suffix = "{in} ->",
        .examples = {{{"in", "hi"}, {"out", "HI"}}},
        .example_separator = "\n",
    });

    EXPECT_EQ(prompt.format({{"in", "ok"}}), "hi -> HI\nok ->");
}

TEST(FewShotPromptTemplate, NoExamplesJustRendersPrefixAndSuffix) {
    FewShotPromptTemplate prompt(FewShotPromptTemplate::Config{
        .prefix = "Prefix.",
        .example_template = "unused",
        .suffix = "{in} ->",
        .example_separator = "\n",
    });

    EXPECT_EQ(prompt.format({{"in", "ok"}}), "Prefix.\nok ->");
}

TEST(FewShotPromptTemplate, ExampleSelectorTakesPrecedenceOverFixedExamples) {
    auto embeddings = std::make_shared<MockEmbeddings>(64);
    auto selector = std::make_shared<SemanticSimilarityExampleSelector>(
        embeddings,
        std::vector<PromptValues>{
            {{"in", "cats and dogs are pets"}, {"out", "animals"}},
            {{"in", "quantum computers use qubits"}, {"out", "physics"}},
        },
        "in", /*k=*/1);

    FewShotPromptTemplate prompt(FewShotPromptTemplate::Config{
        .example_template = "{in} => {out}",
        .suffix = "{in} =>",
        .examples = {{{"in", "should not be used"}, {"out", "wrong"}}},
        .example_selector = selector,
        .example_separator = "\n",
    });

    std::string result = prompt.format({{"in", "dogs and cats make good pets"}});

    EXPECT_NE(result.find("animals"), std::string::npos);
    EXPECT_EQ(result.find("wrong"), std::string::npos);
    EXPECT_EQ(result.find("physics"), std::string::npos);
}

TEST(SemanticSimilarityExampleSelector, SelectsMostSimilarExampleFirst) {
    auto embeddings = std::make_shared<MockEmbeddings>(64);
    SemanticSimilarityExampleSelector selector(
        embeddings,
        std::vector<PromptValues>{
            {{"in", "RAII ties resource lifetime to object lifetime"}},
            {{"in", "vector databases store embeddings for search"}},
            {{"in", "quantum computers use superconducting qubits"}},
        },
        "in", /*k=*/1);

    auto selected = selector.select_examples({{"in", "what is RAII in C++?"}});

    ASSERT_EQ(selected.size(), 1u);
    EXPECT_NE(selected[0].at("in").find("RAII"), std::string::npos);
}

TEST(SemanticSimilarityExampleSelector, RespectsK) {
    auto embeddings = std::make_shared<MockEmbeddings>(64);
    SemanticSimilarityExampleSelector selector(
        embeddings,
        std::vector<PromptValues>{
            {{"in", "one"}},
            {{"in", "two"}},
            {{"in", "three"}},
        },
        "in", /*k=*/2);

    EXPECT_EQ(selector.select_examples({{"in", "one"}}).size(), 2u);
}
