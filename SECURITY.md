# Security Policy

## Reporting a Vulnerability

This is a hobby/community project, not an officially supported product — there's no SLA on
response time, but security reports are taken seriously.

If you find a security issue (e.g. a way a malicious prompt/tool response could lead to unsafe
behavior in `AgentExecutor`, an injection vector in a provider's request construction, or a memory
safety bug), please report it privately rather than opening a public issue:

- Open a [GitHub security advisory](../../security/advisories/new) for this repository, or
- If that's unavailable, open a regular issue asking a maintainer to contact you privately, without
  including exploit details in the issue itself.

Please include:

- The affected version/commit
- Steps to reproduce, or a minimal example
- The potential impact as you understand it

## Scope notes specific to this project

- This library makes outbound HTTP requests (via `cpr`/libcurl) to whatever `base_url` a provider is
  configured with, and to the URL passed to `TextLoader`/`MarkdownLoader` only in the sense of local
  file paths (no remote fetch). Constructing a provider with an untrusted `base_url`, or handing an
  `AgentExecutor` a `Tool` that executes untrusted input without validation, are both effectively
  arbitrary-code/request execution vectors by design — that responsibility sits with the application
  embedding this library, not the library itself. If you find a way the library *itself* mishandles
  trusted input into one of these unsafe, that's the kind of thing to report here.
