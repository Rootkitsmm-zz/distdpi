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

#include <DPIEngine.h>

void bind_navl_externals();

using namespace std::placeholders;

std::mutex dpiengine_lock;
void *ptrForCb = NULL;

namespace distdpi {

DPIEngine::DPIEngine(std::shared_ptr<FlowTable> ftbl,
                     int numthreads):
    numOfDpiThreads_(numthreads), ftbl_(ftbl) {
    ptrForCb = (void *) this;
    for (int i = 0; i < numOfDpiThreads_; i++)
        dpiEngineQueueList_.push_back(
            std::unique_ptr<Queue<ConnMetadata>>(
                new Queue<ConnMetadata>()));
}

int
DPIEngine::navl_classify_callback(navl_handle_t handle,
                                  navl_result_t result,
                                  navl_state_t state,
                                  navl_conn_t nc,
                                  void *arg, int error)
{
    dpiengine_lock.lock(); 
    ConnInfo *info = static_cast<ConnInfo *>(arg);
    int ret = ((DPIEngine *)ptrForCb)->ftbl_->updateFlowTableDPIData(info,
                                                                     handle,
                                                                     result,
                                                                     state,
                                                                     nc,
                                                                     error);
    dpiengine_lock.unlock();
    return ret;
}

int DPIEngine::cleanupFlow(navl_handle_t handle,
                           void *dpi_state) {
    return navl_conn_destroy(handle, dpi_state);
}

void DPIEngine::Dequeue(int queue) {
    bind_navl_externals();
    navl_handle_t g_navlhandle_;

    if((g_navlhandle_ = navl_open("plugins")) == 0)
        fprintf(stderr, "navl_open failed\n");

    if (navl_init(g_navlhandle_) == -1)
        fprintf(stderr, "navl_init failed\n");

    navl_config_set(g_navlhandle_, "tcp.timeout", "10");
    navl_config_set(g_navlhandle_, "tcp.track_user_conns", "1");
    //navl_config_set(g_navlhandle_, "udp.track_user_conns", "1");
    //navl_config_set(g_navlhandle_, "tcp.timeout", "10");

    try {
        for (;;) {
            ConnKey *key;
            ConnInfo *info;
            ConnMetadata m = dpiEngineQueueList_[queue]->pop();
            if (m.exit_flag)
                break;

            key = m.key;
            info = m.info;
        
            if (info->dpi_state == NULL) {
                navl_host_t src_addr, dst_addr;

                src_addr.family = NAVL_AF_INET;
                src_addr.port = htons(key->srcport);
                src_addr.in4_addr = htonl(key->srcaddr);
                dst_addr.family = NAVL_AF_INET;
                dst_addr.port = htons(key->dstport);
                dst_addr.in4_addr = htonl(key->dstaddr);
                info->handle = g_navlhandle_;
                if (navl_conn_create(g_navlhandle_,
                                     &src_addr,
                                     &dst_addr,
                                     key->ipproto,
                                     &(info->dpi_state)) != 0) {
                    syslog(LOG_INFO, "Failed to create connection in navl");
                    continue;
                }
            }
            else {
                if (!info->error &&
                    m.data.size() &&
                    info->dpi_state &&
                    info->class_state == NAVL_STATE_INSPECTING) {

                    if (navl_classify(g_navlhandle_, 
                                      NAVL_ENCAP_NONE,
                                      m.data.c_str(),
                                      m.data.size(),
                                      info->dpi_state,
                                      m.dir,
                                      DPIEngine::navl_classify_callback,
                                      (void *)info))
                    {
                        syslog(LOG_INFO, "Failed to call navl classify packet");
                        continue;
                    }
                }
                else {
                }
            }
        }
    } catch (const std::exception& ex) {
    
        ftbl_->decrementNumQueues();
        dpiEngineQueueList_[queue]->clear();
        //dpiEngineQueueList_[queue].reset();
    }

    navl_fini(g_navlhandle_);
    navl_close(g_navlhandle_);
    std::cout << " Exiting thread for queue " << queue << std::endl;
}

void DPIEngine::start() {
    for (int i = 0; i < numOfDpiThreads_; i++)
        dpithreads.push_back(std::thread(&DPIEngine::Dequeue, this, i));
}

void DPIEngine::stop () {
    for (int i = 0; i < numOfDpiThreads_; i++)
        dpithreads[i].join();
    syslog(LOG_INFO, "DPI Engines stopped ");;
}

DPIEngine::~DPIEngine() {
    std::cout << "Calling DPIEngine Destructor" << std::endl;
}

} 
