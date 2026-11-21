#if !defined NOMINMAX
#define NOMINMAX
#endif
#if !defined WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "WindowsSession.hpp"
#include <WtsApi32.h>

#include "debug.h"
#include "LightpackApplication.hpp"
#include <exception>

namespace SystemSession
{
	class register_exception : public std::exception {
		virtual const char* what() const throw()
		{
		return "Failed to register session notification";
		}
	};

	WindowsEventFilter::WindowsEventFilter()
	{
		if (WTSRegisterSessionNotification(getLightpackApp()->getMainWindowHandle(), NOTIFY_FOR_THIS_SESSION) == FALSE)
			throw register_exception();

		m_powerSettingNotificationHandle = RegisterPowerSettingNotification(
			getLightpackApp()->getMainWindowHandle(),
			&GUID_CONSOLE_DISPLAY_STATE,
			DEVICE_NOTIFY_WINDOW_HANDLE
		);
		if (m_powerSettingNotificationHandle == NULL) {
			WTSUnRegisterSessionNotification(getLightpackApp()->getMainWindowHandle());
			throw register_exception();
		}
		DEBUG_MID_LEVEL << Q_FUNC_INFO << "Event Filter is set up";
	}

	#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
	bool WindowsEventFilter::nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result)
	#else
	bool WindowsEventFilter::nativeEventFilter(const QByteArray& eventType, void* message, long* result)
	#endif
	{
		Q_UNUSED(result);
		Q_UNUSED(eventType);
		Q_UNUSED(message);

		MSG* msg = (MSG*)message;

		if (msg->message == WM_QUERYENDSESSION)
		{
			DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Session is ending";
			emit sessionChangeDetected(Ending);
		}
		else if (msg->message == WM_WTSSESSION_CHANGE)
		{
			if (msg->wParam == WTS_SESSION_LOCK)
			{
				DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Session is locking";
				emit sessionChangeDetected(Locking);
			}
			else if (msg->wParam == WTS_SESSION_UNLOCK)
			{
				DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Session is unlocking";
				emit sessionChangeDetected(Unlocking);
			}
		}
		else if (msg->message == WM_POWERBROADCAST)
		{
			if (msg->wParam == PBT_APMSUSPEND)
			{
				DEBUG_LOW_LEVEL << Q_FUNC_INFO << "System is going to sleep";
				emit sessionChangeDetected(Sleeping);
			} else if (msg->wParam == PBT_APMRESUMESUSPEND)
			{
				DEBUG_LOW_LEVEL << Q_FUNC_INFO << "System is resuming";
				emit sessionChangeDetected(Resuming);
			} else if (msg->wParam == PBT_APMRESUMEAUTOMATIC)
			{
				DEBUG_LOW_LEVEL << Q_FUNC_INFO << "System is resuming automatically";
				emit sessionChangeDetected(ResumingAutomatic);
			} else if (msg->wParam == PBT_POWERSETTINGCHANGE)
			{
				DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Power settings changed";
				POWERBROADCAST_SETTING* ps = (POWERBROADCAST_SETTING*)msg->lParam;
				if (ps->PowerSetting == GUID_CONSOLE_DISPLAY_STATE)
				{
					if (ps->Data[0] == 0)
					{
						DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Display is off";
						emit sessionChangeDetected(DisplayOff);
					}
					else if (ps->Data[0] == 1)
					{
						DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Display is on";
						emit sessionChangeDetected(DisplayOn);
					}
					else if (ps->Data[0] == 2)
					{
						DEBUG_LOW_LEVEL << Q_FUNC_INFO << "Display is dimmed";
						emit sessionChangeDetected(DisplayDimmed);
					}
				}
			}
		}
		return false;
	}

	WindowsEventFilter::~WindowsEventFilter()
	{
		WTSUnRegisterSessionNotification(getLightpackApp()->getMainWindowHandle());
		UnregisterPowerSettingNotification(m_powerSettingNotificationHandle);
	}

}
