/**
 * \file This file contains defintions for NVFBCToSys
 *
 * Copyright 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 *
 */

#ifndef NVFBC_TO_SYS_H_
#define NVFBC_TO_SYS_H_
/**
 * \defgroup NVFBC_TOSYS NVFBCToSys Interface
 * \brief Interface for grabbing Desktop images and generating output in system memory.
 */

/**
 * \defgroup NVFBC_TOSYS_ENUMS Enums
 * \ingroup NVFBC_TOSYS
 * \brief Enumerations used with NVFBCToSys interface.
 */

/**
 * \defgroup NVFBC_TOSYS_STRUCTS Structs
 * \ingroup NVFBC_TOSYS
 * \brief  Parameter Structs Defined for use with NVFBCToSys interface.
 */

/**
 * \defgroup NVFBC_TOSYS_INTERFACE Object Interface
 * \ingroup NVFBC_TOSYS
 * \brief Interface class definition for NVFBCToSys Capture API
 */

/**
 * \ingroup NVFBC_TOSYS
 * \brief Macro to define the interface ID to be passed as NvFBCCreateParams::dwInterfaceType
 * for creating an NVFBCToSys capture session object.
 */
#define NVFBC_TO_SYS (0x1204) 

/**
 * \ingroup NVFBC_TOSYS_ENUMS
 *  Enumerates output buffer pixel data formats supported by NVFBCToSys.
 */
typedef enum
{
    NVFBC_TOSYS_ARGB       = 0,              /**< Output Pixels in ARGB format: 32bpp, one byte per channel. */
    NVFBC_TOSYS_RGB           ,              /**< Output Pixels in RGB format: 24bpp, one byte per channel. */
    NVFBC_TOSYS_YYYYUV420p    ,              /**< Output Pixels in YUV420 format: 12bpp, 
                                                  the Y' channel at full resolution, U channel at half resolution (1 byte for four pixels), V channel at half resolution. */
    NVFBC_TOSYS_RGB_PLANAR    ,              /**< Output Pixels in planar RGB format: 24bpp, 
                                                  stored sequentially in memory as complete red channel, complete green channel, complete blue channel. */
    NVFBC_TOSYS_XOR           ,              /**< Output Pixels in RGB format: 24bpp XOR’d with the prior frame. */
    NVFBC_TOSYS_YUV444p       ,              /**< Output Pixels in YUV444 planar format, i.e. separate 8-bpp Y, U, V planes with no subsampling.*/
    NVFBC_TOSYS_BUF_FMT_LAST  ,              /**< Sentinel value. Do not use.*/
} NVFBCToSysBufferFormat;

/**
 * \ingroup NVFBC_TOSYS_ENUMS
 *  Enumerates Capture\Grab modes supported by NVFBCToSys.
 */
typedef enum 
{
    NVFBC_TOSYS_SOURCEMODE_FULL  = 0,        /**< Grab full res */
    NVFBC_TOSYS_SOURCEMODE_SCALE    ,        /**< Will convert current res to supplied resolution (dwTargetWidth and dwTargetHeight) */
    NVFBC_TOSYS_SOURCEMODE_CROP     ,        /**< Native res, crops a subwindow, of dwTargetWidth and dwTargetHeight sizes, starting at dwStartX and dwStartY */
    NVFBC_TOSYS_SOURCEMODE_LAST     ,        /**< Sentinel value. Do not use. */
}NVFBCToSysGrabMode;

/**
 * \ingroup NVFBC_TOSYS_ENUMS
 * \enum NVFBC_TOSYS_GRAB_FLAGS Enumerates special commands for grab\capture supported by NVFBCToSys.
 */
typedef enum 
{
    NVFBC_TOSYS_NOFLAGS           = 0x0,     /**< Default (no flags set). Grabbing will wait for a new frame or HW mouse move. */
    NVFBC_TOSYS_NOWAIT            = 0x1,     /**< Grabbing will not wait for a new frame nor a HW cursor move. */
    NVFBC_TOSYS_WAIT_WITH_TIMEOUT = 0x10,    /**< Grabbing will wait for a new frame or HW mouse move with a maximum wait time of NVFBC_TOSYS_GRAB_FRAME_PARAMS::dwWaitTime millisecond*/
} NVFBC_TOSYS_GRAB_FLAGS;

/**
 * \ingroup NVFBC_TOSYS_STRUCTS
 * \brief Defines parameters used to configure NVFBCToSys capture session.
 */
typedef struct
{
    NvU32 dwVersion;                         /**< [in]: Struct version. Set to NVFBC_TOSYS_SETUP_PARAMS_VER.*/
    NvU32 bWithHWCursor :1;                  /**< [in]: The client should set this to 1 if it requires the HW cursor to be composited on the captured image.*/
    NvU32 bDiffMap      :1;                  /**< [in]: The client should set this to use the DiffMap feature.*/
    NvU32 bEnableSeparateCursorCapture : 1;  /**< [in]: The client should set this to 1 if it wants to enable mouse capture in separate stream.*/
    NvU32 bReservedBits :29;                 /**< [in]: Reserved. Set to 0.*/
    NVFBCToSysBufferFormat eMode;            /**< [in]: Output image format.*/
    NvU32 dwReserved1;                       /**< [in]: Reserved. Set to 0.*/
    void **ppBuffer;                         /**< [out]: Container to hold NvFBC output buffers.*/
    void **ppDiffMap;                        /**< [out]: Container to hold NvFBC output diffmap buffers.*/
    void  *hCursorCaptureEvent;                 /**< [out]: Client should wait for mouseEventHandle event before calling MouseGrab function. */
    NvU32 dwReserved[58];                    /**< [in]: Reserved. Set to 0.*/
    void *pReserved[29];                     /**< [in]: Reserved. Set to 0.*/
} NVFBC_TOSYS_SETUP_PARAMS_V2;
#define NVFBC_TOSYS_SETUP_PARAMS_VER2 NVFBC_STRUCT_VERSION(NVFBC_TOSYS_SETUP_PARAMS, 2)
typedef  NVFBC_TOSYS_SETUP_PARAMS_V2 NVFBC_TOSYS_SETUP_PARAMS;
#define NVFBC_TOSYS_SETUP_PARAMS_VER NVFBC_TOSYS_SETUP_PARAMS_VER2

/**
 * \ingroup NVFBC_TOSYS_STRUCTS
 * \brief Defines parameters for a Grab\Capture call in the NVFBCToSys capture session.
 * Also holds information regarding the grabbed data.
 */
typedef struct
{
    NvU32 dwVersion;                         /**< [in]: Struct version. Set to NVFBC_TOSYS_GRAB_FRAME_PARAMS_VER.*/
    NvU32 dwFlags;                           /**< [in]: Special grabbing requests. This should be a bit-mask of NVFBC_TOSYS_GRAB_FLAGS values.*/
    NvU32 dwTargetWidth;                     /**< [in]: Target image width. NvFBC will scale the captured image to fit taret width and height. Used with NVFBC_TOSYS_SOURCEMODE_SCALE and NVFBC_TOSYS_SOURCEMODE_CROP. */
    NvU32 dwTargetHeight;                    /**< [in]: Target image height. NvFBC will scale the captured image to fit taret width and height. Used with NVFBC_TOSYS_SOURCEMODE_SCALE and NVFBC_TOSYS_SOURCEMODE_CROP. */
    NvU32 dwStartX;                          /**< [in]: x-coordinate of starting pixel for cropping. Used with NVFBC_TOSYS_SOURCEMODE_CROP. */
    NvU32 dwStartY;                          /**< [in]: y-coordinate of starting pixel for cropping. Used with NVFBC_TOSYS_SOURCEMODE_CROP. .*/
    NVFBCToSysGrabMode eGMode;               /**< [in]: Frame grab mode.*/
    NvU32 dwWaitTime;                        /**< [in]: Time limit for NvFBCToSysGrabFrame() to wait until a new frame is available or a HW mouse moves. Use with NVFBC_TOSYS_WAIT_WITH_TIMEOUT */
    NvFBCFrameGrabInfo *pNvFBCFrameGrabInfo; /**< [in/out]: Frame grab information and feedback from NvFBC driver.*/
    NvU32 dwReserved[56];                    /**< [in]: Reserved. Set to 0.*/
    void *pReserved[31];                     /**< [in]: Reserved. Set to NULL.*/
} NVFBC_TOSYS_GRAB_FRAME_PARAMS_V1;
#define NVFBC_TOSYS_GRAB_FRAME_PARAMS_VER1 NVFBC_STRUCT_VERSION(NVFBC_TOSYS_GRAB_FRAME_PARAMS, 1)
typedef NVFBC_TOSYS_GRAB_FRAME_PARAMS_V1 NVFBC_TOSYS_GRAB_FRAME_PARAMS;
#define NVFBC_TOSYS_GRAB_FRAME_PARAMS_VER NVFBC_TOSYS_GRAB_FRAME_PARAMS_VER1


/**
 * \ingroup NVFBC_TOSYS_INTERFACE
 * Interface class definition for NVFBCToSys Capture API
 */
class INvFBCToSys_v3
{
public:
    /**
     * \brief Sets up NVFBC System Memory capture according to the provided parameters.
     * \param [in] pParam Pointer to a struct of type ::NVFBC_TOSYS_SETUP_PARAMS.
     * \return An applicable ::NVFBCRESULT value.
     */
    virtual NVFBCRESULT NVFBCAPI NvFBCToSysSetUp              (NVFBC_TOSYS_SETUP_PARAMS_V2 *pParam) = 0;

    /**
     * \brief Captures the desktop and dumps the captured data to a System memory buffer.
     *  If the API returns a failure, the client should check the return codes and ::NvFBCFrameGrabInfo output fields to determine if the session needs to be re-created.
     * \param [inout] pParam Pointer to a struct of type ::NVFBC_TOSYS_GRAB_FRAME_PARAMS.
     * \return An applicable ::NVFBCRESULT value.
     */
    virtual NVFBCRESULT NVFBCAPI NvFBCToSysGrabFrame          (NVFBC_TOSYS_GRAB_FRAME_PARAMS *pParam) = 0;

    /**
     * \brief Captures HW cursor data whenever shape of mouse is changed
     * \param [inout] pParam Pointer to a struct of type ::NVFBC_CURSOR_CAPTURE_PARAMS.
     * \return An applicable ::NVFBCRESULT value.
     */
    virtual NVFBCRESULT NVFBCAPI NvFBCToSysCursorCapture      (NVFBC_CURSOR_CAPTURE_PARAMS *pParam) = 0;

    /**
     * \brief A high precision implementation of Sleep(). 
     *  Can provide sub quantum (usually 16ms) sleep that does not burn CPU cycles.
     * \param [in] qwMicroSeconds The number of microseconds that the thread should sleep for.
     * \return An applicable ::NVFBCRESULT value.
     */
    virtual NVFBCRESULT NVFBCAPI NvFBCToSysGPUBasedCPUSleep   (__int64 qwMicroSeconds) = 0;

    /**
     * \brief Destroys the NVFBCToSys capture session.
     * \return An applicable ::NVFBCRESULT value.
     */
    virtual NVFBCRESULT NVFBCAPI NvFBCToSysRelease            () = 0;
};

typedef INvFBCToSys_v3 NvFBCToSys;

#endif // NVFBC_TO_SYS_H_

