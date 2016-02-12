#ifndef DATAPATHUPDATE_H
#define DATAPATHUPDATE_H

#include "Queue.h"

#include <unordered_map>
#include <list>
#include <vector>
#include <queue>
#include <memory>
#include <cstdatomic>
#include <cassert>
#include <cstdlib>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace distdpi {

class DataPathUpdate {
  public:
    struct DPIFlowData {
        uint32_t srcaddr;
        uint32_t dstaddr;
        uint16_t srcport;
        uint16_t dstport;
        u_char  ipproto;
        bool exit_flag;
        int dir;
        void *filter;
        int len;
        std::string dpiresult;
    };
    
    std::unique_ptr<Queue<DPIFlowData>> updateDPQueue_;

    DataPathUpdate();
    ~DataPathUpdate();

    void start();
    void stop();

  private:
    std::thread updateDPThread_;
    void GetDPIFlowData();
};

}

#endif
