// PdfLoader tests use hand-crafted, minimal-but-valid PDF byte fixtures
// (correct xref offsets and all) rather than shipping real binary PDF
// files or depending on an external PDF-generation tool -- verified to
// open correctly with `pdftotext` while building this. This mirrors the
// project's existing preference for small, deterministic, real fixtures
// over external tooling (see ImageContent's tests writing raw bytes).

#include "langchain/rag/loaders/pdf_loader.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

using namespace langchain::rag;

namespace {

// clang-format off
constexpr const char* kOnePagePdf =
R"PDF(%PDF-1.1
1 0 obj << /Type /Catalog /Pages 2 0 R >> endobj
2 0 obj << /Type /Pages /Kids [3 0 R] /Count 1 >> endobj
3 0 obj << /Type /Page /Parent 2 0 R /Resources << /Font << /F1 4 0 R >> >> /MediaBox [0 0 300 144] /Contents 5 0 R >> endobj
4 0 obj << /Type /Font /Subtype /Type1 /BaseFont /Helvetica >> endobj
5 0 obj << /Length 43 >>
stream
BT /F1 24 Tf 100 100 Td (Hello World) Tj ET
endstream
endobj
xref
0 6
0000000000 65535 f
0000000009 00000 n
0000000058 00000 n
0000000115 00000 n
0000000241 00000 n
0000000311 00000 n
trailer << /Root 1 0 R /Size 6 >>
startxref
404
%%EOF)PDF";

constexpr const char* kTwoPagePdf =
R"PDF(%PDF-1.1
1 0 obj << /Type /Catalog /Pages 2 0 R >>
endobj
2 0 obj << /Type /Pages /Kids [3 0 R 4 0 R] /Count 2 >>
endobj
3 0 obj << /Type /Page /Parent 2 0 R /Resources << /Font << /F1 5 0 R >> >> /MediaBox [0 0 300 144] /Contents 6 0 R >>
endobj
4 0 obj << /Type /Page /Parent 2 0 R /Resources << /Font << /F1 5 0 R >> >> /MediaBox [0 0 300 144] /Contents 7 0 R >>
endobj
5 0 obj << /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>
endobj
6 0 obj << /Length 45 >>
stream
BT /F1 24 Tf 100 100 Td (Page one text) Tj ET
endstream
endobj
7 0 obj << /Length 45 >>
stream
BT /F1 24 Tf 100 100 Td (Page two text) Tj ET
endstream
endobj
xref
0 8
0000000000 65535 f
0000000009 00000 n
0000000058 00000 n
0000000121 00000 n
0000000247 00000 n
0000000373 00000 n
0000000443 00000 n
0000000538 00000 n
trailer << /Root 1 0 R /Size 8 >>
startxref
633
%%EOF)PDF";
// clang-format on

std::filesystem::path write_fixture(const std::string& name, const char* content) {
    auto path = std::filesystem::temp_directory_path() / name;
    std::ofstream file(path, std::ios::binary);
    file << content;
    return path;
}

} // namespace

TEST(PdfLoader, ExtractsTextFromASinglePagePdf) {
    auto path = write_fixture("langchain_cpp_pdf_loader_one_page.pdf", kOnePagePdf);

    PdfLoader loader(path.string());
    auto documents = loader.load();

    ASSERT_EQ(documents.size(), 1u);
    EXPECT_NE(documents[0].content.find("Hello World"), std::string::npos);
    EXPECT_EQ(documents[0].metadata["source"], path.string());
    EXPECT_EQ(documents[0].metadata["page"], 0);

    std::filesystem::remove(path);
}

TEST(PdfLoader, YieldsOneDocumentPerPage) {
    auto path = write_fixture("langchain_cpp_pdf_loader_two_page.pdf", kTwoPagePdf);

    PdfLoader loader(path.string());
    auto documents = loader.load();

    ASSERT_EQ(documents.size(), 2u);
    EXPECT_NE(documents[0].content.find("Page one text"), std::string::npos);
    EXPECT_EQ(documents[0].metadata["page"], 0);
    EXPECT_NE(documents[1].content.find("Page two text"), std::string::npos);
    EXPECT_EQ(documents[1].metadata["page"], 1);

    std::filesystem::remove(path);
}

TEST(PdfLoader, ThrowsWhenFileDoesNotExist) {
    PdfLoader loader("/no/such/file.pdf");
    EXPECT_THROW(loader.load(), std::runtime_error);
}

TEST(PdfLoader, ThrowsWhenFileIsNotAValidPdf) {
    auto path = std::filesystem::temp_directory_path() / "langchain_cpp_pdf_loader_not_a_pdf.pdf";
    {
        std::ofstream file(path, std::ios::binary);
        file << "this is not a PDF";
    }

    PdfLoader loader(path.string());
    EXPECT_THROW(loader.load(), std::runtime_error);

    std::filesystem::remove(path);
}
