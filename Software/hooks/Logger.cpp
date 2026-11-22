#include "Logger.hpp"
#include <stdio.h>
#include <stdlib.h>

Logger *Logger::_this = NULL;

Logger *Logger::getInstance() {
	if (!_this)
		_this = new Logger();
	return _this;
}

Logger::Logger() {
	m_hEventSrc = NULL;
	m_reportLogBuf = NULL;
}

Logger::~Logger() {
	this->closeLog();
}

void Logger::initLog(LPCWSTR name, DWORD logLevel) {
	if (!m_hEventSrc) m_hEventSrc = RegisterEventSourceW(NULL, name);
	if (!m_reportLogBuf) m_reportLogBuf = (WCHAR*)malloc(REPORT_LOG_BUF_SIZE);
	m_logLevel = logLevel;
}

void Logger::setLogLevel(DWORD logLevel) {
	m_logLevel = logLevel;
}

void Logger::reportLogDebug(LPCWSTR message, ...) {
	va_list ap;
	va_start( ap, message );
	if (m_logLevel <= PRISMATIK_LOG_SEVERITY_DEBUG) {
		this->reportLog(EVENTLOG_INFORMATION_TYPE, PRISMATIK_LOG_MSG_DEBUG, message, ap);
	}
	va_end( ap );
}

void Logger::reportLogInfo(LPCWSTR message, ...) {
	va_list ap;
	va_start( ap, message );
	if (m_logLevel <= PRISMATIK_LOG_SEVERITY_INFO) {
		this->reportLog(EVENTLOG_INFORMATION_TYPE, PRISMATIK_LOG_MSG_INFO, message, ap);
	}
	va_end( ap );
}

void Logger::reportLogWarning(LPCWSTR message, ...) {
	va_list ap;
	va_start( ap, message );
	if (m_logLevel <= PRISMATIK_LOG_SEVERITY_WARNING) {
		this->reportLog(EVENTLOG_WARNING_TYPE, PRISMATIK_LOG_MSG_WARNING, message, ap);
	}
	va_end( ap );
}

void Logger::reportLogError(LPCWSTR message, ...) {
	va_list ap;
	va_start( ap, message );
		this->reportLog(EVENTLOG_ERROR_TYPE, PRISMATIK_LOG_MSG_ERROR, message, ap);
	va_end( ap );
}

void Logger::reportLog(DWORD type, DWORD msgId, LPCWSTR message, va_list ap) {
	if (m_hEventSrc && message) {
		WCHAR localBuf[REPORT_LOG_BUF_SIZE / sizeof(WCHAR)] = {};
		int sprintfResult = _vsnwprintf_s(localBuf, _countof(localBuf), _TRUNCATE, message, ap);
		if (sprintfResult >= 0 || localBuf[0] != L'\0') {
			LPCWSTR msgPtr = localBuf;
			ReportEventW(m_hEventSrc, type & 0xFFFF, 0, msgId, NULL, 1, 0, &msgPtr, NULL);
		}
	}
}

void Logger::closeLog() {
	if (m_hEventSrc) {
		DeregisterEventSource(m_hEventSrc);
		m_hEventSrc = NULL;
	}
	if (m_reportLogBuf) {
		free(m_reportLogBuf);
		m_reportLogBuf = NULL;
	}
}
