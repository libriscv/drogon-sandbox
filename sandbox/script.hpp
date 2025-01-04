#pragma once
#include <cassert>
#include <functional>
#include <libriscv/machine.hpp>
#include <optional>
struct TenantInstance;
struct MachineInstance;

class Script {
public:
	static constexpr int MARCH = riscv::RISCV64;
	using gaddr_t = riscv::address_type<MARCH>;
	using sgaddr_t = riscv::signed_address_type<MARCH>;
	using machine_t = riscv::Machine<MARCH>;

	// Call a function in the sandboxed program
	template <typename... Args>
	std::optional<sgaddr_t> call(gaddr_t addr, Args&&...);

	template <typename... Args>
	std::optional<sgaddr_t> preempt(gaddr_t addr, Args&&...);

	std::optional<sgaddr_t> resume(uint64_t cycles);

	auto& machine() { return m_machine; }
	const auto& machine() const { return m_machine; }

	const auto* vrm() const noexcept { return m_vrm; }
	const auto& instance() const noexcept { return m_inst; }
	void assign_instance(std::shared_ptr<MachineInstance>&& ref) { m_inst_ref = std::move(ref); }

	uint64_t max_instructions() const noexcept;
	const std::string& name() const noexcept;
	const std::string& group() const noexcept;
	bool is_paused() const noexcept { return m_is_paused; }

	gaddr_t guest_alloc(size_t len);

	std::string symbol_name(gaddr_t address) const;
	gaddr_t resolve_address(std::string_view name) const;
	riscv::Memory<MARCH>::Callsite callsite(gaddr_t addr) const { return machine().memory.lookup(addr); }

	void print_backtrace(const gaddr_t addr);

	bool reset(); // true if the reset was successful

	Script(const std::vector<uint8_t>&, const TenantInstance*, const MachineInstance&);
	Script(const Script& source, const TenantInstance*, const MachineInstance&);
	~Script();

private:
	void handle_exception(gaddr_t);
	void handle_timeout(gaddr_t);
	bool install_binary(const std::string& file, bool shared = true);
	void machine_initialize();
	void machine_setup(machine_t&, bool init);
	void setup_virtual_memory(bool init);
	static void setup_syscall_interface();

	machine_t m_machine;
	const struct TenantInstance* m_vrm = nullptr;
	const MachineInstance& m_inst;
	const machine_t* m_parent = nullptr;

	bool m_is_paused = false;

	std::vector<riscv::PageData*> m_loaned_pages;

	/* Delete this last */
	std::shared_ptr<MachineInstance> m_inst_ref = nullptr;
};

template <typename... Args>
inline std::optional<Script::sgaddr_t> Script::call(gaddr_t address, Args&&... args)
{
	try {
		// reset the stack pointer to an initial location (deliberately)
		machine().cpu.reset_stack_pointer();
		// setup calling convention
		machine().setup_call(std::forward<Args>(args)...);
		// execute function
		machine().simulate_with<true>(max_instructions(), 0u, address);
		// address-sized integer return value
		return machine().return_value<sgaddr_t>();
	}
	catch (const riscv::MachineTimeoutException& e) {
		this->handle_timeout(address);
	}
	catch (const std::exception& e) {
		this->handle_exception(address);
	}
	return std::nullopt;
}

template <typename... Args>
inline std::optional<Script::sgaddr_t> Script::preempt(gaddr_t address, Args&&... args)
{
	const auto regs = machine().cpu.registers();
	try {
		const sgaddr_t ret = machine().preempt<50'000, true, false>(
			address, std::forward<Args>(args)...);
		machine().cpu.registers() = regs;
		return ret;
	}
	catch (const riscv::MachineTimeoutException& e) {
		this->handle_timeout(address);
	}
	catch (const std::exception& e) {
		this->handle_exception(address);
	}
	machine().cpu.registers() = regs;
	return std::nullopt;
}

inline std::optional<Script::sgaddr_t> Script::resume(uint64_t cycles)
{
	try {
		machine().simulate<false>(cycles);
		return machine().cpu.reg(10);
	}
	catch (const std::exception& e) {
		this->handle_exception(machine().cpu.pc());
	}
	return std::nullopt;
}
