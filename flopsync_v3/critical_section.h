 
/***************************************************************************
 *   Copyright (C)  2013 by Terraneo Federico                              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/
#ifndef CRITICAL_SECTION_H
#define	CRITICAL_SECTION_H

#if 0

#include "protocol_constants.h"
#include <miosix.h>

/**
 * To send (in the root node) or rebroadcast (in the synchronized nodes) the
 * synchronization packet with a jitter as low as possible, disable the systick
 * interrupt, that in Miosix is used for preemption. Note that it's not
 * possible to simply disable all interrupts as the radio code is
 * interrupt-driven.
 * This is a bit of a hack, and may break in future Miosix versions.
 */
class CriticalSection
{
public:
    /**
     * Constructor
     */
    CriticalSection();
    
    /**
     * Destructor
     */
    ~CriticalSection();
};

#endif

#endif //CRITICAL_SECTION_H