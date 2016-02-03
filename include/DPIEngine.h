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
    
        static int
        navl_classify_callback(navl_handle_t handle, navl_result_t result, navl_state_t state, navl_conn_t nc, void *arg, int error);

        //std::function<int(navl_handle_t, navl_result_t, navl_state_t, navl_conn_t, void*, int)> callback;
   
    private:
        int num_of_dpi_threads;

        std::shared_ptr<FlowTable> ftbl_;

        navl_handle_t g_navlhandle_;
};


}
#endif
