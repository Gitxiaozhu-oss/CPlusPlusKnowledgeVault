add_executable(container_benchmarks
    container_benchmarks.cpp
)

target_link_libraries(container_benchmarks
    PRIVATE
    hcstl
    benchmark::benchmark
    benchmark::benchmark_main
) 