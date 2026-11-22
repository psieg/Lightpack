#include "hooksutils.h"
#include "Logger.hpp"

bool WriteToProtectedMem(void * mem, void * newVal, void * savedVal, size_t size) {
	DWORD protection = PAGE_EXECUTE_READWRITE;
	DWORD oldProtection;
	Logger *logger = Logger::getInstance();

	// align protected/unprotected memory by 8 bytes
	size_t protectedSize = ((size + 7)/8)*8;

	if (VirtualProtect(mem, protectedSize, protection, &oldProtection)) {
		if (savedVal) {
			logger->reportLogDebug(L"Writing to memory. Saving old content. dest 0x%x, src 0x%x, size %u", savedVal, mem, size);
			memcpy(savedVal, mem, size);
			logger->reportLogDebug(L"Writing to memory. Saving old content. finished");
		}
		logger->reportLogDebug(L"Writing to memory. dest 0x%x, src 0x%x, size %u", mem, newVal, size);
		memcpy(mem, newVal, size);
		logger->reportLogDebug(L"Writing to memory. finished");

		if (VirtualProtect(mem, protectedSize, oldProtection, &oldProtection)) {
			FlushInstructionCache(GetCurrentProcess(), mem, size);
			return true;
		}
	}
	return false;
}

bool IsModuleOffsetValid(HMODULE module, UINT offset, size_t minSize) {
	if (!module || offset < sizeof(IMAGE_DOS_HEADER) || minSize == 0)
		return false;

	const BYTE *base = reinterpret_cast<const BYTE *>(module);
	const IMAGE_DOS_HEADER *dos = reinterpret_cast<const IMAGE_DOS_HEADER *>(base);
	if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0 || dos->e_lfanew > 0x100000)
		return false;

	const IMAGE_NT_HEADERS *nt = reinterpret_cast<const IMAGE_NT_HEADERS *>(base + dos->e_lfanew);
	if (nt->Signature != IMAGE_NT_SIGNATURE)
		return false;

	const DWORD imageSize = nt->OptionalHeader.SizeOfImage;
	if (offset >= imageSize)
		return false;

	return static_cast<size_t>(imageSize - offset) >= minSize;
}
