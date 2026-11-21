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
