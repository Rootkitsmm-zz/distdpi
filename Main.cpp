#include <string.h>
#include <getopt.h>
#include <thread>
#include <iostream>
#include <syslog.h>
#include <vector>

#include <PacketHandler.h>
#include <FlowTable.h>
#include <DPIEngine.h>
#include <DistDpi.h>
#include <netx_service.h>

using namespace distdpi;

int main(int argc, char* argv[]) {
    DistDpi distdpi;
    std::thread t = std::thread(&DistDpi::start, &distdpi);
    t.join();
    return 0;
}
