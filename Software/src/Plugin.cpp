#include "Plugin.hpp"
#include <QSettings>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QImageReader>
#include "Settings.hpp"
#include "../common/defs.h"

#if defined(Q_OS_WIN)
#if !defined NOMINMAX
#define NOMINMAX
#endif
#if !defined WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsafer.h>
#include <sddl.h>
#include <userenv.h>
#include <QWinEventNotifier>
#include "WinUtils.hpp"
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "userenv.lib")
#endif

using namespace SettingsScope;

#if defined(Q_OS_WIN)
	const QString kOsSpecificExecuteKey = QStringLiteral("ExecuteOnWindows");
#elif defined(MAC_OS)
	const QString kOsSpecificExecuteKey = QStringLiteral("ExecuteOnOSX");
#elif defined(Q_OS_UNIX)
	const QString kOsSpecificExecuteKey = QStringLiteral("ExecuteOnNix");
#endif

Plugin::Plugin(const QString& name, const QString& path, QObject *parent) :
	QObject(parent)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << name << path;
	_pathPlugin = path;
	QDir pluginPath(_pathPlugin);

	const QString fileName(QStringLiteral("%1/%2.ini").arg(path, name));
	if (!QFile::exists(fileName)) {
		qWarning() << Q_FUNC_INFO << name << fileName << "does not exist, generating defaults, make sure to edit the file!";
		if (!QFile::copy(QStringLiteral(":/plugin-template.ini"), fileName))
			qWarning() << Q_FUNC_INFO << name << "failed to generate" << fileName;
		else if (!QFile::setPermissions(fileName, QFile::permissions(fileName) | QFileDevice::WriteOwner))
			qWarning() << Q_FUNC_INFO << name << "could not set write permissions to" << fileName;
	}
	QSettings settings(fileName, QSettings::IniFormat);
	settings.beginGroup(QStringLiteral("Main"));
	_name = settings.value(QStringLiteral("Name"), QLatin1String("")).toString();
	if (_name.isEmpty())
		_name = name;

	const QString rawExecCmd = settings.contains(kOsSpecificExecuteKey)
		? settings.value(kOsSpecificExecuteKey, QLatin1String("")).toString().trimmed()
		: settings.value(QStringLiteral("Execute"), QLatin1String("")).toString().trimmed();
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
	_arguments = QProcess::splitCommand(rawExecCmd);
#else
	_arguments = rawExecCmd.split(' ', QString::SkipEmptyParts);
#endif

	if (!_arguments.isEmpty())
		_exec = _arguments.takeFirst();

	if (_exec.isEmpty())
		qWarning() << Q_FUNC_INFO << name << "no executable, check" << fileName;

	_guid = settings.value(QStringLiteral("Guid"), QLatin1String("")).toString();
	_author = settings.value(QStringLiteral("Author"), QLatin1String("")).toString();
	_description = settings.value(QStringLiteral("Description"), QLatin1String("")).toString();
	_version = settings.value(QStringLiteral("Version"), QLatin1String("")).toString();

	const QString iconName = settings.value(QStringLiteral("Icon"), QLatin1String("")).toString();
	const QString iconPath = pluginPath.absoluteFilePath(iconName);
	if (!iconName.isEmpty() && QFile::exists(iconPath) && !QImageReader::imageFormat(iconPath).isEmpty())
		_icon = QIcon(iconPath);
	if (_icon.isNull())
		_icon = QIcon(QStringLiteral(":/icons/plugins.png"));

	settings.endGroup();

	process = new QProcess(this);
}

Plugin::~Plugin()
{
	Stop();
}

QString Plugin::Name() const {
	return _name;
}

QString Plugin::Guid() const {
	return _guid;
}

QString Plugin::Author() const {
	return _author;
}

QString Plugin::Description() const	{
	return _description;
}

QString Plugin::Version() const	{
	return _version;
}


QIcon Plugin::Icon() const {
	return _icon;
}


int Plugin::getPriority() const {
	const QString key = QStringLiteral("%1/Priority").arg(_name);
	return Settings::valueMain(key).toInt();
}

void Plugin::setPriority(int priority) {
	const QString key = QStringLiteral("%1/Priority").arg(_name);
	Settings::setValueMain(key,priority);
}

bool Plugin::isEnabled() const {
	const QString key = QStringLiteral("%1/Enable").arg(_name);
	return Settings::valueMain(key).toBool();
}

void Plugin::setEnabled(bool enable) {
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << _name << enable;
	const QString key = QStringLiteral("%1/Enable").arg(_name);
	Settings::setValueMain(key,enable);
	if (!enable) Stop();
	if (enable) Start();
}


#if defined(Q_OS_WIN)
namespace {

std::wstring quoteWindowsArg(const QString &arg)
{
	const std::wstring s = arg.toStdWString();
	if (!s.empty() && s.find_first_of(L" \t\n\v\"") == std::wstring::npos)
		return s;

	std::wstring out = L"\"";
	size_t backslashes = 0;
	for (wchar_t ch : s) {
		if (ch == L'\\') {
			++backslashes;
		} else if (ch == L'"') {
			out.append(backslashes * 2 + 1, L'\\');
			out.push_back(L'"');
			backslashes = 0;
		} else {
			out.append(backslashes, L'\\');
			backslashes = 0;
			out.push_back(ch);
		}
	}
	out.append(backslashes * 2, L'\\');
	out.push_back(L'"');
	return out;
}

QString resolvePluginExecutable(const QString &program, const QString &workingDir)
{
	if (program.isEmpty())
		return QString();

	QString normalized = QDir::toNativeSeparators(program);
	if (normalized.contains(QLatin1Char('\\')) || normalized.contains(QLatin1Char(':'))) {
		QFileInfo fi(QDir(workingDir), normalized);
		if (fi.exists() && fi.isFile())
			return QDir::toNativeSeparators(fi.absoluteFilePath());
		if (fi.suffix().isEmpty()) {
			QFileInfo fiExe(fi.absoluteFilePath() + QStringLiteral(".exe"));
			if (fiExe.exists() && fiExe.isFile())
				return QDir::toNativeSeparators(fiExe.absoluteFilePath());
		}
		return QString();
	}

	const std::wstring progW = normalized.toStdWString();
	const std::wstring workDirW = QDir::toNativeSeparators(workingDir).toStdWString();
	wchar_t foundBuf[MAX_PATH * 4] = {};

	// 1. Check plugin working directory first
	DWORD len = SearchPathW(workDirW.c_str(), progW.c_str(), L".exe", static_cast<DWORD>( sizeof(foundBuf) / sizeof(wchar_t) ), foundBuf, NULL);
	if (len > 0 && len < (sizeof(foundBuf) / sizeof(wchar_t)))
		return QString::fromWCharArray(foundBuf, static_cast<int>(len));

	// 2. Check PATH environment variable + System32
	const QString pathEnv = qEnvironmentVariable("PATH");
	if (!pathEnv.isEmpty()) {
		const std::wstring pathEnvW = pathEnv.toStdWString();
		len = SearchPathW(pathEnvW.c_str(), progW.c_str(), L".exe", static_cast<DWORD>( sizeof(foundBuf) / sizeof(wchar_t) ), foundBuf, NULL);
		if (len > 0 && len < (sizeof(foundBuf) / sizeof(wchar_t)))
			return QString::fromWCharArray(foundBuf, static_cast<int>(len));
	}

	wchar_t sysDir[MAX_PATH] = {};
	if (GetSystemDirectoryW(sysDir, MAX_PATH) > 0) {
		len = SearchPathW(sysDir, progW.c_str(), L".exe", static_cast<DWORD>( sizeof(foundBuf) / sizeof(wchar_t) ), foundBuf, NULL);
		if (len > 0 && len < (sizeof(foundBuf) / sizeof(wchar_t)))
			return QString::fromWCharArray(foundBuf, static_cast<int>(len));
	}

	return QString();
}

bool isTokenServiceOrSystemAccount(HANDLE hToken)
{
	DWORD needed = 0;
	GetTokenInformation(hToken, TokenUser, NULL, 0, &needed);
	if (needed == 0)
		return true;
	std::vector<BYTE> buf(needed, 0);
	if (!GetTokenInformation(hToken, TokenUser, buf.data(), needed, &needed))
		return true;
	const TOKEN_USER *tu = reinterpret_cast<const TOKEN_USER *>(buf.data());
	if (!tu->User.Sid || !IsValidSid(tu->User.Sid))
		return true;
	return IsWellKnownSid(tu->User.Sid, WinLocalSystemSid) ||
		IsWellKnownSid(tu->User.Sid, WinLocalServiceSid) ||
		IsWellKnownSid(tu->User.Sid, WinNetworkServiceSid);
}

bool verifySaferTokenNonAdminMediumIl(HANDLE hPrimaryToken)
{
	if (isTokenServiceOrSystemAccount(hPrimaryToken))
		return false;

	// Verify Integrity Level <= Medium (0x2000)
	DWORD ilLen = 0;
	GetTokenInformation(hPrimaryToken, TokenIntegrityLevel, NULL, 0, &ilLen);
	if (ilLen == 0)
		return false;
	std::vector<BYTE> ilBuf(ilLen, 0);
	if (!GetTokenInformation(hPrimaryToken, TokenIntegrityLevel, ilBuf.data(), ilLen, &ilLen))
		return false;
	const TOKEN_MANDATORY_LABEL *pTml = reinterpret_cast<const TOKEN_MANDATORY_LABEL *>(ilBuf.data());
	if (!pTml->Label.Sid || !IsValidSid(pTml->Label.Sid) || *GetSidSubAuthorityCount(pTml->Label.Sid) == 0)
		return false;
	const DWORD ilRid = *GetSidSubAuthority(pTml->Label.Sid, *GetSidSubAuthorityCount(pTml->Label.Sid) - 1);
	if (ilRid > SECURITY_MANDATORY_MEDIUM_RID)
		return false;

	// Verify Builtin Administrators SID is disabled (deny-only) or absent
	HANDLE hImpToken = NULL;
	if (!DuplicateToken(hPrimaryToken, SecurityIdentification, &hImpToken))
		return false;

	SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
	PSID pAdminsSid = NULL;
	BOOL isMember = TRUE;
	bool nonAdmin = false;
	if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &pAdminsSid)) {
		if (CheckTokenMembership(hImpToken, pAdminsSid, &isMember) && !isMember) {
			nonAdmin = true;
		}
		FreeSid(pAdminsSid);
	}
	CloseHandle(hImpToken);
	return nonAdmin;
}

bool isProcessUnelevated(HANDLE hProcess)
{
	HANDLE hToken = NULL;
	if (!OpenProcessToken(hProcess, TOKEN_QUERY, &hToken))
		return false;

	TOKEN_ELEVATION elev = {};
	DWORD retLen = 0;
	bool unelevated = false;
	if (GetTokenInformation(hToken, TokenElevation, &elev, sizeof(elev), &retLen)) {
		unelevated = (elev.TokenIsElevated == 0);
	}
	CloseHandle(hToken);
	return unelevated;
}

bool launchPluginUnelevatedWin32(const QString &program, const QStringList &arguments, const QString &workingDir, HANDLE &outProcessHandle)
{
	outProcessHandle = NULL;
	const QString resolvedExe = resolvePluginExecutable(program, workingDir);
	if (resolvedExe.isEmpty()) {
		qWarning() << "Plugin executable could not be resolved:" << program;
		return false;
	}

	const std::wstring appPathW = resolvedExe.toStdWString();
	const std::wstring workDirW = QDir::toNativeSeparators(workingDir).toStdWString();

	std::wstring cmdLine = quoteWindowsArg(resolvedExe);
	for (const QString &arg : arguments) {
		cmdLine.push_back(L' ');
		cmdLine.append(quoteWindowsArg(arg));
	}

	// Strategy 1 (preferred): Reparent under the non-elevated desktop shell (explorer.exe) via PROC_THREAD_ATTRIBUTE_PARENT_PROCESS.
	HWND hShellWnd = GetShellWindow();
	DWORD shellPid = 0;
	if (hShellWnd)
		GetWindowThreadProcessId(hShellWnd, &shellPid);

	if (shellPid != 0 && shellPid != GetCurrentProcessId()) {
		HANDLE hQueryProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, shellPid);
		const bool shellIsUnelevated = hQueryProc && isProcessUnelevated(hQueryProc);
		if (hQueryProc)
			CloseHandle(hQueryProc);

		if (shellIsUnelevated) {
			HANDLE hShellCreate = OpenProcess(PROCESS_CREATE_PROCESS, FALSE, shellPid);
			if (hShellCreate) {
				SIZE_T attrListSize = 0;
				InitializeProcThreadAttributeList(NULL, 1, 0, &attrListSize);
				QByteArray attrBuf(static_cast<int>(attrListSize), 0);
				LPPROC_THREAD_ATTRIBUTE_LIST pAttrList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attrBuf.data());
				if (InitializeProcThreadAttributeList(pAttrList, 1, 0, &attrListSize)) {
					if (UpdateProcThreadAttribute(pAttrList, 0, PROC_THREAD_ATTRIBUTE_PARENT_PROCESS, &hShellCreate, sizeof(hShellCreate), NULL, NULL)) {
						STARTUPINFOEXW siEx = {};
						siEx.StartupInfo.cb = sizeof(siEx);
						siEx.lpAttributeList = pAttrList;
						PROCESS_INFORMATION pi = {};
						std::vector<wchar_t> mutableCmd(cmdLine.begin(), cmdLine.end());
						mutableCmd.push_back(L'\0');

						BOOL created = CreateProcessW(
							appPathW.c_str(),
							mutableCmd.data(),
							NULL,
							NULL,
							FALSE,
							EXTENDED_STARTUPINFO_PRESENT | CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP,
							NULL,
							workDirW.c_str(),
							&siEx.StartupInfo,
							&pi);
						DeleteProcThreadAttributeList(pAttrList);
						CloseHandle(hShellCreate);
						if (created) {
							CloseHandle(pi.hThread);
							outProcessHandle = pi.hProcess;
							return true;
						}
					} else {
						DeleteProcThreadAttributeList(pAttrList);
						CloseHandle(hShellCreate);
					}
				} else {
					CloseHandle(hShellCreate);
				}
			}

			// Strategy 2: Duplicate the non-elevated shell token and invoke CreateProcessWithTokenW.
			HANDLE hShellQuery = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, shellPid);
			if (hShellQuery) {
				HANDLE hShellToken = NULL;
				if (OpenProcessToken(hShellQuery, TOKEN_DUPLICATE, &hShellToken)) {
					HANDLE hPrimaryToken = NULL;
					if (DuplicateTokenEx(hShellToken, TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY | TOKEN_ADJUST_DEFAULT | TOKEN_ADJUST_SESSIONID,
						NULL, SecurityImpersonation, TokenPrimary, &hPrimaryToken))
					{
						HANDLE hSelfToken = NULL;
						if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hSelfToken)) {
							WinUtils::SetPrivilege(hSelfToken, SE_IMPERSONATE_NAME, TRUE);
							CloseHandle(hSelfToken);
						}

						LPVOID pEnv = NULL;
						CreateEnvironmentBlock(&pEnv, hPrimaryToken, FALSE);
						STARTUPINFOW si = {};
						si.cb = sizeof(si);
						PROCESS_INFORMATION pi = {};
						std::vector<wchar_t> mutableCmd(cmdLine.begin(), cmdLine.end());
						mutableCmd.push_back(L'\0');

						BOOL created = CreateProcessWithTokenW(
							hPrimaryToken,
							0,
							appPathW.c_str(),
							mutableCmd.data(),
							CREATE_UNICODE_ENVIRONMENT | CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP,
							pEnv,
							workDirW.c_str(),
							&si,
							&pi);
						if (pEnv)
							DestroyEnvironmentBlock(pEnv);
						CloseHandle(hPrimaryToken);
						CloseHandle(hShellToken);
						CloseHandle(hShellQuery);
						if (created) {
							CloseHandle(pi.hThread);
							outProcessHandle = pi.hProcess;
							return true;
						}
					} else {
						CloseHandle(hShellToken);
						CloseHandle(hShellQuery);
					}
				} else {
					CloseHandle(hShellQuery);
				}
			}
		}
	}

	// Strategy 3: Fallback when shell window is absent — compute a NormalUser SAFER token forced to Medium Integrity.
	// Refuse Strategy 3 if running as SYSTEM/Service (a SAFER token from SYSTEM retains TokenUser == S-1-5-18).
	{
		HANDLE hSelfQuery = NULL;
		const bool isServiceCaller = !OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hSelfQuery) || isTokenServiceOrSystemAccount(hSelfQuery);
		if (hSelfQuery)
			CloseHandle(hSelfQuery);
		if (isServiceCaller)
			return false;
	}

	SAFER_LEVEL_HANDLE hSafer = NULL;
	if (SaferCreateLevel(SAFER_SCOPEID_USER, SAFER_LEVELID_NORMALUSER, SAFER_LEVEL_OPEN, &hSafer, NULL)) {
		HANDLE hReducedToken = NULL;
		if (SaferComputeTokenFromLevel(hSafer, NULL, &hReducedToken, 0, NULL)) {
			PSID pMediumSid = NULL;
			bool ilSetOk = false;
			if (ConvertStringSidToSidW(L"S-1-16-8192", &pMediumSid) && pMediumSid) {
				TOKEN_MANDATORY_LABEL tml = {};
				tml.Label.Attributes = SE_GROUP_INTEGRITY;
				tml.Label.Sid = pMediumSid;
				ilSetOk = SetTokenInformation(hReducedToken, TokenIntegrityLevel, &tml, sizeof(TOKEN_MANDATORY_LABEL)) != FALSE;
				LocalFree(pMediumSid);
			}
			if (!ilSetOk || !verifySaferTokenNonAdminMediumIl(hReducedToken)) {
				CloseHandle(hReducedToken);
				SaferCloseLevel(hSafer);
				return false;
			}

			HANDLE hSelfToken = NULL;
			if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hSelfToken)) {
				WinUtils::SetPrivilege(hSelfToken, SE_IMPERSONATE_NAME, TRUE);
				CloseHandle(hSelfToken);
			}

			STARTUPINFOW si = {};
			si.cb = sizeof(si);
			PROCESS_INFORMATION pi = {};
			std::vector<wchar_t> mutableCmd(cmdLine.begin(), cmdLine.end());
			mutableCmd.push_back(L'\0');

			BOOL created = CreateProcessAsUserW(
				hReducedToken,
				appPathW.c_str(),
				mutableCmd.data(),
				NULL,
				NULL,
				FALSE,
				CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP,
				NULL,
				workDirW.c_str(),
				&si,
				&pi);
			if (!created) {
				std::vector<wchar_t> mutableCmd2(cmdLine.begin(), cmdLine.end());
				mutableCmd2.push_back(L'\0');
				created = CreateProcessWithTokenW(
					hReducedToken,
					0,
					appPathW.c_str(),
					mutableCmd2.data(),
					CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP,
					NULL,
					workDirW.c_str(),
					&si,
					&pi);
			}
			CloseHandle(hReducedToken);
			SaferCloseLevel(hSafer);
			if (created) {
				CloseHandle(pi.hThread);
				outProcessHandle = pi.hProcess;
				return true;
			}
		} else {
			SaferCloseLevel(hSafer);
		}
	}

	return false;
}

} // namespace

bool Plugin::launchProgramUnelevatedDetached(const QString &program, const QStringList &arguments, const QString &workingDir)
{
	const QString effectiveWorkDir = workingDir.isEmpty() ? QCoreApplication::applicationDirPath() : workingDir;
	if (!WinUtils::IsUserAdmin()) {
		return QProcess::startDetached(program, arguments, effectiveWorkDir);
	}
	HANDLE hProc = NULL;
	if (!launchPluginUnelevatedWin32(program, arguments, effectiveWorkDir, hProc)) {
		return false;
	}
	if (hProc)
		CloseHandle(hProc);
	return true;
}
#endif

void Plugin::Start()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << _name << "starting" << _exec << _arguments;
	if (_exec.isEmpty())
		return;

	Stop();

	const QDir dir(_pathPlugin);
	const QString workDir = dir.absolutePath();

#if defined(Q_OS_WIN)
	if (WinUtils::IsUserAdmin()) {
		HANDLE hProc = NULL;
		if (!launchPluginUnelevatedWin32(_exec, _arguments, workDir, hProc) || !hProc) {
			qCritical() << Q_FUNC_INFO << _name << "refusing to launch plugin elevated after de-elevation failed:" << _exec;
			return;
		}
		m_unelevatedProcessHandle = hProc;
		m_unelevatedState = QProcess::Running;
		m_processExitNotifier = new QWinEventNotifier(hProc, this);
		connect(m_processExitNotifier, &QWinEventNotifier::activated, this, &Plugin::onUnelevatedProcessFinished);
		started();
		emit pluginStateChanged(QProcess::Running);
		return;
	}
#endif

	process->disconnect();

	connect(process, &QProcess::stateChanged, this, &Plugin::stateChanged);
	connect(process, &QProcess::errorOccurred, this, &Plugin::errorOccurred);

	connect(process, &QProcess::started, this, &Plugin::started);
	connect(process, qOverload<int,QProcess::ExitStatus>(&QProcess::finished), this, &Plugin::finished);

	connect(process, &QProcess::readyReadStandardError, this, &Plugin::readyReadStandardError);
	connect(process, &QProcess::readyReadStandardOutput, this, &Plugin::readyReadStandardOutput);

	process->setWorkingDirectory(workDir);
	process->setEnvironment(QProcess::systemEnvironment());
	process->setProgram(_exec);
	process->setArguments(_arguments);
	process->start();
}

void Plugin::Stop()
{
#if defined(Q_OS_WIN)
	if (m_unelevatedProcessHandle != nullptr) {
		if (m_processExitNotifier) {
			m_processExitNotifier->setEnabled(false);
			delete m_processExitNotifier;
			m_processExitNotifier = nullptr;
		}
		HANDLE hProc = static_cast<HANDLE>(m_unelevatedProcessHandle);
		m_unelevatedProcessHandle = nullptr;
		TerminateProcess(hProc, 0);
		WaitForSingleObject(hProc, 1000);
		CloseHandle(hProc);
		if (m_unelevatedState != QProcess::NotRunning) {
			m_unelevatedState = QProcess::NotRunning;
			emit pluginStateChanged(QProcess::NotRunning);
		}
	}
#endif
	if (process->state() != QProcess::NotRunning)
		process->kill();
}

QProcess::ProcessState Plugin::state() const
{
#if defined(Q_OS_WIN)
	if (m_unelevatedProcessHandle != nullptr)
		return m_unelevatedState;
#endif
	return process->state();
}

#if defined(Q_OS_WIN)
void Plugin::onUnelevatedProcessFinished()
{
	if (m_processExitNotifier) {
		m_processExitNotifier->setEnabled(false);
		m_processExitNotifier->deleteLater();
		m_processExitNotifier = nullptr;
	}
	DWORD exitCode = 0;
	if (m_unelevatedProcessHandle != nullptr) {
		HANDLE hProc = static_cast<HANDLE>(m_unelevatedProcessHandle);
		m_unelevatedProcessHandle = nullptr;
		GetExitCodeProcess(hProc, &exitCode);
		CloseHandle(hProc);
	}
	m_unelevatedState = QProcess::NotRunning;
	finished(static_cast<int>(exitCode), QProcess::NormalExit);
	emit pluginStateChanged(QProcess::NotRunning);
}
#endif

// QProcess slots
void Plugin::stateChanged(QProcess::ProcessState newState)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << _name << newState;
	emit pluginStateChanged(newState);
}

void Plugin::errorOccurred(QProcess::ProcessError error)
{
	qWarning() << Q_FUNC_INFO << _name << error << _exec << _arguments;
}

void Plugin::started()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << _name;
}

void Plugin::finished(int exitCode, QProcess::ExitStatus exitStatus)
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << _name << "exitCode=" << exitCode << exitStatus;
}

void Plugin::readyReadStandardError()
{
	qWarning() << Q_FUNC_INFO << _name << process->readAllStandardError();
}

void Plugin::readyReadStandardOutput()
{
	DEBUG_LOW_LEVEL << Q_FUNC_INFO << _name << process->readAllStandardOutput();
}
