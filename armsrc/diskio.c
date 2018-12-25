/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2007        */
/*-----------------------------------------------------------------------*/
/* This is a stub disk I/O module that acts as front end of the existing */
/* disk I/O modules and attach it to FatFs module with common interface. */
/*-----------------------------------------------------------------------*/

#include "diskio.h"

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */

DSTATUS disk_initialize (
	uint8_t drv				/* Physical drive nmuber (0..) */
)
{
//	DSTATUS stat;
//	int result;

//	stat = ATA_disk_initialize();
//	return stat;

	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Return Disk Status                                                    */

DSTATUS disk_status (
	uint8_t drv		/* Physical drive nmuber (0..) */
)
{
//	DSTATUS stat;
//	int result;

//	stat = ATA_disk_status();
//	return stat;

	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */

DRESULT disk_read (
	uint8_t drv,		  /* Physical drive nmuber (0..) */
	uint8_t *buff,		/* Data buffer to store read data */
	uint32_t sector,	/* Sector address (LBA) */
	uint8_t count		  /* Number of sectors to read (1..255) */
)
{
//	DRESULT res;
//	int result;

//	res = ATA_disk_read(buff, sector, count);
//	return res;

	return RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */

#if _READONLY == 0
DRESULT disk_write (
	uint8_t       drv,			/* Physical drive nmuber (0..) */
	const uint8_t *buff,	/* Data to be written */
	uint32_t      sector,		/* Sector address (LBA) */
	uint8_t       count			/* Number of sectors to write (1..255) */
)
{
//	DRESULT res;
//	int result;

//	result = ATA_disk_write(buff, sector, count);
//	return res;

	return RES_PARERR;
}
#endif /* _READONLY */



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */

DRESULT disk_ioctl (
	uint8_t drv,		/* Physical drive nmuber (0..) */
	uint8_t ctrl,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
//	DRESULT res;
//	int result;

//	result = ATA_disk_ioctl(ctrl, buff);
//	return res;

	return RES_PARERR;
}

