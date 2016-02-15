#ifndef DPIENGINE_H
#define DPIENGINE_H

#include <memory>

#include "Queue.h"
#include "navl.h"
#include "FlowTable.h"
#include "ConnectionDS.h"

namespace distdpi {

class FlowTable;

class DPIEngine {
  public:

    DPIEngine(std::shared_ptr<FlowTable> ftbl,
              int numthreads);
    ~DPIEngine();

    /**
     * De-queue Pkt Metadata from the DPI queues
     * 
     * Calls navl_open and navl_init for the first time
     * and enters a loop to deque pkt metadata.
     *
     * Check if running flag is set to false to exit
     *
     * If it is the first packet for a connection, create
     * a new navl connection entry.
     *
     * If its not the first packet, the classified state is 
     * still inspecting, the packet has data and there are no
     * errors the packet is sent for navl classification.
     * 
     * calls navl_fini and navl_close on exit.
     */
    void Dequeue(int queue);

    /**
     * Start the DPI thread instances
     */
    void start();

    /**
     * Stop the DPI thread instances
     */
    void stop();

    /**
     *list to store DPI queue objects
     */
    std::vector<std::unique_ptr<Queue<ConnMetadata>>> dpiEngineQueueList_;

    /**
     * callback function called when DPI results
     * are returned.
     */
    static int
    navl_classify_callback(navl_handle_t handle,
                           navl_result_t result,
                           navl_state_t state,
                           navl_conn_t nc,
                           void *arg, int error);
   
    int
    cleanupFlow(navl_handle_t handle,
                void *dpi_state);
  private:
    /**
     * Number of DPI instances
     */
    int numOfDpiThreads_;

    /**
     * Pointer to flow table object
     */
    std::shared_ptr<FlowTable> ftbl_;

    /**
     * vector to store dpi threads
     */
    std::vector<std::thread> dpithreads;
};


}
#endif        
