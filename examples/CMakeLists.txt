add_executable(mock_chain mock_chain.cpp)
target_link_libraries(mock_chain PRIVATE langchain::core)

add_executable(basic_chat basic_chat.cpp)
target_link_libraries(basic_chat PRIVATE langchain::core)

add_executable(structured_output structured_output.cpp)
target_link_libraries(structured_output PRIVATE langchain::core)

add_executable(tools_demo tools_demo.cpp)
target_link_libraries(tools_demo PRIVATE langchain::core)
