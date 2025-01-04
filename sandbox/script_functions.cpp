#include "script_functions.hpp"
#include <libriscv/native_heap.hpp>
#include "machine/syscalls.h"

//#define ENABLE_TIMING
#define TIMING_LOCATION(x) \
	asm("" ::: "memory"); \
	auto x = time_now();  \
	asm("" ::: "memory");

inline timespec time_now();
inline long nanodiff(timespec start_time, timespec end_time);

APICALL(self_test)
{
}
APICALL(assertion_failed)
{
	auto [expr, file, line, func] =
		machine.sysargs<std::string, std::string, int, std::string> ();

	fprintf(stderr, ">>> assertion failed: %s in %s:%d, function %s\n",
		expr.c_str(), file.c_str(), line, func.c_str());
	machine.stop();
}
APICALL(print)
{
	auto [buffer] = machine.sysargs<riscv::Buffer> ();
	auto string = buffer.to_string();

	printf(">>> %s: %.*s", get_script(machine).name().c_str(),
		(int) string.size(), string.c_str());
	machine.set_result(string.size());
}
APICALL(write_log)
{
	auto [string] = machine.sysargs<std::string> ();

	printf(">>> %s: %.*s", get_script(machine).name().c_str(),
		(int)string.size(), string.c_str());
	machine.set_result(string.size());
}

APICALL(my_name)
{
	auto& script = get_script(machine);
	const auto& name = script.name();
	/* Put pointer, length in registers A0, A1 */
	machine.cpu.reg(11) = name.size();
	machine.cpu.reg(10) = machine.arena().malloc(name.size()+1);
	machine.copy_to_guest(machine.cpu.reg(10),
		name.c_str(), name.size()+1);
}
APICALL(set_decision)
{
}

APICALL(create_response)
{
	machine.stop();
}

APICALL(regex_compile)
{
}
APICALL(regex_match)
{
}
APICALL(regex_subst)
{
}
APICALL(regex_subst_hdr)
{
}
APICALL(regex_delete)
{
}

void Script::setup_syscall_interface()
{
	machine_t::install_syscall_handlers({
		{ECALL_SELF_TEST, self_test},
		{ECALL_ASSERT_FAIL, assertion_failed},
		{ECALL_PRINT, print},
		{ECALL_LOG, write_log},

		{ECALL_REGEX_COMPILE, regex_compile},
		{ECALL_REGEX_MATCH, regex_match},
		{ECALL_REGEX_SUBST, regex_subst},
		{ECALL_REGSUB_HDR, regex_subst_hdr},
		{ECALL_REGEX_FREE, regex_delete},

		{ECALL_MY_NAME, my_name},
		{ECALL_SET_DECISION, set_decision},
		{ECALL_CREATE_RESPONSE, create_response},
	});
}

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
