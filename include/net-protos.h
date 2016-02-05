/* **********************************************************
 * Copyright 2007 VMware, Inc.  All rights reserved. -- VMware Confidential
 * **********************************************************/

/*
 * net-protos.h --
 *
 *   Network protocols definitions
 *
 */

#ifndef _NET_PROTOS_H
#define _NET_PROTOS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dvfilterlib.h"
#include <arpa/inet.h>

#define ETH_ADDR_LENGTH    6

typedef uint8_t Eth_Address[ETH_ADDR_LENGTH];

// Ethernet header type
typedef enum {
   ETH_HEADER_TYPE_DIX,
   ETH_HEADER_TYPE_802_1PQ,
   ETH_HEADER_TYPE_802_3,
   ETH_HEADER_TYPE_802_1PQ_802_3
} Eth_HdrType;

// DIX type fields we care about
typedef enum {
   ETH_TYPE_IPV4        = 0x0800,
   ETH_TYPE_IPV6        = 0x86DD,
   ETH_TYPE_ARP         = 0x0806,
   ETH_TYPE_RARP        = 0x8035,
   ETH_TYPE_802_1PQ     = 0x8100,  // not really a DIX type, but used as such
} Eth_DixType;
typedef enum {
   ETH_TYPE_IPV4_NBO    = 0x0008,
   ETH_TYPE_IPV6_NBO    = 0xDD86,
   ETH_TYPE_ARP_NBO     = 0x0608,
   ETH_TYPE_RARP_NBO    = 0x3580,
   ETH_TYPE_802_1PQ_NBO = 0x0081,  // not really a DIX type, but used as such
} Eth_DixTypeNBO;

typedef struct Eth_DIX {
   uint16_t  typeNBO;     // indicates the higher level protocol
}
__attribute__((__packed__))
Eth_DIX;

// XXX incomplete, but probably useless for us anyway...
typedef

struct Eth_LLC {
   uint8_t   dsap;
   uint8_t   ssap;
   uint8_t   control;
   uint8_t   snapOrg[3];          // would be more correct to have another struct
   Eth_DIX snapType;            // to manage the SNAP info ...
}
__attribute__((__packed__))
Eth_LLC;

typedef struct Eth_802_3 {
   uint16_t  lenNBO;      // length of the frame
   Eth_LLC llc;         // LLC header
}
__attribute__((__packed__))
Eth_802_3;

// 802.1p QOS/priority tags
enum {
   ETH_802_1_P_ROUTINE      = 0,
   ETH_802_1_P_PRIORITY     = 1,
   ETH_802_1_P_IMMEDIATE    = 2,
   ETH_802_1_P_FLASH        = 3,
   ETH_802_1_P_FLASH_OVR    = 4,
   ETH_802_1_P_CRITICAL     = 5,
   ETH_802_1_P_INTERNETCTL  = 6,
   ETH_802_1_P_NETCTL       = 7
};

typedef struct Eth_802_1pq_Tag {
   uint16_t typeNBO;            // always ETH_TYPE_802_1PQ
   uint16_t vidHi:4,            // 802.1q vlan ID high nibble
            canonical:1,        // bit order? (should always be 0)
            priority:3,         // 802.1p priority tag
            vidLo:8;            // 802.1q vlan ID low byte
}
__attribute__((__packed__))
Eth_802_1pq_Tag;

typedef struct Eth_802_1pq {
   Eth_802_1pq_Tag tag;       // VLAN/QOS tag
   union {
      Eth_DIX      dix;       // DIX header follows
      Eth_802_3    e802_3;    // or 802.3 header follows
   };
}
__attribute__((__packed__))
Eth_802_1pq;

typedef struct Eth_Header {
   Eth_Address     dst;       // all types of ethernet frame have dst first
   Eth_Address     src;       // and the src next (at least all the ones we'll see)
   union {
      Eth_DIX      dix;       // followed by a DIX header...
      Eth_802_3    e802_3;    // ...or an 802.3 header
      Eth_802_1pq  e802_1pq;  // ...or an 802.1[pq] tag and a header
   };
}
__attribute__((__packed__))
Eth_Header;

/*
 * header length macros
 *
 * first two are typical: ETH_HEADER_LEN_DIX, ETH_HEADER_LEN_802_1PQ
 * last two are suspicious, due to 802.3 incompleteness
 */

#define ETH_HEADER_LEN_DIX      (sizeof(Eth_Address) + \
                                 sizeof(Eth_Address) + \
                                 sizeof(Eth_DIX))
#define ETH_HEADER_LEN_802_1PQ  (sizeof(Eth_Address) + \
                                 sizeof(Eth_Address) + \
                                 sizeof(Eth_802_1pq_Tag) + \
                                 sizeof(Eth_DIX))
#define ETH_HEADER_LEN_802_3     (sizeof(Eth_Address) + \
                                 sizeof(Eth_Address) + \
                                 sizeof(Eth_802_3))
#define ETH_HEADER_LEN_802_1PQ_802_3 (sizeof(Eth_Address) + \
                                 sizeof(Eth_Address) + \
                                 sizeof(Eth_802_1pq_Tag) + \
                                 sizeof(Eth_802_3))

#define ETH_MIN_HEADER_LEN   (ETH_HEADER_LEN_DIX)
#define ETH_MAX_HEADER_LEN   (ETH_HEADER_LEN_802_1PQ_802_3)
/*
#ifndef ntohs
static inline uint16_t
ntohs(uint16_t in)
{
   return ((in >> 8) & 0x00ff) | ((in << 8) & 0xff00);
}
#endif

#ifndef htons
static inline uint16_t
   htons(uint16_t in)
{
   return ntohs(in);
}
#endif
*/
typedef struct IPHdr {
   uint8_t	ihl:4,
                version:4;
   uint8_t	tos;
   uint16_t	tot_len;
   uint16_t	id;
   uint16_t	frag_off;
   uint8_t	ttl;
   uint8_t	protocol;
   uint16_t	check;
   uint32_t	saddr;
   uint32_t	daddr;
} IPHdr;

typedef struct IPv6Hdr {
   uint8_t      trafficClass_0:4;
   uint8_t      version:4;
   uint8_t      flowLabel_0:4;
   uint8_t      trafficClass_1:4;
   uint16_t     flowLabel_1;
   uint16_t     payloadLen;
   uint8_t      nextHeader;
   uint8_t      hopLimit;
   uint8_t      saddr[16];
   uint8_t      daddr[16];
} IPv6Hdr;

typedef struct UDPHdr {
   uint16_t	source;
   uint16_t	dest;
   uint16_t	len;
   uint16_t	check;
} UDPHdr;

typedef struct TCPHdr {
   uint16_t	source;
   uint16_t	dest;
   uint32_t	seq;
   uint32_t	ack_seq;
   uint16_t	res1:4,
                doff:4,
                fin:1,
                syn:1,
                rst:1,
                psh:1,
                ack:1,
                urg:1,
                ece:1,
                cwr:1;
   uint16_t	window;
   uint16_t	check;
   uint16_t	urg_ptr;
} TCPHdr;



/******************************************************************************
 *                                                                       */ /**
 * \brief Get ethernet header type
 *
 * \returns Eth_HdrType value
 *
 ******************************************************************************
 */
static inline Eth_HdrType
EthHeaderType(Eth_Header *eh)
{
   uint16_t type = ntohs(eh->dix.typeNBO);

   /*
    * we use 1500 instead of some #def symbol to prevent
    * inadvertant reuse of the same macro for buffer size decls.
    */

   if (type > 1500) {
      if (type != ETH_TYPE_802_1PQ) {

         /*
          * typical case
          */

         return ETH_HEADER_TYPE_DIX;
      }

      /*
       * some type of 802.1pq tagged frame
       */

      type = ntohs(eh->e802_1pq.dix.typeNBO);

      if (type > 1500) {

         /*
          * vlan tagging with dix style type
          */

         return ETH_HEADER_TYPE_802_1PQ;
      }

      /*
       * vlan tagging with 802.3 header
       */

      return ETH_HEADER_TYPE_802_1PQ_802_3;
   }

   /*
    * assume 802.3
    */

   return ETH_HEADER_TYPE_802_3;
}

/******************************************************************************
 *                                                                       */ /**
 * \brief Get the encapsulated (layer 3) frame type
 *
 * \returns The frame type read in network byte order
 *
 ******************************************************************************
 */
static inline uint16_t
EthEncapsulatedPktType(Eth_Header *eh, uint32_t *offset)
{
   Eth_HdrType type = EthHeaderType(eh);

   switch (type) {

      case ETH_HEADER_TYPE_DIX :
	 *offset = ETH_HEADER_LEN_DIX;
	 return eh->dix.typeNBO;

      case ETH_HEADER_TYPE_802_1PQ :
	 *offset = ETH_HEADER_LEN_802_1PQ;
	 return eh->e802_1pq.dix.typeNBO;

      case ETH_HEADER_TYPE_802_3 :
	 *offset = ETH_HEADER_LEN_802_3;
	 return eh->e802_3.llc.snapType.typeNBO;

      case ETH_HEADER_TYPE_802_1PQ_802_3 :
	 *offset = ETH_HEADER_LEN_802_1PQ_802_3;
	 return eh->e802_1pq.e802_3.llc.snapType.typeNBO;
   }

   //VMK_ASSERT(VMK_FALSE);
   return 0;
}

/******************************************************************************
 *                                                                       */ /**
 * \brief Extract the VLAN ID from the vlanTag.
 *
 * \returns The VLAN ID.
 ******************************************************************************
 */

static inline uint16_t
EthVlanTagGetVlanID(const Eth_802_1pq_Tag *tag)
{
       return (tag->vidHi << 8) | tag->vidLo;
}

#ifdef __cplusplus
}
#endif

#endif // _NET_PROTOS_H
