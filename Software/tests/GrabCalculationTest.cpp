#include "GrabCalculationTest.hpp"

void GrabCalculationTest::testCase1()
{
	unsigned char buf[16];
	memset(buf, 0xfa, 16);
	QRgb result = Grab::Calculations::calculateAvgColor(buf, BufferFormatArgb, 16, QRect(0,0,4,1));
	QVERIFY2(result == QColor(0xfa, 0xfa, 0xfa).rgb(), qPrintable(QString("Failure. calculateAvgColor returned wrong errorcode %1").arg(result, 1, 16)));
}

void GrabCalculationTest::testEmptyRectDoesNotCrash()
{
	unsigned char buf[16];
	memset(buf, 0xfa, 16);
	QCOMPARE(Grab::Calculations::calculateAvgColor(buf, BufferFormatArgb, 16, QRect(0, 0, 0, 0)), qRgb(0, 0, 0));
	QCOMPARE(Grab::Calculations::calculateAvgColor(buf, BufferFormatArgb, 16, QRect(0, 0, 4, 0)), qRgb(0, 0, 0));
	QCOMPARE(Grab::Calculations::calculateAvgColor(buf, BufferFormatArgb, 16, QRect(0, 0, 0, 4)), qRgb(0, 0, 0));
	QCOMPARE(Grab::Calculations::calculateAvgColor(nullptr, BufferFormatArgb, 16, QRect(0, 0, 4, 1)), qRgb(0, 0, 0));
}

void GrabCalculationTest::testOutOfBoundsAndNegativeCoordsRejected()
{
	unsigned char buf[16];
	memset(buf, 0xfa, 16);
	QCOMPARE(Grab::Calculations::calculateAvgColor(buf, BufferFormatArgb, 16, QRect(-1, 0, 4, 1)), qRgb(0, 0, 0));
	QCOMPARE(Grab::Calculations::calculateAvgColor(buf, BufferFormatArgb, 16, QRect(0, -1, 4, 1)), qRgb(0, 0, 0));
	QCOMPARE(Grab::Calculations::calculateAvgColor(buf, BufferFormatArgb, 16, QRect(1, 0, 4, 1)), qRgb(0, 0, 0)); // (1+4)*4 = 20 > pitch 16
	QCOMPARE(Grab::Calculations::calculateAvgColor(buf, BufferFormatArgb, 3, QRect(0, 0, 1, 1)), qRgb(0, 0, 0));  // pitch < 4
}

void GrabCalculationTest::testVaryingWidthsAndFormats()
{
	constexpr int kStridePixels = 40;
	constexpr int kHeight = 3;
	constexpr size_t kPitchBytes = kStridePixels * 4;
	std::vector<unsigned char> buf(kPitchBytes * kHeight, 0);

	const int widths[] = { 1, 3, 4, 7, 8, 15, 16, 17, 31, 32, 35 };
	for (int w : widths) {
		for (size_t i = 0; i < buf.size(); i += 4) {
			buf[i + 0] = 0x10;
			buf[i + 1] = 0x20;
			buf[i + 2] = 0x30;
			buf[i + 3] = 0xFF;
		}
		QCOMPARE(Grab::Calculations::calculateAvgColor(buf.data(), BufferFormatArgb, kPitchBytes, QRect(2, 0, w, kHeight)), qRgb(0x30, 0x20, 0x10));
		QCOMPARE(Grab::Calculations::calculateAvgColor(buf.data(), BufferFormatAbgr, kPitchBytes, QRect(2, 0, w, kHeight)), qRgb(0x10, 0x20, 0x30));
		QCOMPARE(Grab::Calculations::calculateAvgColor(buf.data(), BufferFormatRgba, kPitchBytes, QRect(2, 0, w, kHeight)), qRgb(0xFF, 0x30, 0x20));
		QCOMPARE(Grab::Calculations::calculateAvgColor(buf.data(), BufferFormatBgra, kPitchBytes, QRect(2, 0, w, kHeight)), qRgb(0x20, 0x30, 0xFF));
	}
}
