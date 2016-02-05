#ifndef PACKETHANDLER_H
#define PACKETHANDLER_H

#include <memory>

#include "FlowTable.h"
#include "ProducerConsumerQueue.h"
#include "Timer.h"
#include "SignalHandler.h"

namespace distdpi {

class PacketHandler: public Timer/*, public SignalHandler*/ {
public:

    PacketHandler(std::string AgentName, std::shared_ptr<FlowTable> ftbl);
    ~PacketHandler();
    void start();
    void PacketProducer(uint8_t *pkt, uint32_t len);
    static void StaticPacketProducer(void *obj, uint8_t *pkt, uint32_t len);
    void populateFlowTable(const u_char *ptr,
                           u_int len,
                           FlowTable::ConnKey *key);
    void PacketConsumer();
    void ConnectToPktProducer();
    void classifyFlows(std::string &packet);

private:
    std::shared_ptr<FlowTable> ftbl_;
    std::vector<std::thread> pkthdl_threads;
    std::vector<int> signals;
    ProducerConsumerQueue<std::string> queue_;
    std::string AgentName_;

protected:
    virtual void executeCb();
    //virtual void stop();
};


}
#endif
