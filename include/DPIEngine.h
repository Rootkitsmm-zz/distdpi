#ifndef DPIENGINE_H
#define DPIENGINE_H

#include <memory>

#include "FlowTable.h"

namespace distdpi {

class DPIEngine {
public:

    DPIEngine(std::shared_ptr<FlowTable> ftbl,
              int numthreads);
    ~DPIEngine();
    void Dequeue(int queue);
    void start();
    void stop();

    static int
    navl_classify_callback(navl_handle_t handle, navl_result_t result, navl_state_t state, navl_conn_t nc, void *arg, int error);

private:
    int num_of_dpi_threads;
    std::shared_ptr<FlowTable> ftbl_;
    std::vector<std::thread> dpithreads;
};


}
#endif        
