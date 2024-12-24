cmake_minimum_required (VERSION 3.1)
project (sandbox CXX)

option(RISCV_32I "" OFF)
option(RISCV_FLAT_RW_ARENA "" OFF)
option(RISCV_BINARY_TRANSLATION "" ON)
option(RISCV_MEMORY_TRAPS "" OFF)

add_subdirectory(libriscv/lib libriscv)
target_compile_definitions(riscv PUBLIC
	RISCV_SYSCALLS_MAX=100)

set(RISCV_SOURCES
	machine_instance.cpp
	tenant_instance.cpp
	script.cpp
	script_functions.cpp
)

add_library(sandbox STATIC ${RISCV_SOURCES})
set_target_properties(sandbox PROPERTIES CXX_STANDARD 17)
target_include_directories(sandbox PUBLIC .)
target_link_libraries(sandbox PUBLIC riscv)


if (NATIVE)
	target_compile_options(riscv PUBLIC -march=native -Ofast -fno-fast-math)
endif()
