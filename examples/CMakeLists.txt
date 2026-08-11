add_executable(mock_chain mock_chain.cpp)
target_link_libraries(mock_chain PRIVATE langchain::core)

add_executable(basic_chat basic_chat.cpp)
target_link_libraries(basic_chat PRIVATE langchain::core)
