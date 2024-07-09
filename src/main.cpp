#include <drogon/drogon.h>
#include <sandbox.hpp>
using namespace drogon;

static TenantInstance* guest = nullptr;
static uint64_t addr = 0x0;

int main(int argc, char** argv)
{
	if (argc < 2) {
		fprintf(stderr, "%s [program]\n", argv[0]);
		exit(1);
	}
    guest = new TenantInstance({
        .name = "Pythran",
        .group = "Tenants",
        .filename = std::string(argv[1]),
        .max_instructions = 2'000'000ull,
        .max_memory = 64'000'000ull,
        .max_heap   = 8'000'000ull
    });
    assert(!guest->no_program_loaded());
    addr = guest->lookup("on_client_request");
    assert(addr != 0x0);

    app().setLogPath("./")
        .setLogLevel(trantor::Logger::kWarn)
        .addListener("0.0.0.0", 8080)
        .addListener("0.0.0.0", 8080)
        .setThreadNum(0)
        .registerSyncAdvice(
		[] (const HttpRequestPtr &req) -> HttpResponsePtr {
            const auto &path = req->path();
            if (path.length() == 1 && path[0] == '/')
            {
                auto response = HttpResponse::newHttpResponse();
                response->setBody(
R"(	this is a very
	long string if I had the
	energy to type more and more ...
)");
                return response;
            }
            else if (path.length() == 2 && path[1] == 'z')
            {
                auto resp = HttpResponse::newHttpResponse();

				try {
					constexpr size_t BUFMAX = 64;
					std::array<riscv::vBuffer, BUFMAX> buffers;

					auto res = guest->forkcall(addr, BUFMAX, buffers.data());

					resp->setBody({buffers[0].ptr, buffers[0].len});
				} catch (const std::exception& e) {
					resp->setStatusCode(k500InternalServerError);
					resp->setBody(e.what());
				}
                return resp;
            }
            return nullptr;
        })
        .run();
}
