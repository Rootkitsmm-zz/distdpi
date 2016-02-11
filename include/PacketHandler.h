#ifndef PACKETHANDLER_H
#define PACKETHANDLER_H

#include <memory>

#include "FlowTable.h"
#include "ProducerConsumerQueue.h"
#include "Timer.h"
#include <netx_service.h>

namespace distdpi {

class PacketHandler: public Timer/*, public SignalHandler*/ {
public:

    struct PktMdata {
        void *filter;
        string pkt;
    };

    PacketHandler(std::string AgentName, std::shared_ptr<FlowTable> ftbl);
    ~PacketHandler();
    void start();
    void stop();
    void PacketProducer(PktMetadata *pktmdata, uint32_t len);
    static void StaticPacketProducer(void *obj, PktMetadata *pktmdata, uint32_t len);
    void populateFlowTable(const u_char *ptr,
                           u_int len,
                           FlowTable::ConnKey *key,
                           void *filter);
    void PacketConsumer();
    void ConnectToPktProducer();
    void classifyFlows(PktMdata *mdata);
    //void classifyFlows(std::string &packet);
    //void classifyFlows(std::vector<std::string> &pktlist);

private:
    std::shared_ptr<FlowTable> ftbl_;
    std::vector<std::thread> pkthdl_threads;
    //ProducerConsumerQueue<std::string> queue_;
    ProducerConsumerQueue<PktMdata> queue_;
    std::string AgentName_;

    bool running_;
    std::mutex mutex_;
    std::condition_variable cv_;
protected:
    virtual void executeCb();
};


}
#endif
