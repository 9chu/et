cmake_minimum_required(VERSION 3.1)
project(et)

set(CMAKE_CXX_STANDARD 11)

# Lua依赖
add_subdirectory(3rd/lua-5.3.4)
get_target_property(LUA_INCLUDE_DIRS lua INTERFACE_INCLUDE_DIRECTORIES)
include_directories(${LUA_INCLUDE_DIRS})

# 目标
file(GLOB_RECURSE ET_SRC include/*.hpp src/*.cpp)

add_library(et SHARED ${ET_SRC})
target_include_directories(et PUBLIC include)

if (APPLE)
    set_target_properties(et PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif ()

add_library(et-static STATIC ${ET_SRC})
target_link_libraries(et-static lua-static)
target_include_directories(et-static PUBLIC include)

# 单元测试
if (ET_ENABLE_TEST)
    enable_testing()

    find_package(GTest REQUIRED)
    find_package(Threads REQUIRED)

    file(GLOB_RECURSE ET_TEST_SRC test/*.cpp)

    include_directories(${GTEST_INCLUDE_DIRS})

    add_executable(ettest ${ET_TEST_SRC})
    target_link_libraries(ettest et-static lua-static ${GTEST_BOTH_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
    add_test(ettest ettest)
endif ()
