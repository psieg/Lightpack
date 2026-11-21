#include "ProxyFuncVFTable.hpp"
#include "hooksutils.h"

bool ProxyFuncVFTable::init() {
	if (!m_isInited) {
		if (!m_pTarget || !m_pSubstFunc) {
			if (m_logger)
				m_logger->reportLogError(L"VFTableFuncInitialization. m_pTarget=null or m_pSubstFunc=null");
			return false;
		}
		if (m_logger)
			m_logger->reportLogDebug(L"VFTableFuncInitialization. m_pTarget=0x%p, m_pSubstFunc=0x%p", m_pTarget, m_pSubstFunc);

		m_pOriginalFunc = *static_cast<void **>(m_pTarget);
		m_isInited = true;
	}
	return m_isInited;
}

bool ProxyFuncVFTable::installHook() {
	return (m_isInited) && WriteToProtectedMem(m_pTarget, &m_pSubstFunc, NULL, sizeof(m_pSubstFunc));
}

bool ProxyFuncVFTable::removeHook() {
	return (m_isInited) && WriteToProtectedMem(m_pTarget, &m_pOriginalFunc, NULL, sizeof(m_pOriginalFunc));
}
