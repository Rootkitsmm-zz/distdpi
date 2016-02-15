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

    /**
     * List of signals to register for
     */
    std::vector<int> signals;

    /**
     * Pointer to DPI Engine object
     */
    std::shared_ptr<DPIEngine> dpi_engine_;

    /**
     * Pointer to Flow Table object
     */
    std::shared_ptr<FlowTable> ftb;

    /**
     * Pointer to DPI Engine object
     */
    std::shared_ptr<DataPathUpdate> dp_update_;

    /**
     * Pointer to DPI Engine object
     */
    std::unique_ptr<PacketHandler> pkt_hdl_;

    /**
     * Pointer to DPI Engine object
     */
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
