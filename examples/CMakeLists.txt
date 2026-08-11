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
