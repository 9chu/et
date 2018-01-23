cmake_minimum_required(VERSION 3.1)
project(et)

set(CMAKE_CXX_STANDARD 11)

# Lua依赖
add_subdirectory(3rd/lua-5.3.4)

# 目标
file(GLOB_RECURSE ET_SRC include/*.hpp src/*.cpp)

add_library(et SHARED ${ET_SRC})
target_include_directories(et PUBLIC include)
set_target_properties(et PROPERTIES PREFIX "")
set_target_properties(et PROPERTIES OUTPUT_NAME "et")

if (WIN32)
    target_link_libraries(et lua-shared)
else ()
    get_target_property(LUA_INCLUDE_DIRS lua-shared INTERFACE_INCLUDE_DIRECTORIES)
    target_include_directories(et PRIVATE ${LUA_INCLUDE_DIRS})

    if (APPLE)
        set_target_properties(et PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
    endif ()
endif ()

add_library(et-static STATIC ${ET_SRC})
target_include_directories(et-static PUBLIC include)
set_target_properties(et-static PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(et-static PROPERTIES OUTPUT_NAME et)
if (WIN32)
    target_link_libraries(et-static lua-shared)
else ()
    target_link_libraries(et-static lua-static)
endif ()

add_executable(et-exec bin/Main.cpp)
set_target_properties(et-exec PROPERTIES OUTPUT_NAME et)
if (WIN32)
    target_link_libraries(et-exec et-static lua-shared)
else ()
    target_link_libraries(et-exec et-static lua-static)
endif ()

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
