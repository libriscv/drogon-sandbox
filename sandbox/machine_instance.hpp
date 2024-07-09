#pragma once
#include "script.hpp"
#include <atomic>

struct MachineInstance
{
	using SharedBinary = std::shared_ptr<std::vector<uint8_t>>;

	MachineInstance(SharedBinary elf, TenantInstance* vrm);
	~MachineInstance();

	inline Script::gaddr_t lookup(const std::string& name) const {
		const auto& it = sym_lookup.find(name);
		if (it != sym_lookup.end()) return it->second;
		// fallback
		return script.resolve_address(name);
	}

	Script script;
	const SharedBinary binary;
	/* Lookup tree for ELF symbol names */
	std::unordered_map<std::string, Script::gaddr_t> sym_lookup;
	/* Index vector for ELF symbol names, used by call_index(..) */
	struct Lookup {
		std::string_view func;
		Script::gaddr_t addr;
		size_t size;
	};
	std::vector<Lookup> sym_vector;
};
