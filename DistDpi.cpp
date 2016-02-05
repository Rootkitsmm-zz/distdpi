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

namespace distdpi {

DistDpi::DistDpi() {
}

DistDpi::~DistDpi() {
}

void DistDpi::start() {
    int dpi_instances = 4;
    std::shared_ptr<FlowTable> ftb = std::make_shared<FlowTable> (dpi_instances);
    PacketHandler pkthdl("serviceinstance-2", ftb);
    DPIEngine dpi(ftb, dpi_instances);

    std::vector<std::thread> th;
    th.push_back(std::thread(&PacketHandler::start, &pkthdl));
    std::cout << "Starting DPI engine " << std::endl;
    th.push_back(std::thread(&DPIEngine::start, &dpi));
    th[0].join();
    th[1].join();
}

}
