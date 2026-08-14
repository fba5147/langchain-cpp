Output docs/demo.gif

Set Shell "bash"
Set FontSize 18
Set Width 1200
Set Height 620
Set Theme "Dracula"
Set Padding 20
Set TypingSpeed 45ms

# Requires `npx` (Node.js) on PATH and `ollama serve` running locally with
# llama3.2 pulled. Connects to the official MCP reference server (a real
# subprocess over stdio, not a mock), lists and calls its tools directly,
# then wires them into a real AgentExecutor backed by local Ollama --
# the model picks the MCP-backed tool itself and gets the right answer
# back through the full round trip. See examples/mcp_client_demo.cpp.
Type "./build/examples/mcp_client_demo"
Enter
Sleep 9000ms
