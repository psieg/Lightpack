#ifndef LightpackPluginInterface_HPP
#define LightpackPluginInterface_HPP

#include <QtGui>
#include <QObject>
#include "enums.hpp"

class Plugin;

class LightpackPluginInterface : public QObject
{
	Q_OBJECT
public:
	LightpackPluginInterface(QObject *parent = 0);
	~LightpackPluginInterface();

 public slots:
// Plugin section
	QString GetSessionKey(QString module);
	int CheckLock(QString sessionKey);
	bool Lock(QString sessionKey);

// need LOCK
	bool UnLock(QString sessionKey);
	bool SetStatus(QString sessionKey, int status);
	bool SetColors(QString sessionKey, int r, int g, int b);
	bool SetFrame(QString sessionKey, QList<QColor> colors);
	bool SetColor(QString sessionKey, int ind,int r, int g, int b);
	bool SetGamma(QString sessionKey, double gamma);
	bool SetBrightness(QString sessionKey, int brightness);
	bool SetCountLeds(QString sessionKey, int countLeds);
	bool SetSmooth(QString sessionKey, int smooth);
	bool SetProfile(QString sessionKey, QString profile);
	bool SetDevice(QString sessionKey,QString device);
#ifdef SOUNDVIZ_SUPPORT
	bool SetSoundVizColors(QString sessionKey, QColor min, QColor max);
	bool SetSoundVizLiquidMode(QString sessionKey, bool enabled);
#endif
	bool SetPersistOnUnlock(QString sessionKey, bool enabled);

	bool SetLeds(QString sessionKey, QList<QRect> leds);
	bool NewProfile(QString sessionKey, QString profile);
	bool DeleteProfile(QString sessionKey, QString profile);
	bool SetBacklight(QString sessionKey, int backlight);

// no LOCK
	QString Version();
	int GetCountLeds();
	int GetStatus();
	bool GetStatusAPI();
	QStringList GetProfiles();
	QString GetProfile();
	QList<QRect> GetLeds();
	QList<QRgba64> GetColors();
	double GetFPS();
	QRect GetScreenSize();
	int GetBacklight();
	double GetGamma();
	int GetBrightness();
	int GetSmooth();
#ifdef SOUNDVIZ_SUPPORT
	QPair<QColor, QColor> GetSoundVizColors();
	bool GetSoundVizLiquidMode();
#endif
	bool GetPersistOnUnlock();

// Settings
	QString GetPluginsDir();
	void SetSettingProfile(QString key, QVariant value);
	QVariant GetSettingProfile(QString key);
	void SetSettingMain(QString key, QVariant value);
	QVariant GetSettingMain(QString key);

	bool VerifySessionKey(QString sessionKey);
	void SetLockAlive(QString sessionKey);

signals:
	void ChangeProfile(QString profile);
	void ChangeStatus(int status);
	void ChangeLockStatus(bool lock);

//end Plugin section

signals:
	void requestBacklightStatus();
	void updateDeviceLockStatus(DeviceLocked::DeviceLockStatus status, QList<QString> modules);
	void updateLedsColors(const QList<QRgba64> & colors);
	void updateGamma(double value);
	void updateBrightness(int value);
	void updateSmooth(int value);
#ifdef SOUNDVIZ_SUPPORT
	void updateSoundVizMinColor(QColor color);
	void updateSoundVizMaxColor(QColor color);
	void updateSoundVizLiquid(bool value);
#endif
	void updateProfile(QString profileName);
	void updateStatus(Backlight::Status status);
	void updateBacklight(Lightpack::Mode status);
	void updateCountLeds(int value);
	void changeDevice(QString device);


public slots:
	void setNumberOfLeds(int numberOfLeds);
	void resultBacklightStatus(Backlight::Status status);
	void changeProfile(QString profile);
	void refreshAmbilightEvaluated(double updateResultMs);
	void refreshScreenRect(QRect rect);
	void updateColorsCache(const QList<QRgba64> & colors);
	void updateGammaCache(double value);
	void updateBrightnessCache(int value);
	void updateSmoothCache(int value);
#ifdef SOUNDVIZ_SUPPORT
	void updateSoundVizMinColorCache(QColor color);
	void updateSoundVizMaxColorCache(QColor color);
	void updateSoundVizLiquidCache(bool value);
#endif
	void updatePlugin(QList<Plugin*> plugins);

private slots:
	void timeoutLock();

private:
	bool lockAlive;

	static const int SignalWaitTimeoutMs;
	QElapsedTimer m_timer;
	bool m_isRequestBacklightStatusDone;
	Backlight::Status m_backlightStatusResult;

	double hz;
	QRect screen;

	QList<QString> lockSessionKeys;
	QList<QRgba64> m_setColors;
	QList<QRgba64> m_curColors;
	QTimer *m_timerLock;

	double m_gamma;
	int m_brightness;
	int m_smooth;
#ifdef SOUNDVIZ_SUPPORT
	QColor m_soundVizMin;
	QColor m_soundVizMax;
	bool m_soundVizLiquid;
#endif
	bool m_persistOnUnlock;

	void initColors(int numberOfLeds);

	QList<Plugin*> _plugins;
	Plugin* findName(QString name);
	Plugin* findSessionKey(QString sessionKey);
};
#endif

