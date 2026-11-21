#include "ProxyFuncJmp.hpp"
#include "ProxyFuncVFTable.hpp"
#include "HooksTest.h"
#include "hooksutils.h"
#include "../common/D3D10GrabberDefs.hpp"
#include <QtTest>
#include <cstdint>

HooksTest::HooksTest(QObject *parent) :
	QObject(parent)
{
}

void HooksTest::jmpHookTestCase() {
//	Mockup of target memory					v-- our target starts here, first byte is to be sure we haven't broken it
	unsigned char procMockup[8]			={0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22};

//	Original memory mockup to compare with
	unsigned char procUntouchedMockup[8]={0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22};

//	Hooked memory mockup						|jmp | relative 32-bit offset |
	unsigned char procHookedMockup[8]	={0x22,0xe9,0x00,0x00,0x00,0x00,0x22,0x22};

	// Place substAdr within +/-2 GiB of procMockup so the 32-bit relative JMP displacement is well-defined on both x86 and x64.
	void *substAdr = procMockup + 0x1000;

	int32_t relativeAdr = static_cast<int32_t>(
		reinterpret_cast<uintptr_t>(substAdr) - reinterpret_cast<uintptr_t>(procMockup + 1) - 5); /*+1 because target starts at second byte, -5 because JMP rel32 is 5 bytes*/
	memcpy(procHookedMockup + 2, &relativeAdr, sizeof(relativeAdr));

	ProxyFuncJmp *test = new ProxyFuncJmp(procMockup + 1, substAdr, Logger::getInstance());
	QVERIFY2(!test->isHookInstalled(),"Hook shouldn't be installed");
	QVERIFY2(test->init(), "init crashed");
	QVERIFY2(test->installHook(), "installHook crashed 1");
	QVERIFY2(test->isHookInstalled(),"Hook should be installed");

	QVERIFY2(memcmp(procMockup, procHookedMockup, sizeof(procMockup))==0,"Hook has been installed incorrectly");

	QVERIFY2(test->removeHook(), "removeHook crashed");

	QVERIFY2(memcmp(procMockup, procUntouchedMockup, sizeof(procMockup))==0,"Hook has been removed incorrectly 1");

	QVERIFY2(test->installHook(), "installHook crashed 2");

	delete test;

	QVERIFY2(memcmp(procMockup, procUntouchedMockup, sizeof(procMockup))==0,"Hook should be removed after object deletion");
}

void HooksTest::jmpHookOutOfRangeAndNullTestCase() {
	unsigned char procMockup[8] = {0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22};
	const unsigned char procUntouched[8] = {0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22};

	// Null target or substitute must fail cleanly without dereferencing or installing
	ProxyFuncJmp nullTarget(nullptr, procMockup, Logger::getInstance());
	QVERIFY(!nullTarget.init());
	QVERIFY(!nullTarget.installHook());

	ProxyFuncVFTable nullVf(nullptr, procMockup, Logger::getInstance());
	QVERIFY(!nullVf.init());
	QVERIFY(!nullVf.installHook());

#ifdef _WIN64
	// Substitute >2 GiB away from target must be rejected on x64 instead of truncating rel32 or overflowing m_pJmpCode
	void *farSubst = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(procMockup) + (static_cast<uintptr_t>(1) << 33));
	ProxyFuncJmp farHook(procMockup + 1, farSubst, Logger::getInstance());
	QVERIFY(!farHook.init());
	QVERIFY(!farHook.installHook());
	QVERIFY(memcmp(procMockup, procUntouched, sizeof(procMockup)) == 0);
#else
	Q_UNUSED(procUntouched);
#endif
}

void HooksTest::frameSizeValidationTestCase() {
	// Standard 1080p and 4K frames fit within 50 MiB shared memory
	QVERIFY(hooksGrabberValidateFrameSize(1920, 1080, 1920 * 4));
	QVERIFY(hooksGrabberValidateFrameSize(3840, 2160, 3840 * 4 + 256));

	// Zero dimensions, pitch smaller than width*4, 8K frames (>50 MiB), and integer-overflow dimensions must be rejected
	QVERIFY(!hooksGrabberValidateFrameSize(0, 1080, 1920 * 4));
	QVERIFY(!hooksGrabberValidateFrameSize(1920, 0, 1920 * 4));
	QVERIFY(!hooksGrabberValidateFrameSize(1920, 1080, 1920 * 4 - 4));
	QVERIFY(!hooksGrabberValidateFrameSize(7680, 4320, 7680 * 4));
	QVERIFY(!hooksGrabberValidateFrameSize(0x40000001U, 4, 0x40000001U * 4));
}

void HooksTest::vfTableHookTestCase() {
	constexpr size_t kPtrSize = sizeof(void *);
	constexpr size_t kBufSize = kPtrSize + 2; // leading and trailing sentinel bytes

//	Mockup of target memory					v-- our target starts here, first & last bytes are to be sure we haven't broken them
	unsigned char procMockup[kBufSize];
	memset(procMockup, 0x22, sizeof(procMockup));

//	Original memory mockup to compare with
	unsigned char procUntouchedMockup[kBufSize];
	memset(procUntouchedMockup, 0x22, sizeof(procUntouchedMockup));

//	Hooked memory mockup						| absolute pointer |
	unsigned char procHookedMockup[kBufSize];
	memset(procHookedMockup, 0x22, sizeof(procHookedMockup));

	uintptr_t substAdr = static_cast<uintptr_t>(0x12345678);

	memcpy(procHookedMockup + 1, &substAdr, sizeof(substAdr));

	ProxyFuncVFTable *test = new ProxyFuncVFTable(procMockup + 1, reinterpret_cast<void *>(substAdr), Logger::getInstance());
	QVERIFY2(!test->isHookInstalled(),"Hook shouldn't be installed");
	QVERIFY2(test->init(), "init crashed");
	QVERIFY2(test->installHook(), "installHook crashed 1");
	QVERIFY2(test->isHookInstalled(),"Hook should be installed");

	QVERIFY2(memcmp(procMockup, procHookedMockup, sizeof(procMockup))==0,"Hook has been installed incorrectly");

	QVERIFY2(test->removeHook(), "removeHook crashed");

	QVERIFY2(memcmp(procMockup, procUntouchedMockup, sizeof(procMockup))==0,"Hook has been removed incorrectly 1");

	QVERIFY2(test->installHook(), "installHook crashed 2");

	delete test;

	QVERIFY2(memcmp(procMockup, procUntouchedMockup, sizeof(procMockup))==0,"Hook should be removed after object deletion");
}

void HooksTest::reportLogTestCase() {

//	int a = -1;
//	InitLog(L"test", 0);
//	ReportLogInfo(L"0x%x",a);
//	CloseLog();

}
