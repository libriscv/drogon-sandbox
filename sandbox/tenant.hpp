#pragma once
#include <cstdint>
#include <string>

struct TenantConfig
{
	std::string  name;
	std::string  group;
	std::string  filename;
	uint64_t     max_instructions;
	uint64_t     max_memory;
	uint64_t     max_heap;
};
