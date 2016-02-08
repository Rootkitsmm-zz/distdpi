#ifndef NETX_SERVICE_H
#define NETX_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*PacketCbPtr)(void *, uint8_t*, uint32_t);

void stop_netx_service();

int start_netx_service(const char*, PacketCbPtr ptr, void *p);
#ifdef __cplusplus
}
#endif

#endif
