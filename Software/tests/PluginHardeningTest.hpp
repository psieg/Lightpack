#pragma once

#include <QObject>
#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include "Plugin.hpp"

class PluginHardeningTest : public QObject
{
	Q_OBJECT
public:
	PluginHardeningTest() = default;

private Q_SLOTS:
	void testPluginStartPreservesApplicationWorkingDirectory()
	{
		QTemporaryDir tempDir;
		QVERIFY(tempDir.isValid());

		const QString origCwd = QDir::currentPath();
		const QString pluginDir = tempDir.path() + QStringLiteral("/TestPlugin");
		QVERIFY(QDir().mkpath(pluginDir));

		QFile iniFile(pluginDir + QStringLiteral("/TestPlugin.ini"));
		QVERIFY(iniFile.open(QIODevice::WriteOnly | QIODevice::Text));
#ifdef Q_OS_WIN
		iniFile.write("[Main]\nName=TestPlugin\nExecuteOnWindows=cmd.exe /c echo ok > marker.txt\n");
#else
		iniFile.write("[Main]\nName=TestPlugin\nExecute=/bin/sh -c \"echo ok > marker.txt\"\n");
#endif
		iniFile.close();

		Plugin plugin(QStringLiteral("TestPlugin"), pluginDir);
		plugin.Start();

		// Host process working directory must never be mutated to the plugin directory
		QCOMPARE(QDir::currentPath(), origCwd);

		QTRY_VERIFY_WITH_TIMEOUT(QFile::exists(pluginDir + QStringLiteral("/marker.txt")), 5000);
		QTRY_COMPARE_WITH_TIMEOUT(plugin.state(), QProcess::NotRunning, 5000);
		QCOMPARE(QDir::currentPath(), origCwd);
	}
};
