cmake_minimum_required(VERSION 3.27)
project(course_pc)

set(CMAKE_CXX_STANDARD 23)

add_executable(course_pc
        indexer.cpp
        thread_pool.cpp
        file_manager.cpp
        main1.cpp
)

add_executable(course_pc_user1
        user_search.cpp
)

find_package(Threads REQUIRED)
target_link_libraries(course_pc PRIVATE Threads::Threads ws2_32)
target_link_libraries(course_pc_user1 PRIVATE ws2_32)

set(PYTHON_SCRIPT ${CMAKE_SOURCE_DIR}/user_s_a_s.py)

add_custom_target(course_py ALL
        COMMAND ${CMAKE_COMMAND} -E copy ${PYTHON_SCRIPT} ${CMAKE_BINARY_DIR}
        COMMENT "Copying Python script to build directory"
)