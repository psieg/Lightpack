#ifndef IPCCONTEXT_HPP
#define IPCCONTEXT_HPP

#if !defined NOMINMAX
#define NOMINMAX
#endif

#if !defined WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "windows.h"
#include "../common/D3D10GrabberDefs.hpp"
#include "LoggableTrait.hpp"

class IPCContext : public LoggableTrait
{
public:
	IPCContext(Logger *logger)
		: LoggableTrait(logger),
		  m_hConfigMem(NULL),
		  m_hSharedMem(NULL),
		  m_hMutex(NULL),
		  m_hFrameGrabbedEvent(NULL),
		  m_pConfigDesc(NULL),
		  m_pMemDesc(NULL),
		  m_pConfigMap(NULL),
		  m_pMemMap(NULL)
	{}
	~IPCContext();

	bool init();
	void free();

	HANDLE m_hConfigMem;
	HANDLE m_hSharedMem;
	HANDLE m_hMutex;
	HANDLE m_hFrameGrabbedEvent;

	const HOOKSGRABBER_CONFIG_DESC *m_pConfigDesc;
	HOOKSGRABBER_SHARED_MEM_DESC *m_pMemDesc;

	PVOID m_pConfigMap;
	PVOID m_pMemMap;
};

#endif // IPCCONTEXT_HPP
