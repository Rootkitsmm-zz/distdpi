#ifndef CONNECTIONDS_H
#define CONNECTIONDS_H

#include "navl.h"

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

    /**
     * Connection Key for Flow Table
     */
    struct ConnKey {
        ConnKey()
        : srcaddr(0),
          dstaddr(0),
          srcport(0),
          dstport(0),
          ipproto(0) {}

        uint32_t srcaddr; // source address
        uint32_t dstaddr; // destination address
        uint16_t srcport; // source port
        uint16_t dstport; // destination port
        u_char  ipproto; // IP protocol
    };

    /**
     * Connection Info passed to navl library
     */
    struct ConnInfo {
        ConnInfo(ConnKey *key)
        : key(*key),
          queue(0),
          connid(0),
          packetnum(0),
          classified_num(0),
          class_state(NAVL_STATE_INSPECTING),
          conn(NULL),
          dpi_state(NULL), dpi_result(0),
          dpi_confidence(0),
          error(0), classified_timestamp(0),
          handle(0), dir(0),
          filter(NULL), pkt_ref_cnt(0)
          {}

        ConnInfo(const ConnInfo& other)
        : key(other.key),
          queue(other.queue),
          connid(other.connid),
          packetnum(other.packetnum),
          classified_num(other.classified_num),
          class_state(other.class_state),
          conn(other.conn),
          dpi_state(other.dpi_state), dpi_result(other.dpi_result),
          dpi_confidence(other.dpi_confidence),
          error(other.error), classified_timestamp(other.classified_timestamp),
          handle(other.handle), dir(other.dir),
          filter(other.filter), pkt_ref_cnt(other.pkt_ref_cnt.load())
          {}

        ConnKey key; // Pointer to connection key
        int queue; // Queue to enqueue Connection Metadata
        uint64_t connid; // Connection Id
        uint32_t packetnum; // Current number of packets
        u_int   classified_num; // navl Classified protocol number
        navl_state_t class_state; // navl Current state of classification
        navl_conn_t conn; // navl Connection identifier

        void    *dpi_state; // navl DPI state
        u_int   dpi_result; // navl DPI result
        int     dpi_confidence; // navl Confidence Level
        int     error; // navl current error state
        int     classified_timestamp; // timestamp when connection got classified
        int     lastpacket_timestamp; // timestamp for last packet
        int     handle; // navl instance handle
        int     dir; // Initial direction of connection
        void    *filter; // Filter associated with connection
        std::atomic<int> pkt_ref_cnt; // Pkt reference count
    };

    /**
     * Connection Metadata passed between
     * flow table and DPI engine
     */
    struct ConnMetadata {
        ConnKey *key; // pointer to Connection key
        ConnInfo *info; // pointer to Connection info
        uint32_t pktnum; // Current packet number
        uint32_t dir; // Current direction
        std::string data; // Packet data
        uint8_t exit_flag; // Exit flag to stop conn metadata dequeing
    };

    /**
     * DPI Flow metadata passed between flow table
     * and DatapathUpdate
     */
    struct DPIFlowData {
        ConnKey key; // Connection key
        bool exit_flag; // Exit flag to stop flow data dequeing
        int dir; // Initial direction of packet
        void *filter; // Filter of packet origin
        int len; // Length of result
        std::string dpiresult; // DPI result
    };

}

#endif
