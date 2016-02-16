#include <iostream>
#include <string.h>
#include <thread>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <functional>
#include <navl.h>
#include <utility>
#include <syslog.h>

#include <DataPathUpdate.h>

using namespace std::placeholders;

namespace distdpi {

DataPathUpdate::DataPathUpdate() {
    updateDPQueue_ = std::unique_ptr<Queue<DPIFlowData>>(new Queue<DPIFlowData>());
}

void DataPathUpdate::GetDPIFlowData() {
    for (;;) {
        DPIFlowData data = updateDPQueue_->pop();
        if (data.exit_flag)
            break;
        //syslog(LOG_INFO, "DPI result %s", data.dpiresult.c_str());
        //printf("\n DPI result %s", data.dpiresult.c_str());
    }
}

void DataPathUpdate::start() {
    updateDPThread_ = std::thread(&DataPathUpdate::GetDPIFlowData, this);
}

void DataPathUpdate::stop () {
    updateDPThread_.join();
}

DataPathUpdate::~DataPathUpdate() {
    std::cout << "Calling DataPathUpdate Destructor" << std::endl;
}

} 
