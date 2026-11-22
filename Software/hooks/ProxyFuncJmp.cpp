#include "ProxyFuncJmp.hpp"
#include "hooksutils.h"
#include "string.h"
#include <stdint.h>

#ifdef _WIN64
namespace {
void *allocateNearAbsoluteJumpTrampoline(uintptr_t targetEnd, void *absDest) {
	SYSTEM_INFO sysInfo = {};
	GetSystemInfo(&sysInfo);
	const uintptr_t gran = sysInfo.dwAllocationGranularity ? sysInfo.dwAllocationGranularity : 0x10000;
	const uintptr_t maxDelta = 0x7FFF0000ULL; // Safely inside signed 32-bit rel32 range
	const uintptr_t minAddr = (targetEnd > maxDelta + gran) ? ((targetEnd - maxDelta + gran - 1) & ~(gran - 1)) : gran;
	const uintptr_t maxAddr = (UINTPTR_MAX - maxDelta > targetEnd) ? (targetEnd + maxDelta) : (UINTPTR_MAX - gran);

	auto tryAllocateAt = [&](uintptr_t candidate) -> void * {
		const intptr_t d = static_cast<intptr_t>(candidate) - static_cast<intptr_t>(targetEnd);
		if (d < INT32_MIN || d > INT32_MAX)
			return nullptr;
		unsigned char *mem = static_cast<unsigned char *>(
			VirtualAlloc(reinterpret_cast<void *>(candidate), 64, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
		if (!mem)
			return nullptr;

		// x64 RIP-relative indirect jump: FF 25 00 00 00 00 <64-bit destination>
		mem[0] = 0xFF;
		mem[1] = 0x25;
		mem[2] = 0x00;
		mem[3] = 0x00;
		mem[4] = 0x00;
		mem[5] = 0x00;
		const uint64_t dst64 = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(absDest));
		memcpy(mem + 6, &dst64, sizeof(dst64));

		DWORD oldProt = 0;
		if (!VirtualProtect(mem, 64, PAGE_EXECUTE_READ, &oldProt)) {
			VirtualFree(mem, 0, MEM_RELEASE);
			return nullptr;
		}
		FlushInstructionCache(GetCurrentProcess(), mem, 14);
		return mem;
	};

	for (int attempt = 0; attempt < 3; ++attempt) {
		// Search upward from targetEnd first, then downward from targetEnd
		for (uintptr_t addr = targetEnd & ~(gran - 1); addr <= maxAddr; ) {
			MEMORY_BASIC_INFORMATION mbi = {};
			if (VirtualQuery(reinterpret_cast<LPCVOID>(addr), &mbi, sizeof(mbi)) == 0)
				break;
			if (mbi.State == MEM_FREE) {
				const uintptr_t alignedBase = (reinterpret_cast<uintptr_t>(mbi.BaseAddress) + gran - 1) & ~(gran - 1);
				const uintptr_t regionEnd = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
				if (alignedBase >= minAddr && alignedBase <= maxAddr && alignedBase + 64 <= regionEnd) {
					if (void *p = tryAllocateAt(alignedBase))
						return p;
				}
			}
			const uintptr_t next = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
			if (next <= addr)
				break;
			addr = next;
		}

		for (uintptr_t probe = (targetEnd > gran) ? ((targetEnd - gran) & ~(gran - 1)) : 0; probe >= minAddr; ) {
			MEMORY_BASIC_INFORMATION mbi = {};
			if (VirtualQuery(reinterpret_cast<LPCVOID>(probe), &mbi, sizeof(mbi)) == 0)
				break;
			if (mbi.State == MEM_FREE) {
				const uintptr_t regionEnd = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
				const uintptr_t cand = (regionEnd >= gran) ? ((regionEnd - gran) & ~(gran - 1)) : 0;
				if (cand >= reinterpret_cast<uintptr_t>(mbi.BaseAddress) && cand >= minAddr && cand <= maxAddr) {
					if (void *p = tryAllocateAt(cand))
						return p;
				}
			}
			const uintptr_t base = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
			if (base < gran || base <= minAddr)
				break;
			probe = (base - gran) & ~(gran - 1);
		}

		if (attempt + 1 < 3)
			Sleep(0);
	}

	return nullptr;
}
} // namespace
#endif

ProxyFuncJmp::~ProxyFuncJmp() {
	if (this->isHookInstalled())
		this->removeHook();
#ifdef _WIN64
	if (m_pTrampoline) {
		VirtualFree(m_pTrampoline, 0, MEM_RELEASE);
		m_pTrampoline = nullptr;
	}
#endif
}

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
		const uintptr_t targetEndU = reinterpret_cast<uintptr_t>(m_pTarget) + static_cast<uintptr_t>(kJmpInstructionSize);
		const intptr_t targetEnd = static_cast<intptr_t>(targetEndU);
		intptr_t diff = reinterpret_cast<intptr_t>(m_pSubstFunc) - targetEnd;
		if (diff < INT32_MIN || diff > INT32_MAX) {
			m_pTrampoline = allocateNearAbsoluteJumpTrampoline(targetEndU, m_pSubstFunc);
			if (!m_pTrampoline) {
				if (m_logger)
					m_logger->reportLogError(L"JmpFuncInitialization. Target and substitute exceed 32-bit relative JMP range and near trampoline allocation failed");
				return false;
			}
			diff = reinterpret_cast<intptr_t>(m_pTrampoline) - targetEnd;
			if (diff < INT32_MIN || diff > INT32_MAX) {
				VirtualFree(m_pTrampoline, 0, MEM_RELEASE);
				m_pTrampoline = nullptr;
				return false;
			}
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
