#pragma once

// Convenience umbrella header pulling in the current milestone's public API:
// core types, the Runnable/`|` composition mechanism, prompt templates, the
// string output parser, and the mock/OpenAI/Anthropic chat providers.

#include "langchain/core/document.hpp"
#include "langchain/core/message.hpp"
#include "langchain/core/result.hpp"
#include "langchain/core/runnable.hpp"

#include "langchain/agents/agent_executor.hpp"

#include "langchain/llm/chat_model.hpp"

#include "langchain/parsers/json_parser.hpp"
#include "langchain/parsers/string_parser.hpp"
#include "langchain/parsers/structured_parser.hpp"

#include "langchain/prompts/chat_prompt_template.hpp"
#include "langchain/prompts/prompt_template.hpp"

#include "langchain/providers/anthropic/anthropic_chat.hpp"
#include "langchain/providers/mock/mock_chat.hpp"
#include "langchain/providers/openai/openai_chat.hpp"

#include "langchain/tools/function_tool.hpp"
#include "langchain/tools/tool.hpp"
#include "langchain/tools/tool_registry.hpp"
