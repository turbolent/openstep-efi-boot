/*
 * Copyright (c) 1992 NeXT Computer, Inc.
 *
 * Byte ordering conversion.
 *
 * HISTORY
 *
 * 20 October 1992 David E. Bohman at NeXT
 *	Added #ifdef wrapper to prevent multiple inclusions of this file.
 *
 * 8 October 1992 David E. Bohman at NeXT
 *	Converted to NXxxx versions.  Condensed history.
 *
 * 18 May 1992 David E. Bohman at NeXT
 *	Created.
 */

#ifndef	_ARCHITECTURE_BYTE_ORDER_H_
#define _ARCHITECTURE_BYTE_ORDER_H_
 
typedef unsigned long NXSwappedFloat;
typedef unsigned long long NXSwappedDouble;

#import <architecture/ARCH_INCLUDE.h>

#import ARCH_INCLUDE(architecture/, byte_order.h)

/*
 * Identify the byte order
 * of the current host.
 */

enum NXByteOrder {
    NX_UnknownByteOrder,
    NX_LittleEndian,
    NX_BigEndian
};

static __inline__
enum NXByteOrder
NXHostByteOrder(void)
{
    unsigned int	_x;
    
    _x = (NX_BigEndian << 24) | NX_LittleEndian;
        
    return ((enum NXByteOrder)*((unsigned char *)&_x));
}

/*
 * The predicated versions
 * are defined here in terms
 * of the unpredicated ones.
 */

#if	__BIG_ENDIAN__

static __inline__
unsigned short
NXSwapBigShortToHost(
    unsigned short	x
)
{
    return (x);
}

static __inline__
unsigned int
NXSwapBigIntToHost(
    unsigned int	x
)
{
    return (x);
}

static __inline__
unsigned long
NXSwapBigLongToHost(
    unsigned long	x
)
{
    return (x);
}

static __inline__
unsigned long long
NXSwapBigLongLongToHost(
    unsigned long long	x
)
{
    return (x);
}

static __inline__
double
NXSwapBigDoubleToHost(
    NXSwappedDouble	x
)
{
    return NXConvertSwappedDoubleToHost(x);
}

static __inline__
float
NXSwapBigFloatToHost(
    NXSwappedFloat	x
)
{
    return NXConvertSwappedFloatToHost(x);
}

static __inline__
unsigned short
NXSwapHostShortToBig(
    unsigned short	x
)
{
    return (x);
}

static __inline__
unsigned int
NXSwapHostIntToBig(
    unsigned int	x
)
{
    return (x);
}

static __inline__
unsigned long
NXSwapHostLongToBig(
    unsigned long	x
)
{
    return (x);
}

static __inline__
unsigned long long
NXSwapHostLongLongToBig(
    unsigned long long	x
)
{
    return (x);
}

static __inline__
NXSwappedDouble
NXSwapHostDoubleToBig(
    double		x
)
{
    return NXConvertHostDoubleToSwapped(x);
}

static __inline__
NXSwappedFloat
NXSwapHostFloatToBig(
    float		x
)
{
    return NXConvertHostFloatToSwapped(x);
}

static __inline__
unsigned short
NXSwapLittleShortToHost(
    unsigned short	x
)
{
    return (NXSwapShort(x));
}

static __inline__
unsigned int
NXSwapLittleIntToHost(
    unsigned int	x
)
{
    return (NXSwapInt(x));
}

static __inline__
unsigned long
NXSwapLittleLongToHost(
    unsigned long	x
)
{
    return (NXSwapLong(x));
}

static __inline__
unsigned long long
NXSwapLittleLongLongToHost(
    unsigned long long	x
)
{
    return (NXSwapLongLong(x));
}

static __inline__
double
NXSwapLittleDoubleToHost(
    NXSwappedDouble	x
)
{
    return NXConvertSwappedDoubleToHost(NXSwapDouble(x));
}

static __inline__
float
NXSwapLittleFloatToHost(
    NXSwappedFloat	x
)
{
    return NXConvertSwappedFloatToHost(NXSwapFloat(x));
}

static __inline__
unsigned short
NXSwapHostShortToLittle(
    unsigned short	x
)
{
    return (NXSwapShort(x));
}

static __inline__
unsigned int
NXSwapHostIntToLittle(
    unsigned int	x
)
{
    return (NXSwapInt(x));
}

static __inline__
unsigned long
NXSwapHostLongToLittle(
    unsigned long	x
)
{
    return (NXSwapLong(x));
}

static __inline__
unsigned long long
NXSwapHostLongLongToLittle(
    unsigned long long	x
)
{
    return (NXSwapLongLong(x));
}

static __inline__
NXSwappedDouble
NXSwapHostDoubleToLittle(
    double		x
)
{
    return NXSwapDouble(NXConvertHostDoubleToSwapped(x));
}

static __inline__
NXSwappedFloat
NXSwapHostFloatToLittle(
    float		x
)
{
    return NXSwapFloat(NXConvertHostFloatToSwapped(x));
}

#endif

#if	__LITTLE_ENDIAN__

static __inline__
unsigned short
NXSwapBigShortToHost(
    unsigned short	x
)
{
    return (NXSwapShort(x));
}

static __inline__
unsigned int
NXSwapBigIntToHost(
    unsigned int	x
)
{
    return (NXSwapInt(x));
}

static __inline__
unsigned long
NXSwapBigLongToHost(
    unsigned long	x
)
{
    return (NXSwapLong(x));
}

static __inline__
unsigned long long
NXSwapBigLongLongToHost(
    unsigned long long	x
)
{
    return (NXSwapLongLong(x));
}

static __inline__
double
NXSwapBigDoubleToHost(
    NXSwappedDouble	x
)
{
    return NXConvertSwappedDoubleToHost(NXSwapDouble(x));
}

static __inline__
float
NXSwapBigFloatToHost(
    NXSwappedFloat	x
)
{
    return NXConvertSwappedFloatToHost(NXSwapFloat(x));
}

static __inline__
unsigned short
NXSwapHostShortToBig(
    unsigned short	x
)
{
    return (NXSwapShort(x));
}

static __inline__
unsigned int
NXSwapHostIntToBig(
    unsigned int	x
)
{
    return (NXSwapInt(x));
}

static __inline__
unsigned long
NXSwapHostLongToBig(
    unsigned long	x
)
{
    return (NXSwapLong(x));
}

static __inline__
unsigned long long
NXSwapHostLongLongToBig(
    unsigned long long	x
)
{
    return (NXSwapLongLong(x));
}

static __inline__
NXSwappedDouble
NXSwapHostDoubleToBig(
    double		x
)
{
    return (NXSwapDouble(NXConvertHostDoubleToSwapped(x)));
}

static __inline__
NXSwappedFloat
NXSwapHostFloatToBig(
    float		x
)
{
    return (NXSwapFloat(NXConvertHostFloatToSwapped(x)));
}

static __inline__
unsigned short
NXSwapLittleShortToHost(
    unsigned short	x
)
{
    return (x);
}

static __inline__
unsigned int
NXSwapLittleIntToHost(
    unsigned int	x
)
{
    return (x);
}

static __inline__
unsigned long
NXSwapLittleLongToHost(
    unsigned long	x
)
{
    return (x);
}

static __inline__
unsigned long long
NXSwapLittleLongLongToHost(
    unsigned long long	x
)
{
    return (x);
}

static __inline__
double
NXSwapLittleDoubleToHost(
    NXSwappedDouble	x
)
{
    return NXConvertSwappedDoubleToHost(x);
}

static __inline__
float
NXSwapLittleFloatToHost(
    NXSwappedFloat	x
)
{
    return NXConvertSwappedFloatToHost(x);
}

static __inline__
unsigned short
NXSwapHostShortToLittle(
    unsigned short	x
)
{
    return (x);
}

static __inline__
unsigned int
NXSwapHostIntToLittle(
    unsigned int	x
)
{
    return (x);
}

static __inline__
unsigned long
NXSwapHostLongToLittle(
    unsigned long	x
)
{
    return (x);
}

static __inline__
unsigned long long
NXSwapHostLongLongToLittle(
    unsigned long long	x
)
{
    return (x);
}

static __inline__
NXSwappedDouble
NXSwapHostDoubleToLittle(
    double		x
)
{
    return NXConvertHostDoubleToSwapped(x);
}

static __inline__
NXSwappedFloat
NXSwapHostFloatToLittle(
    float		x
)
{
    return NXConvertHostFloatToSwapped(x);
}

#endif

#endif	_ARCHITECTURE_BYTE_ORDER_H_
