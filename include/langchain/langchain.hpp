#pragma once

// Convenience umbrella header pulling in the current milestone's public API:
// core types, the Runnable/`|` composition mechanism, prompt templates,
// output parsers, tools/agents, an MCP client, the RAG stack, and the chat
// providers (Mock, OpenAI, Azure OpenAI, Anthropic, Gemini, plus
// OpenAI-compatible presets for Groq/Mistral/DeepSeek).

#include "langchain/core/callbacks.hpp"
#include "langchain/core/document.hpp"
#include "langchain/core/dotenv.hpp"
#include "langchain/core/message.hpp"
#include "langchain/core/result.hpp"
#include "langchain/core/runnable.hpp"
#include "langchain/core/similarity.hpp"

#include "langchain/agents/agent_executor.hpp"

#include "langchain/callbacks/callbacking_chat_model.hpp"
#include "langchain/callbacks/callbacking_tool.hpp"
#include "langchain/callbacks/console_callback_handler.hpp"

#include "langchain/llm/caching_chat_model.hpp"
#include "langchain/llm/chat_message_history.hpp"
#include "langchain/llm/chat_model.hpp"
#include "langchain/llm/chat_model_cache.hpp"
#include "langchain/llm/chat_model_with_history.hpp"
#include "langchain/llm/file_chat_message_history.hpp"
#include "langchain/llm/in_memory_chat_message_history.hpp"
#include "langchain/llm/in_memory_chat_model_cache.hpp"
#include "langchain/llm/rate_limited_chat_model.hpp"
#include "langchain/llm/rate_limiter.hpp"

#include "langchain/mcp/mcp_client.hpp"
#include "langchain/mcp/mcp_server.hpp"
#include "langchain/mcp/mcp_http_server.hpp"
#include "langchain/mcp/mcp_tools.hpp"

#include "langchain/parsers/json_parser.hpp"
#include "langchain/parsers/output_fixing_parser.hpp"
#include "langchain/parsers/string_parser.hpp"
#include "langchain/parsers/structured_parser.hpp"

#include "langchain/prompts/chat_prompt_template.hpp"
#include "langchain/prompts/example_selector.hpp"
#include "langchain/prompts/few_shot_prompt_template.hpp"
#include "langchain/prompts/prompt_template.hpp"
#include "langchain/prompts/semantic_similarity_example_selector.hpp"

#include "langchain/providers/anthropic/anthropic_chat.hpp"
#include "langchain/providers/azure/azure_openai_chat.hpp"
#include "langchain/providers/google/gemini_chat.hpp"
#include "langchain/providers/mock/mock_chat.hpp"
#include "langchain/providers/openai/openai_chat.hpp"
#include "langchain/providers/openai/openai_compatible_presets.hpp"

#include "langchain/tools/function_tool.hpp"
#include "langchain/tools/tool.hpp"
#include "langchain/tools/tool_registry.hpp"

#include "langchain/rag/embeddings/embeddings.hpp"
#include "langchain/rag/embeddings/mock_embeddings.hpp"
#include "langchain/rag/embeddings/openai_embeddings.hpp"
#include "langchain/rag/embeddings/azure_openai_embeddings.hpp"
#include "langchain/rag/embeddings/gemini_embeddings.hpp"
#include "langchain/rag/format_documents.hpp"
#include "langchain/rag/loaders/csv_loader.hpp"
#include "langchain/rag/loaders/loader.hpp"
#include "langchain/rag/loaders/markdown_loader.hpp"
#include "langchain/rag/loaders/pdf_loader.hpp"
#include "langchain/rag/loaders/text_loader.hpp"
#include "langchain/rag/loaders/web_loader.hpp"
#include "langchain/rag/retrievers/retriever.hpp"
#include "langchain/rag/splitters/recursive_character_text_splitter.hpp"
#include "langchain/rag/vectorstores/faiss_vector_store.hpp"
#include "langchain/rag/vectorstores/in_memory_vector_store.hpp"
#include "langchain/rag/vectorstores/pgvector_store.hpp"
#include "langchain/rag/vectorstores/qdrant_vector_store.hpp"
#include "langchain/rag/vectorstores/vector_store.hpp"
