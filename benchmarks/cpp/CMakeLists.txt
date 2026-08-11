add_executable(bench_cpp bench.cpp)
target_link_libraries(bench_cpp PRIVATE langchain::core)
