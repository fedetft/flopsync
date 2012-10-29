/*-----------------------------------------------------------------------
/  Low level disk interface modlue include file  R0.06   (C)ChaN, 2007
/-----------------------------------------------------------------------*/

//Added by TFT, for C++ compatibility
#ifdef __cplusplus
extern "C" {
#endif

#ifndef _DISKIO
#define _DISKIO

#include "integer.h"

/* Status of Disk Functions */
typedef BYTE	DSTATUS;

/* Results of Disk Functions */
typedef enum {
    RES_OK = 0,		/* 0: Successful */
    RES_ERROR,		/* 1: R/W Error */
    RES_WRPRT,		/* 2: Write Protected */
    RES_NOTRDY,		/* 3: Not Ready */
    RES_PARERR		/* 4: Invalid Parameter */
} DRESULT;

/*---------------------------------------*/
/* Prototypes for disk control functions */

DSTATUS disk_initialize (BYTE);
DSTATUS disk_status (BYTE);
DRESULT disk_read (BYTE, BYTE*, DWORD, BYTE);
#if	_READONLY == 0
DRESULT disk_write (BYTE, const BYTE*, DWORD, BYTE);
#endif
DRESULT disk_ioctl (BYTE, BYTE, void*);
void	disk_timerproc (void);

/* Disk Status Bits (DSTATUS) */
#define STA_NOINIT		0x01	/* Drive not initialized */
#define STA_NODISK		0x02	/* No medium in the drive */
#define STA_PROTECT		0x04	/* Write protected */

/* Command code for disk_ioctrl() */

/* Generic command */
#define CTRL_SYNC		0	/* Mandatory for read/write configuration */
#define GET_SECTOR_COUNT	1	/* Mandatory for only f_mkfs() */
//#define GET_SECTOR_SIZE		2
#define GET_BLOCK_SIZE		3	/* Mandatory for only f_mkfs() */
//#define CTRL_POWER			4
//#define CTRL_LOCK 			5
//#define CTRL_EJECT			6

#endif

//Added by TFT, for C++ compatibility
#ifdef __cplusplus
}
#endif
