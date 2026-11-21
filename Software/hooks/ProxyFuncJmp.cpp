#include "ProxyFuncJmp.hpp"
#include "hooksutils.h"
#include "string.h"
#include <stdint.h>

bool ProxyFuncJmp::init() {
	if (!m_isInited) {
		if (!m_pTarget || !m_pSubstFunc) {
			if (m_logger)
				m_logger->reportLogError(L"JmpFuncInitialization. m_pTarget=null or m_pSubstFunc=null");
			return false;
		}
		if (m_logger)
			m_logger->reportLogDebug(L"JmpFuncInitialization. m_pTarget=0x%p, m_pSubstFunc=0x%p", m_pTarget, m_pSubstFunc);

#ifdef _WIN64
		const intptr_t targetEnd = reinterpret_cast<intptr_t>(m_pTarget) + static_cast<intptr_t>(kJmpInstructionSize);
		const intptr_t substAddr = reinterpret_cast<intptr_t>(m_pSubstFunc);
		const intptr_t diff = substAddr - targetEnd;
		if (diff < INT32_MIN || diff > INT32_MAX) {
			if (m_logger)
				m_logger->reportLogError(L"JmpFuncInitialization. Target and substitute exceed 32-bit relative JMP range");
			return false;
		}
		const int32_t relativeSubstFunc = static_cast<int32_t>(diff);
#else
		const int32_t relativeSubstFunc = static_cast<int32_t>(
			reinterpret_cast<uintptr_t>(m_pSubstFunc) -
			reinterpret_cast<uintptr_t>(m_pTarget) - kJmpInstructionSize);
#endif
		m_pJmpCode[0] = 0xe9;
		memcpy(m_pJmpCode + 1, &relativeSubstFunc, sizeof(relativeSubstFunc));

		memcpy(m_pOriginalCode, m_pTarget, kJmpInstructionSize);

		m_isInited = true;
	}
	return m_isInited;
}

bool ProxyFuncJmp::isHookInstalled() {
	return m_isInited && memcmp(m_pTarget, m_pOriginalCode, kJmpInstructionSize)!=0;
}

bool ProxyFuncJmp::installHook() {
	return m_isInited && WriteToProtectedMem(m_pTarget, m_pJmpCode, NULL, kJmpInstructionSize);
}

bool ProxyFuncJmp::removeHook() {
	return m_isInited && WriteToProtectedMem(m_pTarget, m_pOriginalCode, NULL, kJmpInstructionSize);
}
