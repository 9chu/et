cmake_minimum_required(VERSION 3.1)
project(et)

set(CMAKE_CXX_STANDARD 11)

if (NOT TARGET lua)
    add_subdirectory(3rd/lua-5.3.4)
endif ()

file(GLOB_RECURSE ET_SRC include/*.hpp src/*.cpp)

add_library(et STATIC ${ET_SRC})
target_link_libraries(et lua)
target_include_directories(et PUBLIC include)

# 单元测试
if (ET_ENABLE_TEST)
    enable_testing()

    find_package(GTest REQUIRED)
    find_package(Threads REQUIRED)

    file(GLOB_RECURSE ET_TEST_SRC test/*.cpp)

    include_directories(${GTEST_INCLUDE_DIRS})

    add_executable(ettest ${ET_TEST_SRC})
    target_link_libraries(ettest et lua ${GTEST_BOTH_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
    add_test(ettest ettest)
endif ()
