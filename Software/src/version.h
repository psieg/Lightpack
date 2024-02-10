/*
 * version.h
 *
 *	Created on: 27.07.2010
 *		Author: Mike Shatohin (brunql)
 *		Project: Lightpack
 *
 *	Lightpack is very simple implementation of the backlight for a laptop
 *
 *	Copyright (c) 2010, 2011 Mike Shatohin, mikeshatohin [at] gmail.com
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

#pragma once

// Current version of Lightpack software
#define VERSION_STR "5.11.2.30"

// Resource defines using in Lightpack.rc
#define VER_FILEVERSION				5,11,2,30
#define VER_PRODUCTVERSION			VER_FILEVERSION

#define VER_FILEVERSION_STR			VERSION_STR "\0"
#define VER_PRODUCTVERSION_STR		VER_FILEVERSION_STR

#define VER_PRODUCTNAME_STR			"Prismatik (unofficial)"
#define VER_COMPANYNAME_STR			"Patrick Siegler"
#define VER_FILEDESCRIPTION_STR		"Lightpack Software"
#define VER_INTERNALNAME_STR		"Prismatik"
#define VER_LEGALCOPYRIGHT_STR		""
#define VER_ORIGINALFILENAME_STR	"Prismatik.exe"


// Prismatik API version. When breaking compatibilty to Lightpack API v1.4,
// change ApiServer::ApiVersion and the API help accordingly!
#define API_VERSION "2.2"
