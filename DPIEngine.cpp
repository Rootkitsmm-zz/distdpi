#include <iostream>
#include <string.h>
#include <thread>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <functional>
#include <navl.h>
#include <DPIEngine.h>

void bind_navl_externals();
void* ptrForCb;

using namespace std::placeholders;

namespace distdpi {

navl_handle_t g_navl;

DPIEngine::DPIEngine(std::shared_ptr<FlowTable> ftbl,
                     int numthreads):
    num_of_dpi_threads(numthreads), ftbl_(ftbl) {
    ptrForCb = (void *) this;
}

static const char *
get_state_string(navl_state_t state)
{
    switch (state)
    {
    case NAVL_STATE_INSPECTING:
        return "INSPECTING";
    case NAVL_STATE_CLASSIFIED:
        return "CLASSIFIED";
    case NAVL_STATE_TERMINATED:
        return "TERMINATED";
    default:
        return "UNKNOWN";
    }
}

int
DPIEngine::navl_classify_callback(navl_handle_t handle, navl_result_t result, navl_state_t state, navl_conn_t nc, void *arg, int error)
{
    DPIEngine *ptr = (DPIEngine *) ptrForCb;
    FlowTable::ConnInfo *info = static_cast<FlowTable::ConnInfo *>(arg);
    static char name[9];
    
    info->error = error;

    // save the classification state
    info->class_state = state;

    // fetch the result
    info->dpi_result = navl_app_get(ptr->g_navlhandle_, result, &info->dpi_confidence);

    // if we've just classified the connection, record what data packet (sum)
    // the event occur on
    if (!info->classified_num && state == NAVL_STATE_CLASSIFIED)
        info->classified_num = info->initiator_total_packets + info->recipient_total_packets;

    printf("    Classification: %s %s after %u packets %u%% confidence\n", info->dpi_result ? navl_proto_get_name(ptr->g_navlhandle_, info->dpi_result, name, sizeof(name)) : "UNKNOWN"
        , get_state_string(info->class_state)
        , info->classified_num
            ? info->classified_num : (info->initiator_total_packets + info->recipient_total_packets)
        , info->dpi_confidence);

#if 0
    // Remove connections for terminated flows
    if (state == NAVL_STATE_TERMINATED)
    {
        conn_table::iterator it = g_conn_table.find(info->key);
        if (it != g_conn_table.end())
        {
            display_conn(&it->first, &it->second);
            g_conn_table.erase(it);
        }
    }

    return 0;
#endif
}

void DPIEngine::Dequeue() {
    bind_navl_externals();

    if((g_navlhandle_ = navl_open("plugins")) == 0)
        fprintf(stderr, "navl_open failed\n");

    if (navl_init(g_navlhandle_) == -1)
        fprintf(stderr, "navl_init failed\n");

    for (;;) {
        //while (!ftbl_->ftbl_queue_.empty())
        //{
            FlowTable::ConnMetadata m = ftbl_->ftbl_queue_list_[0]->pop();
            //ftbl_->ftbl_queue_list_.pop();
 
            std::cout << "g_navl " << g_navlhandle_ << " Src add " << m.key->srcaddr << " Dst addr " << m.key->dstaddr << " src port " <<
            m.key->srcport << " dst port " << m.key->dstport << " ip proto " << m.key->ipproto << " packet num " << m.info->packetnum << 
            " DPI state " << m.info->dpi_state << std::endl;

            if (m.info->packetnum == 0) {
                navl_host_t src_addr, dst_addr;
                src_addr.family = NAVL_AF_INET;
                src_addr.port = htons(m.key->srcport);
                src_addr.in4_addr = htonl(m.key->srcaddr);
                dst_addr.family = NAVL_AF_INET;
                dst_addr.port = htons(m.key->dstport);
                dst_addr.in4_addr = htonl(m.key->dstaddr);
                if (navl_conn_create(g_navlhandle_, &src_addr, &dst_addr, m.key->ipproto, &(m.info->dpi_state)) != 0) {
                    std::cout << "Returning -----------" << std::endl; 
                    continue;
                }
            }
            else {
                if (navl_classify(g_navlhandle_, NAVL_ENCAP_NONE, m.data.c_str(), m.data.size(), m.info->dpi_state, m.dir, DPIEngine::navl_classify_callback, (void *)&m.info))
                {
                    std::cout << "Returning unable to dpi :P dir " << std::endl;
                }
            }

        //}
    }
}

void DPIEngine::start() {
    std::vector<std::thread> dpithreads;
    printf("\n AIEEEEEEEEEEEEEEE %p ", &(ftbl_->ftbl_queue_list_[0]));
    for (int i = 0; i < num_of_dpi_threads; i++)
        dpithreads.push_back(std::thread(&DPIEngine::Dequeue, this));

    for (int i = 0; i < num_of_dpi_threads; i++)
        dpithreads[i].join();
}

DPIEngine::~DPIEngine() {
    std::cout << "Calling destructor" << std::endl;
}

} 
