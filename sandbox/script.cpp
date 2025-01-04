#include "script.hpp"

#include <libriscv/native_heap.hpp>
#include <stdexcept>
#include "tenant_instance.hpp"
inline timespec time_now();
inline long nanodiff(timespec start_time, timespec end_time);

static constexpr bool VERBOSE_ERRORS       = true;
static constexpr int  NATIVE_SYSCALLS_BASE = 80;

//#define ENABLE_TIMING
#define TIMING_LOCATION(x) \
	asm("" ::: "memory"); \
	auto x = time_now();  \
	asm("" ::: "memory");

Script::Script(
	const Script& source,
	const TenantInstance* tenant, const MachineInstance& inst)
	: m_machine(source.machine(), riscv::MachineOptions<MARCH>{
		.memory_max = 0,
		.verbose_loader = false,
		.minimal_fork = true,
		.use_memory_arena = false,
	  }),
	  m_vrm(tenant), m_inst(inst),
	  m_parent(&source.machine())
{
	/* No initialization */
	this->machine_setup(machine(), false);

	/* Transfer data from the old arena, to fully replicate heap */
	machine().transfer_arena_from(source.machine());
}

Script::Script(
	const std::vector<uint8_t>& binary,
	const TenantInstance* tenant, const MachineInstance& inst)
	: m_machine(binary, riscv::MachineOptions<MARCH>{
		.memory_max = tenant->config.max_memory,
		.verbose_loader = true,
		.use_memory_arena = true,
	  }),
	  m_vrm(tenant), m_inst(inst)
{
	this->machine_initialize();
}

thread_local std::vector<riscv::PageData> pagedata_cache;
static riscv::PageData& loan_page(riscv::PageData::Initialization init)
{
	auto& cache = pagedata_cache;
	if (cache.empty()) {
		riscv::PageData* pagedata = new riscv::PageData(init);
		return *pagedata;
	}

	auto& pagedata = cache.back();
	cache.pop_back();
	if (init == riscv::PageData::INITIALIZED)
		std::memset(pagedata.buffer8.data(), 0, pagedata.buffer8.size());

	return pagedata;
}

Script::~Script()
{
	auto& cache = pagedata_cache;
	for (auto page : m_loaned_pages)
		cache.push_back(*page);
}

void Script::machine_initialize()
{
	// setup system calls and traps
	this->machine_setup(machine(), true);
	// run through the initialization
	try {
		machine().simulate<true>(max_instructions());
	} catch (const riscv::MachineTimeoutException& e) {
		handle_timeout(machine().cpu.pc());
		throw;
	} catch (const std::exception& e) {
		handle_exception(machine().cpu.pc());
		throw;
	}
}
void Script::machine_setup(machine_t& machine, bool init)
{
	machine.set_userdata<Script>(this);

	if (init == false)
	{
		machine.memory.set_page_fault_handler(
		[] (riscv::Memory<MARCH>& mem, gaddr_t pageno, bool init) -> riscv::Page& {
			//printf("Creating page %zu @ 0x%lX\n", pageno, long(pageno * 4096u));
			riscv::PageData& pagedata = loan_page(init ?
				riscv::PageData::INITIALIZED : riscv::PageData::UNINITIALIZED);
			auto& script = *mem.machine().template get_userdata<Script>();
			script.m_loaned_pages.push_back(&pagedata);
			// Create new read-write attribute page with loaned data
			auto& page = mem.allocate_page(pageno, riscv::PageAttributes{
				.is_cow = false,    // We are creating a new page, not a COW page
				.non_owning = true, // We don't own the page
			}, &pagedata);
			return page;
		});
		machine.memory.set_page_readf_handler(
		[] (const riscv::Memory<MARCH>& mem, gaddr_t pageno) -> const riscv::Page& {
			//printf("Reading page %zu @ 0x%lX\n", pageno, long(pageno * 4096u));
			Script& script = *mem.machine().template get_userdata<Script>();
			const riscv::Page& foreign_page = script.m_parent->memory.get_pageno(pageno);
			// Install the page as a non-owning, COW page
			riscv::PageAttributes attr = foreign_page.attr;
			attr.non_owning = true;
			attr.is_cow = true;
			return const_cast<riscv::Memory<MARCH>&>(mem).allocate_page(pageno, attr, foreign_page.page());
		});
	}
	else
	{
		// Full Linux-compatible stack
		machine.setup_linux(
			{ name() },
			{ "LC_CTYPE=C", "LC_ALL=C", "USER=groot" });

		// add system call interface
	#ifdef ENABLE_TIMING
		TIMING_LOCATION(t0);
	#endif
		auto heap_base = machine.memory.mmap_allocate(vrm()->config.max_heap);
		machine.setup_native_heap(NATIVE_SYSCALLS_BASE,
			heap_base, vrm()->config.max_heap);

	#ifdef ENABLE_TIMING
		TIMING_LOCATION(t1);
	#endif
		machine.setup_native_memory(NATIVE_SYSCALLS_BASE+5);
	#ifdef ENABLE_TIMING
		TIMING_LOCATION(t2);
	#endif

		Script::setup_syscall_interface();

		// FIXME: EBREAK is used as a stop mechanism
		machine.install_syscall_handler(riscv::SYSCALL_EBREAK,
			[] (auto& m) {
				m.stop();
			});
		machine.on_unhandled_syscall =
			[] (auto&, size_t number) {
				printf("VM unhandled system call: %zu\n", number);
			};
	#ifdef ENABLE_TIMING
		TIMING_LOCATION(t3);
		printf("Time spent setting up arena: %ld ns, nat.mem: %ld ns, syscalls: %ld ns\n",
			nanodiff(t0, t1), nanodiff(t1, t2), nanodiff(t2, t3));
	#endif
	}
}

void Script::handle_exception(gaddr_t address)
{
	try {
		throw;
	}
	catch (const riscv::MachineException& e) {
		if constexpr (VERBOSE_ERRORS) {
		fprintf(stderr, "Script exception: %s (data: 0x%lX)\n", e.what(), long(e.data()));
		fprintf(stderr, ">>> Machine registers:\n[PC\t%08lX] %s\n",
			(long) machine().cpu.pc(),
			machine().cpu.registers().to_string().c_str());
		}
	}
	catch (const std::exception& e) {
		if constexpr (VERBOSE_ERRORS) {
			fprintf(stderr, "Script exception: %s\n", e.what());
		}
	}
	if constexpr (VERBOSE_ERRORS) {
		printf("Program page: %s\n", machine().memory.get_page_info(machine().cpu.pc()).c_str());
		printf("Stack page: %s\n", machine().memory.get_page_info(machine().cpu.reg(2)).c_str());

		auto callsite = machine().memory.lookup(address);
		fprintf(stderr, "Function call: %s\n", callsite.name.c_str());
		this->print_backtrace(address);
	}
}
void Script::handle_timeout(gaddr_t address)
{
	if constexpr (VERBOSE_ERRORS) {
		auto callsite = machine().memory.lookup(address);
		fprintf(stderr, "Script hit max instructions for: %s\n",
			callsite.name.c_str());
	}
}
void Script::print_backtrace(const gaddr_t addr)
{
	machine().memory.print_backtrace(
		[] (const std::string_view line) {
			printf("-> %.*s\n", (int)line.size(), line.begin());
		});
	auto origin = machine().memory.lookup(addr);
	printf("-> [-] 0x%08lx + 0x%.3x: %s\n",
			(long) origin.address,
			origin.offset, origin.name.c_str());
}

uint64_t Script::max_instructions() const noexcept
{
	return vrm()->config.max_instructions;
}
const std::string& Script::name() const noexcept
{
	return vrm()->config.name;
}
const std::string& Script::group() const noexcept
{
	return vrm()->config.group;
}

Script::gaddr_t Script::guest_alloc(size_t len)
{
	return machine().arena().malloc(len);
}

Script::gaddr_t Script::resolve_address(std::string_view name) const {
	return machine().address_of(name);
}
std::string Script::symbol_name(gaddr_t address) const
{
	auto callsite = machine().memory.lookup(address);
	return callsite.name;
}

#ifdef ENABLE_TIMING
timespec time_now()
{
	timespec t;
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t);
	return t;
}
long nanodiff(timespec start_time, timespec end_time)
{
	assert(end_time.tv_sec == 0); /* We should never use seconds */
	return end_time.tv_nsec - start_time.tv_nsec;
}
#endif
