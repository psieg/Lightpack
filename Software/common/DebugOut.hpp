/*
 *  DebugOut.hpp
 *
 *  Created on: 22.03.2011
 *      Author: Mike Shatohin (brunql)
 *     Project: Lightpack
 *
 *  Lightpack is very simple implementation of the backlight for a laptop
 *
 *  Copyright (c) 2011 Mike Shatohin, mikeshatohin [at] gmail.com
 *
 *  Lightpack is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Lightpack is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <QtDebug>

// Set and store in main.cpp file
extern unsigned g_debugLevel;

// Using:
//
//  DEBUG_HIGH_LEVEL << "This will be logged only if debugLevel >= 3";
//  DEBUG_OUT << "This will be logged always";
//

namespace Debug
{
    enum DebugLevels
    {
        HighLevel = 3,
        MidLevel  = 2,
        LowLevel  = 1,
        ZeroLevel = 0
    };
}

#define DEBUG_HIGH_LEVEL    DEBUG_OUT_FUNC_INFO( 3 )
#define DEBUG_MID_LEVEL     DEBUG_OUT_FUNC_INFO( 2 )
#define DEBUG_LOW_LEVEL     DEBUG_OUT_FUNC_INFO( 1 )
#define DEBUG_OUT           qDebug()

#define DEBUG_OUT_FUNC_INFO( DEBUG_LEVEL )   if(g_debugLevel >= DEBUG_LEVEL) qDebug()

// Define this to 1 and rebuild project to enable API debug mode
#define API_DEBUG           0

#if API_DEBUG
#   define API_DEBUG_OUT   qDebug()
#else
#   define API_DEBUG_OUT   if(0) qDebug()
#endif
