#ifndef FLOWTABLE_H
#define FLOWTABLE_H

#include "Queue.h"
#include "Timer.h"
#include "DataPathUpdate.h"
#include "navl.h"
#include "DPIEngine.h"
#include "ConnectionDS.h"

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

class DPIEngine;

class FlowTable {

  public:
    /**
     * Hashing function for the unordered map used by
     * Flow Table
     */
    struct ConnKeyHasher {
        size_t operator()(const ConnKey &key) const {
            return std::hash<uint32_t>() (key.srcaddr +
                                          key.dstaddr +
                                          key.srcport +
                                          key.dstport +
                                          key.ipproto);
        }
    };

    /**
     * Key comparator for the unordered map used by
     * Flow Table
     *
     * Does a forward and reverse lookup of the 5 tuples
     * in the packet to match the flow irrespective of
     * direction
     */
    struct ConnKeyEqual {
        bool forward_lookup(const ConnKey &k1, const ConnKey &k2) const {
            return
                k1.srcaddr == k2.srcaddr &&
                k1.dstaddr == k2.dstaddr &&
                k1.srcport == k2.srcport &&
                k1.dstport == k2.dstport;
        }

        bool reverse_lookup(const ConnKey &k1, const ConnKey &k2) const {
            return
                k1.srcaddr == k2.dstaddr &&
                k1.dstaddr == k2.srcaddr &&
                k1.srcport == k2.dstport &&
                k1.dstport == k2.srcport;
        }

        bool operator()(const ConnKey &k1, const ConnKey &k2) const {
            return
                (k1.ipproto == k2.ipproto &&
                (forward_lookup(k1, k2) || reverse_lookup(k1, k2)));
        }
    };

    /**
     * unordered_map with custom hashing and equal functions
     * used by Flow Table.
     */
    typedef std::unordered_map<ConnKey, ConnInfo, ConnKeyHasher, ConnKeyEqual> unorderedmap;
    std::unordered_map<ConnKey, ConnInfo, ConnKeyHasher, ConnKeyEqual> conn_table;

    /* Create new FlowTable
     * 
     * Number of DPI Engine queues to send packets.
     *
     * Pointer to datapath update object to program
     * classified flows in the datapath.
     */
    FlowTable(int numOfQueues,
              std::shared_ptr<DataPathUpdate> dp_update);
    ~FlowTable();

    /**
     * Insert connection structures into the flow table
     *
     * Makes a pair of Key and Info to be inserted into the Flow Table.
     *
     * The Info field stores DPI metadata returned by the DPI Engine
     * and also information such as, last received packet timestamp,
     * classified timestamp, the number of packets in the pipeline to the
     * DPI Engine.
     */
    void InsertOrUpdateFlows(ConnKey *key,
                             std::string pkt_string,
                             void *filter,
                             uint8_t dir);
    /**
     * Update flow table entries with DPI result from DPI engine.
     *
     * If flows are in classified or terminated state destroy 
     * corresponding flow entry in navl dpi library.
     *
     * If in inspecting state navl flow entry is not destroyed
     */
    int updateFlowTableDPIData(ConnInfo *info, 
                               navl_handle_t handle, 
                               navl_result_t result, 
                               navl_state_t state, 
                               navl_conn_t nc, 
                               int error);

    /**
     * Start the Flow Table cleanup thread
     */
    void start(std::shared_ptr<DPIEngine> &dpiEngine);

    /**
     * Notifies the flow table cleanup thread to stop.
     *
     * Sends exit message to DPI engine queues.
     *
     * Sends exit message to Datapath Update queue.
     */
    void stop();

    /**
     * decrement number of DPI engine queues
     */
    void decrementNumQueues () {
        numQueues_--;
    }

  private:
    /**
     * Flow Table Cleanup Thread. Runs every 100 milliseconds
     */ 
    void FlowTableCleanup();

    /**
     * Flow Table cleanup function. 
     *
     * If final_delete is true, all flows are deleted.
     *
     * If final_delete is false two things happen
     *   - Flows in classified state and have packet ref
     *     count zero are deleted.
     *   - Flows in inspection state and have not received
     *     a packet since 10 secs are deleted.
     */
    void cleanupFlows(bool final_delete);

    /**
     * Mutex to lock the Flow Table
     */
    std::mutex ftbl_mutex;
    
    /**
     * Mutex to lock till cleanup thread is woken up
     */
    std::mutex cleanupThreadMutex_;

    /**
     * conditional variable to wait for 100 milliseconds
     * and wake to start Flow Table cleanup
     */
    std::condition_variable cleanupThreadcv_;

    /**
     * Number of DPI Engine queues
     */
    int numQueues_;

    /**
     * Set to false to stop the cleanup thread
     */
    bool running_;

    /**
     * flow cleanup thread
     */
    std::thread flowCleanupThread_;

    /**
     * Pointer to datapath update object
     */
    std::shared_ptr<DataPathUpdate> dpUpdate_;

    /**
     * Pointer to DPI engine object
     */
    std::shared_ptr<DPIEngine> dpiEngine_;
};

}

#endif
