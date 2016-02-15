#ifndef PACKETHANDLER_H
#define PACKETHANDLER_H

#include <memory>

#include "FlowTable.h"
#include "ProducerConsumerQueue.h"
#include "Timer.h"
#include "ConnectionDS.h"
#include <netx_service.h>

namespace distdpi {

class PacketHandler {
  public:

    /**
     * Packet Metadata received from 
     * dvfiter layer
     */
    struct PktMdata {
        void *filter;
        string pkt;
        uint8_t dir;
    };

    /**
     * Create new PacketHandler with
     * serviceinstance name and pointer to 
     * the Flow Table
     */
    PacketHandler(std::string AgentName,
                  std::shared_ptr<FlowTable> ftbl);
    ~PacketHandler();

    /**
     * Start PacketHandler
     *
     * Creates three threads
     *
     * First one to accept packets from the dvfilter
     * layer and push to SPSC queue.
     *
     * Second thread reads from the SPSC queue, checks
     * packet ethernet and IP headers and fills Connection
     * structures for insert or update of Flow Table
     *
     * Third thread to update the packet rate (debug mode)
     */ 
    void start();

    /**
     * Stop PacketHandler
     *
     * Notifies the PacketProducer, PacketConsumer and
     * PacketRate measuring threads to stop.
     */ 
    void stop();

    /**
     * Pushes packets received from dvfilter layer 
     * to the queue.
     *
     * After pushing in the queue, notifies the consumer
     * thread that there is data in the queue.
     *
     * If the queue is full, does a busy wait till
     * it can push the data in the queue.
     */
    void PacketProducer(PktMetadata *pktmdata,
                        uint32_t len);

    /**
     * Callback function registered with the
     * netx and dvfilter service to receive packets
     */ 
    static void StaticPacketProducer(void *obj,
                                     PktMetadata *pktmdata,
                                     uint32_t len);
    
    /**
     * Reads packets from the queue and sends
     * them for packet processing and flow table
     * insertion
     *
     * Waits for notification from the Producer
     * thread before reading packets.
     *
     * Reads the queue, till it is empty. 
     */
    void PacketConsumer();

    /**
     * Starts the netx and dvfilter service
     * to receive packets and pass the callback function
     * to it.
     *
     * Also passes the 'this' pointer to be used 
     * in the callback function to call the 
     * Packet Producer.
     */
    void ConnectToPktProducer();
 
    /**
     * Receives packets from the Consumer
     * and parses packet ethernet and IP
     * headers
     */
    void classifyFlows(PktMdata *mdata);

    /**
     * Fills Connection structures and sends
     * them to the Flow Table insertion
     * or update
     */  
    void populateFlowTable(const u_char *ptr,
                           u_int len,
                           ConnKey *key,
                           void *filter,
                           uint8_t dir);

  private:

    /**
     * Flow Table pointer
     */
    std::shared_ptr<FlowTable> ftbl_;

    /**
     * vector of PacketHandler threads
     */
    std::vector<std::thread> pkthdl_threads;

    /**
     * SPSC queue for Packet Metadata
     */
    ProducerConsumerQueue<PktMdata> queue_;

    /**
     * Service instance name 
     */
    std::string AgentName_;

    /**
     * Set false to stop the PacketHandler
     * threads
     */
    bool running_;

    /**
     * Measure packets/second being received
     */
    void PktRateMeasurer();
};


}
#endif
