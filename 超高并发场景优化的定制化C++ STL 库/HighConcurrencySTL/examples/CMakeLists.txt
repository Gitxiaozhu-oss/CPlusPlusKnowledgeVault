add_executable(container_example
    container_example.cpp
)

target_link_libraries(container_example
    PRIVATE
    hcstl
) 