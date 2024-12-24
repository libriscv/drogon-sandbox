#pragma once
#include "script.hpp"
#include "machine_instance.hpp"
#include "tenant.hpp"

struct TenantInstance
{
	struct ForkCall {
		Script script;
		size_t cnt;
	};
	ForkCall forkcall(Script::gaddr_t addr, size_t cnt, riscv::vBuffer[]);
	Script* vmfork();
	bool no_program_loaded() const noexcept { return this->machine == nullptr; }

	inline Script::gaddr_t lookup(const char* name) const {
		auto program = machine;
		if (LIKELY(program != nullptr))
			return program->lookup(name);
		return 0x0;
	}

	inline auto callsite(const char* name) {
		auto program = machine;
		if (LIKELY(program != nullptr)) {
			auto addr = program->lookup(name);
			return program->script.callsite(addr);
		}
		return decltype(program->script.callsite(0)) {};
	}

	TenantInstance(const TenantConfig&);

	const TenantConfig config;
	/* Hot-swappable machine */
	std::shared_ptr<MachineInstance> machine = nullptr;
};
