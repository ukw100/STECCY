/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2016        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include <time.h>
#include "diskio.h"		/* FatFs lower layer API */

/* Definitions of physical drive number for each drive */
#define DEV_MMC		0	/* Map MMC/SD card to physical drive 0 */
#define DEV_RAM		1	/* Map Ramdisk to physical drive 1 */
#define DEV_USB		2	/* Map USB MSD to physical drive 2 */

#include "stm32_sdcard.h"

#ifdef __GNUC__
#  define UNUSED(x)         UNUSED_ ## x __attribute__((__unused__))
#else
#  define UNUSED(x)         UNUSED_ ## x
#endif

DWORD
get_fattime (void)
{
    struct tm   tm;
    DWORD       rtc;

    // later, if we have a rtc, we can fill tm struct with correct data
    // yet, we use 2020-02-02 22:22:22 as timestamp
    tm.tm_year      = 2020 - 1900;                          // year since 1900: 2020 -> 120
    tm.tm_mon       = 1;                                    // february
    tm.tm_mday      = 2;                                    // 2nd of february
    tm.tm_hour      = 22;
    tm.tm_min       = 22;
    tm.tm_sec       = 22;

    rtc = ((tm.tm_year - 80)    << 25) |                    // year since 1980
          ((tm.tm_mon + 1)      << 21) |
          (tm.tm_mday           << 16) |
          (tm.tm_hour           << 11) |
          (tm.tm_min            << 5)  |
          (tm.tm_sec            >> 1);

    return rtc;
}

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS stat;
	int result;

	switch (pdrv) {
#if 0
	case DEV_RAM :
		result = RAM_disk_status();

		// translate the reslut code here

		return stat;
#endif

	case DEV_MMC :
		result = MMC_disk_status();

        // translate the result code here
        if(result == 0)
        {
            stat = 0;
        }
        else
        {
            stat = STA_NODISK | STA_NOINIT;
        }

		return stat;
#if 0
	case DEV_USB :
		result = USB_disk_status();

		// translate the reslut code here

		return stat;
#endif
	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS stat;
	int result;

	switch (pdrv) {
#if 0
	case DEV_RAM :
		result = RAM_disk_initialize();

		// translate the reslut code here

		return stat;
#endif
	case DEV_MMC :
		result = MMC_disk_initialize();

        // translate the result code here
        if(result == 0)
        {
            stat = 0;
        }
        else
        {
            stat = STA_NOINIT;
        }

		return stat;

#if 0
	case DEV_USB :
		result = USB_disk_initialize();

		// translate the reslut code here

		return stat;
#endif
	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	DRESULT res;
	int result;

	switch (pdrv) {
#if 0
	case DEV_RAM :
		// translate the arguments here

		result = RAM_disk_read(buff, sector, count);

		// translate the reslut code here

		return res;
#endif

	case DEV_MMC :
		// translate the arguments here

		result = MMC_disk_read (buff, sector, count);

        // translate the result code here
        if (result == 0)
        {
            res = RES_OK;
        }
        else
        {
            res = RES_ERROR;
        }

		return res;

#if 0
	case DEV_USB :
		// translate the arguments here

		result = USB_disk_read(buff, sector, count);

		// translate the reslut code here

		return res;
#endif
	}

	return RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	DRESULT res;
	int result;

	switch (pdrv) {
#if 0
	case DEV_RAM :
		// translate the arguments here

		result = RAM_disk_write(buff, sector, count);

		// translate the reslut code here

		return res;
#endif

	case DEV_MMC :
		// translate the arguments here

		result = MMC_disk_write(buff, sector, count);

        // translate the result code here
        if (result == 0)
        {
            res = RES_OK;
        }
        else
        {
            res = RES_ERROR;
        }

		return res;

#if 0
	case DEV_USB :
		// translate the arguments here

		result = USB_disk_write(buff, sector, count);

		// translate the reslut code here

		return res;
#endif
	}

	return RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		                                                /* Physical drive nmuber (0..) */
    BYTE UNUSED(cmd),		                                        /* Control code */
	void * UNUSED(buff)		                                        /* Buffer to send/receive control data */
)
{
	DRESULT res = 0;

	switch (pdrv) {
	case DEV_RAM :

		// Process of the command for the RAM drive

		return res;

	case DEV_MMC :

        // translate the result code here

		return res;

	case DEV_USB :

		// Process of the command the USB drive

		return res;
	}

	return RES_PARERR;
}

