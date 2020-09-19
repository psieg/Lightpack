/*
 * colorspace_types.h
 *
 *	Created on: 05.02.2013
 *		Project: Lightpack
 *
 *	Copyright (c) 2011 Mike Shatohin, mikeshatohin [at] gmail.com
 *				2013 Timur Sattarov, tim.helloworld [at] gmail.com
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

#ifndef COLORSPACE_TYPES_H
#define COLORSPACE_TYPES_H

/*!
	48bit RGB, 16 bit per channel!
	*/
struct StructRgb {
	unsigned int r, g, b;
	StructRgb() { r = g = b = 0;}
};

struct StructLab {
	unsigned char l;
	char a;
	char b;
	StructLab() { l = a = b = 0;}
};

struct StructXyz {
	double x, y, z;
	StructXyz() { x = y = z = 0; }
};

#endif // COLORSPACE_TYPES_H
