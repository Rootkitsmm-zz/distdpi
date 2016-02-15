#ifndef DATAPATHUPDATE_H
#define DATAPATHUPDATE_H

#include "Queue.h"
#include "ConnectionDS.h"

#include <memory>

namespace distdpi {

class DataPathUpdate {
  public:

    /**
     * Queue to pass Flow DPI data to be programmed
     * into the datapath
     */    
    std::unique_ptr<Queue<DPIFlowData>> updateDPQueue_;

    /**
     * Create new DataPathUpdate
     */
    DataPathUpdate();
    ~DataPathUpdate();

    /**
     * Start a new thread to deqeue Flow DPI data
     * received from the FlowTable
     */
    void start();

    /**
     * Stop the thread to dequee Flow DPI data
     */
    void stop();

  private:
    /**
     * Thread to update DPI flow data to the datapath
     */
    std::thread updateDPThread_;

    void GetDPIFlowData();
};

}

#endif
