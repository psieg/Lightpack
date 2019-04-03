/**
 * \file This file contains definitions for NVFBC API.
 * \copyright
 *
 * Copyright 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 *
 */

#pragma once

typedef unsigned char NvU8;
typedef unsigned long NvU32;
typedef unsigned long long NvU64;

/**
 * \defgroup NVFBC The NVIDIA Frame Buffer Capture API.
 * \brief Defines a set of interfaces for high performance Capture of desktop content.
 */

/**
 * \defgroup NVFBC_ENUMS Enums
 * \ingroup NVFBC
 * \brief Enumerations to be used with NVFBC API
 */

/**
 * \defgroup NVFBC_STRUCTS Structs
 * \ingroup NVFBC
 * \brief Defines Parameter Structures to be used with NVFBC APIs.
 */

/**
 * \defgroup NVFBC_ENTRYPOINTS Entrypoints
 * \ingroup NVFBC
 * \brief Declarations for NVFBC Entrypoint functions
 */

/**
 * \ingroup NVFBC
 * Macro to define the NVFBC API version corresponding to this distribution.
 */
#define NVFBC_DLL_VERSION 0x50

/**
 * \ingroup NVFBC
 * Macro to construct version numbers for parameter structs.
 */
#define NVFBC_STRUCT_VERSION(typeName, ver) (NvU32)(sizeof(typeName) | ((ver)<<16) | (NVFBC_DLL_VERSION << 24))

/**
 * \ingroup NVFBC
 * Calling Convention
 */
#define NVFBCAPI __stdcall

/**
 * \ingroup NVFBC
 * Indicates that there are no global overrides specified for NVFBC. To be used with NVFBC_SetGlobalFlags API
 */
#define NVFBC_GLOBAL_FLAGS_NONE                     0x00000000

/**
 * \ingroup NVFBC
 * Indicates to NVFBC that stereo rendering is enabled. Currently unsupported. To be used with NVFBC_SetGlobalFlags API.
 */
#define NVFBC_GLOBAL_FLAGS_STEREO_BUFFER            0x00000001

/**
 * \ingroup NVFBC
 * Indicates that NVFBC should not request a repaint of the desktop when initiating NVFBC capture. To be used with NVFBC_SetGlobalFlags API.
 */
#define NVFBC_GLOBAL_FLAGS_NO_INITIAL_REFRESH       0x00000002

/**
 * \ingroup NVFBC
 * Indicates that NVFBC should not reset the graphics driver while servicing subsequent NVFBC_Enable API requests.
*/

#define NVFBC_GLOBAL_FLAGS_NO_DEVICE_RESET_TOGGLE   0x00000004

/**
 * \ingroup NVFBC_ENUMS
 * \brief Enumerates status codes returned by NVFBC APIs.
 */
typedef enum _NVFBCRESULT
{
    NVFBC_SUCCESS = 0,
    NVFBC_ERROR_GENERIC = -1,                     /**< Unexpected failure in NVFBC. */
    NVFBC_ERROR_INVALID_PARAM = -2,               /**< One or more of the paramteres passed to NvFBC are invalid [This include NULL pointers]. */
    NVFBC_ERROR_INVALIDATED_SESSION = -3,         /**< NvFBC session is invalid. Client needs to recreate session. */
    NVFBC_ERROR_PROTECTED_CONTENT = -4,           /**< Protected content detected. Capture failed. */
    NVFBC_ERROR_DRIVER_FAILURE = -5,              /**< GPU driver returned failure to process NvFBC command. */
    NVFBC_ERROR_CUDA_FAILURE   = -6,              /**< CUDA driver returned failure to process NvFBC command. */
    NVFBC_ERROR_UNSUPPORTED    = -7,              /**< API Unsupported on this version of NvFBC. */
    NVFBC_ERROR_HW_ENC_FAILURE  = -8,             /**< HW Encoder returned failure to process NVFBC command. */
    NVFBC_ERROR_INCOMPATIBLE_DRIVER = -9,         /**< NVFBC is not compatible with this version of the GPU driver. */
    NVFBC_ERROR_UNSUPPORTED_PLATFORM = -10,       /**< NVFBC is not supported on this platform. */
    NVFBC_ERROR_OUT_OF_MEMORY  = -11,             /**< Failed to allocate memory. */
    NVFBC_ERROR_INVALID_PTR    = -12,             /**< A NULL pointer was passed. */
    NVFBC_ERROR_INCOMPATIBLE_VERSION = -13,       /**< An API was called with a parameter struct that has an incompatible version. Check dwVersion field of paramter struct. */
    NVFBC_ERROR_OPT_CAPTURE_FAILURE = -14,        /**< Desktop Capture failed. */
    NVFBC_ERROR_INSUFFICIENT_PRIVILEGES  = -15,   /**< User doesn't have appropriate previlages. */
    NVFBC_ERROR_INVALID_CALL = -16,               /**< NVFBC APIs called in wrong sequence. */
    NVFBC_ERROR_SYSTEM_ERROR = -17,               /**< Win32 error. */
    NVFBC_ERROR_INVALID_TARGET = -18,             /**< The target adapter idx can not be used for NVFBC capture. It may not correspond to an NVIDIA GPU, or may not be attached to desktop. */
    NVFBC_ERROR_DYNAMIC_DISABLE = -20,            /**< NvFBC is dynamically disabled. Cannot continue to capture */
} NVFBCRESULT;

/**
 * \ingroup NVFBC_ENUMS
 * \brief Enumerates NVFBC states. To be used with NvFBC_Enable API
 */
typedef enum _NVFBC_STATE
{
    NVFBC_STATE_DISABLE          = 0,   /** Disables NvFBC. */
    NVFBC_STATE_ENABLE              ,   /** Enables NvFBC. */
    NVFBC_STATE_LAST                ,   /** Sentinel value. Shouldn't be used. */
} NVFBC_STATE;

/**
 * \ingroup NVFBC_STRUCTS
 * \brief Defines parameters that describe the grabbed data, and provides detailed information about status of the NVFBC session.
 */
typedef struct _NvFBCFrameGrabInfo
{
    DWORD   dwWidth;                /**< [out] Indicates the current width of captured buffer. */
    DWORD   dwHeight;               /**< [out] Indicates the current height of captured buffer. */
    DWORD   dwBufferWidth;          /**< [out] Indicates the current width of the pixel buffer(padded width). */
    DWORD   dwReserved;             /**< [in] Reserved, do not use. */
    BOOL    bOverlayActive;         /**< [out] Is set to 1 if overlay was active. */
    BOOL    bMustRecreate;          /**< [out] Is set to 1 if the compressor must call NvBFC_Create again. */
    BOOL    bFirstBuffer;           /**< [out] Is set to 1 is this was the first capture call, or first call after a desktop mode change.
                                               Relevant only for XOR and diff modes supported by NVFBCToSys interface. */
    BOOL    bHWMouseVisible;        /**< [out] Is set to 1 if HW cursor was enabled by OS at the time of the grab. */
    BOOL    bProtectedContent;      /**< [out] Is set to 1 if protected content was active (DXVA encryption Session). */
    DWORD   dwDriverInternalError;  /**< [out] Indicates the status code from lower layers. 0 or 0xFBCA11F9 indicates no error was returned. */
    BOOL    bStereoOn;              /**< [out] Is set to 1 if stereo was on. */
    BOOL    bIGPUCapture;           /**< [out] Is set to 1 if the captured frame is from iGPU. 0 if capture fails or if captured from dGPU*/
    DWORD   dwSourcePID;            /**< [out] Indicates which process caused the last screen update that got grabbed*/
    DWORD   dwReserved3;            /**< [in] Reserved, do not use. */
    NvU32   dwReserved2[13];        /**< [in] Resereved, should be set to 0. */
} NvFBCFrameGrabInfo;

/**
 * \ingroup NVFBC_STRUCTS
 * \brief Deines the parameters to be used with NvFBC_GetStatusEx API
 */
typedef struct _NvFBCStatusEx
{
    NvU32  dwVersion;              /**< [in]  Struct version. Set to NVFBC_STATUS_VER. */
    NvU32  bIsCapturePossible :1;  /**< [out] Indicates if NvFBC feature is enabled. */
    NvU32  bCurrentlyCapturing:1;  /**< [out] Indicates if NVFBC is currently capturing for the Adapter ordinal specified in dwAdapterIdx. */
    NvU32  bCanCreateNow      :1;  /**< [out] Deprecated. Do not use. */
    NvU32  bSupportMultiHead  :1;  /**< [out] MultiHead grab supported. */
    NvU32  bSupportMultiClient:1;  /**< [out] Multiple capture clients on same display adapter supported. */
    NvU32  bReservedBits      :27; /**< [in]  Reserved, do not use. */
    NvU32  dwNvFBCVersion;         /**< [out] Indicates the highest NvFBC interface version supported by the loaded NVFBC library. */
    NvU32  dwAdapterIdx;           /**< [in]  Adapter Ordinal corresponding to the display to be grabbed. IGNORED if bCapturePID is set */
    void*  pPrivateData;           /**< [in]  optional **/
    NvU32  dwPrivateDataSize;      /**< [in]  optional **/
    NvU32  dwReserved[59];         /**< [in]  Reserved. Should be set to 0. */
    void*  pReserved[31];          /**< [in]  Reserved. Should be set to NULL. */
} NvFBCStatusEx;
#define NVFBC_STATUS_VER_1  NVFBC_STRUCT_VERSION(NvFBCStatusEx, 1)
#define NVFBC_STATUS_VER_2  NVFBC_STRUCT_VERSION(NvFBCStatusEx, 2)
#define NVFBC_STATUS_VER    NVFBC_STATUS_VER_2

/**
 * \ingroup NVFBC_STRUCTS
 * \brief Defines the parameters to be used with NvFBC_CreateEx API.
 */
typedef struct _NvFBCCreateParams
{
    NvU32  dwVersion;              /**< [in]  Struct version. Set to NVFBC_CREATE_PARAMS_VER. */
    NvU32  dwInterfaceType;        /**< [in]  ID of the NVFBC interface Type being requested. */
    NvU32  dwMaxDisplayWidth;      /**< [out] Max. display width allowed. */
    NvU32  dwMaxDisplayHeight;     /**< [out] Max. display height allowed. */
    void*  pDevice;                /**< [in]  Device pointer. */
    void*  pPrivateData;           /**< [in]  Private data [optional].  */
    NvU32  dwPrivateDataSize;      /**< [in]  Size of private data. */
    NvU32  dwInterfaceVersion;     /**< [in]  Version of the capture interface. */
    void*  pNvFBC;                 /**< [out] A pointer to the requested NVFBC object. */
    NvU32  dwAdapterIdx;           /**< [in]  Adapter Ordinal corresponding to the display to be grabbed. If pDevice is set, this parameter is ignored. */
    NvU32  dwNvFBCVersion;         /**< [out] Indicates the highest NvFBC interface version supported by the loaded NVFBC library. */
    void*  cudaCtx;                /**< [in]  CUDA context created using cuD3D9CtxCreate with the D3D9 device passed as pDevice. Only used for NvFBCCuda interface.
                                              It is mandatory to pass a valid D3D9 device if cudaCtx is passed. The call will fail otherwise.
                                              Client must release NvFBCCuda object before destroying the cudaCtx. */
    void*  pPrivateData2;           /**< [in]  Private data [optional].  */
    NvU32  dwPrivateData2Size;      /**< [in]  Size of private data. */
    NvU32  dwReserved[55];         /**< [in]  Reserved. Should be set to 0. */
    void*  pReserved[27];          /**< [in]  Reserved. Should be set to NULL. */
}NvFBCCreateParams;
#define NVFBC_CREATE_PARAMS_VER_1 NVFBC_STRUCT_VERSION(NvFBCCreateParams, 1)
#define NVFBC_CREATE_PARAMS_VER_2 NVFBC_STRUCT_VERSION(NvFBCCreateParams, 2)
#define NVFBC_CREATE_PARAMS_VER NVFBC_CREATE_PARAMS_VER_2

/**
* \ingroup NVFBC_STRUCTS
* \brief Defines parameters for a Grab\Capture call to get HW cursor data in the NVFBCToSys capture session.
*/
typedef struct
{
    NvU32 dwVersion;                         /**< [in]:  Struct version. Set to NVFBC_MOUSE_GRAB_INFO_VER.*/
    NvU32 dwWidth;                           /**< [out]: Width of mouse glyph captured.*/
    NvU32 dwHeight;                          /**< [out]: Height of mouse glyph captured.*/
    NvU32 dwPitch;                           /**< [out]: Pitch of mouse glyph captured.*/
    NvU32 bIsHwCursor : 1;                   /**< [out]: Tells if cursor is HW cursor or SW cursor. If set to 0, ignore height, width, pitch and pBits.*/
    NvU32 bReserved : 32;                    /**< [in]:  Reserved.*/
    NvU32 dwPointerFlags;                    /**< [out]: Maps to DXGK_POINTERFLAGS::Value.*/
    NvU32 dwXHotSpot;                        /**< [out]: Maps to DXGKARG_SETPOINTERSHAPE::XHot.*/
    NvU32 dwYHotSpot;                        /**< [out]: Maps to DXGKARG_SETPOINTERSHAPE::YHot.*/
    NvU32 dwUpdateCounter;                   /**< [out]: Cursor update Counter. */
    NvU32 dwBufferSize;                      /**< [out]: Size of the buffer contaiing the captured cursor glyph. */
    void * pBits;                            /**< [out]: pointer to buffer containing the captured cursor glyph.*/
    NvU32 dwReservedA[22];                   /**< [in]:  Reserved. Set to 0.*/
    void * pReserved[15];                    /**< [in]:  Reserved. Set to 0.*/
}NVFBC_CURSOR_CAPTURE_PARAMS;
#define NVFBC_CURSOR_CAPTURE_PARAMS_VER NVFBC_STRUCT_VERSION(NVFBC_CURSOR_CAPTURE_PARAMS, 1)

/**
 * \ingroup NVFBC_ENTRYPOINTS
 * \brief NVFBC API to set global overrides
 * \param [in] dwFlags Global overrides for NVFBC. Use ::NVFBC_GLOBAL_FLAGS value.
 */
void NVFBCAPI NvFBC_SetGlobalFlags(DWORD dwFlags);

/**
 * \ingroup NVFBC_ENTRYPOINTS
 * \brief NVFBC API to create an NVFBC capture session.
 *  Instantiates an interface identified by NvFBCCreateParams::dwInterfaceType.
 * \param [inout] pCreateParams Pointer to a struct of type ::NvFBCCreateParams, typecast to void*
 * \return An applicable ::NVFBCRESULT value.
 */
NVFBCRESULT NVFBCAPI NvFBC_CreateEx(void * pCreateParams);

/**
 * \ingroup NVFBC_ENTRYPOINTS
 * \brief NVFBC API to query Current NVFBC status.
 *  Queries the status for the adapter pointed to by the NvFBCStatusEx::dwAdapterIdx parameter.
 * \param [inout] pCreateParams Pointer to a struct of type ::NvFBCStatusEx.
 * \return An applicable ::NVFBCRESULT value.
 */
NVFBCRESULT NVFBCAPI NvFBC_GetStatusEx(NvFBCStatusEx *pNvFBCStatusEx);

/**
 * \ingroup NVFBC_ENTRYPOINTS
 * \brief NVFBC API to enable \ disable NVFBC feature.
 * \param [in] nvFBCState Refer ::NVFBC_STATE
 * \return An applicable ::NVFBCRESULT value.
 */
NVFBCRESULT NVFBCAPI NvFBC_Enable(NVFBC_STATE nvFBCState);

/**
 * \ingroup NVFBC_ENTRYPOINTS
 * \brief NVFBC API to query highest GRID SDK version supported by the loaded NVFBC library.
 * \param [out] pVersion Pointer to a 32-bit integer to hold the supported GRID SDK version.
 * \return An applicable ::NVFBCRESULT value.
 */
NVFBCRESULT NVFBCAPI NvFBC_GetSDKVersion(NvU32 * pVersion);

/**
 * \cond API_PFN
 */
typedef void (NVFBCAPI * NvFBC_SetGlobalFlagsType) (DWORD dwFlags);
typedef NVFBCRESULT (NVFBCAPI * NvFBC_CreateFunctionExType)  (void * pCreateParams);
typedef NVFBCRESULT (NVFBCAPI * NvFBC_GetStatusExFunctionType) (void * pNvFBCStatus);
typedef NVFBCRESULT (NVFBCAPI * NvFBC_EnableFunctionType) (NVFBC_STATE nvFBCState);
typedef NVFBCRESULT (NVFBCAPI * NvFBC_GetSDKVersionFunctionType) (NvU32 * pVersion);
/**
 * \endcond API_PFN
*/
