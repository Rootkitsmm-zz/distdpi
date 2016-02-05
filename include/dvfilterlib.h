/* **********************************************************
 * Copyright 2007 VMware, Inc.  All rights reserved. 
 * -- VMware Confidential
 * **********************************************************/

#ifndef _DVFILTERLIB_H_
#define _DVFILTERLIB_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef VMX86_SERVER
#define DVFILTER_EPOLL
#endif // VMX86_SERVER

#include <stdint.h> // uint8_t
#include <sys/types.h> // size_t
#ifdef DVFILTER_EPOLL
#include <sys/epoll.h> // size_t
#endif // DVFILTER_EPOLL

#define DVFILTER_SLOWPATH_REVISION_NUMBER_KL 0x00000001
#define DVFILTER_SLOWPATH_REVISION_NUMBER_KLNEXT 0x00000002
#define DVFILTER_SLOWPATH_REVISION_NUMBER_MN 0x00000003
#define DVFILTER_SLOWPATH_REVISION_NUMBER_MNNEXT 0x00000004
#define DVFILTER_SLOWPATH_REVISION_NUMBER DVFILTER_SLOWPATH_REVISION_NUMBER_MNNEXT

/*
 *
 * Data structures
 *
 */

typedef void *DVFilterSlowPathImpl;
typedef void *DVFilterImpl;
typedef void *DVFilterHandle;
typedef struct DVFilterMsgFrame PktHandle;

#define DVFILTERSTATUS_CODES \
   def(DVFILTER_STATUS_OK,             "Success") \
   def(DVFILTER_STATUS_TIMEDOUT,       "Timed out") \
   def(DVFILTER_STATUS_BADPARAM,       "Bad parameter") \
   def(DVFILTER_STATUS_NOT_FOUND,      "Not found") \
   def(DVFILTER_STATUS_BUF_TOO_SMALL,  "Buffer too small") \
   def(DVFILTER_STATUS_PROTOCOL_ERR,   "DVFilter Protocol error") \
   def(DVFILTER_STATUS_ALREADY_INIT,   "Already initialized") \
   def(DVFILTER_STATUS_DISCONNECT,     "Disconnected") \
   def(DVFILTER_STATUS_NOT_INITIALIZED,"Not initialized") \
   def(DVFILTER_STATUS_NOT_SUPPORTED,  "Not supported") \
   def(DVFILTER_STATUS_ERROR,          "Error") \

typedef enum DVFilterStatus {
#define def(name, descr) name,
   DVFILTERSTATUS_CODES
#undef def
} DVFilterStatus;

typedef enum DVFilterDirection {
   DVFILTER_FROM_SWITCH   = 1,
   DVFILTER_TO_SWITCH     = 2,
   DVFILTER_BIDIRECTIONAL = 3,
} DVFilterDirection;

#define DVFILTER_INVALID_IOCTL_REQUEST_ID 0
typedef uint32_t DVFilterIoctlRequestId;

#define DVFILTER_MAX_PARAMS 16
#define DVFILTER_MAX_CONFIGPARAM_LEN 256

#define DVFILTER_METADATA_MAX   2048

typedef enum DVFilterFailurePolicy {
   DVFILTER_FAIL_CLOSED,
   DVFILTER_FAIL_OPEN,
} DVFilterFailurePolicy;

typedef enum DVFilterEndPointType {
   DVFILTER_ENDPOINT_VNIC,
   DVFILTER_ENDPOINT_VMKNIC,
   DVFILTER_ENDPOINT_VSWIF,
   DVFILTER_ENDPOINT_UPLINK,
} DVFilterEndPointType;

#define DVFILTER_MAX_VC_UUID_LEN 128

/**
 * \defgroup DVFilterLib DVFilter slow path library
 * 
 *    DVFilter slow path library ...
 * \{ \}
 */

/**
 * \brief Capabilities the slowpath can advertise.
 */
typedef enum DVFilterSlowPathCapabilities {
   /**
    * \brief Checksum offload
    *
    * The slowpath does not need a correct checksum for IP frames.
    */
   DVFILTER_SLOWPATH_CAP_CSUM                = 0x0001,

   /**
    * \brief TCP segmentation offload
    *
    * The slowpath can handle 64k TCP packets
    */
   DVFILTER_SLOWPATH_CAP_TSO                 = 0x0002,

   /**
    * \brief VLAN offload
    *
    * The slowpath can handle VLAN tags stored outside the frame.
    */
   DVFILTER_SLOWPATH_CAP_VLAN                = 0x0004,
} DVFilterSlowPathCapabilities;

/*
 ***********************************************************************
 *                                                                */ /**
 * \ingroup DVFilterLib
 * \struct DVFilterSlowPathOps
 * \brief DVFilter driver callbacks
 *
 ***********************************************************************
 */
typedef struct DVFilterSlowPathOps {

   /********************************************************************
    *                                                             */ /**
    * \brief Instantiate a filter
    *
    *    This callback is invoked to instanciate a filter and attach
    *    it to the fast path agent.
    *
    * \param[in]  filter Handle to the dvfilterlib's filter object
    * \param[out] privateData Opaque pointer to the driver filter's object
    *
    * \retval DVFILTER_STATUS_OK Success. 
    * \retval Other The error is propagated to the VMkernel and
    *               the initiating fast-path agent is notified
    *               about the failure to associate the slow-path.
    *
    ********************************************************************
    */
   DVFilterStatus (*createFilter)(
      DVFilterHandle filter,
      DVFilterImpl *privateData);

   /********************************************************************
    *                                                             */ /**
    * \brief Destroy a filter
    *
    *    This callback is typically invoked when the VM is destroyed,
    *    after the port has been disabled
    *
    * \param[in]  filter Opaque pointer to the driver's filter object
    *
    * \retval DVFILTER_STATUS_OK Success. No other return value is allowed
    ********************************************************************
    */
   DVFilterStatus (*destroyFilter)(
      DVFilterImpl filter);

   /********************************************************************
    *                                                             */ /**
    * \brief Query the filter for the size of the saved state buffer
    *
    *    This callback is always invoked just before calling
    *    saveState()
    *
    * \param[in]  filter Opaque pointer to the driver's filter object
    * \param[out] len Minimum length (in bytes) required to save the
    *    filter's state.
    *
    * \retval DVFILTER_STATUS_OK Success
    * \retval Other Subsequent saveState() callback is not invoked
    *               and fast-path agent is informed about the failure
    *               to retrieve slow-path state.
    *
    ********************************************************************
    */
   DVFilterStatus (*getSavedStateLen)(
      DVFilterImpl filter,
      uint32_t *len);

   /********************************************************************
    *                                                             */ /**
    * \brief Saves the state of the filter
    *
    *    This callback is always invoked after getSavedStateLen()
    *
    *    If the returned length from getSavedStateLen() is zero,
    *    then when saveState() is called, the value of buf is NULL.
    *
    * \param[in]  filter Opaque pointer to the driver's filter object
    * \param[in]  buf Buffer to save the state
    * \param[in,out] len On entry, size of the supplied buffer.
    *    On exit, size of the data actually populated in the buffer.
    *
    * \retval DVFILTER_STATUS_OK Success
    * \retval Other The fast-path agent is informed about the 
    *               failure to retrieve slow-path state.
    *
    ********************************************************************
    */
   DVFilterStatus (*saveState)(
      DVFilterImpl filter,
      void *buf,
      uint32_t *len);

   /********************************************************************
    *                                                             */ /**
    * \brief Restores the state of the filter
    *
    *    Invoked during a VMotion  to allow the service appliance
    *    to restore state.
    *
    *    Its possible for the vmk_DVFilterRestoreSlowPathState() to
    *    send a zero length restore request.  If the length of the
    *    request is zero, then the value of buf is NULL.
    *
    * \param[in]  filter Opaque pointer to the driver's filter object
    * \param[in]  buf Buffer to the saved state
    * \param[in]  len Size of the supplied buffer.
    *
    * \retval DVFILTER_STATUS_OK Success
    * \retval Other The fast-path agent is informed about the 
    *               failure to restore slow-path state.

    ********************************************************************
    */
   DVFilterStatus (*restoreState)(
      DVFilterImpl filter,
      void *buf,
      uint32_t len);

   /********************************************************************
    *                                                             */ /**
    * \brief Packet callback (inbound or outbound)
    *
    *    Each packet in the packet list must ultimately be sent or
    *    freed using DVFilter_SendPkt() or DVFilter_FreePkt()
    *
    * \param[in]  filter Opaque pointer to the driver filter's object
    * \param[in]  pkt Packet to process.
    * \param[in]  direction The direction of the packet
    *
    ********************************************************************
    */
   void (*processPacket)(
      DVFilterImpl filter,
      PktHandle *pkt,
      DVFilterDirection direction);

   /********************************************************************
    *                                                             */ /**
    * \brief Process an ioctl request for a particular filter 
    *
    *    This callback is invoked when the fast path agent in the VMkernel 
    *    sends an ioctl request associated with a particular filter. 
    *
    *    Use this type of ioctl for communication between the slow 
    *    path and fast path agents when a particular filter is 
    *    concerned.
    *
    *    If the vmk_DVFilterSendFilterIoctlRequest() function is
    *    called with a zero length payload, the payload buffer
    *    pointer passed to filterIoctlRequest() is NULL.
    *
    * \param[in]  filter Opaque pointer to the driver filter's object
    * \param[in]  requestId Identifier of the request used as a parameter
    *    to DVFilterSendIoctlReply() to pair the reply with the request.
    *    DVFILTER_INVALID_IOCTL_REQUEST_ID if the caller does not
    *    expect a reply.
    * \param[in]  payload The payload of the ioctl request
    * \param[in]  payloadLen Size of the payload
    *
    ********************************************************************
    */
   void (*filterIoctlRequest)(
      DVFilterImpl filter,
      DVFilterIoctlRequestId requestId,
      void *payload,
      size_t payloadLen);
   
   /********************************************************************
    *                                                             */ /**
    * \brief Process a slow path ioctl request 
    *
    *    This callback is invoked when the fast path agent in the VMkernel 
    *    sends an ioctl request. In contrast to filterIoctlRequest(), the 
    *    ioctl operation is not specific to a particular filter.
    *
    *    Use this type of ioctl for general communication between the
    *    slow path and fast path agents, i.e. where no particular 
    *    filter is concerned.
    *
    *    If the vmk_DVFilterSendSlowPathIoctlRequest() function is
    *    called with a zero length payload, the payload buffer
    *    pointer passed to slowPathIoctlRequest() is NULL.
    *
    * \param[in]  requestId Identifier of the request used as a parameter
    *    to DVFilterSendIoctlReply() to pair the reply with the request.
    *    DVFILTER_INVALID_IOCTL_REQUEST_ID if the caller does not
    *    expect a reply.
    * \param[in]  payload The payload of the ioctl request
    * \param[in]  payloadLen Size of the payload
    *
    ********************************************************************
    */
   void (*slowPathIoctlRequest)(
      DVFilterIoctlRequestId requestId,
      void *payload,
      size_t payloadLen);

   /********************************************************************
    *                                                             */ /**
    * \brief Handle a filter configuration change
    *
    *    This callback is invoked when the filter's parameters, VC
    *    UUID, failure policy, or vNic index change. In the future
    *    it may be invoked on other configuration changes.
    *
    * \param[in]  filter Opaque pointer to the driver filter's object
    *
    ********************************************************************
    */
   void (*configurationChanged)(DVFilterImpl filter);

} DVFilterSlowPathOps;

/*
 ***********************************************************************
 *                                                                */ /**
 * \ingroup DVFilterLib
 * \brief  DVFilterFilterIoctlReplyCbk
 *
 *    Callback function type for DVFilter_SendFilterIoctlRequest()
 *
 *    If the vmk_DVFilterSendFilterIoctlReply() function is
 *    called with a zero dataLen, then DVFilterFilterIoctlReplyCbk()
 *    is called with a NULL payload poiner.
 *
 * \param[in]  filter Handle to the dvfilterlib's filter object
 * \param[in]  cookie Opaque identifier of the request originally passed
 *    to DVFilter_SendFilterIoctlRequest()
 * \param[in]  data The ioctl's payload
 * \param[in]  dataLen Length of the payload in bytes
 * \param[in]  status DVFILTER_STATUS_OK if the reply is valid. Error
 *    code otherwise.
 *
 ***********************************************************************
 */
typedef void (*DVFilterFilterIoctlReplyCbk)(
   DVFilterImpl filter,
   void *replyCbkData,
   void *payload,
   size_t payloadLen,
   DVFilterStatus status);

/*
 ***********************************************************************
 *                                                                */ /**
 * \ingroup DVFilterLib
 * \brief  DVFilterSlowPathIoctlReplyCbk
 *
 *    Callback function type for DVFilter_SendSlowPathIoctlRequest()
 *
 *    If the vmk_DVFilterSendSlowPathIoctlReply() function is
 *    called with a zero dataLen, then DVFilterSlowPathIoctlReplyCbk()
 *    is called with a NULL payload pointer.
 *
 * \param[in]  cookie Opaque identifier of the request originally passed
 *    to DVFilter_SendSlowPathIoctlRequest()
 * \param[in]  data The ioctl's payload
 * \param[in]  dataLen Length of the payload in bytes
 * \param[in]  status DVFILTER_STATUS_OK if the reply is valid. Error
 *    code otherwise.
 *
 ***********************************************************************
 */
typedef void (*DVFilterSlowPathIoctlReplyCbk)(
   void *replyCbkData,
   void *payload,
   size_t payloadLen,
   DVFilterStatus status);

/*
 *
 * Functions
 *
 */

/*
 ***********************************************************************
 *                                                                */ /**
 * \ingroup DVFilterLib
 * \brief Initialize the DVFilter library and connect to the VMkernel
 *        agent
 *
 * \param[in]  serviceName Name registered by the VMkernel driver
 * \param[in]  serviceVersion Version number of the service. This
 *    field is for information purposes only.
 * \param[in]  ops Slow path callbacks
 * \param[in]  capabilities Slow path capabilities
 * \param[out]  acceptedCapabilities Capabilities accepted by the VMkernel
 *
 * \retval DVFILTER_STATUS_OK successful initialization
 * \retval DVFILTER_STATUS_BADPARAM Invalid Parameter passed;
 *              service name null, ops null, one
 *              of the callback targets is null, or
 *              the capabilities exceed those currently
 *              supported by dvfilterlib.
 * \retval DVFILTER_STATUS_PROTOCOL_ERR An error protocol occurred 
 *              in the communication to the VMkernel endpoint.
 *              Could be caused by resource constraints on the VMkernel
 *              side.
 * \retval DVFILTER_STATUS_ALREADY_INIT DVFilterLib is currently connected.
 * \retval DVFILTER_STATUS_ERROR An error occured. Its code is stored in errno.
 *
 ***********************************************************************
 */
DVFilterStatus DVFilter_Init(
   const char *serviceName,
   uint32_t serviceVersion,
   DVFilterSlowPathOps *ops,
   uint32_t capabilities,
   uint32_t *acceptedCapabilities);

/*
 ***********************************************************************
 *                                                                */ /**
 * \ingroup DVFilterLib
 * \brief Release all resources allocated to the DVFilter library
 *
 ***********************************************************************
 */
void DVFilter_Cleanup(void);

/*
 ***********************************************************************
 *                                                                */ /**
 * \ingroup DVFilterLib
 * \brief Dispatch pending DVFilter events from select result
 *
 *
 * \param[in] readSet The read fd set after select() is done
 * \param[in] writeSet The write fd set after select() is done
 * \param[in] maxFd The highest fd over both sets
 * \param[in] selectNum The return value of select().
 *
 * \retval DVFILTER_STATUS_OK Success
 * \retval DVFILTER_STATUS_DISCONNECT Either control or data channel
 *         were closed. DVFilter_Cleanup() should be called next.
 ***********************************************************************
 */
DVFilterStatus DVFilter_ProcessSelectEvent(
   fd_set *readSet, 
   fd_set *writeSet, 
   int maxFd, 
   int selectNum);

/*
 ***********************************************************************
 *                                                                */ /**
 * \ingroup DVFilterLib
 * \brief Collect an fdset of fd's which are ready to read
 *
 *    Can be used to include the DVFilter control channel into a 
 *    custom select() loop.
 *
 * \param[in] fdset The fd set to which the DVFilterLib's fds are added
 * \param[in] maxFd The currently highest fd value in the fd set
 *
 * \return The now highest fd value in the fd set
 *
 ***********************************************************************
 */
int DVFilter_AddReadyReadFdset(
   fd_set *fdset, 
   int maxFd);

/*
 ***********************************************************************
 *                                                                */ /**
 * \ingroup DVFilterLib
 * \brief  Collect an fdset of fd's which are ready to write
 *
 *    Can be used to include the DVFilter control channel into a 
 *    custom select() loop.
 *
 * \param[in] fdset The fd set to which the DVFilterLib's fds are added
 * \param[in] maxFd The currently highest fd value in the fd set
 *
 * \return The now highest fd value in the fd set
 *
 ***********************************************************************
 */
int DVFilter_AddReadyWriteFdset(
   fd_set *fdset, 
   int maxFd); 

#ifdef DVFILTER_EPOLL
/*
 ***********************************************************************
 *                                                                */ /**
 * \ingroup DVFilterLib
 * \brief  process a single epoll event
 *
 *    If the function is passed a NULL as the epoll event,
 *    timeout values are updated, and zero is returned.
 *
 * \param[in] event A single epoll_event written by epoll_wait()
 *
 * \retval DVFILTER_STATUS_OK Success
 * \retval DVFILTER_STATUS_DISCONNECT Either control or data channel
 *         were closed. DVFilter_Cleanup() should be called next.
 ***********************************************************************
 */
DVFilterStatus
DVFilter_EpollProcessEvent(struct epoll_event *event);

/*
 ***********************************************************************
 *                                                                */ /**
 * \ingroup DVFilterLib
 * \brief  Attach or detach an epoll fd to DVFilterLib
 *
 *    Allows DVFilterLib to associate with an epoll fd and manage
 *    epoll settings for the fd's it owns.
 *    
 * \param[in] epollFd An epoll fd. Use 0 to detach.
 *
 * \retval DVFILTER_STATUS_OK Success
 * \retval DVFILTER_STATUS_ERROR An error occured. errno contains more 
 *              information
 *
 ***********************************************************************
 */
DVFilterStatus
DVFilter_EpollAttach(int epollFd);

#endif // DVFILTER_EPOLL

/*
 ***********************************************************************
 *                                                                */ /**
 * \ingroup DVFilterLib
 * \brief  return a timeout, which is the min of the msec paramter
 *      passed in, and dvfilterlib's timeout for events
 *
 *    XXX: What is the typical use case of this function
 *
 * \return Smaller value of DVFilterLib's timeout for events or msec param
 ***********************************************************************
 */
int
DVFilter_GetTimeoutMS(int msec);

/*
 ***********************************************************************
 *                                                                */ /**
 * \ingroup DVFilterLib
 * \brief Return the length of the packet frame
 *
 * \param[in]  pkt The packet
 *
 * \return Packet frame's length
 *
 ***********************************************************************
 */
uint32_t DVFilter_GetPktLen(
   PktHandle *pkt);

/*
 ***********************************************************************
 *                                                                */ /**
 * \ingroup DVFilterLib
 * \brief Set the length of the packet frame
 *
 *     The packet frame length is set to the provided value. If the
 *     packet size is increasing, a packet copy might be performed. If 
 *     the packet is getting smaller, no packet copy is performed.
 *
 *     Note that all pointers related to the packet (e.g. pointers
 *     to metadata or payload) might become invalid and therefore
 *     need to be refreshed by calling the respective API.
 *
 * \param[in, out]  pkt The packet. Might be changed if packet is copied.
 * \param[in]  len The frame's length
 *
 * \retval DVFILTER_STATUS_OK The length was set successfully
 * \retval Other An error occured
 *
 ***********************************************************************
 */
DVFilterStatus DVFilter_SetPktLen(
   PktHandle **pkt,
   uint32_t len);

/*
 ***********************************************************************
 *                                                                */ /**
 * \ingroup DVFilterLib
 * \brief Set the length of the packet metadata
 *
 *     The packet metadata length is set to the provided value. A full
 *     packet copy will be performed. Note that all pointers related 
 *     to the packet (e.g. pointers to metadata or payload) will become 
 *     invalid and therefore need to be refreshed by calling the 
 *     respective API.
 *
 *     The maximum meta data length supported is DVFILTER_METADATA_MAX.
 *
 * \param[in, out]  pkt The packet. Might be changed if packet is copied.
 * \param[in]  len The metadata's length
 *
 * \retval DVFILTER_STATUS_OK The length was set successfully
 * \retval Other An error occured
 *
 ***********************************************************************
 */
DVFilterStatus DVFilter_SetPktMetadataLen(
   PktHandle **pkt,
   uint32_t len);


/*
 ***********************************************************************
 *                                                                */ /**
 * \ingroup DVFilterLib
 * \brief Return the number of frags (segments) for this pkt
 *
 * \param[in]  pkt The packet
 *
 * \return Number of frags (segments).
 *
 ***********************************************************************
 */
uint32_t
DVFilter_GetPktFragCount(PktHandle *pkt);


/*
 ***********************************************************************
 *                                                                */ /**
 * \ingroup DVFilterLib
 * \brief Return details about a particuler fragment,
 *
 * \param[in]  pkt The packet
 * \param[in]  ndx Identifies which fragment
 * \param[out] address Pointer to the location where to place the address
 *      of the identified fragment.  If this value is NULL, the address
 *      associated with the ndx is not set.
 * \param[out] length Pointer to the location where to place the length
 *      of the identified fragment.  If this value is NULL, the length
 *      associated with the ndx is not set.
 *
 * \retval DVFILTER_STATUS_OK Success
 * \retval DVFILTER_STATUS_ERROR ndx parameter out of bounds
 *
 ***********************************************************************
 */
DVFilterStatus
DVFilter_GetPktFragAddressAndLength(PktHandle *pkt,
                                    uint32_t ndx,
                                    uint8_t **address,
                                    uint32_t *length);

/*
 ***********************************************************************
 *                                                                */ /**
 * \ingroup DVFilterLib
 * \brief Return a reference to contiguous data within a pkt
 *
 *      A reference to a region of contiguous data with a pkt
 *      is returned.  If the reguest reqion described by offset
 *      and length are not contiguous within a single frag,
 *      then NULL is returned. When a contiguous regions
 *      is found, the available configuous bytes are returned
 *      through the lengthLeft parameer.  When lengthLeft is
 *      passed as NULL, this parameter is not set.
 *
 * \param[in]  pkt The packet
 * \param[in]  offset Offset within the pkt describing the location of
 *      the value to return
 * \param[in]  length Length of bytes requested within the pkt starting
 *      at the offset parameter
 * \param[out] lengthLeft Pointer to the a location, describing the total
 *      number of bytes available starting with the returned location.
 *      This value will be at least as large as the length parameter
 *      passed into the procedure.  If the value passed in for the
 *      lengthLeft parameter is NULL, this value will not be set.
 *      
 * \return Pointer within a fragment identified by the offset, where
 *      'length' bytes can be referenced.
 *
 ***********************************************************************
 */
uint8_t *
DVFilter_GetPktFragDirectRange(PktHandle *pkt,
                               uint32_t offset,
                               uint32_t length,
                               uint32_t *lengthLeft);
/*
 ***********************************************************************
 *                                                                */ /**
 * \ingroup DVFilterLib
 * \brief Return a pointer to the packet's metadata. 
 *
 *      A reference to the meta data associated with the pkt is
 *      returned.  When no meta data length has been associated
 *      with the pkt, or if the current meta data length
 *      is zero a NULL is returned.
 *
 * \param[in]  pkt The packet
 *
 * \return Pointer to the packet's metadata, or NULL.
 *
 ***********************************************************************
 */
uint8_t *
DVFilter_GetPktMetadata(PktHandle * pkt);

/*
 ***********************************************************************
 *                                                                */ /**
 * \ingroup DVFilterLib
 * \brief Return the length of the packet metadata
 *
 * \param[in]  pkt The packet
 *
 * \return Packet metadata's length
 *
 ***********************************************************************
 */
uint32_t
DVFilter_GetPktMetadataLen(PktHandle * pkt);


/*
 ***********************************************************************
 *                                                                */ /**
 * \ingroup DVFilterLib
 * \brief Query if the packet has checksum offloading enabled
 *
 *    If the packet has checksum offloading enabled, the TCP checksum
 *    contained in the packet is expected to be the pseudo checksum.
 *    The final checksum of the packet will be computed in hardware
 *    or in the VMkernel.
 *
 * \param[in]  pkt The packet
 *
 * \retval 1 Packet is still to be checksummed
 * \retval 0 No checksum offloading enabled for this packet
 *
 ***********************************************************************
 */
int
DVFilter_IsMustCsum(PktHandle * pkt);

/*
 ***********************************************************************
 *                                                                */ /**
 * \ingroup DVFilterLib
 * \brief Mark the pkt as requiring csum offload.
 *
 *    Csum offload processing is enabled for this pkt.
 *
 *    If the packet has checksum offloading enabled, the TCP checksum
 *    contained in the packet is expected to be the pseudo checksum.
 *    The final checksum of the packet will be computed in hardware
 *    or in the VMkernel.
 *
 * \param[in]  pkt The packet
 *
 *
 ***********************************************************************
 */
void
DVFilter_SetMustCsum(PktHandle * pkt);

/*
 ***********************************************************************
 *                                                                */ /**
 * \ingroup DVFilterLib
 * \brief Clear the csum offload request associated with the pkt.
 *
 *    Csum offload processing is disabled for this pkt.
 *
 * \param[in]  pkt The packet
 *
 ***********************************************************************
 */
void
DVFilter_ClearMustCsum(PktHandle * pkt);

/*
 ***********************************************************************
 *                                                                */ /**
 * \ingroup DVFilterLib
 * \brief Query if the packet has TCP Segmentation Offloading enabled
 *
 *    If the packet has TCP segmentation offloading enabled, the packet
 *    is larger than the MTU would allow. The segmentation will be 
 *    performed in hardware or in the VMkernel.
 *
 *    TCP Segmentation offloading implies checksum offloading.
 *
 *    Note: The current implementation of the fast path will always
 *    perform software TCP segmentation before passing packets to the 
 *    slow path. Thus, this function will always return 0. This will 
 *    likely change in future versions for performance enhancements.
 *
 * \param[in]  pkt The packet
 *
 * \retval 1 Packet is a TSO packet
 * \retval 0 Regular non-TSO packet
 *
 ***********************************************************************
 */
int
DVFilter_IsMustTso(PktHandle * pkt);

/*
 ***********************************************************************
 *                                                                */ /**
 * \ingroup DVFilterLib
 * \brief Mark the pkt as requiring TSO offload.
 *
 *    Mark the packet as requiring TCP segmentation offload.
 *    The pkt ought to have a frame length larger than the MTU
 *    of the associated interfaces associated with the frame.
 *
 *    TCP Segmentation offloading implies checksum offloading.
 *    The TCP header of the offloaded pkt must have a correct
 *    TSO pseudo csum.  This csum is different than csum offload
 *    pkts, since the pseudo csum includes the value of the IP length field.  
 *    Since TSO creates new frames of tsoMss segment length,
 *    the length of the IP length fields is NOT included in the
 *    TSO pseudo csum.  As TSO offload processes each of the
 *    frames, the IP length value is added to the TCP
 *    checksum field as part of segment generation.
 *
 *
 * \param[in]  pkt The packet
 * \param[in]  tsoMss TSO segment size
 *
 ***********************************************************************
 */
void
DVFilter_SetMustTso(PktHandle *pkt, uint32_t tsoMss);

/*
 ***********************************************************************
 *                                                                */ /**
 * \ingroup DVFilterLib
 * \brief Disable TSO offload on the pkt.
 *
 *    Clear TSO offload of the indicated packet, irrespective
 *    of the current value of the TSO offload indication.
 *
 *
 * \param[in]  pkt The packet
 *
 * \retval 1 Packet is a TSO packet
 * \retval 0 Regular non-TSO packet
 *
 ***********************************************************************
 */
void
DVFilter_ClearMustTso(PktHandle *pkt);

/*
 ***********************************************************************
 *                                                                */ /**
 * \ingroup DVFilterLib
 * \brief Returns the maximum segment size used for TSO
 *
 *    The maximum segment size of a TSO packet is the maximum size 
 *    of the TCP payload per packet after segmentation.
 *
 *    This function only returns a meaningful value if the packet
 *    has TCP Segmentation offloading enabled.
 *
 * \param[in]  pkt The packet
 *
 * \retval Maximum segment size. (Undefined if packet is not a TSO packet)
 *
 ***********************************************************************
 */
uint32_t
DVFilter_GetTsoMss(PktHandle * pkt);


/*
 ***********************************************************************
 *                                                                */ /**
 * \ingroup DVFilterLib
 * \brief Free a packet alloacted by DVFilterAllocPkt()
 *
 * \param[in]  pkt The packet to free
 *
 ***********************************************************************
 */
void DVFilter_FreePkt(
   PktHandle *pkt);

/*
 ***********************************************************************
 *                                                                */ /**
 * \ingroup DVFilterLib
 * \brief Allocate a packet with the specified frame length
 *
 *   The newly allocated packet has MustTso and MustCsum set to off.
 * 
 * \param[in] len Desired length of the packet
 *
 * \return Pointer to the new PktHandle
 ***********************************************************************
 */
PktHandle *DVFilter_AllocPkt(
   uint32_t len);

/*
 ***********************************************************************
 *                                                                */ /**
 * \ingroup DVFilterLib
 * \brief Inject a packet into the specified filter
 *
 * \param[in]  filter Filter to inject the packet on
 * \param[in]  pkt The packet to inject
 * \param[in]  direction DVFILTER_TO_SWITCH or DVFILTER_FROM_SWITCH
 *
 * \retval DVFILTER_STATUS_OK The packet was queued successfully
 * \retval Other An error occured
 *
 ***********************************************************************
 */
DVFilterStatus DVFilter_SendPkt(
   DVFilterHandle filter,
   PktHandle *pkt,
   DVFilterDirection direction);

/*
 ***********************************************************************
 *                                                                */ /**
 * \ingroup DVFilterLib
 * \brief Issue an ioctl concerning a particular filter to the fast path agent
 *
 *    Use this type of ioctl for communication between the slow 
 *    path and fast path agents when a particular filter is 
 *    concerned.
 *
 *    It is acceptable for the payloadLen value to zero.
 *    When the payloadLen value is zero, the payload pointer is
 *    never dereferenced.
 *
 * \param[in]  filter Filter to send the ioctl on
 * \param[in]  payload Buffer containing the data to send
 * \param[in]  payloadLen Length of the payload in bytes
 * \param[in]  reply Callback to invoke upon reception of the
 *    reply, disconnection or on timeout.
 * \param[in]  replyCbkData Parameter to reply callback
 * \param[in]  timeoutMs Timeout in milliseconds
 *
 * \retval DVFILTER_STATUS_OK The request was queued successfully
 * \retval DVFILTER_STATUS_BADPARAM request NULL but requestLen is not 0
 * \retval DVFILTER_STATUS_PROTOCOL_ERR Ioctl was too long to be accepted by VMkernel
 * \retval DVFILTER_STATUS_ERROR An error occured. errno contains more information
 *
 ***********************************************************************
 */
DVFilterStatus DVFilter_SendFilterIoctlRequest(
   DVFilterHandle filter,
   void *payload,
   size_t payloadLen,
   DVFilterFilterIoctlReplyCbk reply,
   void *replyCbkData,
   uint32_t timeoutMs);

/*
 ***********************************************************************
 *                                                                */ /**
 * \ingroup DVFilterLib
 * \brief Issue an ioctl to the fast path agent
 *
 *    Use this type of ioctl for general communication between the
 *    slow path and fast path agents, i.e. where no particular 
 *    filter is concerned.
 *
 *    It is acceptable for the payloadLen value to be zero.
 *    When the payloadLen is zero, the payload point is never
 *    dereferenced.
 *
 * \param[in]  payload Buffer containing the data to send
 * \param[in]  payloadLen Length of the payload in bytes
 * \param[in]  reply Callback to invoke upon reception of the
 *    reply, disconnection or on timeout.
 * \param[in]  replyCbkData Parameter to reply callback
 * \param[in]  timeoutMs Timeout in milliseconds
 *
 * \retval DVFILTER_STATUS_OK The request was queued successfully
 * \retval DVFILTER_STATUS_BADPARAM request NULL but requestLen is not 0
 * \retval DVFILTER_STATUS_PROTOCOL_ERR Ioctl was too long to be accepted by VMkernel
 * \retval DVFILTER_STATUS_ERROR An error occured. errno contains more information
 *
 ***********************************************************************
 */
DVFilterStatus DVFilter_SendSlowPathIoctlRequest(
   void *payload,
   size_t payloadLen,
   DVFilterSlowPathIoctlReplyCbk reply,
   void *replyCbkData,
   uint32_t timeoutMs);

/*
 ***********************************************************************
 *                                                                */ /**
 * \ingroup DVFilterLib
 * \brief Respond to an ioctl request from the VMkernel driver
 *
 *      Its acceptable for the dataLen of the response to be zero.
 *      When the dataLen is zero, the data pointer is never
 *      dereferenced.
 *
 * \param[in]  filter Filter to send the reply to. Must be the same
 *    filter that the request came from.
 * \param[in]  requestId Identifier of the original request received
 *    by the DVFilterSlowPathOps::ioctlRequest() callback.
 * \param[in]  data Buffer containing the reply
 * \param[in]  dataLen Length of the payload in bytes
 *
 * \retval DVFILTER_STATUS_OK The reply was queued successfully
 * \retval DVFILTER_STATUS_BADPARAM data NULL but dataLen is not 0
 * \retval DVFILTER_STATUS_PROTOCOL_ERR Ioctl was too long to be accepted by VMkernel
 * \retval DVFILTER_STATUS_ERROR An error occured. errno contains more information
 *
 ***********************************************************************
 */
DVFilterStatus DVFilter_SendFilterIoctlReply(
   DVFilterHandle filter,
   DVFilterIoctlRequestId requestId,
   void *data,
   size_t dataLen);
/*
 ***********************************************************************
 *                                                                */ /**
 * \ingroup DVFilterLib
 * \brief Query the human readable name of the DVFilter
 *
 * \param[in]  filter Filter from which the name should be read
 * \param[in]  name Pointer to a buffer to fill in with the filter name
 * \param[in]  len Length of the supplied buffer
 *
 * \retval DVFILTER_STATUS_OK The filter name was read successfully
 * \retval DVFILTER_STATUS_BUF_TOO_SMALL Buffer was too small
 *
 ***********************************************************************
 */
DVFilterStatus DVFilter_GetName(
   DVFilterHandle filter,
   char *name,
   size_t len);

/*
 ***********************************************************************
 *                                                                */ /**
 * \ingroup DVFilterLib
 * \brief Get the failure policy the user configured for this filter
 *
 * \param[in]  filter Handle to the filter
 * \param[out] policy Pointer of the DVFilterFailurePolicy to fill in
 *
 * \retval DVFILTER_STATUS_OK Success
 * \retval DVFILTER_STATUS_BADPARAM policy is NULL
 *
 ***********************************************************************
 */
DVFilterStatus DVFilter_GetFailurePolicy(
   DVFilterHandle filter,
   DVFilterFailurePolicy *policy);

/*
 ***********************************************************************
 *                                                                */ /**
 * \ingroup DVFilterLib
 * \brief Get the ID of a given filter
 *
 *    Can be used to match a user space (i.e. dvfilterlib) filter 
 *    against a kernel space (i.e. dvfilterklib) filter.
 *
 * \param[in]  filter Handle to the filter
 *
 * \return Filter ID
 *
 ***********************************************************************
 */
uint32_t DVFilter_GetFilterID(
   DVFilterHandle filter);

/*
 ***********************************************************************
 *                                                                */ /**
 * \ingroup DVFilterLib
 * \brief Respond to an ioctl request from the VMkernel driver
 *
 *      Its acceptable for the dataLen of the reponse to be
 *      zero length.  If the dataLen is zero length, then
 *      the data pointer is never dereferenced.
 *
 * \param[in]  requestId Identifier of the original request received
 *    by the DVFilterSlowPathOps::ioctlRequest() callback.
 * \param[in]  data Buffer containing the reply
 * \param[in]  dataLen Length of the payload in bytes
 *
 * \retval DVFILTER_STATUS_OK The reply was queued successfully
 * \retval DVFILTER_STATUS_BADPARAM data NULL but dataLen is not 0
 * \retval DVFILTER_STATUS_PROTOCOL_ERR Ioctl was too long to be accepted by VMkernel
 * \retval DVFILTER_STATUS_ERROR An error occured. errno contains more information
 *
 ***********************************************************************
 */
DVFilterStatus DVFilter_SendSlowPathIoctlReply(
   DVFilterIoctlRequestId requestId,
   void *data,
   size_t dataLen);

/*
 ***********************************************************************
 *                                                                */ /**
 * \ingroup DVFilterLib
 * \brief Get the value of a filter config parameter
 *
 * \param[in]  filter Filter from which the config parameter should be read
 * \param[in]  paramIndex The index of the config parameter
 * \param[in]  param Pointer to a buffer to fill in with the requested
 *                   filter configuration parameter value. 
 * \param[in]  len Length of the supplied buffer
 *
 * \retval DVFILTER_STATUS_OK The config parameter was read successfully
 * \retval DVFILTER_STATUS_BADPARAM paramIndex out of bounds
 * \retval DVFILTER_STATUS_BUF_TOO_SMALL Buffer was too small
 * \retval DVFILTER_STATUS_NOT_FOUND This parameter is not set
 *
 ***********************************************************************
 */
DVFilterStatus DVFilter_GetConfigParameter(
   DVFilterHandle filter,
   uint8_t paramIndex,
   char *param,
   size_t len);

/*
 ***********************************************************************
 *
 *  \ingroup DVFilterLib
 *  \brief Query the VC UUID of the VM the DVFilter is attached to
 *
 * \param[in]  filter Handle to the filter object
 * \param[out] uuid   Pointer to a buffer to fill in with the UUID
 * \param[in]  len    Length of the supplied buffer
 *
 * \retval DVFILTER_STATUS_NOT_INITIALIZED No VC UUID has been set for this VM
 * \retval DVFILTER_STATUS_BUF_TOO_SMALL Supplied buffer is too small
 * \retval DVFILTER_STATUS_NOT_SUPPORTED Filter does not have a VM endpoint
 * \retval DVFILTER_STATUS_BADPARAM uuid is NULL
 * \retval DVFILTER_STATUS_OK Success
 *
 ***********************************************************************
 */
DVFilterStatus DVFilter_GetVcUuid(
   DVFilterHandle filter,
   char *uuid,
   size_t len);

/*
 ***********************************************************************
 *  \ingroup DVFilterLib
 *  \brief Get the type of the filter's end-point
 *
 *     The filter end-point might be a virtual nic of a VM, a VMkernel
 *     NIC (vmknic) or an uplink.
 *
 *     Note: This function is for future extension. Right now DVFilter
 *     only supports VM vNics.
 *
 * \param[in]  filter Handle to the filter object
 * \param[out] type Pointer of the DVFilterEndPointType to fill in
 *
 * \retval DVFILTER_STATUS_OK Success
 * \retval DVFILTER_STATUS_BADPARAM Type is NULL
 *
 ***********************************************************************
 */
DVFilterStatus DVFilter_GetEndPointType(
   DVFilterHandle filter,
   DVFilterEndPointType *type);


/*
 ***********************************************************************
 *  \ingroup DVFilterLib
 *  \brief Get the index of the vNic this filter is attached to.
 *
 *     This function is only supported for VM endpoints.
 *
 * \param[in]  filter Handle to the VMkernel filter object
 * \param[out] index Pointer of the uint32 to fill in
 *
 * \retval DVFILTER_STATUS_OK Success
 * \retval DVFILTER_STATUS_NOT_SUPPORTED Filter does not have a VM endpoint
 * \retval DVFILTER_STATUS_BADPARAM index is NULL
 *
 ***********************************************************************
 */
DVFilterStatus DVFilter_GetVnicIndex(
   DVFilterHandle filter,
   uint32_t *index);

/*
 ***********************************************************************
 *                                                                */ /**
 * \ingroup DVFilterLib
 * \brief Get a string representation of a DVFilter status code
 *
 * \param[in]  status A DVFilter status code
 *
 * \return A string representing the status code
 *
 ***********************************************************************
 */
const char *
DVFilter_StatusToString(DVFilterStatus status);

#ifdef __cplusplus
}
#endif

#endif // _DVFILTERLIB_H_
