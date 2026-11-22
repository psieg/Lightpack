#include <QtGui>
#include <QApplication>
#include "LightpackPluginInterface.hpp"
#include "Plugin.hpp"
#include "Settings.hpp"
#include "version.h"
#include "debug.h"

using namespace SettingsScope;

const int LightpackPluginInterface::SignalWaitTimeoutMs = 1000; // 1 second

LightpackPluginInterface::LightpackPluginInterface(QObject *parent) :
	QObject(parent)
{
	m_isRequestBacklightStatusDone = true;
	m_backlightStatusResult = Backlight::StatusUnknown;
	m_gamma = SettingsScope::Profile::Device::GammaDefault;
	m_brightness = SettingsScope::Profile::Device::BrightnessDefault;
	m_smooth = SettingsScope::Profile::Device::SmoothDefault;
	m_persistOnUnlock = false;

	initColors(10);
	m_timerLock = new QTimer(this);
	using namespace std::chrono_literals;
	m_timerLock->start(5s); // check in 5000 ms
	connect(m_timerLock, &QTimer::timeout, this, &LightpackPluginInterface::timeoutLock);
	_plugins.clear();
}

LightpackPluginInterface::~LightpackPluginInterface()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;

	delete m_timerLock;
}

//TODO timeout lock
void LightpackPluginInterface::timeoutLock()
{
	DEBUG_HIGH_LEVEL << Q_FUNC_INFO;
	if (lockAlive)
	{
		lockAlive = false;
	}
	else
	{
		if (!lockSessionKeys.isEmpty())
			if (lockSessionKeys[0].indexOf(QStringLiteral("API"), 0) == -1)
				UnLock(lockSessionKeys[0]);
	}
}

void LightpackPluginInterface::updatePlugin(const QList<Plugin*>& plugins)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO;
	if (!lockSessionKeys.isEmpty())
		UnLock(lockSessionKeys[0]);
	lockSessionKeys.clear();
	//emit updateDeviceLockStatus(DeviceLocked::Unlocked, lockSessionKeys);
	_plugins = plugins;

}

bool LightpackPluginInterface::VerifySessionKey(const QString& sessionKey)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << sessionKey;

	foreach(Plugin* plugin, _plugins){
			if (plugin->Guid() == sessionKey)
				return true;
		}
	return false;
}

Plugin* LightpackPluginInterface::findName(const QString& name)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << name;
	foreach(Plugin* plugin, _plugins){
		if (plugin->Name() == name)
			return plugin;
	}

	return NULL;
}

Plugin* LightpackPluginInterface::findSessionKey(const QString& sessionKey)
{
	foreach(Plugin* plugin, _plugins){
		if (plugin->Guid() == sessionKey)
			return plugin;
	}

	return NULL;
}


void LightpackPluginInterface::initColors(int numberOfLeds)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << numberOfLeds;
	m_curColors.clear();
	m_setColors.clear();
	for (int i = 0; i < numberOfLeds; i++)
	{
		m_setColors << 0;
		m_curColors << 0;
	}
}

void LightpackPluginInterface::setNumberOfLeds(int numberOfLeds)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << numberOfLeds;

	initColors(numberOfLeds);
}

void LightpackPluginInterface::resultBacklightStatus(Backlight::Status status)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << status;
	m_backlightStatusResult = status;
	m_isRequestBacklightStatusDone = true;
	switch (m_backlightStatusResult)
	{
		case Backlight::StatusOn:
			emit ChangeStatus(1);
			break;
		case Backlight::StatusOff:
			emit ChangeStatus(0);
			break;
		case Backlight::StatusDeviceError:
		default:
			emit ChangeStatus(-1);
			break;
	}
}

void LightpackPluginInterface::changeProfile(const QString& profile)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << profile;

	emit ChangeProfile(profile);
}

void LightpackPluginInterface::refreshAmbilightEvaluated(double updateResultMs)
{
	DEBUG_HIGH_LEVEL << Q_FUNC_INFO << updateResultMs;

	double secs = updateResultMs / 1000;
	hz = 0;

	if(secs != 0){
		hz = 1 / secs;
	}
}

void LightpackPluginInterface::refreshScreenRect(QRect rect)
{
	screen = rect;
}

void LightpackPluginInterface::updateColorsCache(const QList<QRgb> & colors)
{
	DEBUG_HIGH_LEVEL << Q_FUNC_INFO;
	m_curColors = colors;
}


void LightpackPluginInterface::updateGammaCache(double value)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO;
	m_gamma = value;
}

void LightpackPluginInterface::updateBrightnessCache(int value)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO;
	m_brightness = value;
}

void LightpackPluginInterface::updateSmoothCache(int value)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO;
	m_smooth = value;
}

#ifdef SOUNDVIZ_SUPPORT
void LightpackPluginInterface::updateSoundVizMinColorCache(QColor color)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO;
	m_soundVizMin = color;
}

void LightpackPluginInterface::updateSoundVizMaxColorCache(QColor color)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO;
	m_soundVizMax = color;
}

void LightpackPluginInterface::updateSoundVizLiquidCache(bool value)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO;
	m_soundVizLiquid = value;
}
#endif

QString LightpackPluginInterface::Version()
{
	return QStringLiteral(API_VERSION);
}

// TODO identification plugin locked
QString LightpackPluginInterface::GetSessionKey(const QString& module)
{
	if (module==QStringLiteral("API")) return QStringLiteral("Lock");
	Plugin* plugin = findName(module);
	if (plugin == NULL) return QLatin1String("");
		return plugin->Guid();

}

int LightpackPluginInterface::CheckLock(const QString& sessionKey)
{
	if (lockSessionKeys.isEmpty())
		return 0;
	if (lockSessionKeys[0]==sessionKey)
		return 1;
	return -1;
}


//TODO: lock unlock
bool LightpackPluginInterface::Lock(const QString& sessionKey)
{
	if (sessionKey.isEmpty()) return false;
		if (lockSessionKeys.contains(sessionKey)) return true;
		if (sessionKey.indexOf(QStringLiteral("API"), 0) != -1)
			{
				if (lockSessionKeys.count()>0)
					return false;
				lockSessionKeys.insert(0,sessionKey);
				emit updateDeviceLockStatus(DeviceLocked::Api,lockSessionKeys);
			}
			else
			{
				Plugin* plugin = findSessionKey(sessionKey);
				if (plugin == NULL) return false;

					foreach (const QString& key,lockSessionKeys)
					{
						if (key.indexOf(QStringLiteral("API"), 0) != -1) break;

								Plugin* pluginLock = findSessionKey(key);
								if (pluginLock == NULL) return false;
								DEBUG_LOW_LEVEL << Q_FUNC_INFO << lockSessionKeys.indexOf(key);
								if (plugin->getPriority() > pluginLock->getPriority())
									lockSessionKeys.insert(lockSessionKeys.indexOf(key),sessionKey);
								DEBUG_LOW_LEVEL << Q_FUNC_INFO << lockSessionKeys.indexOf(sessionKey);

					}
					if (!lockSessionKeys.contains(sessionKey))
					{
						DEBUG_LOW_LEVEL << Q_FUNC_INFO << "add to end";
						lockSessionKeys.insert(lockSessionKeys.count(),sessionKey);
					}
					if (lockSessionKeys[0].indexOf(QStringLiteral("API"), 0) != -1)
						emit updateDeviceLockStatus(DeviceLocked::Api,lockSessionKeys);
					else
						emit updateDeviceLockStatus(DeviceLocked::Plugin, lockSessionKeys);
}
		DEBUG_LOW_LEVEL << Q_FUNC_INFO << "lock end";
		lockAlive = true;
		emit ChangeLockStatus (true);
		return true;

}

bool LightpackPluginInterface::UnLock(const QString& sessionKey)
{
	if (lockSessionKeys.isEmpty()) return false;
		if (lockSessionKeys[0]==sessionKey)
		{
			lockSessionKeys.removeFirst();
			if (lockSessionKeys.count()==0)
			{
				if (m_persistOnUnlock) {
					emit updateDeviceLockStatus(DeviceLocked::ApiPersist, lockSessionKeys);
					emit ChangeLockStatus(false);
				} else {
					emit updateDeviceLockStatus(DeviceLocked::Unlocked, lockSessionKeys);
					emit ChangeLockStatus(false);
				}
			}
			else
			{
				if (lockSessionKeys[0].indexOf(QStringLiteral("API"), 0) != -1)
					emit updateDeviceLockStatus(DeviceLocked::Api,lockSessionKeys);
				else
					emit updateDeviceLockStatus(DeviceLocked::Plugin, lockSessionKeys);
			}
			return true;
		}
		else
			if (lockSessionKeys.indexOf(sessionKey)!= -1)
			{
				lockSessionKeys.removeOne(sessionKey);
				if (lockSessionKeys[0].indexOf(QStringLiteral("API"), 0) != -1)
					emit updateDeviceLockStatus(DeviceLocked::Api,lockSessionKeys);
				else
					emit updateDeviceLockStatus(DeviceLocked::Plugin, lockSessionKeys);
				return true;
			}
		return false;

}


void LightpackPluginInterface::SetLockAlive(const QString& sessionKey)
{
	if (lockSessionKeys.isEmpty()) return;
	if (lockSessionKeys[0]!=sessionKey) return;
		lockAlive = true;
}

// TODO: setcolor
bool LightpackPluginInterface::SetColors(const QString& sessionKey, int r, int g, int b)
{
	if (lockSessionKeys.isEmpty()) return false;
	if (lockSessionKeys[0]!=sessionKey) return false;
		lockAlive = true;
	for (int i = 0; i < m_setColors.size(); i++)
	{
			m_setColors[i] = qRgb(r,g,b);
	}
	m_curColors = m_setColors;
	emit updateLedsColors(m_setColors);
	return true;
}

bool LightpackPluginInterface::SetFrame(const QString& sessionKey, QList<QColor> colors)
{
	if (lockSessionKeys.isEmpty()) return false;
	if (lockSessionKeys[0]!=sessionKey) return false;
	lockAlive = true;
	int availSize = colors.size() < m_setColors.size() ? colors.size() : m_setColors.size();
	for (int i = 0; i < availSize; i++)
	{
			m_setColors[i] = colors[i].rgb();
	}
	m_curColors = m_setColors;
	emit updateLedsColors(m_setColors);
	return true;
}

bool LightpackPluginInterface::SetColor(const QString& sessionKey, int ind,int r, int g, int b)
{
	DEBUG_MID_LEVEL << Q_FUNC_INFO << sessionKey;
	if (lockSessionKeys.isEmpty()) return false;
	if (lockSessionKeys[0]!=sessionKey) return false;
	lockAlive = true;
	if (ind>m_setColors.size()-1) return false;
	m_setColors[ind] = qRgb(r,g,b);
	m_curColors = m_setColors;
	emit updateLedsColors(m_setColors);
	return true;
}

bool LightpackPluginInterface::SetGamma(const QString& sessionKey, double gamma)
{
	if (lockSessionKeys.isEmpty()) return false;
	if (lockSessionKeys[0]!=sessionKey) return false;
		if (gamma >= Profile::Device::GammaMin && gamma <= Profile::Device::GammaMax)
		{
			emit updateGamma(gamma);
			return true;
		} else
			return false;
}

bool LightpackPluginInterface::SetBrightness(const QString& sessionKey, int brightness)
{
	if (lockSessionKeys.isEmpty()) return false;
	if (lockSessionKeys[0]!=sessionKey) return false;
		if (brightness >= Profile::Device::BrightnessMin && brightness <= Profile::Device::BrightnessMax)
		{
			emit updateBrightness(brightness);
			return true;
		} else
			return false;
}

bool LightpackPluginInterface::SetSmooth(const QString& sessionKey, int smooth)
{
	if (lockSessionKeys.isEmpty()) return false;
	if (lockSessionKeys[0]!=sessionKey) return false;
		if (smooth >= Profile::Device::SmoothMin && smooth <= Profile::Device::SmoothMax)
		{
				emit updateSmooth(smooth);
				return true;
		} else
			return false;
}

bool LightpackPluginInterface::SetProfile(const QString& sessionKey, const QString& profile)
{
	if (lockSessionKeys.isEmpty()) return false;
	if (lockSessionKeys[0]!=sessionKey) return false;
	if (!Settings::isValidProfileName(profile)) return false;
		QStringList profiles = Settings::findAllProfiles();
		if (profiles.contains(profile))
		{
			emit updateProfile(profile);
			return true;
		} else
			return false;
}

bool LightpackPluginInterface::SetDevice(const QString& sessionKey, const QString& device)
{
	qWarning() << Q_FUNC_INFO << "Unsupported/deprectated API/Plugin command: SetDevice";
	return false;

	if (lockSessionKeys.isEmpty()) return false;
	if (lockSessionKeys[0]!=sessionKey) return false;
		QStringList devices = Settings::getSupportedDevices();
		if (devices.contains(device))
		{
			//Settings::setConnectedDeviceName(device);
			emit changeDevice(device);
			return true;
		} else
			return false;
}

bool LightpackPluginInterface::SetStatus(const QString& sessionKey, int status)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << status;
	if (lockSessionKeys.isEmpty()) return false;
	if (lockSessionKeys[0]!=sessionKey) return false;
		Backlight::Status statusSet = Backlight::StatusUnknown;

		if (status == 1)
			statusSet = Backlight::StatusOn;
		else if (status == 0)
			statusSet = Backlight::StatusOff;

		if (statusSet != Backlight::StatusUnknown)
		{
			emit updateStatus(statusSet);
			return true;
		} else
			return false;
}

bool LightpackPluginInterface::SetLeds(const QString& sessionKey, QList<QRect> leds)
{
	if (lockSessionKeys.isEmpty()) return false;
	if (lockSessionKeys[0]!=sessionKey) return false;
	const int maxLeds = Settings::getNumberOfLeds(Settings::getConnectedDevice());
	int num = 0;
	foreach(QRect rectLed, leds) {
		if (num >= maxLeds)
			break;
		Settings::setLedPosition(num, QPoint(rectLed.x(),rectLed.y()));
		Settings::setLedSize(num,QSize(rectLed.width(),rectLed.height()));
		++num;
	}
	emit updateCountLeds(num);
	QString profile = Settings::getCurrentProfileName();
	emit updateProfile(profile);
	return true;
}

bool LightpackPluginInterface::NewProfile(const QString& sessionKey, const QString& profile)
{
	if (lockSessionKeys.isEmpty()) return false;
	if (lockSessionKeys[0]!=sessionKey) return false;
	if (!Settings::isValidProfileName(profile)) return false;

		Settings::loadOrCreateProfile(profile);
		DEBUG_LOW_LEVEL << Q_FUNC_INFO << "OK:" << profile;
		emit updateProfile(profile);

		return true;
}

bool LightpackPluginInterface::DeleteProfile(const QString& sessionKey, const QString& profile)
{
	if (lockSessionKeys.isEmpty()) return false;
	if (lockSessionKeys[0]!=sessionKey) return false;
	if (!Settings::isValidProfileName(profile)) return false;
	QStringList profiles = Settings::findAllProfiles();
	if (profiles.contains(profile))
	{
		Settings::loadOrCreateProfile(profile);
		Settings::removeCurrentProfile();
		QString profileLast = Settings::getLastProfileName();
		emit updateProfile(profileLast);
		return true;
	}
	else
		return false;
}

bool LightpackPluginInterface::SetBacklight(const QString& sessionKey, int backlight)
{
	if (lockSessionKeys.isEmpty()) return false;
	if (lockSessionKeys[0]!=sessionKey) return false;
	Lightpack::Mode status =	Lightpack::UnknownMode;

	if (backlight == 1)
		status = Lightpack::AmbilightMode;
	else if (backlight == 2)
		status = Lightpack::MoodLampMode;
#ifdef SOUNDVIZ_SUPPORT
	else if (backlight == 3)
		status = Lightpack::SoundVisualizeMode;
#endif

	if (status != Lightpack::UnknownMode)
	{
		DEBUG_LOW_LEVEL << Q_FUNC_INFO << "OK:" << status;

		emit updateBacklight(status);
		return true;
	}
	return false;
}

bool LightpackPluginInterface::SetCountLeds(const QString& sessionKey, int countLeds)
{
	qWarning() << Q_FUNC_INFO << "Unsupported/deprectated API/Plugin command: SetCountLeds";
	return false;

	if (lockSessionKeys.isEmpty()) return false;
	if (lockSessionKeys[0]!=sessionKey) return false;

	Settings::setNumberOfLeds(Settings::getConnectedDevice(), countLeds);
	emit updateCountLeds(countLeds);
	return true;

}


#ifdef SOUNDVIZ_SUPPORT
bool LightpackPluginInterface::SetSoundVizColors(const QString& sessionKey, QColor min, QColor max)
{
	if (lockSessionKeys.isEmpty()) return false;
	if (lockSessionKeys[0] != sessionKey) return false;

	emit updateSoundVizMinColor(min);
	updateSoundVizMinColorCache(min);
	emit updateSoundVizMaxColor(max);
	updateSoundVizMaxColorCache(max);
	return true;
}

bool LightpackPluginInterface::SetSoundVizLiquidMode(const QString& sessionKey, bool enabled)
{
	if (lockSessionKeys.isEmpty()) return false;
	if (lockSessionKeys[0] != sessionKey) return false;

	emit updateSoundVizLiquid(enabled);
	updateSoundVizLiquidCache(enabled);
	return true;

}
#endif

bool LightpackPluginInterface::SetPersistOnUnlock(const QString& sessionKey, bool enabled) {
	if (lockSessionKeys.isEmpty()) return false;
	if (lockSessionKeys[0] != sessionKey) return false;

	m_persistOnUnlock = enabled;
	return true;
}

int LightpackPluginInterface::GetCountLeds()
{
	return Settings::getNumberOfLeds(Settings::getConnectedDevice());
}

int LightpackPluginInterface::GetStatus()
{
	if (m_isRequestBacklightStatusDone)
	{
		m_isRequestBacklightStatusDone = false;
		m_backlightStatusResult = Backlight::StatusUnknown;

		emit requestBacklightStatus();

		// Wait signal from SettingsWindow with status of backlight
		// or if timeout -- result will be unknown
		m_timer.restart();
		while (m_isRequestBacklightStatusDone == false && m_timer.hasExpired(SignalWaitTimeoutMs) == false)
		{
			QApplication::processEvents(QEventLoop::WaitForMoreEvents, SignalWaitTimeoutMs);
		}

		if (m_isRequestBacklightStatusDone)
		{
			switch (m_backlightStatusResult)
			{
				case Backlight::StatusOn:
					return 1;
					break;
				case Backlight::StatusOff:
					return 0;
					break;
				case Backlight::StatusDeviceError:
					return -1;
					break;
				default:
					return -2;
					break;
			}
		} else {
			m_isRequestBacklightStatusDone = true;
			return -2;
		}
	}
	return -2;
}

bool LightpackPluginInterface::GetStatusAPI()
{
	return (!lockSessionKeys.isEmpty());
}

QStringList LightpackPluginInterface::GetProfiles()
{
	return Settings::findAllProfiles();
}

QString LightpackPluginInterface::GetProfile()
{
	return Settings::getCurrentProfileName();
}

QList<QRect> LightpackPluginInterface::GetLeds()
{
	const int number = Settings::getNumberOfLeds(Settings::getConnectedDevice());
	QList<QRect> leds;
	leds.reserve(number);
	for (int i = 0; i < number; i++)
	{
		QPoint top = Settings::getLedPosition(i);
		QSize size = Settings::getLedSize(i);
		leds << QRect(top.x(),top.y(),size.width(),size.height());
	}
	return leds;
}

QList<QRgb> LightpackPluginInterface::GetColors()
{
	return m_curColors;
}

double LightpackPluginInterface::GetFPS()
{
	return hz;
}

QRect LightpackPluginInterface::GetScreenSize()
{
	return screen;
}

int LightpackPluginInterface::GetBacklight()
{
	Lightpack::Mode mode =	Settings::getLightpackMode();

	switch (mode)
	{
	case Lightpack::AmbilightMode:
		return 1;
		break;
	case Lightpack::MoodLampMode:
		return 2;
		break;
#ifdef SOUNDVIZ_SUPPORT
	case Lightpack::SoundVisualizeMode:
		return 3;
		break;
#endif
	default:
		qWarning() << "Unsupported Lightpack::Mode: " << (int)mode;
		// TODO: use more suitable value.
		return 0;
	}
}

double LightpackPluginInterface::GetGamma()
{
	return m_gamma;
}

int LightpackPluginInterface::GetBrightness()
{
	return m_brightness;
}

int LightpackPluginInterface::GetSmooth()
{
	return m_smooth;
}

#ifdef SOUNDVIZ_SUPPORT
QPair<QColor, QColor> LightpackPluginInterface::GetSoundVizColors()
{
	return QPair<QColor, QColor>(m_soundVizMin, m_soundVizMax);
}

bool LightpackPluginInterface::GetSoundVizLiquidMode()
{
	return m_soundVizLiquid;
}
#endif

bool LightpackPluginInterface::GetPersistOnUnlock()
{
	return m_persistOnUnlock;
}

QString LightpackPluginInterface::GetPluginsDir()
{
	return	QString(Settings::getApplicationDirPath() + QStringLiteral("Plugins"));
}

// TODO: settings (global or profile?)
void LightpackPluginInterface::SetSettingProfile(const QString& key, const QVariant& value)
{
	Settings::setValue(key,value);
}

QVariant LightpackPluginInterface::GetSettingProfile(const QString& key)
{
	return Settings::value(key);
}

void LightpackPluginInterface::SetSettingMain(const QString& key, const QVariant& value)
{
	Settings::setValueMain(key,value);
}

QVariant LightpackPluginInterface::GetSettingMain(const QString& key)
{
	return Settings::valueMain(key);
}


