---
name: Bug report
about: Something in langchain-cpp doesn't behave as documented
title: ""
labels: bug
assignees: ""
---

**Describe the bug**
A clear description of what's wrong and what you expected instead.

**To reproduce**
Minimal code that reproduces it — a full `.cpp` file or a small diff against one of the `examples/`
is ideal.

**Environment**

- OS:
- Compiler + version:
- CMake version:
- Dependency source: vcpkg / Homebrew / other (`nlohmann-json`, `cpr`, `gtest` versions if known)

**If this involves a provider (`OpenAIChat`, `AnthropicChat`, `OpenAIEmbeddings`)**

- Can you reproduce it against a local Ollama server (`base_url = "http://localhost:11434/v1"`)? This
  helps rule out account/API-key-specific issues.
- The raw request/response JSON, if you can capture it (redact any API key).

**Additional context**
Anything else relevant — stack trace, logs, etc.
