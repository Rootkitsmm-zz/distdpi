#ifndef NETX_SERVICE_H
#define NETX_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*PacketCbPtr)(void *, uint8_t*, uint32_t);

int start_netx_service(char*, PacketCbPtr ptr, void *p);
#ifdef __cplusplus
}
#endif

#endif
