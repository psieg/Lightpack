/*
 * WinUtils.hpp
 *
 *	Created on: 25.07.11
 *		Project: Lightpack
 *
 *	Copyright (c) 2011 Timur Sattarov, Mike Shatohin
 *
 *	Lightpack a USB content-driving ambient lighting system
 *
 *	Lightpack is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Lightpack is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.	If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef WINUTILS_HPP
#define WINUTILS_HPP

#pragma once

#if !defined NOMINMAX
#define NOMINMAX
#endif

#if !defined WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <QList>
#include <QRgb>
#include <QRgba64>
#include "BlueLightReduction.hpp"

#if defined(NIGHTLIGHT_SUPPORT)
namespace NightLightLibrary { class NightLightWrapper; };
#endif // NIGHTLIGHT_SUPPORT

namespace WinUtils
{
extern const WCHAR lightpackHooksDllName[];
extern const WCHAR lightpackUnhookDllName[];
#ifdef _WIN64
extern const WCHAR lightpackHooksDllName32[];
extern const WCHAR lightpackOffsetFinderName[];
#endif

BOOL SetPrivilege(HANDLE hToken, LPCTSTR szPrivName, BOOL fEnable);

BOOL AcquirePrivileges();

BOOL IsUserAdmin(VOID);

QList<DWORD> * getDxProcessesIDs(QList<DWORD> * processes, LPCWSTR wstrSystemRootPath);
QList<DWORD> * getDxgiProcessesIDs(QList<DWORD> * processes, LPCWSTR wstrSystemRootPath);
QList<DWORD> * getHookedProcessesIDs(QList<DWORD> * processes, LPCWSTR wstrSystemRootPath);

PVOID BuildRestrictedSD(PSECURITY_DESCRIPTOR pSD);

// The following function frees memory allocated in the
// BuildRestrictedSD() function
VOID FreeRestrictedSD(PVOID ptr);

#if defined(NIGHTLIGHT_SUPPORT)
	class NightLight : public BlueLightReduction::Client
	{
	public:
		NightLight();
		~NightLight();
		void apply(QList<QRgb>& colors, const double gamma);
		void apply(QList<QRgba64>& colors, const double gamma);
		static bool isSupported();
	private:
		NightLightLibrary::NightLightWrapper* _client;
	};
#endif // NIGHTLIGHT_SUPPORT
	class GammaRamp : public BlueLightReduction::Client
	{
	public:
		GammaRamp() = default;
		~GammaRamp() = default;
		void apply(QList<QRgb>& colors, const double/*gamma*/);
		void apply(QList<QRgba64>& colors, const double/*gamma*/);
		static bool isSupported();
	private:
		time_t _gammaAge = 0;
		WORD _gammaArray[3][256];
		WORD _gammaArray16[3][65536];
		static bool loadGamma(LPVOID gamma, HDC* dc);
		void interpolateGamma();
	};
}

#endif // WINUTILS_HPP
