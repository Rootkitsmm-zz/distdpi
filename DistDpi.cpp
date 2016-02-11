#include <string.h>
#include <getopt.h>
#include <thread>
#include <iostream>
#include <syslog.h>
#include <vector>
#include <csignal>

#include <PacketHandler.h>
#include <FlowTable.h>
#include <DPIEngine.h>
#include <DistDpi.h>

namespace distdpi {

DistDpi::DistDpi() {
}

DistDpi::~DistDpi() {
}

void DistDpi::stop() {
    std::cout << "Stop called " << std::endl;

    pkt_hdl_->stop();
    ftb->stop();
    dpi_engine_->stop();
    th[0].join();
    th[1].join();
    th[2].join();
    std::unique_lock<std::mutex> lk(mutex_);
    running_ = true;
    notify = true;
    cv_.notify_one();
}

void DistDpi::start() {
    int dpi_instances = 4;

    running_ = false;
    notify = false;
    std::cout << "Starting DPI engine " << std::endl;
    signals.push_back(SIGTERM);
    signals.push_back(SIGINT);
    this->install(this, signals);
   
    //std::shared_ptr<FlowTable> ftb = std::make_shared<FlowTable> (dpi_instances);
    //PacketHandler pkthdl("serviceinstance-2", ftb);
    //DPIEngine dpi(ftb, dpi_instances);
    ftb = std::make_shared<FlowTable> (dpi_instances);
    pkt_hdl_ = std::unique_ptr<PacketHandler>(new PacketHandler("serviceinstance-2", ftb));
    dpi_engine_ = std::unique_ptr<DPIEngine>(new DPIEngine(ftb, dpi_instances));
    
    //std::vector<std::thread> th;
    //th.push_back(std::thread(&PacketHandler::start, &pkthdl));
    //th.push_back(std::thread(&DPIEngine::start, &dpi));
    th.push_back(std::thread(&PacketHandler::start, pkt_hdl_.get()));
    th.push_back(std::thread(&DPIEngine::start, dpi_engine_.get()));
    th.push_back(std::thread(&FlowTable::start, ftb));

    while(!running_) {
        std::unique_lock<std::mutex> lk(mutex_);
        while(!notify)
            cv_.wait(lk);
    }
    std::cout << "Exiting Distdpi " << std::endl;
}

}
