/*
 * SettingsMigrationTest.cpp
 *
 *	Project: Prismatik
 *
 *	Prismatik is a free, open-source software: you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as published
 *	by the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Prismatik and Lightpack files is distributed in the hope that it will be
 *	useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the GNU
 *	General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.	If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "SettingsMigrationTest.hpp"

#include <QtTest>
#include <QTemporaryDir>
#include <QSettings>
#include <QFile>
#include <QGuiApplication>
#include <QScreen>
#include "../src/Settings.hpp"

void SettingsMigrationTest::testLegacyProfileCreatesBackupAndMigrates()
{
	QScreen *primary = QGuiApplication::primaryScreen();
	if (!primary)
		QSKIP("No primary screen available in test environment");

	QTemporaryDir tempDir;
	QVERIFY(tempDir.isValid());

	const QString profilePath = tempDir.filePath(QStringLiteral("LegacyProfile.ini"));
	const qreal dpr = primary->devicePixelRatio();
	const QPoint screenOrigin = primary->geometry().topLeft();

	const QPoint origPhysicalPos(screenOrigin.x() + qRound(100 * dpr), screenOrigin.y() + qRound(60 * dpr));
	const QSize origPhysicalSize(qRound(200 * dpr), qRound(120 * dpr));

	{
		QSettings legacy(profilePath, QSettings::IniFormat);
		legacy.setValue(QStringLiteral("LED_1/Position"), origPhysicalPos);
		legacy.setValue(QStringLiteral("LED_1/Size"), origPhysicalSize);
		legacy.setValue(QStringLiteral("LED_2/Position"), screenOrigin);
		legacy.setValue(QStringLiteral("LED_2/Size"), QSize(1, 1));
		legacy.sync();
	}

	QSettings profile(profilePath, QSettings::IniFormat);
	SettingsScope::Settings::migrateProfileCoordinates(&profile);

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
	QCOMPARE(profile.value(QStringLiteral("CoordinateFormat")).toString(), QStringLiteral("Logical"));

#if defined(Q_OS_MACOS)
	QVERIFY(!QFile::exists(profilePath + QStringLiteral(".qt5bak")));
	QCOMPARE(profile.value(QStringLiteral("LED_1/Position")).toPoint(), origPhysicalPos);
	QCOMPARE(profile.value(QStringLiteral("LED_1/Size")).toSize(), origPhysicalSize);
#else
	const QString backupPath = profilePath + QStringLiteral(".qt5bak");
	QVERIFY(QFile::exists(backupPath));

	QSettings backup(backupPath, QSettings::IniFormat);
	QCOMPARE(backup.value(QStringLiteral("LED_1/Position")).toPoint(), origPhysicalPos);
	QCOMPARE(backup.value(QStringLiteral("LED_1/Size")).toSize(), origPhysicalSize);

	QCOMPARE(profile.value(QStringLiteral("LED_1/Position")).toPoint(), screenOrigin + QPoint(100, 60));
	QCOMPARE(profile.value(QStringLiteral("LED_1/Size")).toSize(), QSize(200, 120));

	const QSize clampedSize = profile.value(QStringLiteral("LED_2/Size")).toSize();
	QVERIFY(clampedSize.width() >= 1);
	QVERIFY(clampedSize.height() >= 1);
#endif
#endif
}

void SettingsMigrationTest::testIdempotentWhenAlreadyLogical()
{
	QTemporaryDir tempDir;
	QVERIFY(tempDir.isValid());

	const QString profilePath = tempDir.filePath(QStringLiteral("LogicalProfile.ini"));
	const QPoint expectedPos(400, 300);
	const QSize expectedSize(150, 150);

	{
		QSettings existing(profilePath, QSettings::IniFormat);
		existing.setValue(QStringLiteral("CoordinateFormat"), QStringLiteral("Logical"));
		existing.setValue(QStringLiteral("LED_1/Position"), expectedPos);
		existing.setValue(QStringLiteral("LED_1/Size"), expectedSize);
		existing.sync();
	}

	QSettings profile(profilePath, QSettings::IniFormat);
	SettingsScope::Settings::migrateProfileCoordinates(&profile);

	QVERIFY(!QFile::exists(profilePath + QStringLiteral(".qt5bak")));
	QCOMPARE(profile.value(QStringLiteral("LED_1/Position")).toPoint(), expectedPos);
	QCOMPARE(profile.value(QStringLiteral("LED_1/Size")).toSize(), expectedSize);
}

void SettingsMigrationTest::testNewNonExistentProfileSkipsBackup()
{
	QTemporaryDir tempDir;
	QVERIFY(tempDir.isValid());

	const QString profilePath = tempDir.filePath(QStringLiteral("BrandNewProfile.ini"));
	QVERIFY(!QFile::exists(profilePath));

	QSettings profile(profilePath, QSettings::IniFormat);
	SettingsScope::Settings::migrateProfileCoordinates(&profile);

	QVERIFY(!QFile::exists(profilePath + QStringLiteral(".qt5bak")));
}
