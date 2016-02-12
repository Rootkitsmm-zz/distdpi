#ifndef DISTDPI_H
#define DISTDPI_H

#include "SignalHandler.h"
#include "FlowTable.h"
#include "PacketHandler.h"
#include "DPIEngine.h"
#include "DataPathUpdate.h"

#include <memory>

namespace distdpi {

class DistDpi: public SignalHandler {
public:

    DistDpi();
    ~DistDpi();

    void start();
private:
    std::vector<int> signals;
    std::unique_ptr<DPIEngine> dpi_engine_;
    std::shared_ptr<FlowTable> ftb;
    std::shared_ptr<DataPathUpdate> dp_update_;
    std::unique_ptr<PacketHandler> pkt_hdl_;
    std::vector<std::thread> th;

    bool running_;
    bool notify;
    std::mutex mutex_;
    std::condition_variable cv_;
protected:
    void stop();
};


}
#endif
