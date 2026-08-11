## What does this change?

## Why?

## Testing

- [ ] Added/updated unit tests
- [ ] `ctest --test-dir build` passes locally
- [ ] If this touches provider wire-format code (`OpenAIChat`, `AnthropicChat`, `OpenAIEmbeddings`):
      sanity-checked against a real endpoint (a local Ollama server is enough — see
      `examples/ollama_demo.cpp`)

## Checklist

- [ ] Scoped to one change (refactors and behavior changes are in separate PRs)
- [ ] Follows the conventions in [CONTRIBUTING.md](../CONTRIBUTING.md)
- [ ] Updated `README.md` / `CHANGELOG.md` if this changes public API or completes a roadmap item
