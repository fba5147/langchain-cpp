Output docs/demo.gif

Set Shell "bash"
Set FontSize 18
Set Width 1200
Set Height 620
Set Theme "Dracula"
Set Padding 20
Set TypingSpeed 45ms

# Requires `ollama serve` running locally with llama3.2 + nomic-embed-text
# pulled -- this is OpenAIChat/OpenAIEmbeddings talking to a real local
# server via Ollama's OpenAI-compatible API, not a mocked/scripted run.
Type "./build/examples/ollama_demo"
Enter
Sleep 7000ms
