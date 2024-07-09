#include "machine_instance.hpp"

static const std::vector<std::string> lookup_wishlist {
	"on_init",
	"on_client_request",
};

MachineInstance::MachineInstance(SharedBinary elf, TenantInstance* vrm)
	: script{*elf, vrm, *this}, binary{std::move(elf)}
{
	for (const auto& func : lookup_wishlist) {
		/* NOTE: We can't check if addr is 0 here, because
			the wishlist applies to ALL machines. */
		const auto addr = lookup(func);
		sym_lookup.try_emplace(func, addr);
		const auto callsite = script.callsite(addr);
		sym_vector.push_back({func, addr, callsite.size});
	}
}

MachineInstance::~MachineInstance()
{
}
