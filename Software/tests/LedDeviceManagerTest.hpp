#pragma once

#include <QObject>
#include <QHash>
#include <QMutex>
#include <QSemaphore>
#include <QStringList>
#include <QThread>
#include <atomic>

#include "../src/AbstractLedDevice.hpp"
#include "../src/LedDeviceManager.hpp"

class MockLedDevice : public AbstractLedDevice
{
	Q_OBJECT
public:
	explicit MockLedDevice(QObject *parent = nullptr);

	QString name() const override { return QStringLiteral("mock"); }
	int defaultLedsCount() override { return 10; }
	int maxLedsCount() override { return 10; }

	void emitOutOfBandError();
	bool runNestedAckBatch(bool firstOk, bool secondOk);

	QStringList takeEvents();
	QStringList eventsSnapshot();
	QList<QThread*> closeThreadsSnapshot();
	QList<QThread*> openThreadsSnapshot();
	QList<QList<QRgb>> colorsHistorySnapshot();

	std::atomic<bool> nextOpenResult{ true };
	std::atomic<bool> failFirstOpenOnly{ false };
	std::atomic<bool> dropFirstOpenEmit{ false };
	std::atomic<bool> nextSetColorsResult{ true };
	std::atomic<bool> failFirstSetColorsOnly{ false };
	std::atomic<bool> nextUpdateSettingsResult{ true };
	std::atomic<bool> nextOffLedsResult{ true };
	std::atomic<bool> callBaseUpdateSettings{ true };

	std::atomic<bool> holdOpen{ false };
	std::atomic<bool> holdSetColors{ false };
	QSemaphore openEnteredSem{ 0 };
	QSemaphore openReleaseSem{ 0 };
	QSemaphore setColorsEnteredSem{ 0 };
	QSemaphore setColorsReleaseSem{ 0 };

public slots:
	void open() override;
	void close() override;
	void setColors(const QList<QRgb> &colors) override;
	void switchOffLeds() override;
	void setRefreshDelay(int value) override;
	void setSmoothSlowdown(int value) override;
	void setColorSequence(const QString &value) override;
	void requestFirmwareVersion() override;
	void setColorDepth(int value) override;
	void updateDeviceSettings() override;

private:
	void recordEvent(const QString &event);

	QMutex m_mutex;
	QStringList m_events;
	QList<QThread*> m_closeThreads;
	QList<QThread*> m_openThreads;
	QList<QList<QRgb>> m_colorsHistory;
};

class TestableLedDeviceManager : public LedDeviceManager
{
	Q_OBJECT
public:
	explicit TestableLedDeviceManager(MockLedDevice *device, QObject *parent = nullptr)
		: LedDeviceManager(parent), m_defaultDevice(device) {}

	explicit TestableLedDeviceManager(QHash<SupportedDevices::DeviceType, AbstractLedDevice *> devices, QObject *parent = nullptr)
		: LedDeviceManager(parent), m_deviceMap(std::move(devices)) {}

protected:
	AbstractLedDevice *createLedDevice(SupportedDevices::DeviceType type) override
	{
		if (m_deviceMap.contains(type))
			return m_deviceMap.value(type);
		return m_defaultDevice;
	}

private:
	MockLedDevice *m_defaultDevice{ nullptr };
	QHash<SupportedDevices::DeviceType, AbstractLedDevice *> m_deviceMap;
};

class LedDeviceManagerTest : public QObject
{
	Q_OBJECT
public:
	LedDeviceManagerTest() = default;

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();
	void testAckBatchAndUpdateColorsFalse();
	void testCommandFailureTriggersRecoveryAndReplay();
	void testCommandTimeoutSingleEmitAndLateAckIgnored();
	void testBackoffResetsOnlyOnFrameCommand();
	void testSwitchOffOnQueueRaceAndPersistence();
	void testSleepCancelsRecoveryClosesOnDeviceThreadAndResumes();
	void testManualRecreateAndSwitchOnClearSleep();
	void testUnattendedResumeAutomaticKeepsLedsOff();
	void testOverlappingOpensAndOutOfBandErrors();
	void testDeviceSwitchABAStaleOpenIgnored();
	void testPendingOpensSelfHealsAfterConsecutiveTimeouts();
	void testFirmwareVersionRequestedWhileOpeningIsReplayed();
	void testOpenTimeoutRetriesCleanly();
	void testRealDevicesSingleAckAndCleanDestructor();

private:
	SupportedDevices::DeviceType m_prevConnectedDevice{ SupportedDevices::DefaultDeviceType };
	bool m_prevKeepLightsOnAfterSuspend{ false };
	bool m_prevBacklightEnabled{ true };
	bool m_prevPingEverySecond{ true };
};
