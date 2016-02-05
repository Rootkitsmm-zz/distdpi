/* **********************************************************
 * Copyright 2013, 2014 VMware, Inc.
 * All rights reserved. -- VMware Confidential
 * **********************************************************
 *
 * netx_lib_api.h
 *
 * Header file for Netx library APIs
 * This file is shared with Netx partners
 */

#ifndef __NETX_LIB_API_H__
#define __NETX_LIB_API_H__

/*
 * NetX public APIs.
 * This file defines the NetX APIs that are visible to a service VM.
 */

#include <netx_lib_types.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \defgroup netxlib Netx Library SDK
 *
 *  The Netx SDK provides a framework to communicate with the Vmware Service
 *  Insertion Platform kernel module present in ESXi via the Netx Library
 *  (libnetx). It is to be used in the development of Netx Data Plane Agent
 *  (DPA) in a Service Virtual Machine (SVM) developed by a ecosystem partner.
 *
 *  The Netx API's are asynchronous. The output for the Get APIs is available
 *  in the callback function that has been registered as part of the library
 *  handle.  The Netx DPA sends the request as a command/message to the
 *  VMkernel. The VMKernel executes the command and dispatches the result back
 *  to the DPA.  The NetX DPA uses the DVFilter framework and
 *  VMCI for communication  between the SVM and the VMkernel .
 *
 *  \{ \}
 */

/*
 * Library APIs
 */

/*
 ******************************************************************************
 *                                                                       */ /**
 * \ingroup netxlib
 * \brief Initialize the Netx Library.
 *
 * \param[in] lib Library handle defined by the application
 *
 * \retval NETX_STATUS_OK Successful initialization
 * \retval NETX_STATUS_INVALID_ARGUMENT  NULL argument passed
 * \retval NETX_STATUS_INVALID_LIBRARY_HANDLE Invalid Library handle passed;
 *                Netx Callback functions are not registered.
 * \retval NETX_STATUS_VSIPFW_INIT_FAIL Failed initializing internal library
 *                -Vsipfwlib
 *
 ******************************************************************************
 */
Netx_Status_t
Netx_LibInit(Netx_LibHandle_t *lib);

/*
 ******************************************************************************
 *                                                                       */ /**
 * \ingroup netxlib
 * \brief Free a NetX Library Handle and all resources.
 *
 * \param[in] lib Library handle defined by the application
 *
 * \retval NETX_STATUS_OK Successful exit freeing all resources
 * \retval NETX_STATUS_INVALID_ARGUMENT  NULL argument passed
 * \retval NETX_STATUS_VSIPFW_EXIT_FAIL Failed exiting vsipfwlib(internal lib)
 *
 ******************************************************************************
 */
Netx_Status_t
Netx_LibExit(Netx_LibHandle_t *lib);

/*
 * Utility Functions
 */

/*
 ******************************************************************************
 *                                                                       */ /**
 * \ingroup netxlib
 * \brief Print Brief Connection Information
 *
 * \param[in]  count Number of connections whose info is to be printed
 * \param[in]  conn_b One or many Netx_Conn_Info_Brief_t structure
 *             instances with filled in values.
 * \param[in]  buf_size Length of conn_b passed in
 * \param[out] out pointer to an output buffer which will have the formatted output. This
 *             will be malloced by the function and needs to be freed by the caller. On
 *             error, this will be NULL.
 *
 * \retval NETX_STATUS_OK Successful
 * \retval NETX_STATUS_INVALID_ARGUMENT  NULL argument passed for conn_b or
 *                      incorrect size for buf_size or
 *                      negative number or 0 passed for count
 *
 ******************************************************************************
 */
Netx_Status_t
Netx_PrintConnInfoBrief(int32_t count, Netx_Conn_Info_Brief_t *conn_b,
                       int32_t buf_size, char **out);

/*
 * Connection Tracker APIs - Asynchronous
 */

/*
 ******************************************************************************
 *                                                                       */ /**
 * \ingroup netxlib
 * \brief Get the number of connections on the filter
 *
 *    This API returns output asynchronously. The number will be available
 *    in the callback function registered in the netx library handle.
 *
 * \param[in] filter_handle Handle to the filter object
 * \param[in] pvt_data pointer to a private data that can be used like
 * a cookie. The caller can get the same data back when handling the return
 * callback.
 * \param[in] pvt_data_len Length of the pvt_data object
 *
 * \retval NETX_STATUS_OK Successful
 * \retval NETX_STATUS_INVALID_ARGUMENT  NULL argument passed
 * \retval NETX_STATUS_MALLOC_FAIL Failed to allocate memory for payload.
 * \retval NETX_STATUS_DVFILTER_SEND_IOCTL_FAIL Failed to send command payload
 *                      to VMkernel.
 *
 ******************************************************************************
 */
Netx_Status_t
Netx_GetConnCount(void *filter_handle,
                  void *pvt_data,
                  int32_t pvt_data_len);

/*
 ******************************************************************************
 *                                                                       */ /**
 * \ingroup netxlib
 * \brief Get brief info about the connections on the filter.
 *
 *   This API returns output asynchronously. The info will be available
 *   in the callback function registered in the netx library handle.
 *
 * \param[in] filter_handle Handle to the filter object
 * \param[in] num_conn_req Number of connections to fetch info about
 * \param[in] pvt_data Pointer to a private data that can be used like
 * a cookie. The caller can get the same data back when handling the return
 * callback.
 * \param[in] pvt_data_len Length of the pvt_data object
 *
 * \retval NETX_STATUS_OK Successful
 * \retval NETX_STATUS_INVALID_ARGUMENT  NULL argument passed or
 *                      num_conn_req/pvt_data_len is not greater than 0.
 * \retval NETX_STATUS_MALLOC_FAIL Failed to allocate memory for payload.
 * \retval NETX_STATUS_DVFILTER_SEND_IOCTL_FAIL Failed to send command payload
 *                      to VMkernel.
 *
 ******************************************************************************
 */
Netx_Status_t
Netx_GetConnInfoBrief(void *filter_handle,
                      int32_t num_conn_req,
                      void *pvt_data,
                      int32_t pvt_data_len);

/*
 ******************************************************************************
 *                                                                       */ /**
 * \ingroup netxlib
 * \brief Set brief info about the connections on the filter.
 *
 *    The action field for a connection can be filled in the conn_info structure
 *    and set using this API
 *
 * \param[in] filter_handle Handle to the filter object
 * \param[in] conn_info One or many Netx_Conn_Info_Brief_t structure
 *            instances with filled in values.
 * \param[in] conn_info_size Length of conn_info
 * \param[in] num_conn_set Number of connections to set info
 * \param[in] flags Pass flags input to the function
 * \param[in] pvt_data Pointer to a private data that can be used like
 * a cookie. The caller can get the same data back when handling the return
 * callback.
 * \param[in] pvt_data_len Length of the pvt_data object
 *
 * \retval NETX_STATUS_OK Successful
 * \retval NETX_STATUS_INVALID_ARGUMENT  NULL argument passed or
 *                      num_conn_set/pvt_data_len is not greater than 0.
 * \retval NETX_STATUS_MALLOC_FAIL Failed to allocate memory for payload.
 * \retval NETX_STATUS_DVFILTER_SEND_IOCTL_FAIL Failed to send command payload
 *                      to VMkernel.
 *
 ******************************************************************************
 */
Netx_Status_t
Netx_SetConnInfoBrief(void *filter_handle,
                     void *conn_info, /* Netx_Conn_Info_Brief_t */
                     int32_t conn_info_size,
                     int32_t num_conn_set,
                     Netx_Flags_t flags,
                     void *pvt_data,
                     int32_t pvt_data_len);

/*
 ******************************************************************************
 *                                                                       */ /**
 * \ingroup netxlib
 * \brief Set Default Flow Action for all connnections on the filter.
 *
 *    On Vmotion the default flow action is set for the flows.
 *
 * \param[in] filter_handle Handle to the filter object
 * \param[in] action Action to be set on the flows
 *
 * \retval NETX_STATUS_OK Successful
 * \retval NETX_STATUS_INVALID_ARGUMENT  NULL argument passed or
 *                    invalid value(neither DROP nor ACCEPT) passed for action.
 * \retval NETX_STATUS_MALLOC_FAIL Failed to allocate memory for payload.
 * \retval NETX_STATUS_DVFILTER_SEND_IOCTL_FAIL Failed to send command payload
 *                    to VMkernel.
 *
 ******************************************************************************
 */
Netx_Status_t
Netx_SetDefaultConnAction(void *filter_handle, Netx_Action_t action);

/*
 ******************************************************************************
 *                                                                       */ /**
 * \ingroup netxlib
 * \brief Get the Service ProfileID corresponding to the filter.
 *
 *    This API returns output asynchronously. The ID will be available in the
 *    callback function registered in the netx library handle.
 *
 * \param[in] filter_handle Handle to the filter object
 * \param[in] pvt_data pointer to a private data that can be used like
 * a cookie. The caller can get the same data back when handling the return
 * callback.
 * \param[in] pvt_data_len Length of the pvt_data object
 *
 * \retval NETX_STATUS_OK Successful
 * \retval NETX_STATUS_INVALID_ARGUMENT  NULL argument passed
 * \retval NETX_STATUS_MALLOC_FAIL Failed to allocate memory for payload.
 * \retval NETX_STATUS_DVFILTER_SEND_IOCTL_FAIL Failed to send command payload
 *                      to VMkernel.
 *
 ******************************************************************************
 */
Netx_Status_t
Netx_GetServiceProfileId(void *filter_handle,
                         void *pvt_data,
                         int pvt_data_len);

/*
 ******************************************************************************
 *                                                                       */ /**
 * \ingroup netxlib
 * \brief Get the Service Information corresponding to the filter.
 *
 *    This API returns output asynchronously. The info will be available in the
 *    callback function registered in the netx library handle.
 *
 * \param[in] filter_handle Handle to the filter object
 * \param[in] pvt_data pointer to a private data that can be used like
 * a cookie. The caller can get the same data back when handling the return
 * callback.
 * \param[in] pvt_data_len Length of the pvt_data object
 *
 * \retval NETX_STATUS_OK Successful
 * \retval NETX_STATUS_INVALID_ARGUMENT  NULL argument passed
 * \retval NETX_STATUS_MALLOC_FAIL Failed to allocate memory for payload.
 * \retval NETX_STATUS_DVFILTER_SEND_IOCTL_FAIL Failed to send command payload
 *                      to VMkernel.
 *
 ******************************************************************************
 */
Netx_Status_t
Netx_GetServiceInfo(void *filter_handle,
                    void *pvt_data,
                    int pvt_data_len);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __NETX_LIB_API_H__ */
