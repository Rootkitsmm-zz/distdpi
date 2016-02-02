#ifndef PACKETHANDLER_H
#define PACKETHANDLER_H

#include <memory>

#include "FlowTable.h"
#include "ProducerConsumerQueue.h"
#include "PacketHandlerOptions.h"
#include "Timer.h"

namespace distdpi {

class PacketHandler: public Timer {

    public:

        PacketHandler(PacketHandlerOptions options,
                      std::shared_ptr<FlowTable> ftbl);
        ~PacketHandler();
        void start();
        void PacketProducer(uint8_t *pkt, uint32_t len);
        void populateFlowTable(const u_char *ptr,
                               u_int len,
                               FlowTable::ConnKey *key);
        void PacketConsumer();
        void classifyFlows(std::string &packet);
        

    private:
        std::shared_ptr<PacketHandlerOptions> options_;
        std::shared_ptr<FlowTable> ftbl_;
        ProducerConsumerQueue<std::string> queue_;

    protected:
        virtual void executeCb();
};


}
#endif
