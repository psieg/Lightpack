#include "LedDeviceManagerTest.hpp"

#include <QtTest>
#include <QTemporaryDir>
#include <QSignalSpy>
#include <QTimer>

#include "../src/Settings.hpp"
#include "../src/SystemSession.hpp"
#include "../src/LedDeviceLightpack.hpp"
#include "../src/LedDeviceAdalight.hpp"
#include "../src/LedDeviceArdulight.hpp"
#include "../src/LedDeviceVirtual.hpp"

using namespace SettingsScope;

MockLedDevice::MockLedDevice(QObject *parent)
	: AbstractLedDevice(parent)
{
}

void MockLedDevice::recordEvent(const QString &event)
{
	QMutexLocker locker(&m_mutex);
	m_events.append(event);
}

QStringList MockLedDevice::takeEvents()
{
	QMutexLocker locker(&m_mutex);
	QStringList out = m_events;
	m_events.clear();
	return out;
}

QStringList MockLedDevice::eventsSnapshot()
{
	QMutexLocker locker(&m_mutex);
	return m_events;
}

QList<QThread*> MockLedDevice::closeThreadsSnapshot()
{
	QMutexLocker locker(&m_mutex);
	return m_closeThreads;
}

QList<QThread*> MockLedDevice::openThreadsSnapshot()
{
	QMutexLocker locker(&m_mutex);
	return m_openThreads;
}

QList<QList<QRgb>> MockLedDevice::colorsHistorySnapshot()
{
	QMutexLocker locker(&m_mutex);
	return m_colorsHistory;
}

void MockLedDevice::emitOutOfBandError()
{
	QMetaObject::invokeMethod(this, [this]() {
		emit ioDeviceSuccess(false);
	}, Qt::QueuedConnection);
}

bool MockLedDevice::runNestedAckBatch(bool firstOk, bool secondOk)
{
	AckBatch outer(this);
	{
		AckBatch inner(this);
		ack(firstOk);
	}
	ack(secondOk);
	return true;
}

void MockLedDevice::open()
{
	{
		QMutexLocker locker(&m_mutex);
		m_openThreads.append(QThread::currentThread());
		m_events.append(QStringLiteral("open"));
	}

	if (holdOpen.load()) {
		openEnteredSem.release();
		openReleaseSem.acquire();
	}

	if (dropFirstOpenEmit.exchange(false))
		return; // Intentionally violate open() contract to test self-healing

	bool ok = nextOpenResult.load();
	if (failFirstOpenOnly.exchange(false))
		ok = false;

	emit openDeviceSuccess(ok);
}

void MockLedDevice::close()
{
	QMutexLocker locker(&m_mutex);
	m_closeThreads.append(QThread::currentThread());
	m_events.append(QStringLiteral("close"));
}

void MockLedDevice::setColors(const QList<QRgb> &colors)
{
	{
		QMutexLocker locker(&m_mutex);
		m_colorsHistory.append(colors);
		m_events.append(QStringLiteral("setColors"));
	}

	if (holdSetColors.load()) {
		setColorsEnteredSem.release();
		setColorsReleaseSem.acquire();
	}

	m_colorsSaved = colors;
	bool ok = nextSetColorsResult.load();
	if (failFirstSetColorsOnly.exchange(false))
		ok = false;
	ack(ok);
}

void MockLedDevice::switchOffLeds()
{
	recordEvent(QStringLiteral("offLeds"));
	ack(nextOffLedsResult.load());
}

void MockLedDevice::setRefreshDelay(int)
{
	recordEvent(QStringLiteral("setRefreshDelay"));
	ack(true);
}

void MockLedDevice::setSmoothSlowdown(int)
{
	recordEvent(QStringLiteral("setSmoothSlowdown"));
	ack(true);
}

void MockLedDevice::setColorSequence(const QString &)
{
	recordEvent(QStringLiteral("setColorSequence"));
	ack(true);
}

void MockLedDevice::requestFirmwareVersion()
{
	recordEvent(QStringLiteral("requestFirmwareVersion"));
	emit firmwareVersion(QStringLiteral("mock-1.0"));
	ack(true);
}

void MockLedDevice::setColorDepth(int)
{
	recordEvent(QStringLiteral("setColorDepth"));
	ack(true);
}

void MockLedDevice::updateDeviceSettings()
{
	recordEvent(QStringLiteral("updateDeviceSettings"));
	AckBatch batch(this);
	if (callBaseUpdateSettings.load())
		AbstractLedDevice::updateDeviceSettings();
	ack(nextUpdateSettingsResult.load());
}

void LedDeviceManagerTest::initTestCase()
{
	qRegisterMetaType<QList<QRgb>>("QList<QRgb>");
	static QTemporaryDir s_configDir;
	QVERIFY(s_configDir.isValid());
	Settings::Initialize(s_configDir.path(), false);

	// Snapshot existing settings so this test suite does not pollute others
	m_prevConnectedDevice = Settings::getConnectedDevice();
	m_prevKeepLightsOnAfterSuspend = Settings::isKeepLightsOnAfterSuspend();
	m_prevBacklightEnabled = Settings::isBacklightEnabled();
	m_prevPingEverySecond = Settings::isPingDeviceEverySecond();

	Settings::setConnectedDevice(SupportedDevices::DeviceTypeVirtual);
	Settings::setKeepLightsOnAfterSuspend(false);
	Settings::setIsBacklightEnabled(true);
	Settings::setPingDeviceEverySecond(true);
}

void LedDeviceManagerTest::cleanupTestCase()
{
	Settings::setConnectedDevice(m_prevConnectedDevice);
	Settings::setKeepLightsOnAfterSuspend(m_prevKeepLightsOnAfterSuspend);
	Settings::setIsBacklightEnabled(m_prevBacklightEnabled);
	Settings::setPingDeviceEverySecond(m_prevPingEverySecond);
}

void LedDeviceManagerTest::testAckBatchAndUpdateColorsFalse()
{
	MockLedDevice dev;
	QSignalSpy ackSpy(&dev, &AbstractLedDevice::commandCompleted);

	// Nested AckBatch emits exactly 1 ack (ANDing results)
	dev.runNestedAckBatch(true, true);
	QCOMPARE(ackSpy.count(), 1);
	QCOMPARE(ackSpy.takeFirst().at(0).toBool(), true);

	dev.runNestedAckBatch(true, false);
	QCOMPARE(ackSpy.count(), 1);
	QCOMPARE(ackSpy.takeFirst().at(0).toBool(), false);

	dev.runNestedAckBatch(false, true);
	QCOMPARE(ackSpy.count(), 1);
	QCOMPARE(ackSpy.takeFirst().at(0).toBool(), false);

	// Section 1.6: every setter with updateColors == false must ack(true) once
	dev.setGamma(2.0, false);
	dev.setBrightness(80, false);
	dev.setBrightnessCap(90, false);
	dev.setDitheringEnabled(true, false);
	dev.setLedMilliAmps(60, false);
	dev.setPowerSupplyAmps(2.0, false);
	dev.setLuminosityThreshold(5, false);
	dev.setMinimumLuminosityThresholdEnabled(true, false);
	dev.updateWBAdjustments(QList<WBAdjustment>(), false);

	QCOMPARE(ackSpy.count(), 9);
	for (int i = 0; i < 9; ++i)
		QCOMPARE(ackSpy.at(i).at(0).toBool(), true);
	ackSpy.clear();

	// Composite updateDeviceSettings() emits exactly 1 ack
	dev.updateDeviceSettings();
	QCOMPARE(ackSpy.count(), 1);
	QCOMPARE(ackSpy.takeFirst().at(0).toBool(), true);
}

void LedDeviceManagerTest::testCommandFailureTriggersRecoveryAndReplay()
{
	MockLedDevice *dev = new MockLedDevice();
	TestableLedDeviceManager mgr(dev);

	QSignalSpy ioSpy(&mgr, &LedDeviceManager::ioDeviceSuccess);
	QSignalSpy openSpy(&mgr, &LedDeviceManager::openDeviceSuccess);

	mgr.init();
	QTRY_COMPARE(openSpy.count(), 1);
	QTRY_VERIFY(ioSpy.count() >= 1 && ioSpy.last().at(0).toBool() == true);

	const QList<QRgb> red = { qRgb(255, 0, 0) };
	const QList<QRgb> blue = { qRgb(0, 0, 255) };

	mgr.setColors(red);
	QTRY_COMPARE(ioSpy.count(), 2);
	QCOMPARE(ioSpy.last().at(0).toBool(), true);

	// Now fail setColors via commandCompleted(false) ONLY (no ioDeviceSuccess from device)
	ioSpy.clear();
	openSpy.clear();
	dev->takeEvents();
	dev->nextSetColorsResult.store(false);

	mgr.setColors(blue);

	// Must emit ioDeviceSuccess(false) once, enter Closed, and arm recreate timer
	QTRY_COMPARE(ioSpy.count(), 1);
	QCOMPARE(ioSpy.at(0).at(0).toBool(), false);
	QCOMPARE(mgr.m_linkState, LedDeviceManager::LinkState::Closed);
	QVERIFY(mgr.m_recreateTimer->isActive());

	// Allow writes to succeed again before recreate fires
	dev->nextSetColorsResult.store(true);

	// Wait for recreate -> open -> UpdateDeviceSettings -> SetColors(blue) replay
	QTRY_VERIFY_WITH_TIMEOUT(openSpy.count() == 1 && ioSpy.count() >= 3, 2000);
	QCOMPARE(ioSpy.at(0).at(0).toBool(), false);
	QCOMPARE(ioSpy.at(1).at(0).toBool(), true);
	QCOMPARE(ioSpy.at(2).at(0).toBool(), true);
	QCOMPARE(mgr.m_linkState, LedDeviceManager::LinkState::Ready);

	// Verify close() ran on m_ledDeviceThread, never on the manager thread
	const QList<QThread*> closeThreads = dev->closeThreadsSnapshot();
	QVERIFY(!closeThreads.isEmpty());
	for (QThread *t : closeThreads) {
		QCOMPARE(t, mgr.m_ledDeviceThread);
		QVERIFY(t != QThread::currentThread());
	}

	// Verify replay order after reopen: close -> open -> updateDeviceSettings -> setColors(blue)
	const QStringList events = dev->eventsSnapshot();
	const int closeIdx = events.indexOf(QStringLiteral("close"));
	const int openIdx = events.indexOf(QStringLiteral("open"), closeIdx + 1);
	const int settingsIdx = events.indexOf(QStringLiteral("updateDeviceSettings"), openIdx + 1);
	const int colorsIdx = events.lastIndexOf(QStringLiteral("setColors"));
	QVERIFY(closeIdx >= 0 && openIdx > closeIdx && settingsIdx > openIdx && colorsIdx > settingsIdx);
	QCOMPARE(dev->colorsHistorySnapshot().last(), blue);
}

void LedDeviceManagerTest::testCommandTimeoutSingleEmitAndLateAckIgnored()
{
	MockLedDevice *dev = new MockLedDevice();
	TestableLedDeviceManager mgr(dev);
	mgr.m_cmdTimeoutMs = 60;

	QSignalSpy ioSpy(&mgr, &LedDeviceManager::ioDeviceSuccess);
	QSignalSpy openSpy(&mgr, &LedDeviceManager::openDeviceSuccess);

	mgr.init();
	QTRY_COMPARE(openSpy.count(), 1);
	QTRY_COMPARE(ioSpy.count(), 1);
	ioSpy.clear();
	openSpy.clear();

	// Hold setColors on device thread until command timeout fires
	dev->holdSetColors.store(true);
	dev->failFirstSetColorsOnly.store(true); // timed-out command's late ack will be false, replay succeeds

	const QList<QRgb> green = { qRgb(0, 255, 0) };
	mgr.setColors(green);

	QVERIFY(dev->setColorsEnteredSem.tryAcquire(1, 1000));
	QTRY_COMPARE_WITH_TIMEOUT(ioSpy.count(), 1, 1000);
	QCOMPARE(ioSpy.at(0).at(0).toBool(), false);
	QCOMPARE(mgr.m_linkState, LedDeviceManager::LinkState::Closed);

	// Allow future setColors to succeed without blocking, then release the timed-out one
	dev->holdSetColors.store(false);
	dev->setColorsReleaseSem.release();

	QTRY_VERIFY_WITH_TIMEOUT(openSpy.count() == 1 && mgr.m_linkState == LedDeviceManager::LinkState::Ready && ioSpy.count() >= 3, 2000);

	// Exactly one failure emitted from the timeout (not 2 from timeout, and not another from late ack)
	int falseCount = 0;
	for (const auto &sig : ioSpy) {
		if (!sig.at(0).toBool())
			++falseCount;
	}
	QCOMPARE(falseCount, 1);
}

void LedDeviceManagerTest::testBackoffResetsOnlyOnFrameCommand()
{
	MockLedDevice *dev = new MockLedDevice();
	TestableLedDeviceManager mgr(dev);

	// Simulate review item #3: open() AND UpdateDeviceSettings BOTH succeed,
	// while the replayed SetColors keeps failing (e.g. Lightpack colors.count() > maxLedsCount()).
	dev->callBaseUpdateSettings.store(false);
	dev->nextOpenResult.store(true);
	dev->nextUpdateSettingsResult.store(true);
	dev->nextSetColorsResult.store(false);

	QSignalSpy ioSpy(&mgr, &LedDeviceManager::ioDeviceSuccess);

	const QList<QRgb> frame = { qRgb(100, 150, 200) };
	mgr.setColors(frame); // saved while Closed so replay queues UpdateDeviceSettings -> SetColors
	mgr.init();

	// 1st attempt: open ok -> UpdateDeviceSettings ok (must NOT reset backoff!) -> SetColors fails -> 200ms
	QTRY_VERIFY_WITH_TIMEOUT(mgr.m_failedCreationAttempts >= 1 && mgr.m_recreateTimer->isActive(), 1000);
	QCOMPARE(mgr.m_recreateTimer->interval(), 200);
	QCOMPARE(mgr.m_failedCreationAttempts, 1);

	// 2nd attempt: open ok -> UpdateDeviceSettings ok -> SetColors fails -> 500ms
	QTRY_VERIFY_WITH_TIMEOUT(mgr.m_failedCreationAttempts >= 2 && mgr.m_recreateTimer->isActive(), 1500);
	QCOMPARE(mgr.m_recreateTimer->interval(), 500);
	QCOMPARE(mgr.m_failedCreationAttempts, 2);

	// 3rd attempt: open ok -> UpdateDeviceSettings ok -> SetColors fails -> 1000ms
	QTRY_VERIFY_WITH_TIMEOUT(mgr.m_failedCreationAttempts >= 3 && mgr.m_recreateTimer->isActive(), 2000);
	QCOMPARE(mgr.m_recreateTimer->interval(), 1000);
	QCOMPARE(mgr.m_failedCreationAttempts, 3);

	// Once SetColors succeeds, backoff counter resets to 0
	dev->nextSetColorsResult.store(true);
	QTRY_VERIFY_WITH_TIMEOUT(
		mgr.m_linkState == LedDeviceManager::LinkState::Ready && mgr.m_failedCreationAttempts == 0,
		2500);
	QVERIFY(ioSpy.last().at(0).toBool());
}

void LedDeviceManagerTest::testSwitchOffOnQueueRaceAndPersistence()
{
	MockLedDevice *dev = new MockLedDevice();
	TestableLedDeviceManager mgr(dev);

	QSignalSpy ioSpy(&mgr, &LedDeviceManager::ioDeviceSuccess);
	mgr.init();
	QTRY_VERIFY(ioSpy.count() >= 1 && mgr.m_linkState == LedDeviceManager::LinkState::Ready);

	// Part A (§1.3): hold an in-flight setColors, then rapid switchOff -> switchOn
	dev->takeEvents();
	dev->holdSetColors.store(true);
	const QList<QRgb> white = { qRgb(255, 255, 255) };
	mgr.setColors(white);
	QVERIFY(dev->setColorsEnteredSem.tryAcquire(1, 1000));

	mgr.switchOffLeds();
	QCOMPARE(mgr.m_backlightStatus, Backlight::StatusOff);
	QVERIFY(mgr.m_cmdQueue.contains(LedDeviceCommands::OffLeds));

	mgr.switchOnLeds();
	QCOMPARE(mgr.m_backlightStatus, Backlight::StatusOn);
	QVERIFY(!mgr.m_cmdQueue.contains(LedDeviceCommands::OffLeds));
	QVERIFY(mgr.m_cmdQueue.contains(LedDeviceCommands::SetColors));

	dev->holdSetColors.store(false);
	dev->setColorsReleaseSem.release();

	QTRY_VERIFY_WITH_TIMEOUT(mgr.m_isLastCommandCompleted && mgr.m_cmdQueue.isEmpty(), 1000);
	QCOMPARE(mgr.m_backlightStatus, Backlight::StatusOn);
	QVERIFY(!dev->eventsSnapshot().contains(QStringLiteral("offLeds")));

	// Part B (R5): switchOffLeds while device is in Closed/recovering state must replay OffLeds on reopen
	dev->takeEvents();
	dev->nextSetColorsResult.store(false);
	mgr.setColors(white);
	QTRY_COMPARE(mgr.m_linkState, LedDeviceManager::LinkState::Closed);

	mgr.switchOffLeds();
	QCOMPARE(mgr.m_backlightStatus, Backlight::StatusOff);

	dev->nextSetColorsResult.store(true);
	QTRY_VERIFY_WITH_TIMEOUT(
		mgr.m_linkState == LedDeviceManager::LinkState::Ready &&
		dev->eventsSnapshot().contains(QStringLiteral("offLeds")),
		2000);

	const QStringList events = dev->eventsSnapshot();
	const int settingsIdx = events.lastIndexOf(QStringLiteral("updateDeviceSettings"));
	const int offIdx = events.lastIndexOf(QStringLiteral("offLeds"));
	QVERIFY(settingsIdx >= 0 && offIdx > settingsIdx);
}

void LedDeviceManagerTest::testSleepCancelsRecoveryClosesOnDeviceThreadAndResumes()
{
	MockLedDevice *dev = new MockLedDevice();
	TestableLedDeviceManager mgr(dev);

	QSignalSpy ioSpy(&mgr, &LedDeviceManager::ioDeviceSuccess);
	QSignalSpy openSpy(&mgr, &LedDeviceManager::openDeviceSuccess);

	mgr.init();
	QTRY_VERIFY(openSpy.count() == 1 && ioSpy.count() >= 1);

	const QList<QRgb> color = { qRgb(10, 20, 30) };
	mgr.setColors(color);
	QTRY_COMPARE(ioSpy.count(), 2);

	// Fail a command so m_recreateTimer is armed
	dev->nextSetColorsResult.store(false);
	mgr.setColors(color);
	QTRY_VERIFY(mgr.m_recreateTimer->isActive());

	// Enter Sleeping: must cancel recreate timer, send offLeds then close on device thread
	dev->takeEvents();
	dev->nextSetColorsResult.store(true);
	mgr.onSessionChange(SystemSession::Status::Sleeping);

	QVERIFY(mgr.m_isSleeping);
	QVERIFY(!mgr.m_recreateTimer->isActive());
	QCOMPARE(mgr.m_linkState, LedDeviceManager::LinkState::Closed);

	QTRY_VERIFY_WITH_TIMEOUT(dev->eventsSnapshot().contains(QStringLiteral("close")), 1000);
	const QStringList sleepEvents = dev->eventsSnapshot();
	const int offIdx = sleepEvents.indexOf(QStringLiteral("offLeds"));
	const int closeIdx = sleepEvents.indexOf(QStringLiteral("close"));
	QVERIFY(offIdx >= 0 && closeIdx > offIdx);
	for (QThread *t : dev->closeThreadsSnapshot()) {
		QCOMPARE(t, mgr.m_ledDeviceThread);
	}

	// Frames arriving during sleep must NOT schedule recovery
	mgr.setColors(color);
	QTest::qWait(50);
	QVERIFY(!mgr.m_recreateTimer->isActive());

	// Resume twice (e.g. ResumingAutomatic + Resuming): must be idempotent (only 1 open!)
	openSpy.clear();
	dev->takeEvents();
	mgr.onSessionChange(SystemSession::Status::ResumingAutomatic);
	mgr.onSessionChange(SystemSession::Status::Resuming);

	QVERIFY(!mgr.m_isSleeping);
	QTRY_VERIFY_WITH_TIMEOUT(openSpy.count() == 1 && mgr.m_linkState == LedDeviceManager::LinkState::Ready, 1500);
	QTest::qWait(100);
	QCOMPARE(openSpy.count(), 1);
	QCOMPARE(dev->eventsSnapshot().count(QStringLiteral("open")), 1);

	// Sleep again, then wake via DisplayOn fallback (missed Resuming event)
	mgr.onSessionChange(SystemSession::Status::Sleeping);
	QVERIFY(mgr.m_isSleeping);
	QTRY_VERIFY_WITH_TIMEOUT(dev->eventsSnapshot().last() == QStringLiteral("close"), 1000);

	openSpy.clear();
	mgr.onSessionChange(SystemSession::Status::DisplayOn);
	QVERIFY(!mgr.m_isSleeping);
	QTRY_VERIFY_WITH_TIMEOUT(openSpy.count() == 1 && mgr.m_linkState == LedDeviceManager::LinkState::Ready, 1500);
}

void LedDeviceManagerTest::testManualRecreateAndSwitchOnClearSleep()
{
	MockLedDevice *dev = new MockLedDevice();
	TestableLedDeviceManager mgr(dev);

	QSignalSpy openSpy(&mgr, &LedDeviceManager::openDeviceSuccess);
	mgr.init();
	QTRY_VERIFY(openSpy.count() == 1 && mgr.m_linkState == LedDeviceManager::LinkState::Ready);

	// Enter Sleeping and verify timer-driven recreate stays blocked while sleeping
	mgr.onSessionChange(SystemSession::Status::Sleeping);
	QVERIFY(mgr.m_isSleeping);
	openSpy.clear();
	mgr.onRecreateTimerTimeout();
	QTest::qWait(50);
	QVERIFY(mgr.m_isSleeping);
	QCOMPARE(openSpy.count(), 0);

	// Manual public recreateLedDevice() MUST clear m_isSleeping and recover
	mgr.recreateLedDevice();
	QVERIFY(!mgr.m_isSleeping);
	QTRY_VERIFY_WITH_TIMEOUT(openSpy.count() == 1 && mgr.m_linkState == LedDeviceManager::LinkState::Ready, 1500);

	// Sleep again, then verify user switchOnLeds() also wakes and recovers
	mgr.onSessionChange(SystemSession::Status::Sleeping);
	QVERIFY(mgr.m_isSleeping);
	openSpy.clear();
	mgr.switchOnLeds();
	QVERIFY(!mgr.m_isSleeping);
	QTRY_VERIFY_WITH_TIMEOUT(openSpy.count() == 1 && mgr.m_linkState == LedDeviceManager::LinkState::Ready, 1500);
}

void LedDeviceManagerTest::testUnattendedResumeAutomaticKeepsLedsOff()
{
	MockLedDevice *dev = new MockLedDevice();
	TestableLedDeviceManager mgr(dev);

	QSignalSpy openSpy(&mgr, &LedDeviceManager::openDeviceSuccess);
	mgr.init();
	QTRY_VERIFY(openSpy.count() == 1 && mgr.m_linkState == LedDeviceManager::LinkState::Ready);

	const QList<QRgb> warm = { qRgb(255, 180, 80) };
	mgr.setColors(warm);
	QTRY_VERIFY(mgr.m_isLastCommandCompleted);

	// Simulate LightpackApplication::onSessionChange(Sleeping) + LedDeviceManager::onSessionChange(Sleeping)
	mgr.switchOffLeds();
	mgr.onSessionChange(SystemSession::Status::Sleeping);
	QVERIFY(mgr.m_isSleeping);
	QCOMPARE(mgr.m_backlightStatus, Backlight::StatusOff);
	QTRY_VERIFY_WITH_TIMEOUT(dev->eventsSnapshot().contains(QStringLiteral("close")), 1000);

	// Unattended wake: ResumingAutomatic ONLY (app does NOT call switchOnLeds)
	openSpy.clear();
	dev->takeEvents();
	dev->callBaseUpdateSettings.store(false);
	mgr.onSessionChange(SystemSession::Status::ResumingAutomatic);

	// Device reopens, but backlight stays StatusOff and replays OffLeds (not SetColors)
	QTRY_VERIFY_WITH_TIMEOUT(
		openSpy.count() == 1 &&
		mgr.m_linkState == LedDeviceManager::LinkState::Ready &&
		dev->eventsSnapshot().contains(QStringLiteral("offLeds")),
		1500);
	QCOMPARE(mgr.m_backlightStatus, Backlight::StatusOff);
	QVERIFY(!dev->eventsSnapshot().contains(QStringLiteral("setColors")));

	// Later interactive resume: app calls switchOnLeds() + Resuming arrives
	dev->takeEvents();
	mgr.switchOnLeds();
	mgr.onSessionChange(SystemSession::Status::Resuming);
	QTRY_VERIFY_WITH_TIMEOUT(dev->eventsSnapshot().contains(QStringLiteral("setColors")), 1000);
	QCOMPARE(mgr.m_backlightStatus, Backlight::StatusOn);
	QCOMPARE(dev->eventsSnapshot().count(QStringLiteral("open")), 0); // no redundant reopen
}

void LedDeviceManagerTest::testOverlappingOpensAndOutOfBandErrors()
{
	MockLedDevice *dev = new MockLedDevice();
	TestableLedDeviceManager mgr(dev);

	QSignalSpy openSpy(&mgr, &LedDeviceManager::openDeviceSuccess);
	QSignalSpy ioSpy(&mgr, &LedDeviceManager::ioDeviceSuccess);

	// Hold open #1 on device thread, and make open #1 fail while open #2 succeeds
	dev->holdOpen.store(true);
	dev->failFirstOpenOnly.store(true);
	dev->nextOpenResult.store(true);

	mgr.init();
	QVERIFY(dev->openEnteredSem.tryAcquire(1, 1000));
	QCOMPARE(mgr.m_pendingOpens.value(dev), 1);
	QCOMPARE(mgr.m_linkState, LedDeviceManager::LinkState::Opening);

	// Out-of-band error while Opening must be ignored
	dev->emitOutOfBandError();
	QTest::qWait(50);
	QCOMPARE(ioSpy.count(), 0);
	QCOMPARE(mgr.m_linkState, LedDeviceManager::LinkState::Opening);

	// Trigger overlapping reopen while open #1 is still running
	mgr.recreateLedDevice();
	QCOMPARE(mgr.m_pendingOpens.value(dev), 2);

	// Release open #1 (fails) and let open #2 proceed (succeeds)
	dev->holdOpen.store(false);
	dev->openReleaseSem.release();

	QTRY_VERIFY_WITH_TIMEOUT(openSpy.count() == 1 && mgr.m_linkState == LedDeviceManager::LinkState::Ready, 1500);
	QCOMPARE(mgr.m_pendingOpens.value(dev), 0);
	for (const auto &sig : ioSpy)
		QCOMPARE(sig.at(0).toBool(), true);

	QTRY_VERIFY(ioSpy.count() >= 1 && mgr.m_isLastCommandCompleted);

	// Now that link is Ready, out-of-band error MUST trigger recovery
	ioSpy.clear();
	openSpy.clear();
	dev->emitOutOfBandError();

	QTRY_VERIFY_WITH_TIMEOUT(ioSpy.count() >= 1 && ioSpy.at(0).at(0).toBool() == false, 1000);
	QTRY_VERIFY_WITH_TIMEOUT(openSpy.count() == 1 && mgr.m_linkState == LedDeviceManager::LinkState::Ready, 2000);
}

void LedDeviceManagerTest::testDeviceSwitchABAStaleOpenIgnored()
{
	MockLedDevice *devA = new MockLedDevice();
	MockLedDevice *devB = new MockLedDevice();
	QHash<SupportedDevices::DeviceType, AbstractLedDevice *> map;
	map.insert(SupportedDevices::DeviceTypeVirtual, devA);
	map.insert(SupportedDevices::DeviceTypeLightpack, devB);

	TestableLedDeviceManager mgr(map);
	QSignalSpy openSpy(&mgr, &LedDeviceManager::openDeviceSuccess);
	QSignalSpy ioSpy(&mgr, &LedDeviceManager::ioDeviceSuccess);

	// Start opening A and hold open #1 (which will fail)
	Settings::setConnectedDevice(SupportedDevices::DeviceTypeVirtual);
	devA->holdOpen.store(true);
	devA->failFirstOpenOnly.store(true);
	devA->nextOpenResult.store(true);
	devB->nextOpenResult.store(true);

	mgr.init();
	QVERIFY(devA->openEnteredSem.tryAcquire(1, 1000));
	QCOMPARE(mgr.m_pendingOpens.value(devA), 1);

	// Switch A -> B -> A while A's first open is still running on m_ledDeviceThread
	Settings::setConnectedDevice(SupportedDevices::DeviceTypeLightpack);
	mgr.recreateLedDevice();
	QCOMPARE(mgr.m_pendingOpens.value(devA), 1);
	QCOMPARE(mgr.m_pendingOpens.value(devB), 1);

	Settings::setConnectedDevice(SupportedDevices::DeviceTypeVirtual);
	mgr.recreateLedDevice();
	QCOMPARE(mgr.m_pendingOpens.value(devA), 2);
	QCOMPARE(mgr.m_pendingOpens.value(devB), 1);

	// Release A's first (failing) open; B's open and A's second (succeeding) open run in FIFO order
	devA->holdOpen.store(false);
	devA->openReleaseSem.release();

	QTRY_VERIFY_WITH_TIMEOUT(openSpy.count() == 1 && mgr.m_linkState == LedDeviceManager::LinkState::Ready, 1500);
	QCOMPARE(mgr.m_pendingOpens.value(devA), 0);
	QCOMPARE(mgr.m_pendingOpens.value(devB), 0);
	for (const auto &sig : ioSpy)
		QCOMPARE(sig.at(0).toBool(), true);
}

void LedDeviceManagerTest::testPendingOpensSelfHealsAfterConsecutiveTimeouts()
{
	MockLedDevice *dev = new MockLedDevice();
	TestableLedDeviceManager mgr(dev);
	mgr.m_openTimeoutMs = 50;

	// First open drops openDeviceSuccess (leaking m_pendingOpens = 1); subsequent opens work normally.
	dev->dropFirstOpenEmit.store(true);
	dev->nextOpenResult.store(true);

	QSignalSpy openSpy(&mgr, &LedDeviceManager::openDeviceSuccess);

	mgr.init();

	// After 3 consecutive open timeouts, m_pendingOpens resets and the next retry recovers cleanly.
	QTRY_VERIFY_WITH_TIMEOUT(
		openSpy.count() == 1 && mgr.m_linkState == LedDeviceManager::LinkState::Ready,
		4000);
	QCOMPARE(mgr.m_pendingOpens.value(dev), 0);
	QCOMPARE(mgr.m_consecutiveOpenTimeouts, 0);
}

void LedDeviceManagerTest::testFirmwareVersionRequestedWhileOpeningIsReplayed()
{
	MockLedDevice *dev = new MockLedDevice();
	TestableLedDeviceManager mgr(dev);

	QSignalSpy fwSpy(&mgr, &LedDeviceManager::firmwareVersion);

	dev->holdOpen.store(true);
	mgr.init();
	QVERIFY(dev->openEnteredSem.tryAcquire(1, 1000));
	QCOMPARE(mgr.m_linkState, LedDeviceManager::LinkState::Opening);

	mgr.requestFirmwareVersion();
	QVERIFY(mgr.m_firmwareVersionRequested);

	dev->holdOpen.store(false);
	dev->openReleaseSem.release();

	QTRY_COMPARE_WITH_TIMEOUT(fwSpy.count(), 1, 1500);
	QCOMPARE(fwSpy.at(0).at(0).toString(), QStringLiteral("mock-1.0"));
	QTRY_VERIFY(!mgr.m_firmwareVersionRequested);
}

void LedDeviceManagerTest::testOpenTimeoutRetriesCleanly()
{
	MockLedDevice *dev = new MockLedDevice();
	TestableLedDeviceManager mgr(dev);
	mgr.m_openTimeoutMs = 60;

	QSignalSpy ioSpy(&mgr, &LedDeviceManager::ioDeviceSuccess);
	QSignalSpy openSpy(&mgr, &LedDeviceManager::openDeviceSuccess);

	dev->holdOpen.store(true);
	mgr.init();
	QVERIFY(dev->openEnteredSem.tryAcquire(1, 1000));

	QTRY_COMPARE_WITH_TIMEOUT(ioSpy.count(), 1, 1000);
	QCOMPARE(ioSpy.at(0).at(0).toBool(), false);
	QCOMPARE(mgr.m_linkState, LedDeviceManager::LinkState::Closed);
	QVERIFY(mgr.m_recreateTimer->isActive());

	// Release slow open #1 (returns true AFTER timeout while Closed -> ignored)
	// Then recreate timer fires open #2 -> transitions cleanly to Ready
	dev->holdOpen.store(false);
	dev->openReleaseSem.release();

	QTRY_VERIFY_WITH_TIMEOUT(openSpy.count() == 1 && mgr.m_linkState == LedDeviceManager::LinkState::Ready, 2000);
	QTRY_VERIFY_WITH_TIMEOUT(ioSpy.count() >= 2 && ioSpy.last().at(0).toBool() == true, 1500);
}

void LedDeviceManagerTest::testRealDevicesSingleAckAndCleanDestructor()
{
	// 1. LedDeviceLightpack: close() must not block ping timer signals; updateDeviceSettings() emits 1 ack
	{
		LedDeviceLightpack lp;
		lp.close();
		const QList<QTimer*> timers = lp.findChildren<QTimer*>();
		QVERIFY(!timers.isEmpty());
		for (QTimer *t : timers) {
			QVERIFY(!t->signalsBlocked());
			QVERIFY(!t->isActive());
		}

		QSignalSpy lpAckSpy(&lp, &AbstractLedDevice::commandCompleted);
		lp.updateDeviceSettings();
		QCOMPARE(lpAckSpy.count(), 1);
	}

	// 2. LedDeviceAdalight & LedDeviceArdulight: updateDeviceSettings() emits 1 ack, close() emits 0 acks
	{
		LedDeviceAdalight ada(QStringLiteral("COM_NONEXISTENT_TEST"), 115200);
		QSignalSpy adaAckSpy(&ada, &AbstractLedDevice::commandCompleted);
		ada.updateDeviceSettings();
		QCOMPARE(adaAckSpy.count(), 1);
		adaAckSpy.clear();
		ada.close();
		QCOMPARE(adaAckSpy.count(), 0);
	}
	{
		LedDeviceArdulight ardu(QStringLiteral("COM_NONEXISTENT_TEST"), 115200);
		QSignalSpy arduAckSpy(&ardu, &AbstractLedDevice::commandCompleted);
		ardu.updateDeviceSettings();
		QCOMPARE(arduAckSpy.count(), 1);
		arduAckSpy.clear();
		ardu.close();
		QCOMPARE(arduAckSpy.count(), 0);
	}

	// 3. LedDeviceVirtual: updateDeviceSettings() emits 1 commandCompleted(true)
	{
		LedDeviceVirtual virt;
		QSignalSpy ackSpy(&virt, &AbstractLedDevice::commandCompleted);
		virt.updateDeviceSettings();
		QCOMPARE(ackSpy.count(), 1);
		QCOMPARE(ackSpy.at(0).at(0).toBool(), true);
	}

	// 4. LedDeviceManager destructor when init() was never called must succeed cleanly
	{
		LedDeviceManager uninitializedMgr;
		Q_UNUSED(uninitializedMgr);
	}

	// 5. LedDeviceManager destructor after active init() must close device on device thread and exit cleanly
	{
		MockLedDevice *dev = new MockLedDevice();
		QThread *deviceThread = nullptr;
		{
			TestableLedDeviceManager mgr(dev);
			QSignalSpy openSpy(&mgr, &LedDeviceManager::openDeviceSuccess);
			mgr.init();
			deviceThread = mgr.m_ledDeviceThread;
			QTRY_COMPARE(openSpy.count(), 1);
			QVERIFY(deviceThread->isRunning());
		}
		// Destructor finished without hanging or warning
		QVERIFY(deviceThread != nullptr);
	}
}
