/***************************************************************************
 *   Copyright (C) 2011 by Terraneo Federico                               *
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

#ifndef ENDIANNESS_IMPL_H
#define	ENDIANNESS_IMPL_H

//This target is little endian
#define MIOSIX_LITTLE_ENDIAN

#ifdef __cplusplus
#define __MIOSIX_INLINE inline
#else //__cplusplus
#define __MIOSIX_INLINE static inline
#endif //__cplusplus


__MIOSIX_INLINE unsigned short swapBytes16(unsigned short x)
{
    return (x>>8) | (x<<8);
}

__MIOSIX_INLINE unsigned int swapBytes32(unsigned int x)
{
    #ifdef __GNUC__
    return __builtin_bswap32(x);
    #else //__GNUC__
    return ( x>>24)               |
           ((x<< 8) & 0x00ff0000) |
           ((x>> 8) & 0x0000ff00) |
           ( x<<24);
    #endif //__GNUC__
}

__MIOSIX_INLINE unsigned long long swapBytes64(unsigned long long x)
{
    #ifdef __GNUC__
    return __builtin_bswap64(x);
    #else //__GNUC__
    return ( x>>56)                          |
           ((x<<40) & 0x00ff000000000000ull) |
           ((x<<24) & 0x0000ff0000000000ull) |
           ((x<< 8) & 0x000000ff00000000ull) |
           ((x>> 8) & 0x00000000ff000000ull) |
           ((x>>24) & 0x0000000000ff0000ull) |
           ((x>>40) & 0x000000000000ff00ull) |
           ( x<<56);
    #endif //__GNUC__
}

#undef __MIOSIX_INLINE

#endif //ENDIANNESS_IMPL_H
