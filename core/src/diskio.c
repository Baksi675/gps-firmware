/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2025        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "err.h"
#include "ff.h"			/* Basic definitions of FatFs */
#include "diskio.h"		/* Declarations FatFs MAI */

/* Example: Declarations of the platform and disk functions in the project */
#include "sd.h"

/* Example: Mapping of physical drive number for each drive */
#define DEV_FLASH	1	/* Map FTL to physical drive 0 */
#define DEV_MMC		0	/* Map MMC/SD card to physical drive 1 */
#define DEV_USB		2	/* Map USB MSD to physical drive 2 */

SD_HANDLE_ts *sd_handle;


/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	//DSTATUS stat;
	//int result;
	bool initialized;

	switch (pdrv) {
	case DEV_FLASH :
		//result = FLASH_disk_status();

		// translate the reslut code here

		//return stat;
		break;

	case DEV_MMC :
		//result = MMC_disk_status();

		// translate the reslut code here
		sd_get_handle_init(sd_handle, &initialized);
		if(initialized == true) {
			return 0;
		}

		return STA_NOINIT;

		//return stat;

	case DEV_USB :
		//result = USB_disk_status();

		// translate the reslut code here

		//return stat;
		break;
	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/
static bool initialized = false;
DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	if(initialized) {
		return 0;
	}

	//DSTATUS stat;
	//int result;
	ERR_te err;

	if(pdrv == DEV_FLASH) {
		//result = FLASH_disk_initialize();

		// translate the reslut code here
	}
	else if(pdrv == DEV_MMC) {
		//result = MMC_disk_initialize();
		SD_CONFIG_ts sd_config = { 0 };
		str_cpy(sd_config.name, "sdcard", get_str_len("sdcard") + 1);
		sd_config.spi_instance = SPI1;
		sd_config.sclk_gpio_port = GPIOA;
		sd_config.sclk_gpio_pin = GPIO_PIN_5;
		sd_config.miso_gpio_port = GPIOA;
		sd_config.miso_gpio_pin = GPIO_PIN_6;
		sd_config.mosi_gpio_port = GPIOA;
		sd_config.mosi_gpio_pin = GPIO_PIN_7;
		sd_config.cs_gpio_port = GPIOB;
		sd_config.cs_gpio_pin = GPIO_PIN_7;									// on prototype it's pin 6
		sd_config.gpio_alternate_function = GPIO_ALTERNATE_FUNCTION_AF5;
		
		// translate the reslut code here
		err = sd_init_subsys();
		if(err != ERR_OK) {
			return STA_NOINIT;
		}
		err = sd_init_handle(&sd_config, &sd_handle);
		if(err != ERR_OK) {
			return STA_NOINIT;
		}
		err = sd_start_subsys();
		if(err != ERR_OK) {
			return STA_NOINIT;
		}

		initialized = true;
		// Return no error
		return 0;
	}
	else if(pdrv == DEV_USB) {
		//result = USB_disk_initialize();

		// translate the reslut code here
	}

	/*
	switch (pdrv) {
	case DEV_FLASH :
		//result = FLASH_disk_initialize();

		// translate the reslut code here

		return stat;

	case DEV_MMC :
		//result = MMC_disk_initialize();

		// translate the reslut code here

		return stat;

	case DEV_USB :
		//result = USB_disk_initialize();

		// translate the reslut code here

		return stat;
	}
	*/

	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	//DRESULT res;
	//int result;
	ERR_te err;

	switch (pdrv) {
	case DEV_FLASH :
		// translate the arguments here

		//result = FLASH_disk_read(buff, sector, count);

		// translate the reslut code here

		//return res;
		break;

	case DEV_MMC :
		// translate the arguments here

		//result = MMC_disk_read(buff, sector, count);
		err = sd_read(sd_handle, (BYTE *)buff, (LBA_t)sector, (UINT)count);

		// translate the reslut code here
		if(err != ERR_OK) {
			return RES_ERROR;
		}

		return RES_OK;

		//return res;

	case DEV_USB :
		// translate the arguments here

		//result = USB_disk_read(buff, sector, count);

		// translate the reslut code here

		//return res;
		break;
	}

	return RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	//DRESULT res;
	//int result;
	ERR_te err;

	switch (pdrv) {
	case DEV_FLASH :
		// translate the arguments here

		//result = FLASH_disk_write(buff, sector, count);

		// translate the reslut code here

		//return res;
		break;

	case DEV_MMC :
		// translate the arguments here

		//result = MMC_disk_write(buff, sector, count);
		err = sd_write(sd_handle, (BYTE*)buff, (LBA_t)sector, (UINT)count);

		// translate the reslut code here
		if(err != ERR_OK) {
			return RES_ERROR;
		}

		return RES_OK;

		//return res;

	case DEV_USB :
		// translate the arguments here

		//result = USB_disk_write(buff, sector, count);

		// translate the reslut code here

		//return res;
		break;
	}

	return RES_PARERR;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	//DRESULT res;
	//int result;
	ERR_te err = 0;

	switch (pdrv) {
	case DEV_FLASH :

		// Process of the command for the RAM drive

		//return res;
		break;

	case DEV_MMC :
		// Process of the command for the MMC/SD card
		if(cmd == CTRL_SYNC) {
			// Nothing to do there, SD write API already takes care of syncing
			return RES_OK;
		}
		else if(cmd == GET_SECTOR_COUNT) {
			uint32_t sector_count;
			err = sd_get_sector_count(sd_handle, &sector_count);
			*(uint32_t*)buff = sector_count;
		}
		else if(cmd == GET_SECTOR_SIZE) {
			uint32_t sector_size;
			err = sd_get_sector_size(sd_handle, &sector_size);
			*(uint32_t*)buff = sector_size;
		}
		/*else if(cmd == GET_BLOCK_SIZE) {
			
		}
		else if(cmd == CTRL_TRIM) {
			
		}*/

		if(err != ERR_OK) {
			return RES_ERROR;
		}

		return RES_OK;
		//return res;

	case DEV_USB :

		// Process of the command the USB drive

		//return res;
		break;
	}

	return RES_PARERR;
}

DWORD get_fattime (void)
{
    /*time_t t;
    struct tm *stm;


    t = time(0);
    stm = localtime(&t);

    return (DWORD)(stm->tm_year - 80) << 25 |
           (DWORD)(stm->tm_mon + 1) << 21 |
           (DWORD)stm->tm_mday << 16 |
           (DWORD)stm->tm_hour << 11 |
           (DWORD)stm->tm_min << 5 |
           (DWORD)stm->tm_sec >> 1;
	*/

	return 0;
}

