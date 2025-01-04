#include "tenant_instance.hpp"
#include "machine_instance.hpp"
#include <stdexcept>

//#define ENABLE_TIMING
#define TIMING_LOCATION(x) \
	asm("" ::: "memory"); \
	auto x = time_now();  \
	asm("" ::: "memory");


inline timespec time_now();
inline long nanodiff(timespec start_time, timespec end_time);
std::vector<uint8_t> file_loader(const std::string& file);

TenantInstance::TenantInstance(const TenantConfig& conf)
	: config{conf}
{
	try {
		auto shared_elf = std::make_shared<std::vector<uint8_t>>(file_loader(conf.filename));
		this->machine =
			std::make_shared<MachineInstance> (std::move(shared_elf), this);
	} catch (const std::exception& e) {
		fprintf(stderr,
			"Exception when creating machine '%s': %s",
			conf.name.c_str(), e.what());
		machine = nullptr;
	}
}
TenantInstance::~TenantInstance()
{
	SharedMachine release = nullptr;
	/* Release the machine */
	std::atomic_exchange(&this->machine, release);
}

TenantInstance::SharedMachine TenantInstance::get_current_instance() const
{
	return std::atomic_load(&this->machine);
}

Script* TenantInstance::vmfork()
{
#ifdef ENABLE_TIMING
	TIMING_LOCATION(t0);
#endif
	SharedMachine program = this->get_current_instance();
	/* First-time tenants could have no program */
	if (UNLIKELY(program == nullptr))
		return nullptr;

	try {
		Script* script = new Script{program->script, this, *program};
		script->assign_instance(std::move(program));
		return script;

	} catch (const std::exception& e) {
		fprintf(stderr,
			"VM '%s' exception: %s", program->script.name().c_str(), e.what());
		return nullptr;
	}
#ifdef ENABLE_TIMING
	TIMING_LOCATION(t1);
	printf("Time spent in initialization: %ld ns\n", nanodiff(t0, t1));
#endif
}

TenantInstance::ForkCall::ForkCall(const Script& script, TenantInstance* tenant, MachineInstance& inst)
	: script{script, tenant, inst}, cnt{0}
{
	/* No initialization */
}

TenantInstance::ForkCall TenantInstance::forkcall(Script::gaddr_t addr,
	size_t bufcnt, riscv::vBuffer buffers[])
{
	SharedMachine program = this->get_current_instance();
	if (UNLIKELY(program == nullptr))
		throw std::runtime_error("No program loaded");

	ForkCall result(program->script, this, *program);
	Script& script = result.script;
	script.assign_instance(std::move(program));

	/* Call into the virtual machine */
#ifdef ENABLE_TIMING
	TIMING_LOCATION(t2);
#endif
	script.call(addr);
	TIMING_LOCATION(t3);
	#ifdef ENABLE_TIMING
	printf("Time spent in forkcall(): %ld ns\n", nanodiff(t2, t3));
#endif

	// TODO: perform check to see if the result
	// is forging a response
	Script::machine_t& machine = script.machine();
	//const auto [type, data] = machine.sysargs<riscv::Buffer, riscv::Buffer> ();
	auto cnt_addr = machine.cpu.reg(12);
	auto cnt_len  = machine.cpu.reg(13);
	result.cnt = machine.memory.gather_buffers_from_range(bufcnt, buffers, cnt_addr, cnt_len);
	return result;
}

Script::gaddr_t TenantInstance::lookup(const char* name) const {
	SharedMachine program = this->get_current_instance();
	if (LIKELY(program != nullptr))
		return program->lookup(name);
	return 0x0;
}

#include <unistd.h>
std::vector<uint8_t> file_loader(const std::string& filename)
{
    size_t size = 0;
    FILE* f = fopen(filename.c_str(), "rb");
    if (f == NULL) throw std::runtime_error("Could not open file: " + filename);

    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);

    std::vector<uint8_t> result(size);
    if (size != fread(result.data(), 1, size, f))
    {
        fclose(f);
        throw std::runtime_error("Error when reading from file: " + filename);
    }
    fclose(f);
    return result;
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
