#include "IPCContext.hpp"

IPCContext::~IPCContext() {
	this->free();
}

bool IPCContext::init() {
	if (NULL == (m_hMutex = OpenMutexW(SYNCHRONIZE, FALSE, HOOKSGRABBER_MUTEX_MEM_NAME))) {
		m_logger->reportLogError(L"error occured while opening mutex 0x%x", GetLastError());
		this->free();
		return false;
	}

	if (NULL == (m_hConfigMem = OpenFileMappingW(FILE_MAP_READ, FALSE, HOOKSGRABBER_CONFIG_MEM_NAME))) {
		m_logger->reportLogError(L"error occured while opening config mapping 0x%x", GetLastError());
		this->free();
		return false;
	}

	if (NULL == (m_hSharedMem = OpenFileMappingW(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, HOOKSGRABBER_SHARED_MEM_NAME))) {
		m_logger->reportLogError(L"error occured while opening memory-mapped file 0x%x", GetLastError());
		this->free();
		return false;
	}

	if (NULL == (m_hFrameGrabbedEvent = OpenEventW(EVENT_MODIFY_STATE, FALSE, HOOKSGRABBER_FRAMEGRABBED_EVENT_NAME))) {
		m_logger->reportLogError(L"error occured while opening event 0x%x", GetLastError());
		this->free();
		return false;
	}

	m_pConfigMap = MapViewOfFile(m_hConfigMem, FILE_MAP_READ, 0, 0, HOOKSGRABBER_CONFIG_MEM_SIZE);
	if (m_pConfigMap == NULL) {
		m_logger->reportLogError(L"error occured while creating config mapview 0x%x", GetLastError());
		this->free();
		return false;
	}

	m_pMemMap = MapViewOfFile(m_hSharedMem, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, HOOKSGRABBER_SHARED_MEM_SIZE);
	if (m_pMemMap == NULL) {
		m_logger->reportLogError(L"error occured while creating mapview 0x%x", GetLastError());
		this->free();
		return false;
	}

	m_pConfigDesc = static_cast<const HOOKSGRABBER_CONFIG_DESC *>(m_pConfigMap);
	m_pMemDesc = static_cast<HOOKSGRABBER_SHARED_MEM_DESC *>(m_pMemMap);

	return true;
}

void IPCContext::free() {
	m_pConfigDesc = NULL;
	m_pMemDesc = NULL;

	if (m_pConfigMap) {
		UnmapViewOfFile(m_pConfigMap);
		m_pConfigMap = NULL;
	}

	if (m_hConfigMem) {
		CloseHandle(m_hConfigMem);
		m_hConfigMem = NULL;
	}

	if (m_pMemMap) {
		UnmapViewOfFile(m_pMemMap);
		m_pMemMap = NULL;
	}

	if (m_hSharedMem) {
		CloseHandle(m_hSharedMem);
		m_hSharedMem = NULL;
	}

	if (m_hMutex) {
		CloseHandle(m_hMutex);
		m_hMutex = NULL;
	}

	if (m_hFrameGrabbedEvent) {
		CloseHandle(m_hFrameGrabbedEvent);
		m_hFrameGrabbedEvent = NULL;
	}
}
