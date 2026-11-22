#ifndef HOOKSTEST_H
#define HOOKSTEST_H

#include <QObject>

class HooksTest : public QObject
{
	Q_OBJECT
public:
	explicit HooksTest(QObject *parent = 0);

private slots:
	void jmpHookTestCase();
	void jmpHookOutOfRangeAndNullTestCase();
	void frameSizeValidationTestCase();
	void vfTableHookTestCase();
	void reportLogTestCase();

};

#endif // HOOKSTEST_H
