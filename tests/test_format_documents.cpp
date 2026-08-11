#include "langchain/rag/format_documents.hpp"

#include <gtest/gtest.h>

using namespace langchain::core;
using namespace langchain::rag;

TEST(FormatDocumentsAsString, JoinsContentsWithBlankLine) {
    FormatDocumentsAsString formatter;
    auto result = formatter.invoke({Document{"first"}, Document{"second"}});
    EXPECT_EQ(result, "first\n\nsecond");
}

TEST(FormatDocumentsAsString, EmptyInputProducesEmptyString) {
    FormatDocumentsAsString formatter;
    EXPECT_EQ(formatter.invoke({}), "");
}
