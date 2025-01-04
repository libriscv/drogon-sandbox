#pragma once
#include "script.hpp"
#include "tenant.hpp"
struct MachineInstance;

struct TenantInstance
{
	using SharedMachine = std::shared_ptr<MachineInstance>;

	struct ForkCall {
		Script script;
		size_t cnt;

		ForkCall(const Script& script, TenantInstance* tenant, MachineInstance& inst);
	};
	ForkCall forkcall(Script::gaddr_t addr, size_t cnt, riscv::vBuffer[]);
	Script* vmfork();
	bool no_program_loaded() const noexcept { return this->machine == nullptr; }

	Script::gaddr_t lookup(const char* name) const;

	TenantInstance(const TenantConfig&);
	~TenantInstance();

	const TenantConfig config;

private:
	inline SharedMachine get_current_instance() const;

	/* Hot-swappable machine */
	SharedMachine machine = nullptr;
};
