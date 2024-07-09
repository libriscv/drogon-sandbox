cmake_minimum_required (VERSION 3.11.0)
project (drogon-riscv CXX)
#set (CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++20 -O2 -g -flto=thin -fdiagnostics-color")
set(CMAKE_CXX_LINK_FLAGS "-fuse-ld=lld-18")

option(NATIVE       "Enable host CPU features" ON)
option(USE_JEMALLOC "Use jemalloc as system allocator" ON)

if (NATIVE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
endif()

add_subdirectory(sandbox)
add_subdirectory(drogon)

set(SOURCES
	src/main.cpp
)

add_executable(dvm ${SOURCES})
target_link_libraries(dvm PRIVATE drogon sandbox)

if (USE_JEMALLOC)
	target_link_libraries(dvm PRIVATE jemalloc)
endif()
