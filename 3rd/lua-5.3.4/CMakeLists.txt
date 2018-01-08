cmake_minimum_required(VERSION 2.8)
project(lua C)

set(LUA_C_FLAGS "-DLUA_COMPAT_BITLIB" CACHE STRING "Extra C Flags for Lua")

if (APPLE)
    add_definitions(-DLUA_USE_MACOSX)
    set(LUA_LIBRARIES dl)
elseif (UNIX)
    add_definitions(-DLUA_USE_LINUX)
    set(LUA_LIBRARIES dl)
endif ()

# Extra C flags for Lua
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${LUA_C_FLAGS}")

# define the lua source files
file(GLOB LUA_LIB_SRCS src/*.c)
file(GLOB LUA_INTERPRETER src/lua.c)
file(GLOB LUA_COMPILER src/luac.c)
list(REMOVE_ITEM LUA_LIB_SRCS ${LUA_INTERPRETER} ${LUA_COMPILER})

# create the library
add_library(lua SHARED ${LUA_LIB_SRCS})
target_include_directories(lua PUBLIC src)
target_link_libraries(lua ${LUA_LIBRARIES})
