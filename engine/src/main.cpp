#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include "MatchEngine.h"

#ifdef HAS_GRPC
#include <grpcpp/grpcpp.h>
#include "MatchService.h"

void run_server(const std::string& address) {
    auto engine = std::make_shared<MatchEngine>();
    MatchServiceImpl service(engine);

    grpc::ServerBuilder builder;
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    auto server = builder.BuildAndStart();
    std::cout << "MatchEngine gRPC server listening on " << address << "\n";
    server->Wait();
}
#else
void run_server(const std::string& address) {
    std::cerr << "gRPC not compiled in — stub mode. Address would be: " << address << "\n";
}
#endif

int main(int argc, char* argv[]) {
    std::string addr = "0.0.0.0:50051";
    if (argc > 1) addr = argv[1];

    const char* env = std::getenv("GRPC_ADDRESS");
    if (env) addr = env;

    run_server(addr);
    return 0;
}
