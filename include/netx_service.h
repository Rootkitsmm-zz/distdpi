#ifndef NETX_SERVICE_H
#define NETX_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PktMetadata {
   void *filterPtr;
   char *pktPtr;
   uint8_t dir;
} PktMetadata;

typedef void (*PacketCbPtr)(void *, PktMetadata *, uint32_t len);

void stop_netx_service();

int start_netx_service(const char*, PacketCbPtr ptr, void *p);
#ifdef __cplusplus
}
#endif

#endif
