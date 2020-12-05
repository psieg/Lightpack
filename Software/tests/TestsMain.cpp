#include <QtTest/QtTest>
#include "LightpackApiTest.hpp"
#include "GrabCalculationTest.hpp"
#include "lightpackmathtest.hpp"
#ifdef Q_OS_WIN
#include "HooksTest.h"
#endif
#include "LightpackCommandLineParserTest.hpp"
#include "debug.h"

#include <iostream>

using namespace std;

unsigned g_debugLevel = Debug::LowLevel;

int main(int argc, char *argv[])
{
	QTEST_DISABLE_KEYPAD_NAVIGATION
	QApplication app(argc, argv);

	QList<QObject *> tests;
	QStringList summary;

	tests.append(new GrabCalculationTest());

#ifdef Q_OS_WIN
	tests.append(new HooksTest());
#endif

	tests.append(new LightpackMathTest());
	tests.append(new LightpackApiTest());
	tests.append(new LightpackCommandLineParserTest());

	for(int i=0; i < tests.size(); i++) {
		if (QTest::qExec(tests[i], argc, argv)) {
			summary << QString(tests[i]->metaObject()->className()).append("\tFAILED");
		} else {
			summary << QString(tests[i]->metaObject()->className()).append("\tPASSED");
		}
		delete tests[i];
	}

	for (int i = 0; i < summary.size(); ++i)
		cout << endl << summary.at(i).toLocal8Bit().constData() << endl;

	return 0;
}
