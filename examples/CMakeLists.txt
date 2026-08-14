add_executable(mock_chain mock_chain.cpp)
target_link_libraries(mock_chain PRIVATE langchain::core)

add_executable(basic_chat basic_chat.cpp)
target_link_libraries(basic_chat PRIVATE langchain::core)

add_executable(structured_output structured_output.cpp)
target_link_libraries(structured_output PRIVATE langchain::core)

add_executable(tools_demo tools_demo.cpp)
target_link_libraries(tools_demo PRIVATE langchain::core)

add_executable(agent_demo agent_demo.cpp)
target_link_libraries(agent_demo PRIVATE langchain::core)

add_executable(rag_demo rag_demo.cpp)
target_link_libraries(rag_demo PRIVATE langchain::core)
target_compile_definitions(rag_demo PRIVATE EXAMPLES_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")

add_executable(ollama_demo ollama_demo.cpp)
target_link_libraries(ollama_demo PRIVATE langchain::core)

add_executable(more_providers_demo more_providers_demo.cpp)
target_link_libraries(more_providers_demo PRIVATE langchain::core)

add_executable(callbacks_demo callbacks_demo.cpp)
target_link_libraries(callbacks_demo PRIVATE langchain::core)

add_executable(caching_demo caching_demo.cpp)
target_link_libraries(caching_demo PRIVATE langchain::core)

add_executable(chat_history_demo chat_history_demo.cpp)
target_link_libraries(chat_history_demo PRIVATE langchain::core)

add_executable(few_shot_demo few_shot_demo.cpp)
target_link_libraries(few_shot_demo PRIVATE langchain::core)

add_executable(rate_limit_demo rate_limit_demo.cpp)
target_link_libraries(rate_limit_demo PRIVATE langchain::core)

add_executable(multimodal_demo multimodal_demo.cpp)
target_link_libraries(multimodal_demo PRIVATE langchain::core)

add_executable(mcp_client_demo mcp_client_demo.cpp)
target_link_libraries(mcp_client_demo PRIVATE langchain::core)

add_executable(mcp_server_demo mcp_server_demo.cpp)
target_link_libraries(mcp_server_demo PRIVATE langchain::core)

add_executable(qdrant_demo qdrant_demo.cpp)
target_link_libraries(qdrant_demo PRIVATE langchain::core)
target_compile_definitions(qdrant_demo PRIVATE EXAMPLES_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")
