/* **********************************************************
 * Copyright 2013,2014 VMware, Inc.
 * All rights reserved. -- VMware Confidential
 * **********************************************************/

/**
 * netx_lib_types.h - Header file for Netx library
 * This file is shared with Netx partners
 */

#ifndef __NETX_LIB_TYPES_H__
#define __NETX_LIB_TYPES_H__

//Data Types
#if defined(NETX_SDK_BUILD)
#include <stdint.h>
typedef uint64_t      vmk_uint64;
typedef uint32_t      vmk_uint32;
typedef uint16_t      vmk_uint16;
typedef uint8_t       vmk_uint8;
#endif /* NETX_SDK_BUILD */

//Netx Limits
#ifndef NETX_IP_SIZE
#define NETX_IP_SIZE   (16)
#endif

#ifndef NETX_ID_SIZE
#define NETX_ID_SIZE      (63)
#endif

#ifndef NETX_VMUUID_SIZE
#define NETX_VMUUID_SIZE     (128)
#endif

#ifndef NETX_CTL_STR_SIZE
#define NETX_CTL_STR_SIZE   (128)
#endif

#ifndef NETX_SP_PROFILE_ID_SIZE_MAX
#define NETX_SP_PROFILE_ID_SIZE_MAX 31
#endif

#ifndef PACKED
#define PACKED __attribute__((__packed__))
#endif

/*
 ***********************************************************************
 *                                                                */ /**
 * \ingroup netxlib
 * \struct Netx_LibHandle_t
 * \brief Netx Library handle
 *
 ***********************************************************************
 */
/*
 * NetX Lib Handle
 */
typedef struct netxlibhandle_ {

   /********************************************************************
    *                                                             */ /**
    * \brief Reply Callbacks
    *
    *    This callback is invoked by netxlib to provide response to the
    *    requests that the application makes using NETX opcodes.
    *
    * \param[in]  filter Handle to the filter object
    * \param[out]  callback_data opaque pointer to the app's data obj
    * \param[in]  payload pointer to payload data
    * \param[in]  payload_len length of payload data
    *
    ********************************************************************
    */
   void (*Netx_LibReply_Callback) (void *filter, void *callback_data,
                                   void *payload, size_t payload_len);
} Netx_LibHandle_t;

/*
 * NetX API return status
 */
typedef enum {
   NETX_STATUS_OK = 0,

    /* Generic errors */
   NETX_STATUS_MALLOC_FAIL,
   NETX_STATUS_DVFILTER_SEND_IOCTL_FAIL,
   NETX_STATUS_VSIPFW_INIT_FAIL,
   NETX_STATUS_VSIPFW_EXIT_FAIL,
   NETX_STATUS_CANNOT_LOAD_FILE,
   NETX_STATUS_EVAL_CFG_FAIL,
   NETX_STATUS_CANNOT_FREE_FILE,
   NETX_STATUS_LIBRARY_NOT_INITIALIZED,
   NETX_STATUS_INVALID_LIBRARY_HANDLE,
   NETX_STATUS_INVALID_ARGUMENT,
   NETX_STATUS_GET_CONN_COUNT_FAIL,
   NETX_STATUS_GET_CONN_INFO_FAIL,
   NETX_STATUS_DVFILTER_CALLBACK_ERROR,
   NETX_STATUS_UNKNOWN_ERROR,
   NETX_STATUS_INTERNAL_ERROR = NETX_STATUS_UNKNOWN_ERROR,
   NETX_STATUS_NOT_SUPPORTED,

   NETX_STATUS_MAX_ERROR,
} Netx_Status_t;

/*
 ***********************************************************************
 *                                                                */ /**
 * \ingroup netxlib
 * \struct Netx_Flags_t
 * \brief Structure to pass flags to Netx APIs
 *
 ***********************************************************************
 */
/*
 * Netx API Flags - for passing any flags to turn on/off portions of the
 * function
 */
typedef struct {
   vmk_uint8 nocallback; /**< Toggle to set whether the caller needs callback or not.
                           * 1 - no callback, 0 - callback */
} Netx_Flags_t;

/* The direction enum values must match direction enum defined in vmkernel */
typedef enum {
   NETX_INOUT = 0,
   NETX_IN = 1,
   NETX_OUT = 2
} Netx_Direction_t;

typedef enum {
   NETX_ACTION_INVALID = 0,
   NETX_ACTION_PENDING_DROP = 1,
   NETX_ACTION_DROP = 2,
   NETX_ACTION_ACCEPT = 3
} Netx_Action_t;

/*
 * Commands
 */
typedef enum {
   NETXCMD_UNKNOWN = 1,
   NETXCMD_GET_CONNTRACK_COUNT,
   NETXCMD_GET_CONNTRACK_ALL,
   NETXCMD_GET_CONNTRACK_ALL_BRIEF,
   NETXCMD_GET_SERVICE_PROFILE_ID,
   NETXCMD_SET_RULES,
   NETXCMD_SET_CONNTRACK_ACTION,
   NETXCMD_SET_CONNTRACK_ACTION_BRIEF,
   NETXCMD_SET_DEFAULT_ACTION_ON_VMOTION,
   NETXCMD_GET_SERVICE_INFO,
   NETXCMD_GET_CONNTRACK_BY_ID,
   NETXCMD_MAX_CMD
} Netx_Cmd_t;

/* Protocols */
typedef enum {
    VSIPW_DPI_PROTO_BASE = 0,
    VSIPW_DPI_PROTO_IP = 1,
    VSIPW_DPI_PROTO_TCP = 2,
    VSIPW_DPI_PROTO_HTTP = 3,
    VSIPW_DPI_PROTO_SSL = 4,
    VSIPW_DPI_PROTO_SSH = 5,
    VSIPW_AW_PROTO = 6,
} FWRuleDPIProto_t;

/* Protocol Attributes */
typedef enum {
   VSIPFW_DPI_SSL_VERSION = 0,
   VSIPFW_DPI_SSL_SESSION_ID = 1,
} FWRuleDPISSLAttr_t;

typedef enum {
   VSIPW_DPI_SSH_VERSION = 1,
} FWRuleDPISSHAttr_t;

typedef enum {
   VSIPW_DPI_HTTP_CONTENT = 0,
   VSIPW_DPI_HTTP_URI = 1,
} FWRuleDPIHttpAttr_t;

/* Protocol Attribute Values */
typedef enum {
   VSIPFW_DPI_SSL_VERSION_V1 = 0,
   VSIPFW_DPI_SSL_VERSION_V2 = 1,
   VSIPFW_DPI_SSL_VERSION_V3 = 2,
} FWRuleDPISSLAttrVal_t;

typedef enum {
   VSIPFW_DPI_SSH_VERSION_1 = 0,
   VSIPFW_DPI_SSH_VERSION_2 = 3,
} FWRuleDPISSHAttrVal_t;

typedef struct pf_rule_dpi_attr_value {
    vmk_uint32              op:1;
    vmk_uint32              type:2;
    vmk_uint32              datalen:8;
    vmk_uint32              reserved:21;
    vmk_uint32              value;
} pf_rule_dpi_attr_value_t;

typedef struct pf_rule_dpi_attr {
    vmk_uint32                      attrid:16;
    vmk_uint32                      attrnum:16;
    pf_rule_dpi_attr_value_t       data[8];
} pf_rule_dpi_attr_t;

typedef struct pf_rule_dpi_attr_list {
    vmk_uint8               num;
} pf_rule_dpi_attr_list_t;

typedef struct pf_rule_dpi {
    vmk_uint32              len;
    vmk_uint32              value[8];
    vmk_uint32              attr_offset;
    vmk_uint32              totlen;
    pf_rule_dpi_attr_list_t *attrlist;
} pf_rule_dpi_t;

typedef struct {
   vmk_uint64 conn_id;
   vmk_uint8 protocol;
   vmk_uint8 af;
   vmk_uint8 src_ip[NETX_IP_SIZE];
   vmk_uint8 dst_ip[NETX_IP_SIZE];
   vmk_uint16 src_port;
   vmk_uint16 dst_port;
   Netx_Direction_t direction;
   Netx_Action_t action;
   pf_rule_dpi_t dpi;
   pf_rule_dpi_attr_t attr;
} PACKED Netx_Conn_Info_Brief_t;

typedef struct {
   char vendor_template_id[NETX_ID_SIZE+1];
   char vendor_provided_template_id[NETX_ID_SIZE+1];
} Netx_Service_Info_t;

typedef enum {
   NETX_NOTICE_UNKNOWN = 1,
   NETX_NOTICE_SERVICE_INFO,
   NETX_NOTICE_MAX
} Netx_Notice_t;

typedef struct {
   Netx_Notice_t notice;
   vmk_uint32 payload_len;
   char payload[0]; /*Depending on Cmd*/
} Netx_Notice_Msg_t;

/**
 * IOCTL structures
 */
typedef char Netx_Msg;

typedef struct {
   Netx_Cmd_t cmd;
   vmk_uint32 count;
   vmk_uint32 payload_len;
} PACKED Netx_Msg_Hdr_t;

typedef struct {
   vmk_uint32 count;
} PACKED Netx_Conn_Count_t;

typedef struct {
   Netx_Msg_Hdr_t hdr;
   Netx_Msg payload[0]; /* Netx_Conn_Count_t */
} PACKED Netx_Conn_Count_Data_t;

typedef struct {
   Netx_Msg_Hdr_t hdr;
   Netx_Action_t action;
} PACKED Netx_Vmotion_Action_t;

typedef struct {
   Netx_Msg_Hdr_t hdr;
   Netx_Msg sprofile_id[NETX_SP_PROFILE_ID_SIZE_MAX+1];
} PACKED Netx_SP_t;

typedef struct {
   Netx_Msg_Hdr_t hdr;
   vmk_uint32 count;
   vmk_uint64 max_id; /* Get "count" num of conn with conn_id > max_id */
   Netx_Msg payload[0]; /* Netx_Conn_Info_t or Netx_Conn_Info_Brief_t */
} PACKED Netx_Conn_Data_t;

typedef struct {
   Netx_Msg_Hdr_t hdr;
   vmk_uint32 count;
   Netx_Msg payload[0]; /* Netx_Conn_Info_t */
} PACKED Netx_Conn_Set_t;

typedef struct {
   Netx_Msg_Hdr_t hdr;
   vmk_uint32 count;
   Netx_Msg payload[0]; /* Netx_Conn_Info_Brief_t */
} PACKED Netx_Conn_Set_Brief_t;

typedef struct {
   Netx_Msg_Hdr_t hdr;
   Netx_Service_Info_t info;
} Netx_Service_Data_t;

/**
 * Structures for messages from netx_ctl to dataplane agent
 */
typedef struct netx_ctl_msg_ {
   char filterName[NETX_CTL_STR_SIZE];
   Netx_Action_t action;
   uint32_t cmd;
} NetxCtlMsg_t;

typedef struct netx_ctl_reply_ {
   uint32_t status;
} NetxCtlReply_t;

/*
 * Commands to trigger APIs from Netx Ctl
 */
typedef enum {
   NETX_CTL_UNKNOWN = 1,
   NETX_CTL_GET_CONNTRACK_COUNT,
   NETX_CTL_GET_CONNTRACK_ALL,
   NETX_CTL_GET_CONNTRACK_ALL_BRIEF,
   NETX_CTL_GET_SERVICE_PROFILE_ID,
   NETX_CTL_SET_CONNTRACK_ACTION,
   NETX_CTL_SET_CONNTRACK_ACTION_BRIEF,
   NETX_CTL_SET_DEFAULT_ACTION_ON_VMOTION,
   NETX_CTL_GET_SERVICE_INFO,
   NETX_CTL_MAX_CMD
} Netx_Ctl_Cmd_t;

#endif /* __NETX_LIB_TYPES_H__ */
