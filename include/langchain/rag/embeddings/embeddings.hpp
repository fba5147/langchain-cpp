#pragma once

#include <string>
#include <vector>

namespace langchain::rag {

// Turns text into a vector for similarity search. embed_query and
// embed_documents are kept separate because some providers embed a
// search query differently from a document meant to be indexed (e.g.
// different instruction prefixes); the default embed_documents just
// calls embed_query per text for providers that don't distinguish.
class Embeddings {
public:
    virtual ~Embeddings() = default;

    virtual std::vector<float> embed_query(const std::string& text) = 0;

    virtual std::vector<std::vector<float>> embed_documents(const std::vector<std::string>& texts) {
        std::vector<std::vector<float>> result;
        result.reserve(texts.size());
        for (const auto& text : texts) {
            result.push_back(embed_query(text));
        }
        return result;
    }
};

} // namespace langchain::rag
