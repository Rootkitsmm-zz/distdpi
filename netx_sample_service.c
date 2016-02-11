/* **********************************************************
 * Copyright 2013-2015 VMware, Inc.
 * All rights reserved. -- VMware Confidential
 * **********************************************************/

/*
 * netx_sample_service.c --
 *
 *   Sample data plane agent(DPA) using the Netx & DVFilterLib APIs
 *
 */

#include "dvfilterlib.h"
#include "net-protos.h"  // These two files need to be in this order
#include <errno.h>       // errno
#include <stdlib.h>      // NULL
#include <stdio.h>       // FILE
#include <unistd.h>      // close
#include <stdarg.h>      // va_start(), ...
#include <string.h>      // memcpy()
#include <ctype.h>       // toupper()
#include <getopt.h>      // getopt_long()
#include <sys/socket.h>  // socket()
#include <sys/types.h>   // uint8_t
#include <netinet/in.h>  // IF_NET
#include <sys/ioctl.h>     // ioctl
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <assert.h>
#include <signal.h>
#include "netx_lib_api.h"
#include <netx_service.h>

#define ASSERT(x) assert(x)

#define FILTERNAME_MAXLEN 100
#define MAX_VC_UUID_LEN 128
#define INVALID_FD (-1)

typedef enum filterMagic {
   FILTER_MAGIC = 0x57465644 /* "DVFW" */
} filterMagic;

typedef struct filter_kern_config {
   uint8_t filterToSwitch;
   uint8_t filterFromSwitch;
   uint8_t filterICMP;
   uint16_t filterUDPPortNBO;
   uint16_t filterTCPPortNBO;
   uint8_t copyPackets;
   uint32_t delayPacketsMs;
   uint8_t reorderPackets;
   uint16_t dnaptPortNBO;
   uint16_t fakeProcessingUsecs;
} PACKED filter_kern_config;

typedef filter_kern_config filterState;

typedef struct filter {
   filterMagic magic;
   DVFilterHandle handle;
   filterState state;
   char filterName[FILTERNAME_MAXLEN];
   char vcUUID[DVFILTER_MAX_VC_UUID_LEN];
   DVFilterEndPointType endPointType;
   uint32_t vNicIndex;
} filter;

#if DEBUG
#  define LOG(level, format, ...) Log(level, "[%u] filter:%u: %s: " format, time(NULL), __LINE__, AgentName, ##__VA_ARGS__)
#else
#  define LOG(level, format, ...)
#endif

filter *
LookupFilter(char *vcUUID, int vNicIndex);

void
Log(int level, const char *format, ...);

typedef struct filterCliChannel {
   int cliChannelFd; // fd for the channel
   NetxCtlMsg_t msg; // msg buffer for CLI msg
   int msgLength;    // msg length already received in msg
} filterCliChannel;

typedef enum filterAction {
   NO_MATCH,
   MATCH,
} filterAction;

typedef struct filterPktInfo {
   uint16_t ethtype;
   uint8_t protocol;
   uint16_t srcPort;
   uint16_t dstPort;
   uint16_t vlanID;
   uint16_t priority;
} filterPktInfo;

typedef enum {
  ACTION_PUNT = 0,
  ACTION_COPY = 1
} sAction;

char *AgentName;
sAction g_action = ACTION_PUNT;

static filter **filterList;    // list of filter pointers
static int filterListSize;     // number of elements the list can hold
static int filterListCount;     // number of elemts currently in the list

static int logLevel = 5;
static FILE *logFile = NULL;

int num_conn_g = 0;

Netx_Conn_Info_Brief_t *conn_set_g = NULL;
int conn_set_g_size = 0;
int num_conn_set_g = 0;

typedef struct data {
   int id;
   int val;
} internalData;

int pvt_data_g_size = sizeof(internalData);
void *pvt_data_g = NULL;

void *obj = NULL;
PacketCbPtr ptrcb;
int running_ = 0;

void
Netx_LibReply_Callback (void *filter, void *callback_data,
                        void *payload, size_t payload_len)
{
   LOG(14, "In netx lib reply callback. Complete callback \n");
   Netx_Msg_Hdr_t* hdr;
   char *out = NULL;

   hdr = (Netx_Msg_Hdr_t*)payload;
   if(payload_len <sizeof(*hdr)) {
      fprintf(stderr, "Ret Msg Pload Length %lu < sizeof(*hdr) %lu. Msg timed out\n",
              (long unsigned int)payload_len,(long unsigned int) sizeof(*hdr));
      //NETX_STATUS_REPLY_CALLBACK_PROCESSING_FAIL
      goto clean_exit;
   }

   switch(hdr->cmd)
   {
      case NETXCMD_GET_CONNTRACK_COUNT:
         // Check hdr Payload Len = expected len
         // print the private data contents
         if (callback_data) {
            LOG(14, "Private_data id = %d.\n",
                   ((internalData*)callback_data)->id);
            LOG(14, "Private_data val = %d.\n",
                   ((internalData*)callback_data)->val);
         }

         // Parse the return command
         Netx_Conn_Count_Data_t* ccd_msg;
         Netx_Conn_Count_t* cc_msg;
         ccd_msg = (Netx_Conn_Count_Data_t*)payload;
         cc_msg = (Netx_Conn_Count_t*)ccd_msg->payload;

         LOG(14, "ccd_msg hdr payload len is %d\n",
                 ccd_msg->hdr.payload_len);
         LOG(14, "ccd_msg payload is %p\n",ccd_msg->payload);
         LOG(14, "cc_msg payload count %d\n",cc_msg->count);
         num_conn_g = cc_msg->count;
         printf("Connection Count: %d .\n",num_conn_g);
         break;

      case NETXCMD_GET_CONNTRACK_ALL_BRIEF:
         // Check hdr Payload Len = expected len
         // print the private data contents
         if (callback_data) {
            LOG(14, "Private_data id = %d.\n",
                   ((internalData*)callback_data)->id);
            LOG(14, "Private_data val = %d.\n",
                   ((internalData*)callback_data)->val);
         }

         // Parse the return command
         Netx_Conn_Data_t* cd_msg;
         Netx_Conn_Info_Brief_t* cib_msg;
         cd_msg = (Netx_Conn_Data_t*)payload;
         cib_msg = (Netx_Conn_Info_Brief_t*)cd_msg->payload;

         LOG(14, "cd_msg hdr payload len is %u\n",
                 cd_msg->hdr.payload_len);
         LOG(14, "cd_msg hdr count is %u\n",
           cd_msg->hdr.count);
         LOG(14, "cd_msg payload is %p\n",cd_msg->payload);
         LOG(14, "cib_msg payload conn_id %lu\n",
            (long unsigned int)cib_msg->conn_id);

         //print the conn info brief contents received.
         printf("Number of Connection information returned is %d .\n",
                  cd_msg->hdr.count);
         Netx_PrintConnInfoBrief(cd_msg->hdr.count,
                  (Netx_Conn_Info_Brief_t*)cd_msg->payload,
                  cd_msg->hdr.payload_len, &out);
         if (out) {
            printf("Output is : \n%s\n",out);
            //Free the buffer after printing the contents.
            free (out);
         } else {
            printf("Netx_PrintConnInfoBrief did not return output.\n");
         }


         // Copy the buffer returned into global conn_b to be used for setting
         // connection action
         num_conn_set_g = cd_msg->hdr.count;
         conn_set_g_size = num_conn_set_g*sizeof(Netx_Conn_Info_Brief_t);
         conn_set_g = (Netx_Conn_Info_Brief_t *) malloc(conn_set_g_size);
         if (conn_set_g) {
            LOG(14, "conn_set_g ptr is %p\n", conn_set_g);
            // Read only conn_set_g_size
            memcpy(conn_set_g,cd_msg->payload,conn_set_g_size);
         } else {
            fprintf(stderr, "malloc failure netx conn info brief buffer\n");
         }
         break;

      case NETXCMD_SET_CONNTRACK_ACTION_BRIEF:
         // Check hdr Payload Len = expected len
         // print the private data contents
         if (callback_data) {
            LOG(14, "Private_data id = %d.\n",
                   ((internalData*)callback_data)->id);
            LOG(14, "Private_data val = %d.\n",
                   ((internalData*)callback_data)->val);
         }

         // Parse the return command
         Netx_Conn_Set_Brief_t* csb_msg;
         csb_msg = (Netx_Conn_Set_Brief_t*)payload;

         LOG(14, "csb_msg hdr payload len is %u\n",
                 csb_msg->hdr.payload_len);
         LOG(14, "csb_msg hdr count is %u\n",
           csb_msg->hdr.count);
         LOG(14, "csb_msg payload is %p\n",csb_msg->payload);

         //print the conn info brief contents received.
         printf("Number of Connection information set is %d .\n",
                  csb_msg->hdr.count);
         Netx_PrintConnInfoBrief(csb_msg->hdr.count,
                  (Netx_Conn_Info_Brief_t*)csb_msg->payload,
                  csb_msg->hdr.payload_len,&out);

         if (out) {
            printf("Output is : \n%s\n",out);
            //Free the buffer after printing the contents.
            free (out);
         } else {
            printf("Netx_PrintConnInfoBrief did not return output.\n");
         }
         break;

      case NETXCMD_GET_SERVICE_PROFILE_ID:
         // Check hdr Payload Len = expected len
         // print the private data contents
         if (callback_data) {
            LOG(14, "Private_data id = %d.\n",
                   ((internalData*)callback_data)->id);
            LOG(14, "Private_data val = %d.\n",
                   ((internalData*)callback_data)->val);
         }

         // Parse the return command
         Netx_SP_t* sp_msg;
         sp_msg = (Netx_SP_t*)payload;

         LOG(14, "sp_msg hdr payload len is %d\n",
                 sp_msg->hdr.payload_len);
         printf("Service profile id is %s\n",sp_msg->sprofile_id);
         break;

      case NETXCMD_GET_SERVICE_INFO:
         // Check hdr Payload Len = expected len
         // print the private data contents
         if (callback_data) {
            LOG(14, "Private_data id = %d.\n",
                   ((internalData*)callback_data)->id);
            LOG(14, "Private_data val = %d.\n",
                   ((internalData*)callback_data)->val);
         }

         // Parse the return command
         Netx_Service_Data_t* sd_msg;
         sd_msg = (Netx_Service_Data_t*)payload;

         LOG(14, "sd_msg hdr payload len is %d\n",
                 sd_msg->hdr.payload_len);
         printf("Service vendor id is %s\n",sd_msg->info.vendor_template_id);
         printf("Service vendor provided template id is %s\n",
                 sd_msg->info.vendor_provided_template_id);
         break;

      default:
         LOG(14, "Processing default.\n");
         break;

   }

clean_exit:
   return;
}

static void PrintState(filterState *state);

/******************************************************************************
 *                                                                       */ /**
 * \brief Log a message to the default log file
 *
 ******************************************************************************
 */
void
Log(int level, const char *format, ...)
{
   va_list ap;

   if (logFile == NULL) {
      logFile = stderr;
   }

   if (level > logLevel) {
      return;
   }

   va_start(ap, format);
   vfprintf(logFile, format, ap);
   va_end(ap);
}

filter *
LookupFilter(char *vcUUID, unsigned int vNicIndex)
{
   int i;

   for (i = 0; i < filterListCount; i++) {
      if (strncmp(filterList[i]->vcUUID, vcUUID,
                  sizeof(filterList[i]->vcUUID)) == 0 &&
          filterList[i]->vNicIndex == vNicIndex) {
         return filterList[i];
      }
   }

   return NULL;
}

/******************************************************************************
 *                                                                       */ /**
 * \brief Initialize one filterCliChannel data structure
 *
 ******************************************************************************
 */
static void
InitCliChannel(filterCliChannel *cliChannel)
{
   memset((void *)cliChannel, 0, sizeof(filterCliChannel));
   cliChannel->cliChannelFd = INVALID_FD;
}


/******************************************************************************
 *                                                                       */ /**
 * \brief Copy data out from a pkt
 *
 *    Copy data out from the fram of a PktHandle, starting at
 *    an offset, for an indicated number of bytes.
 *    dst must contain copyLen.
 *
 *    If the copyLen passed is zero, this indicates all remaining
 *    bytes starting with offset are to be moved.
 *
 ******************************************************************************
 */
DVFilterStatus
CopyPacketData(uint8_t *dst,
                         PktHandle *src,
                         uint32_t offset,
                         uint32_t copyLen)
{
   if (copyLen == 0) {
      copyLen = DVFilter_GetPktLen(src) - offset;
   }

   while (copyLen > 0) {
      uint32_t moveLen;
      uint8_t *srcData;

      srcData = DVFilter_GetPktFragDirectRange(src, offset, 0, &moveLen);
      if (srcData == NULL) {
         return DVFILTER_STATUS_ERROR;
      }
      if (moveLen > copyLen) {
         moveLen = copyLen;
      }
      ASSERT(moveLen > 0);

      memcpy(dst, srcData, moveLen);
      dst += moveLen;
      offset += moveLen;
      copyLen -= moveLen;
   }
   return DVFILTER_STATUS_OK;
}


/******************************************************************************
 *                                                                       */ /**
 * \brief Copy packet contents to a another packet.
 *
 *      Frag's complicate the movement of data
 *
 ******************************************************************************
 */
DVFilterStatus
CopyPacket(PktHandle *dst, PktHandle *src, uint32_t copyLen)
{
   uint32_t srcLen = DVFilter_GetPktLen(src);
   uint32_t dstLen = DVFilter_GetPktLen(dst);
   uint32_t moveLen;
   uint32_t sNdx, dNdx, sLen, dLen;


   if (copyLen == 0) {
      copyLen = srcLen;
   } else if (copyLen < srcLen) {
      return DVFILTER_STATUS_ERROR;
   }
   if (dstLen < srcLen) {
      return DVFILTER_STATUS_ERROR;
   }

   sNdx = 0; sLen = 0;
   dNdx = 0; dLen = 0;

   for (; copyLen > 0; copyLen -= moveLen) {
      DVFilterStatus status;
      uint8_t *sData, *dData;

      if (sLen == 0) {
         status = DVFilter_GetPktFragAddressAndLength(src, sNdx++, &sData, &sLen);
         if (status != DVFILTER_STATUS_OK) {
            return status;
         }
      }
      if (dLen == 0) {
         status = DVFilter_GetPktFragAddressAndLength(dst, dNdx++, &dData, &dLen);
         if (status != DVFILTER_STATUS_OK) {
            return status;
         }
      }
      moveLen = sLen;
      if (moveLen > dLen) {
         moveLen = dLen;
      }
      ASSERT((sData != NULL) && (dData != NULL) && moveLen > 0);
      memcpy(dData, sData, moveLen);

      sLen -= moveLen;
      sData += moveLen;

      dLen -= moveLen;
      dData += moveLen;

      // one of the two needs to be consumed each cycle
      ASSERT((sLen == 0) || (dLen == 0));
   }

   if (0) {
      /*
       * A different approach would be to manage the offset, and recompute
       * the base address each cycle.
       */
      uint32_t offset;
      for (offset = 0; copyLen > 0; offset += moveLen, copyLen -= moveLen) {
         uint32_t dDataLen;
         uint8_t *sData, *dData;

         sData = DVFilter_GetPktFragDirectRange(src, offset, 0, &moveLen);
         ASSERT(sData != NULL);
         dData = DVFilter_GetPktFragDirectRange(dst, offset, 0, &dDataLen);
         ASSERT(dData != NULL);

         if (moveLen > dDataLen) {
            moveLen = dDataLen;
         }
         memcpy(dData, sData, moveLen);
      }
   }

   return DVFILTER_STATUS_OK;
}


/*
 * direct context holds details about what's currently directly available,
 * and helps manage the situations where the header structure under review
 * spans between two frag elements.
 *
 * Two distinct regions are managed.  The 'direct' regions describe
 * recent calls made to DVFilter_GetPktFragDirectRange(), while
 * the backingBuffer region describes bytes copied out from the
 * pkt into a local buffer.
 *
 */
typedef struct filterDirectContext {
   uint8_t                *direct;              // varies as used
   uint32_t                dirOffset;
   uint32_t                dirLen;
   uint8_t                *backingBuffer;       // filled on init
   uint32_t                bbLen;               // filled on init
   uint32_t                bbAvailable;         // varies as used
   uint32_t                bbOffset;            // varies as used
} filterDirectContext;

/******************************************************************************
 *                                                                       */ /**
 * \brief Initialize a filterDirectContext structure
 *
 *    Fill with required parameters at init, zero other fields.
 *
 ******************************************************************************
 */
filterDirectContext *
DirectContextInit(filterDirectContext *direct,
                            uint8_t *buffer,
                            uint32_t bbLen)
{
   direct->direct = NULL;
   direct->dirOffset = 0;
   direct->dirLen = 0;

   direct->backingBuffer = buffer;
   direct->bbLen = bbLen;
   direct->bbAvailable = 0;
   direct->bbOffset = 0;

   return direct;
}

/******************************************************************************
 *                                                                       */ /**
 * \brief Return a reference for the pkt's region
 *
 *    The request may be satisfield by the direct call which describes
 *    frags, or it may be satisfied by a copy of the region pulled out
 *    of the pkt.
 *
 ******************************************************************************
 */
void *
PktDirectRef(PktHandle *pkt,
                       uint32_t offset,
                       uint32_t length,
                       filterDirectContext *direct)
{
   uint32_t fragLen;
   uint8_t *result;

   /*
    * Try to use the directly mapped region from a previous call.
    */
   if (direct->direct && (offset >= direct->dirOffset) &&
       (offset + length < direct->dirOffset + direct->dirLen)) {

      return direct->direct + offset;
   }
   /*
    * Attempt to get a directly mapped region.  If succeeds, then
    * save the region to service a later request.
    */
   result = DVFilter_GetPktFragDirectRange(pkt, offset, length, &fragLen);
   if (result != NULL) {
      direct->direct = result;
      direct->dirOffset = offset;
      direct->dirLen = fragLen;
      LOG(11, "want %d %d, got %d\n", offset, length, fragLen);
      return result;
   }

   /*
    * Attempt to return a reference for a portion of the buffer which
    * was previously copied from the pkt.
    */
   if ((offset >= direct->bbOffset) &&
       (offset + length < direct->bbAvailable + direct->bbOffset)) {

      return direct->backingBuffer + offset;
   }

   /*
    * No help if the local buffer will never fit.
    */
   if (direct->bbLen <= length) {
      uint32_t copyLen;
      DVFilterStatus status;
      /*
       * fill the buffer with as much data as possible from the offset
       */
      copyLen = sizeof(direct->bbLen);
      /*
       * Validate pkt's truncation.
       */
      if (copyLen + offset > DVFilter_GetPktLen(pkt)) {
         copyLen = DVFilter_GetPktLen(pkt) - offset;
         if (copyLen < length) {
            return NULL;
         }
      }

      status = CopyPacketData(direct->backingBuffer,
                                        pkt, offset, copyLen);
      if (status == DVFILTER_STATUS_OK) {
         direct->bbOffset = offset;
         direct->bbAvailable = copyLen;
         return direct->backingBuffer;
      }
   }
   return NULL;
}

/******************************************************************************

 *
 ******************************************************************************
 */
void UpdateFilterConfiguration(filter *f)
{
   int paramIdx;
   DVFilterStatus status;

   for (paramIdx = 0; paramIdx < DVFILTER_MAX_PARAMS; paramIdx++) {
      char param[DVFILTER_MAX_CONFIGPARAM_LEN];
      status = DVFilter_GetConfigParameter(f->handle, paramIdx,
                                           param, sizeof(param));
      if (status == DVFILTER_STATUS_OK) {
         LOG(0, "ConfigParam %d: %s\n", paramIdx, param);
      } else {
         LOG(0, "Parameter %d: %s\n", paramIdx,
             DVFilter_StatusToString(status));
      }
   }

   status = DVFilter_GetEndPointType(f->handle, &f->endPointType);
   ASSERT(status == DVFILTER_STATUS_OK);
   if (f->endPointType == DVFILTER_ENDPOINT_VNIC) {
      LOG(0, "EndPointType: VM vNic\n");
   } else {
      LOG(0, "EndPointType: Not a VM vNic: %d\n",
          f->endPointType);
   }

   status = DVFilter_GetVcUuid(f->handle, f->vcUUID, sizeof(f->vcUUID));
   if (status == DVFILTER_STATUS_OK) {
      LOG(0, "VC UUID: %s\n", f->vcUUID);
   } else {
      LOG(0, "VC UUID not available: %s\n",
          DVFilter_StatusToString(status));
   }

   status = DVFilter_GetVnicIndex(f->handle, &f->vNicIndex);
   if (status == DVFILTER_STATUS_OK) {
      LOG(0, "vNicIndex: %u\n", f->vNicIndex);
   } else {
      LOG(0, "vNicIndex not available: %s\n",
          DVFilter_StatusToString(status));
   }
}

/******************************************************************************
 *                                                                       */ /**
 * \brief Instantiate a filter
 *
 *    Called when the virtual NIC first connects to the virtual switch port.
 *
 ******************************************************************************
 */
static DVFilterStatus
CreateFilter(DVFilterHandle handle,
		       DVFilterImpl *privateData)
{
   filter *f = (filter *) calloc(1, sizeof(*f));
   static char failedFilterName[sizeof(f->filterName)];

   if (f == NULL) {
      return DVFILTER_STATUS_ERROR;
   }

   DVFilter_GetName(handle, f->filterName, sizeof(f->filterName));
   LOG(0, "CreateFilter request: name: %s (%p)\n",
       f->filterName, handle);

   if (strncmp(f->filterName, failedFilterName,
               sizeof(f->filterName)) == 0) {
      /* stress option base failure need to also fail for retries */
      LOG(0, "Failing create AGAIN due to previous stress option (filter: %p %s)\n",
          handle, f->filterName);
      free(f);
      return DVFILTER_STATUS_ERROR;
   }

   f->magic = FILTER_MAGIC;
   f->handle = handle;

   *privateData = f;

   LOG(14, "CreateFilter filter %p handle %p\n", f, handle);

   UpdateFilterConfiguration(f);

   if (filterListCount + 1 == filterListSize) {
      LOG(0, "Filter list is full!\n");
   } else {
      filterList[filterListCount] = f;
      LOG(14, "Stored filter %p at filterList[%d]\n",
             filterList[filterListCount], filterListCount);
      filterListCount++;
   }

   return DVFILTER_STATUS_OK;
}

/******************************************************************************
 *                                                                       */ /**
 * \brief Handle a filter configuration change
 *
 ******************************************************************************
 */
static void
ConfigurationChanged(DVFilterImpl filterImpl)
{
   filter *f = (filter *) filterImpl;
   ASSERT(f->magic == FILTER_MAGIC);

   LOG(0, "ConfigurationChanged %p\n", f->handle);

   UpdateFilterConfiguration(f);
}

/******************************************************************************
 *                                                                       */ /**
 * \brief Destroy a filter
 *
 *    This callback is typically invoked when the VM is destroyed,
 *    after the port has been disabled
 *
 ******************************************************************************
 */
static DVFilterStatus
DestroyFilter(DVFilterImpl filterImpl)
{
   filter *f = (filter *) filterImpl;
   int i;

   ASSERT(f->magic == FILTER_MAGIC);

   LOG(0, "DestroyFilter %p\n", f->handle);

   f->magic = (filterMagic) ~FILTER_MAGIC;

   ASSERT(filterListCount > 0);
   if (filterListCount > 1) {
      for (i = 0; i < filterListCount; i++) {
         if (filterList[i] == f) {
            filterList[i] = filterList[filterListCount - 1];
         }
      }
   }
   filterListCount--;

   free(f);

   return DVFILTER_STATUS_OK;
}

/******************************************************************************
 *                                                                       */ /**
 * \brief Query the filter for the size of the saved state buffer
 *
 *    This callback is always invoked just before calling
 *    savedState()
 *
 ******************************************************************************
 */
static DVFilterStatus
GetSavedStateLen(DVFilterImpl filterImpl,
			   uint32_t *len)
{
   filter *f = (filter *) filterImpl;

   LOG(5, "GetSavedStateLen %p\n", f->handle);

   ASSERT(f->magic == FILTER_MAGIC);

   *len = sizeof(f->state) + (rand() % 128);

   return DVFILTER_STATUS_OK;
}

/******************************************************************************
 *                                                                       */ /**
 * \brief Saves the state of the filter
 *
 *    This callback is always invoked after getSavedStateLen()
 *
 ******************************************************************************
 */
static DVFilterStatus
SaveState(DVFilterImpl filterImpl,
		    void *buf,
		    uint32_t *len)
{
   filter *f = (filter *) filterImpl;
   DVFilterStatus status = DVFILTER_STATUS_OK;

   ASSERT(f->magic == FILTER_MAGIC);

   LOG(5, "SaveState %p (name: %s)\n",
       f->handle, f->filterName);

   PrintState(&f->state);

   if (*len < sizeof(f->state)) {
      LOG(0, "ERROR: *len (%u) < sizeof(f->state) (%u)\n",
          *len, sizeof(f->state));
      status = DVFILTER_STATUS_ERROR;
      goto done;
   }

   memcpy(buf, &f->state, sizeof(f->state));
   *len = sizeof(f->state);

done:

   return status;
}

/******************************************************************************
 *                                                                       */ /**
 * \brief Restores the state of the filter
 *
 *    This callback is invoked after addVPort(), iff the agent has
 *    some persistent state saved
 *
 ******************************************************************************
 */
static DVFilterStatus
RestoreState(DVFilterImpl filterImpl,
		       void *buf,
		       uint32_t len)
{
   filter *f = (filter *) filterImpl;
   DVFilterStatus status = DVFILTER_STATUS_OK;

   ASSERT(f->magic == FILTER_MAGIC);

   LOG(5, "RestoreState %p\n", f->handle);

   if (len != sizeof(f->state)) {
      LOG(0, "unexpected state length (expect %d, got %d)\n",
          sizeof(f->state), len);
      status = DVFILTER_STATUS_ERROR;
      goto done;
   }

   memcpy(&f->state, buf, sizeof(f->state));
   PrintState(&f->state);

done:

   return status;
}

/******************************************************************************
 *                                                                       */ /**
 * \brief Print one liner information about the given packet
 *
 ******************************************************************************
 */

static void
PrintPacketInfo(PktHandle *pkt, filterPktInfo *info, DVFilterDirection direction)
{
   char *ethTypeStr = "unknown";
   char *protoStr = "";
   char portStr[60] = "";
   switch (info->ethtype) {
   case ETH_TYPE_ARP_NBO:
      ethTypeStr = "ARP";
      break;
   case ETH_TYPE_RARP_NBO:
      ethTypeStr = "RARP";
      break;
   case ETH_TYPE_IPV4_NBO:
      ethTypeStr = "IPv4";
      break;
   case ETH_TYPE_IPV6_NBO:
      ethTypeStr = "IPv6";
      break;
   }

   if (info->ethtype == ETH_TYPE_IPV4_NBO
       || info->ethtype == ETH_TYPE_IPV6_NBO) {
      switch (info->protocol) {
      case IPPROTO_ICMP:
      case IPPROTO_ICMPV6:
         protoStr = "ICMP, ";
         break;
      case IPPROTO_UDP:
         protoStr = "UDP, ";
         break;
      case IPPROTO_TCP:
         protoStr = "TCP, ";
         break;
      default:
         break;
      }
   }

   if (info->protocol == IPPROTO_TCP || info->protocol == IPPROTO_UDP) {
      snprintf(portStr, sizeof portStr, "src = %u, dst = %u",
               ntohs(info->srcPort), ntohs(info->dstPort));
   }

   if (info->vlanID != 0 || info->priority != 0) {
      snprintf(portStr+strlen(portStr), sizeof(portStr) - strlen(portStr),
               " vlanID = %u, prio = %u",
               info->vlanID, info->priority);
   }

   printf("packet %s, len %u, metadatalen = %u, ethType = %s (%.4x), %s%s\n",
       direction == DVFILTER_FROM_SWITCH ? "IN" : "OUT",
       DVFilter_GetPktLen(pkt),
       DVFilter_GetPktMetadataLen(pkt),
       ethTypeStr, info->ethtype, protoStr, portStr);
}


/******************************************************************************
 *                                                                       */ /**
 * \brief Dump filter state for debugging
 *
 ******************************************************************************
 */
static void
PrintState(filterState *state)
{
   LOG(6, "State pid=%d:\n", getpid());
   LOG(6, "filterToSwitch: %d\n", state->filterToSwitch);
   LOG(6, "filterFromSwitch: %d\n", state->filterFromSwitch);
   LOG(6, "filterICMP: %d\n", state->filterICMP);
   LOG(6, "filterUDPPort: %d\n", ntohs(state->filterUDPPortNBO));
   LOG(6, "filterTCPPort: %d\n", ntohs(state->filterTCPPortNBO));
   LOG(6, "copyPackets: %d\n", state->copyPackets);
   LOG(6, "delayPacketsMs: %d\n", state->delayPacketsMs);
   LOG(6, "dnaptPort: %d\n", ntohs(state->dnaptPortNBO));
   LOG(6, "fakeProcessingUsecs: %d\n", state->fakeProcessingUsecs);
}

/******************************************************************************
 *                                                                       */ /**
 * \brief Parse the given packet and populate supplied struct
 *
 *    filterPktInfo struct is expected to be zeroed out. If the lower
 *    layer field (e.g. protocol) is 0 then the upper layer fields (e.g.
 *    srcPort) are not valid.
 *
 ******************************************************************************
 */
static void
ParsePacket(PktHandle *pkt,
                      filterPktInfo *info)
{
   Eth_Header *ethHdr;
   uint32_t offset;
   uint8_t  *buf, smallBuffer[128];
   uint16_t ethtype;
   filterDirectContext direct[1];
   DirectContextInit(direct, smallBuffer, sizeof(smallBuffer));

   buf = (uint8_t *) PktDirectRef(pkt, 0, sizeof(Eth_Header), direct);
   if (buf == NULL) {
      return;
   }

   ethHdr = (Eth_Header *) buf;

   if (EthHeaderType(ethHdr) == ETH_HEADER_TYPE_802_1PQ) {
      info->vlanID = EthVlanTagGetVlanID(&ethHdr->e802_1pq.tag);
      info->priority = ethHdr->e802_1pq.tag.priority;
   }

   ethtype = EthEncapsulatedPktType(ethHdr, &offset);
   info->ethtype = ethtype;
   if (ethtype == ETH_TYPE_ARP_NBO) {
      return;
   }

   if (ethtype == ETH_TYPE_IPV4_NBO || ethtype == ETH_TYPE_IPV6_NBO) {
      IPHdr *ipHeader;
      IPv6Hdr *ipv6Header;
      uint8_t protocol = 0;
      uint32_t nextHeaderOffset = 0;

      switch(ethtype) {
      case ETH_TYPE_IPV6_NBO:
         // ipv6Header = (IPv6Hdr *) (buf + offset);
         ipv6Header = (IPv6Hdr *) PktDirectRef(pkt, offset, sizeof(IPv6Hdr), direct);
         if (ipv6Header == NULL) {
            return;
         }
         protocol = ipv6Header->nextHeader;
         nextHeaderOffset = sizeof(IPv6Hdr);
         break;
      case ETH_TYPE_IPV4_NBO:
         // ipHeader = (IPHdr *) (buf + offset);
         ipHeader = (IPHdr *) PktDirectRef(pkt, offset, sizeof(IPHdr), direct);
         if (ipHeader == NULL) {
            return;
         }

         char *pktptr;
         pktptr = (char *) ipHeader;
         //ptrcb(obj, pktptr, ipHeader->tot_len);

         protocol = ipHeader->protocol;
         nextHeaderOffset = (ipHeader->ihl * 4);
         break;
      default:
         ASSERT(0);
         return;
      }
      info->protocol = protocol;

      if (protocol == IPPROTO_ICMP || protocol == IPPROTO_ICMPV6) {
         // do nothing
      } else if (protocol == IPPROTO_UDP) {
         UDPHdr *udpHdr;

         //udpHdr = (UDPHdr *) (buf + offset + nextHeaderOffset);
         udpHdr = (UDPHdr *) PktDirectRef(pkt, offset + nextHeaderOffset,
                                         sizeof(UDPHdr), direct);
         if (udpHdr == NULL) {
            return;
         }

         info->srcPort = udpHdr->source;
         info->dstPort = udpHdr->dest;
      } else if (protocol == IPPROTO_TCP) {
         TCPHdr *tcpHdr;

         // tcpHdr = (TCPHdr *) (buf + offset + nextHeaderOffset);
         tcpHdr = (TCPHdr *) PktDirectRef(pkt, offset + nextHeaderOffset,
                                         sizeof(TCPHdr), direct);
         if (tcpHdr == NULL) {
            return;
         }

         info->srcPort = tcpHdr->source;
         info->dstPort = tcpHdr->dest;
      }
   }
}

/******************************************************************************
 *                                                                       */ /**
 * \brief Filter in/out inbound packet
 *
 * \retval NO_MATCH We're not interested in that packet.
 * \retval MATCH The packet matches the pattern we're interested in
 *
 ******************************************************************************
 */
static filterAction
FilterPacket(filterPktInfo *info,
                       DVFilterDirection direction,
                       filterState *state)
{
   uint8_t protocol = info->protocol;

   switch (info->ethtype) {
   case ETH_TYPE_ARP_NBO:
      LOG(11, "Got ARP packet. No match\n");
      return NO_MATCH;
   case ETH_TYPE_IPV4_NBO:
   case ETH_TYPE_IPV6_NBO:
      if ((protocol == IPPROTO_ICMP || protocol == IPPROTO_ICMPV6)
          && state->filterICMP) {
         LOG(9, "%sbound ICMP packet\n",
             direction == DVFILTER_FROM_SWITCH ? "In" : "Out");
         return MATCH;
      } else if (protocol == IPPROTO_UDP && state->filterUDPPortNBO != 0) {
         if (info->srcPort == state->filterUDPPortNBO
             || info->dstPort == state->filterUDPPortNBO) {
            LOG(9, "Matched packet. UDP port is %d\n",
                      ntohs(state->filterUDPPortNBO));
            return MATCH;
         }
      } else if (protocol == IPPROTO_TCP && state->filterTCPPortNBO != 0) {
         if (info->srcPort == state->filterTCPPortNBO
             || info->dstPort == state->filterTCPPortNBO) {
            LOG(9, "Matched packet. TCP port is %d, direction = %s\n",
                ntohs(state->filterTCPPortNBO),
                direction == DVFILTER_FROM_SWITCH ? "IN" : "OUT");
            return MATCH;
         }
      }
      break;
   default:
      LOG(11, "Got unknown ethtype (%x) packet. No match\n", info->ethtype);
   }

   return NO_MATCH;
}


/******************************************************************************
 *                                                                       */ /**
 * \brief Perform NAPT on TCP packet
 *
 *    If the packet is a TCP packet, its destination port number
 *    is rewritten.
 *
 ******************************************************************************
 */
static DVFilterStatus
PerformNAPT(PktHandle *pkt,
                      uint16_t newport,
                      DVFilterDirection direction)
{
   uint32_t offset;
   uint16_t ethtype;
   uint16_t frameLen;
   uint8_t *buf, lclBuf[128];
   filterDirectContext direct[1];
   DirectContextInit(direct, lclBuf, sizeof(lclBuf));

   frameLen = DVFilter_GetPktLen(pkt);
   buf = (uint8_t *) PktDirectRef(pkt, 0, sizeof(Eth_Header), direct);
   if (buf == NULL) {
      return (DVFilterStatus) NO_MATCH;
   }

   ethtype = EthEncapsulatedPktType((Eth_Header *) buf, &offset);
   if (ethtype == ETH_TYPE_IPV4_NBO || ethtype == ETH_TYPE_IPV6_NBO) {
      struct IPv6Hdr *ipv6Header;
      struct IPHdr * ipHeader;

      uint8_t protocol = 0;
      uint32_t nextHeaderOffset = 0;

      switch(ethtype) {
      case ETH_TYPE_IPV6_NBO:
         ipv6Header = (IPv6Hdr *) PktDirectRef(pkt, offset, sizeof(IPv6Hdr), direct);
         if (ipv6Header == NULL) {
            return (DVFilterStatus) NO_MATCH;
         }
         protocol = ipv6Header->nextHeader;
         nextHeaderOffset = sizeof(IPv6Hdr);
         if (frameLen < offset + sizeof(struct IPv6Hdr)) {
            return DVFILTER_STATUS_ERROR;
         }
         break;
      case ETH_TYPE_IPV4_NBO:
         // ipHeader = (IPHdr *) (buf + offset);
         ipHeader = (IPHdr *) PktDirectRef(pkt, offset, sizeof(IPHdr), direct);
         if (ipHeader == NULL) {
            return (DVFilterStatus) NO_MATCH;
         }
         protocol = ipHeader->protocol;
         nextHeaderOffset = (ipHeader->ihl * 4);
         if (frameLen < offset + sizeof(struct IPHdr)) {
            return DVFILTER_STATUS_ERROR;
         }
         break;
      default:
         ASSERT(0);
         return DVFILTER_STATUS_ERROR;
      }

      if (protocol == IPPROTO_TCP) {
         struct TCPHdr *tcpHdr;
         uint16_t oldport;
         uint32_t csum;
         uint8_t *csumPtr;

         if (frameLen < offset + nextHeaderOffset + sizeof(struct TCPHdr)) {
            return DVFILTER_STATUS_ERROR;
         }
         //tcpHdr = (TCPHdr *) (buf + offset + nextHeaderOffset);
         tcpHdr = (TCPHdr *) PktDirectRef(pkt, offset + nextHeaderOffset,
                                      sizeof(TCPHdr), direct);
         ASSERT(tcpHdr);

         csumPtr = (uint8_t*)&tcpHdr->check;

         if (direction == DVFILTER_FROM_SWITCH) {
            oldport = tcpHdr->dest;
            tcpHdr->dest = newport;
         } else {
            oldport = tcpHdr->source;
            tcpHdr->source = newport;
         }

         /* Lets assume for now that we always get full, final checksums.
            Using RFC 1624 (eqn. 3):
            HC' = ~(~HC + ~m + m')

            If IsMustCsum is set, the packet contains a pseudo hdr
            csum, which does not include the ports, so no update
            required */
         if (!DVFilter_IsMustCsum(pkt)) {
            csum  = (uint16_t)~(*((uint16_t*)csumPtr));
            csum += newport + (uint16_t)(~oldport);
            csum = (csum & 0xffff) + (csum >> 16);
            csum = (csum & 0xffff) + (csum >> 16);
            *((uint16_t*)csumPtr) = ~csum;
         }

         return DVFILTER_STATUS_OK;
      }
   }

   return DVFILTER_STATUS_ERROR;
}


/******************************************************************************
 *                                                                       */ /**
 * \brief Packet callback (inbound or outbound)
 *
 *    Each packet in the packet list must ultimately be sent or
 *    freed using DVFilter_SendPkt() or DVFilter_FreePkt()
 *
 ******************************************************************************
 */
static void
ProcessPacket(DVFilterImpl filterImpl,
			PktHandle *pkt,
			DVFilterDirection direction)
{
   filter *f = (filter *) filterImpl;
   filterState *state = &f->state;
   filterPktInfo pktInfo;
   uint16_t frameLen;
   uint8_t  *buf, smallBuffer[128];
   filterDirectContext direct[1];
   DirectContextInit(direct, smallBuffer, sizeof(smallBuffer));
   PktMetadata pkt_mdata;

   ASSERT(f->magic == FILTER_MAGIC);

   if (1) {
      //memset(&pktInfo, 0, sizeof(pktInfo));
      //ParsePacket(pkt, &pktInfo);
      //PrintPacketInfo(pkt, &pktInfo, direction);
   }


#if DEBUG
   if (DVFilter_GetPktMetadataLen(pkt) == sizeof(int64_t) * 2) {
      uint8_t *metadata = DVFilter_GetPktMetadata(pkt);
      LOG(10, "packet metadata = %ld sec %ld usec\n",
          ((int64_t*)metadata)[0],
          ((int64_t*)metadata)[1]);
   }
#endif
   frameLen = DVFilter_GetPktLen(pkt);
   buf = (uint8_t *) PktDirectRef(pkt, 0, sizeof(Eth_Header), direct);
   pkt_mdata.filterPtr = f;
   pkt_mdata.pktPtr = (char *) buf;
   pkt_mdata.dir = direction;

   //ptrcb(obj, (uint8_t *) buf, frameLen);
   ptrcb(obj, &pkt_mdata, frameLen);

  if (g_action == ACTION_COPY) {
      //Copy Packet. Drop it.
      LOG(11, "ACTION_COPY: Dropping one packet\n");
      DVFilter_FreePkt(pkt);
      return;
   }

   if ((direction == DVFILTER_TO_SWITCH && state->filterToSwitch) ||
       (direction == DVFILTER_FROM_SWITCH && state->filterFromSwitch)) {

      memset(&pktInfo, 0, sizeof(pktInfo));
      ParsePacket(pkt, &pktInfo);
      if (FilterPacket(&pktInfo, direction, state) == NO_MATCH) {
         DVFilter_SendPkt(f->handle, pkt, direction);
         return;
      }

      if (state->copyPackets) {
         DVFilterStatus status;
         PktHandle *copyPacket;
         uint32_t frameLen;

         frameLen = DVFilter_GetPktLen(pkt);
         copyPacket = DVFilter_AllocPkt(frameLen + 8000);
         if (copyPacket == NULL) {
            LOG(9, "Failed to allocate packet copy\n");
            DVFilter_FreePkt(pkt);
            return;
         }

         DVFilter_SetPktLen(&copyPacket, frameLen + 16000);
         DVFilter_SetPktLen(&copyPacket, frameLen);
         DVFilter_SetPktMetadataLen(&copyPacket, 200);
         DVFilter_SetPktLen(&copyPacket, frameLen + 16000);
         DVFilter_SetPktMetadataLen(&copyPacket, 0);
         DVFilter_SetPktMetadataLen(&copyPacket, 100);
         DVFilter_SetPktMetadataLen(&copyPacket, 400);
         DVFilter_SetPktLen(&copyPacket, frameLen);
         DVFilter_SetPktMetadataLen(&copyPacket, 0);

         if (DVFilter_IsMustTso(pkt)) {
            DVFilter_SetMustTso(copyPacket, DVFilter_GetTsoMss(pkt));
         }
         if (DVFilter_IsMustCsum(pkt)) {
            DVFilter_SetMustCsum(copyPacket);
         }

         status = CopyPacket(copyPacket, pkt, frameLen);
         ASSERT(status == DVFILTER_STATUS_OK);

         DVFilter_FreePkt(pkt);
         pkt = copyPacket;
      }

      if (state->reorderPackets) {
         LOG(0, "Reordering packets ... NOT IMPLEMENTED ... no-op\n");
         DVFilter_SendPkt(f->handle, pkt, direction);
      } else if (state->delayPacketsMs) {
         LOG(0, "Delaying packet ... NOT IMPLEMENTED ... no-op\n");

         DVFilter_SendPkt(f->handle, pkt, direction);
      } else if (state->copyPackets) {
         LOG(11, "Copied one packet\n");

         DVFilter_SendPkt(f->handle, pkt, direction);
      } else if (state->dnaptPortNBO != 0) {
         DVFilterStatus status;
         LOG(11, "Performing NAT ...\n");

         status = PerformNAPT(pkt, state->dnaptPortNBO, direction);
         if (status == DVFILTER_STATUS_ERROR) {
            LOG(1, "NAT failed\n");
         }

         DVFilter_SendPkt(f->handle, pkt, direction);
      } else {
         LOG(11, "Dropping one packet\n");
         DVFilter_FreePkt(pkt);
      }
   } else {
      DVFilter_SendPkt(f->handle, pkt, direction);
   }
   return;
}


/******************************************************************************
 *                                                                       */ /**
 * \brief ioctl request from the vmkernel
 *
 *
 ******************************************************************************
 */
static void
ProcessFilterIoctlRequest(DVFilterImpl filterImpl,
			            DVFilterIoctlRequestId requestId,
			            void *payload,
			            size_t payloadLen)
{
   filter *f = (filter *) filterImpl;
   Netx_Service_Info_t info;

   ASSERT(f->magic == FILTER_MAGIC);
   ASSERT(payload);

   LOG(0, "Received Ioctl request #%d: %sreply requested\n",
       requestId, requestId == DVFILTER_INVALID_IOCTL_REQUEST_ID ? "no " : "");

   if (payloadLen < sizeof(Netx_Notice_Msg_t) ) {
      //fprintf(stderr, "Payload len %d is < sizeof(Netx_Notice_Msg_t) %d . Hence Returning \n",
      //        payloadLen, sizeof(Netx_Notice_Msg_t));
      return;
   }

   Netx_Notice_Msg_t *msg = (Netx_Notice_Msg_t*)payload;

   switch(msg->notice){
      case NETX_NOTICE_SERVICE_INFO:
         LOG(14, "processing msg %d\n", msg->notice);

         if (payloadLen < sizeof(Netx_Notice_Msg_t) + sizeof(Netx_Service_Info_t)) {
            //fprintf(stderr, "Payload len %d is not correct for msg type . Hence Returning \n",
            //        payloadLen);
            return;
         }

         if (msg->payload_len != sizeof(Netx_Service_Info_t)) {
            fprintf(stderr, "Netx msg Payload len is incorrect.\n");
            return;
         }

         memcpy(&info, msg->payload, msg->payload_len);

         printf("Notification: Vendor template id is %s\n",info.vendor_template_id);
         printf("Notification: Vendor provided template id is %s\n",info.vendor_provided_template_id);

         break;

      default:
         LOG(14, "processing default\n");
         break;
   }

   /*
    * Only reply if the peer expects a response
    */

   if (requestId != DVFILTER_INVALID_IOCTL_REQUEST_ID) {
      char *s = (char *) payload;
      unsigned int i;

      for (i = 0; i < payloadLen && *s; i++) {
	  s[i] = toupper(s[i]);
      }

      DVFilter_SendFilterIoctlReply(f->handle,
			            requestId,
			            payload,
			            payloadLen);
   }
}

/******************************************************************************
 *                                                                       */ /**
 * \brief ioctl request from the vmkernel
 *
 *
 ******************************************************************************
 */
static void
ProcessSlowPathIoctlRequest(DVFilterIoctlRequestId requestId,
			              void *payload,
			              size_t payloadLen)
{
  /*
    * Echo the requested data right back to the sender.
    * Provides for a simple heartbeat reply.
    */
   if (payloadLen != sizeof(uint32_t)) {
      LOG(14, "Received slow path Ioctl Request %d\n", payloadLen);
   }
   DVFilter_SendSlowPathIoctlReply(requestId,
                                   payload,
                                   payloadLen);
}

/******************************************************************************
 *                                                                       */ /**
 * \brief cli message, common code used by the event management loops
 *
 *
 ******************************************************************************
 */
static int
CliChannelEvent(filterCliChannel *cliChannel)
{
   int readLen = sizeof(cliChannel->msg);
   char *dst = (char *)&(cliChannel->msg);
   int readVal;
   int ret = -1;
   Netx_Status_t ret_val;
   filter *f = NULL;
   int i = 0;
   int selectedFilter = -1;
   Netx_Flags_t svm_flags;
   svm_flags.nocallback = 1;

   LOG(14, "New data waiting on CLI channel\n");
   readVal = recv(cliChannel->cliChannelFd,
                  &dst[cliChannel->msgLength], readLen, MSG_WAITALL);
   if (readVal < 0) {
      fprintf(stderr, "Failed to read from CLI channel\n");
      close(cliChannel->cliChannelFd);
      cliChannel->cliChannelFd = INVALID_FD;
   } else if (readVal == 0) {
      LOG(14, "CLI client disconnected\n");
      close(cliChannel->cliChannelFd);
      cliChannel->cliChannelFd = INVALID_FD;
   } else {
     NetxCtlMsg_t recvd_msg;
     if (!((NetxCtlMsg_t*)dst)->filterName[0]) {
        fprintf(stderr, "\nError: filter name is null.\n");
        goto clean_exit;
     }
     strncpy(recvd_msg.filterName, ((NetxCtlMsg_t*)dst)->filterName ,
             sizeof(recvd_msg.filterName));
     recvd_msg.action = ((NetxCtlMsg_t*)dst)->action;

     LOG(14, "recvd msg filter name is %s\n", recvd_msg.filterName);
     LOG(14, "recvd msg action is %d\n", recvd_msg.action);

     for (i = 0; i < filterListCount; i++) {
        f = filterList[i];
        if (strncmp(f->filterName, recvd_msg.filterName,
           sizeof(f->filterName)) == 0) {
           selectedFilter = i;
           break;
        }
     }

     if (selectedFilter == -1) {
        fprintf(stderr, "Error: Unable to find filter %s. Exiting.\n",
               recvd_msg.filterName);
        goto clean_exit;
     }
     f = filterList[selectedFilter];
     LOG(0, "selected filter is %s\n",f->filterName);

      cliChannel->msgLength += readVal;
      ret = 0;

      if (cliChannel->msg.cmd == NETX_CTL_SET_DEFAULT_ACTION_ON_VMOTION) {
         // Set default vmotion action for this filter
         ret_val = Netx_SetDefaultConnAction(f->handle, recvd_msg.action);
         if (NETX_STATUS_OK != ret_val) {
            fprintf(stderr, "Failed to Set default conn Action err:%d\n",ret_val);
            goto clean_exit;
         }
      }
      if (cliChannel->msg.cmd == NETX_CTL_GET_SERVICE_PROFILE_ID) {
         // Get the service profile id for this filter
         ((internalData*)pvt_data_g)->id = 20;
         ((internalData*)pvt_data_g)->val = 20;

         ret_val = Netx_GetServiceProfileId(f->handle, pvt_data_g, pvt_data_g_size);
         if (NETX_STATUS_OK != ret_val) {
            fprintf(stderr, "Failed to Get service profile id err:%d\n",ret_val);
            goto clean_exit;
         }
      }
      if (cliChannel->msg.cmd == NETX_CTL_GET_CONNTRACK_COUNT) {
         // Get the number of connections for this filter
         ((internalData*)pvt_data_g)->id = 2;
         ((internalData*)pvt_data_g)->val = 2;

         ret_val = Netx_GetConnCount(f->handle, pvt_data_g, pvt_data_g_size);
         if (NETX_STATUS_OK != ret_val) {
            fprintf(stderr, "Failed to Get Connection Count err:%d\n",ret_val);
            goto clean_exit;
         }
      }
      if (cliChannel->msg.cmd == NETX_CTL_GET_CONNTRACK_ALL_BRIEF) {

         ((internalData*)pvt_data_g)->id = 5;
         ((internalData*)pvt_data_g)->val = 5;

         // Get the connections info brief for this filter
         ret_val = Netx_GetConnInfoBrief(f->handle, num_conn_g, pvt_data_g,
                                         pvt_data_g_size);
         if (NETX_STATUS_OK != ret_val) {
            fprintf(stderr, "Failed to Get Connection Info Brief:%d\n",ret_val);
            goto clean_exit;
         }
      }
      if (cliChannel->msg.cmd == NETX_CTL_GET_SERVICE_INFO) {
         // Get the service info for this filter
         ((internalData*)pvt_data_g)->id = 120;
         ((internalData*)pvt_data_g)->val = 120;

         ret_val = Netx_GetServiceInfo(f->handle, pvt_data_g, pvt_data_g_size);
         if (NETX_STATUS_OK != ret_val) {
            fprintf(stderr, "Failed to Get service info err:%d\n",ret_val);
            goto clean_exit;
         }
      }
      if (cliChannel->msg.cmd == NETX_CTL_SET_CONNTRACK_ACTION_BRIEF) {

         if (!conn_set_g) {
            fprintf(stderr, "Cannot set action. No Connection Information.\n");
            fprintf(stderr, "Please execute GetConnInfo first to collect the"
                   " connection information and then try again.\n");
            goto clean_exit;
         }

         //Fill 5 tuple to identify connections to act on & the action to do.
         //For sample, set action to DROP for every connection
         //retrieved from GET call.
        Netx_Conn_Info_Brief_t *set_cib = conn_set_g;

        for (i=0; i<num_conn_set_g; i++) {
           set_cib->action = recvd_msg.action;
           set_cib++;
        }

         ((internalData*)pvt_data_g)->id = 10;
         ((internalData*)pvt_data_g)->val = 10;


         // Set the connections info brief for this filter
         ret_val = Netx_SetConnInfoBrief(f->handle,
                                         conn_set_g,
                                         conn_set_g_size,
                                         num_conn_set_g,
                                         svm_flags,
                                         pvt_data_g,
                                         pvt_data_g_size);
         if (NETX_STATUS_OK != ret_val) {
            fprintf(stderr, "Failed to Set Connection Info Brief:%d\n",ret_val);
            goto clean_exit;
         }

         //Free memory
         free(conn_set_g);
         conn_set_g = NULL;
      }
   }

clean_exit:
   return ret;
}

void stop_netx_service() {
    running_ = 0;
}

/******************************************************************************
 *                                                                       */ /**
 * \brief sample select event processing loop
 *
 *
 ******************************************************************************
 */
static void
selectEventLoop(int cliChannelServerFd)
{
   fd_set readSet;
   fd_set writeSet;
   filterCliChannel cliChannel;

   FD_ZERO(&readSet);
   FD_ZERO(&writeSet);
   InitCliChannel(&cliChannel);

   running_ = 1;

   while (1) {
      int num;
      int maxFd = -1;
      int cliChannelFd;
      int res;

      // Temp cliChannelFd is populated at beginning of each iteration.
      cliChannelFd = cliChannel.cliChannelFd;
      maxFd = DVFilter_AddReadyReadFdset(&readSet, maxFd);

      // If we currently have a client connected, we do not care for new ones
      if (cliChannelFd == INVALID_FD) {
         FD_SET(cliChannelServerFd, &readSet);
         if (cliChannelServerFd > maxFd) {
            maxFd = cliChannelServerFd;
         }
      } else {
         /*
          * Clear cliChannelServerFd from readSet to
          * avoid accept another CLI client when we already have one.
          */
         FD_CLR(cliChannelServerFd, &readSet);

         FD_SET(cliChannelFd, &readSet);
         if (cliChannelFd > maxFd) {
            maxFd = cliChannelFd;
         }
      }

      maxFd = DVFilter_AddReadyWriteFdset(&writeSet, maxFd);

      num = select(maxFd + 1, &readSet, &writeSet, NULL, NULL);
      if(!running_)
         break;
      if (num > 0) {
         res = DVFilter_ProcessSelectEvent(&readSet, &writeSet, maxFd, num);
         if (res == DVFILTER_STATUS_DISCONNECT) {
            LOG(0, "DVFilterlib reported that it disconnected\n");
            return;
         }

         if ((cliChannelFd != INVALID_FD) &&
             (FD_ISSET(cliChannelFd, &readSet) != 0)) {
            LOG(14, "Something new on the DVFilter control socket\n");
            if (CliChannelEvent(&cliChannel) == -1) {
               // Close cli channel fd, and clear it from readSet
               close(cliChannelFd);
               FD_CLR(cliChannelFd, &readSet);
               // reset cliChannel data structure
               InitCliChannel(&cliChannel);
            }
         }
         if ((cliChannelFd == INVALID_FD) &&
             (FD_ISSET(cliChannelServerFd, &readSet) != 0)) {
            LOG(14, "CLI channel server socket has something new to read\n");
            /*
             * note: its possible for multiple accepts to appear
             * simultaneously, but since the listen depth is only
             * set to one, this shouldn't happen
             */
            // Reset cliChannel data structure
            InitCliChannel(&cliChannel);

            cliChannelFd = accept(cliChannelServerFd, NULL, NULL);
            if (cliChannelFd < 0) {
               cliChannelFd = INVALID_FD;
               fprintf(stderr, "CLI channel could not be established\n");
            }
            cliChannel.cliChannelFd = cliChannelFd;
         }
      } else if (num == 0) {
         DVFilter_ProcessSelectEvent(NULL, NULL, 0, 0);
      } else if (num < 0) {
         if (errno == EINTR) {
            DVFilter_ProcessSelectEvent(NULL, NULL, 0, 0);
         } else {
            fprintf(stderr, "select() returned with error\n");
         }
      }
   }
}

/******************************************************************************
 * \brief main
 ******************************************************************************
 */
int
start_netx_service(const char *AgentName, PacketCbPtr ptr, void *p)
{
   static DVFilterSlowPathOps ops;

   ops.createFilter =         CreateFilter,
   ops.destroyFilter =        DestroyFilter,
   ops.getSavedStateLen =     GetSavedStateLen,
   ops.saveState =            SaveState,
   ops.restoreState =         RestoreState,
   ops.processPacket =        ProcessPacket,
   ops.filterIoctlRequest =   ProcessFilterIoctlRequest,
   ops.slowPathIoctlRequest = ProcessSlowPathIoctlRequest,
   ops.configurationChanged = ConfigurationChanged,

   pvt_data_g = malloc(pvt_data_g_size);

   Netx_Status_t ret_val;

   DVFilterStatus status;
   int cliChannelServerFd = INVALID_FD;
   int ret;
   int optVal;
   struct sockaddr_in addr;
   uint16_t port = 5000;
   uint32_t capabilities = DVFILTER_SLOWPATH_CAP_CSUM |
                           DVFILTER_SLOWPATH_CAP_TSO;
   uint32_t acceptedCapabilities;

   obj = p;
   ptrcb = ptr;
/*
   static struct option long_options[] = {
      {"agent", 1, 0, 'a'},
      {"help", 0, 0, 'h'},
      {"loglevel", 1, 0, 'l'},
      {"default-action", 1, 0, 'd'},
      {0, 0, 0, 0},
   };

   progName = argv[0];

   LOG(0, "\nStart Netx sample service Logs.\n");
   LOG(0, "\n================================\n");

   // Parse command line parameters
   while (1) {
      opt = getopt_long (argc, argv, "a:hl:d:",
                         long_options, &option_index);
      if (opt == -1)
         break;

      switch (opt) {
      case 'a':
         AgentName = optarg;
         LOG(0, "\nagent name is %s .\n",AgentName);
         break;
      case 'h':
         usage();
         return 0;
      case 'l':
         logLevel = getNum(optarg);
         break;
      case 'd':
         LOG(0, "\ndefault action is %s.\n",optarg);
         if (strncmp(optarg,"ACTION_COPY",sizeof("ACTION_COPY")) == 0) {
             g_action = ACTION_COPY;
         }
         break;
      default:
         usage();
         return 1;
      }
   }
*/
   g_action = ACTION_COPY;
   // Initialize the filter list
   filterListCount = 0;
   filterListSize = 100;
   filterList = (filter **) malloc(sizeof(filter*) * filterListSize);
   if (filterList == NULL) {
      fprintf(stderr, "Failed to initialize filter list\n");
      return DVFILTER_STATUS_ERROR;
   }


   // Register with the DVFilter fast path agent
   status = DVFilter_Init(AgentName,
                          DVFILTER_SLOWPATH_REVISION_NUMBER,
                          &ops, capabilities, &acceptedCapabilities);

   if (status != DVFILTER_STATUS_OK) {
      fprintf(stderr, "Failed to initialize the DVFilter library\n");
      goto filter_cleanup;
   }

   LOG(0, "negotiated capabilities: 0x%x\n", acceptedCapabilities);


   static Netx_LibHandle_t netx_lib_hdl;
      netx_lib_hdl.Netx_LibReply_Callback = Netx_LibReply_Callback;
   //netxVer.major = NETX_LIB_VERSION_MAJOR;
   //netxVer.minor = NETX_LIB_VERSION_MINOR;
   //netx_lib_hdl.version = netxVer;

   // Initialize the Netx library
   ret_val = Netx_LibInit(&netx_lib_hdl);
   if (NETX_STATUS_OK != ret_val) {
      fprintf(stderr, "Failed to initialize the Netx library err:%d\n",ret_val);
      status = DVFILTER_STATUS_ERROR;
      goto filter_cleanup;
   }

   // Open server socket for the CLI channel
   cliChannelServerFd = socket(PF_INET, SOCK_STREAM, 0);
   if (cliChannelServerFd < 0) {
      fprintf(stderr, "Failed to open CLI channel server socket\n");
      goto filter_cleanup;
   }

   optVal = 1;
   ret = setsockopt(cliChannelServerFd, SOL_SOCKET, SO_REUSEADDR,
                    &optVal, sizeof(optVal));
   if (ret < 0) {
      fprintf(stderr, "Failed to set sock opt SO_REUSEADDR\n");
      close(cliChannelServerFd);
      goto filter_cleanup;
   }

   memset(&addr, 0, sizeof(addr));
   addr.sin_family = AF_INET;
   addr.sin_port = htons(port);
   addr.sin_addr.s_addr = INADDR_ANY;

   ret = bind(cliChannelServerFd, (struct sockaddr*)&addr,
              sizeof(struct sockaddr_in));
   if (ret < 0) {
      fprintf(stderr, "Failed to bind CLI channel server socket\n");
      goto filter_cleanup;
   }

   if (listen(cliChannelServerFd, 1) < 0) {
      fprintf(stderr, "Failed to call listen() on CLI channel server socket\n");
      goto filter_cleanup;
   }

   LOG(0, "Entering main event loop ...\n");
   selectEventLoop(cliChannelServerFd);
   LOG(0, "Exiting ...\n");

filter_cleanup:

   DVFilter_Cleanup();

   if (filterList != NULL) {
      free(filterList);
   }

   if (pvt_data_g != NULL) {
      free(pvt_data_g);
   }

   return status;
}
