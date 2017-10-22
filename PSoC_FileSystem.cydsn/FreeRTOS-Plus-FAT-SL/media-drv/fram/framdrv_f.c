/*
 * FreeRTOS+FAT SL V1.0.1 (C) 2014 HCC Embedded
 *
 * The FreeRTOS+FAT SL license terms are different to the FreeRTOS license
 * terms.
 *
 * FreeRTOS+FAT SL uses a dual license model that allows the software to be used
 * under a standard GPL open source license, or a commercial license.  The
 * standard GPL license (unlike the modified GPL license under which FreeRTOS
 * itself is distributed) requires that all software statically linked with
 * FreeRTOS+FAT SL is also distributed under the same GPL V2 license terms.
 * Details of both license options follow:
 *
 * - Open source licensing -
 * FreeRTOS+FAT SL is a free download and may be used, modified, evaluated and
 * distributed without charge provided the user adheres to version two of the
 * GNU General Public License (GPL) and does not remove the copyright notice or
 * this text.  The GPL V2 text is available on the gnu.org web site, and on the
 * following URL: http://www.FreeRTOS.org/gpl-2.0.txt.
 *
 * - Commercial licensing -
 * Businesses and individuals who for commercial or other reasons cannot comply
 * with the terms of the GPL V2 license must obtain a commercial license before
 * incorporating FreeRTOS+FAT SL into proprietary software for distribution in
 * any form.  Commercial licenses can be purchased from
 * http://shop.freertos.org/fat_sl and do not require any source files to be
 * changed.
 *
 * FreeRTOS+FAT SL is distributed in the hope that it will be useful.  You
 * cannot use FreeRTOS+FAT SL unless you agree that you use the software 'as
 * is'.  FreeRTOS+FAT SL is provided WITHOUT ANY WARRANTY; without even the
 * implied warranties of NON-INFRINGEMENT, MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. Real Time Engineers Ltd. and HCC Embedded disclaims all
 * conditions and terms, be they implied, expressed, or statutory.
 *
 * http://www.FreeRTOS.org
 * http://www.FreeRTOS.org/FreeRTOS-Plus
 *
 */
#include <project.h>
#include <stdio.h>

#include "../../api/api_mdriver_fram.h"
//#include "config_mdriver_fram.h"
#include "config_fat_sl.h"
#include "../../psp/include/psp_string.h"

#include "../../version/ver_mdriver_ram.h"
#if VER_MDRIVER_RAM_MAJOR != 1 || VER_MDRIVER_RAM_MINOR != 2
 #error Incompatible MDRIVER_RAM version number!
#endif

/* The array used as the RAM disk storage. */
//static char  ramdrv[ MDRIVER_RAM_VOLUME0_SIZE ];

/* The F_DRIVER structure that is filled with the RAM disk versions of the read
sector, write sector, etc. functions. */

static F_DRIVER  t_driver;
/* Disk not initialized yet. */
static char in_use = 0;

static const unsigned long maxsector = FDRIVER_VOLUME0_SIZE / F_SECTOR_SIZE;
static const unsigned long halfsector = FDRIVER_VOLUME0_SIZE / F_SECTOR_SIZE / 2;

/****************************************************************************
 *
 * calcAddress
 *
 * This function takes a sector and returns the address in the bank for the
 * start of the secor
 *
 * INPUTS
 *
 * unsigned long sector - which logical sector in the FRAM
 * 
 * RETURNS
 *
 * The FRAM Address of the sector
 *
 ***************************************************************************/
static inline uint32_t calcAddress(unsigned long sector)
{
    if(sector < halfsector)
        return sector * F_SECTOR_SIZE;
    else
        return (sector-halfsector) * F_SECTOR_SIZE;
}

/****************************************************************************
 *
 * calcI2CAddress
 *
 * This function takes a sector from 0 --> maxsector and figures out which bank
 * the address exists.
 *
 * INPUTS
 *
 * unsigned long sector - which logical sector in the FRAM to write to
 * 
 * RETURNS
 *
 * The I2C Address of Bank 0 or Bank 1
 *
 ***************************************************************************/
static inline uint32_t calcI2CAddress(unsigned long sector)
{
    if(sector < halfsector)
        return 0x50; // I2C Bank 0 Address from the datasheet
    else
        return 0x51; // I2C Bank 0 Address - From the datasheet  
}

/****************************************************************************
 *
 * fram_readsector
 *
 * This function reads 512 bytes into the SRAM at the pointer data
 *
 * INPUTS
 *
 * driver - driver structure
 * void *data - a pointer to the SRAM where the 512 bytes of data will be written
 * unsigned long sector - which logical sector in the FRAM to read from
 * 
 * RETURNS
 *
 * error code or MDRIVER_RAM_NO_ERROR if successful
 *
 ***************************************************************************/

static int fram_readsector ( F_DRIVER * driver, void * data, unsigned long sector )
{
    char buff[128]; // A scratch buffer for UART Printing
    (void)driver;
    uint16 address;
    uint32_t status;
    
    sprintf(buff,"Read sector %d\n",(int)sector);
    UART_UartPutString(buff);
    
    address = calcAddress(sector);
    status = I2C_I2CMasterSendStart( calcI2CAddress(sector),I2C_I2C_WRITE_XFER_MODE);
    if(status != I2C_I2C_MSTR_NO_ERROR)
    {
        UART_UartPutString("I2C Error\n");
        return MDRIVER_RAM_ERR_SECTOR;
    }
    int i;
    I2C_I2CMasterWriteByte((address>>8)&0xFF);
    I2C_I2CMasterWriteByte(address & 0xFF); // 
    
    
    I2C_I2CMasterSendRestart(calcI2CAddress(sector),I2C_I2C_READ_XFER_MODE);
    
    uint8_t d;
    for(i=0;i<512;i++)
    {
        
        if(i != F_SECTOR_SIZE - 1)
            d = I2C_I2CMasterReadByte(I2C_I2C_ACK_DATA);
        else
            d = I2C_I2CMasterReadByte(I2C_I2C_NAK_DATA);
         
        *((uint8_t *)data + i) = d;
        
    }
    
    I2C_I2CMasterSendStop();
    
  return MDRIVER_RAM_NO_ERROR;
}


/****************************************************************************
 *
 * fram_writesector
 *
 * This function takes 512 bytes of user input and writes to the FRAM in the
 *
 * INPUTS
 *
 * driver - driver structure
 * void *data - a pointer to the SRAM where the 512 bytes of data exists
 * unsigned long sector - which logical sector in the FRAM to write to
 * 
 * RETURNS
 *
 * error code or MDRIVER_RAM_NO_ERROR if successful
 *
 ***************************************************************************/

static int fram_writesector ( F_DRIVER * driver, void * data, unsigned long sector )
{
    (void)driver;
    char buff[128]; // A scratch buffer for UART Printing
    uint16 address;
    int i;
        
    sprintf(buff,"Wrote sector %d\n",(int)sector);
    UART_UartPutString(buff);
        
    address = calcAddress(sector);
    I2C_I2CMasterSendStart(calcI2CAddress(sector),I2C_I2C_WRITE_XFER_MODE);
    I2C_I2CMasterWriteByte((address>>8)&0xFF);
    I2C_I2CMasterWriteByte(address & 0xFF); // 
    
    for(i=0;i<512;i++)
    {
        uint8_t d = *((uint8_t *)data +i);
       
        I2C_I2CMasterWriteByte(d); 
        
    }
    I2C_I2CMasterSendStop();
    
  return MDRIVER_RAM_NO_ERROR;
}


/****************************************************************************
 *
 * ram_getphy
 *
 * determinate ramdrive physicals
 *
 * INPUTS
 *
 * driver - driver structure
 * phy - this structure has to be filled with physical information
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/
static int fram_getphy ( F_DRIVER * driver, F_PHY * phy )
{
  /* Not used. */
  ( void ) driver;

  phy->number_of_sectors = maxsector;
  phy->bytes_per_sector = F_SECTOR_SIZE;

  return MDRIVER_RAM_NO_ERROR;
}


/****************************************************************************
 *
 * fram_release
 *
 * Releases a drive
 *
 * INPUTS
 *
 * driver_param - driver parameter
 *
 ***************************************************************************/
static void fram_release ( F_DRIVER * driver )
{
  /* Not used. */
  ( void ) driver;

  /* Disk no longer in use. */
  in_use = 0;
}

/****************************************************************************
 *
 * fram_getstatus
 *
 * This function must return the status of the drive... F_ST_MISSING, F_ST_CHANGED
 * or F_ST_WRPROECT or 0 (for OK)
 * INPUTS
 *
 * driver_param - driver parameter
 *
 *
 * For the FRAM I dont support any of these other status's
 ***************************************************************************/

static long fram_getstatus(F_DRIVER *driver)
{
    (void) driver;
    // F_ST_MISSING	The media is not present (it has been removed or was never inserted).
    //F_ST_CHANGED	Since F_GETSTATUS() was last called the media has either been removed and re-inserted, or a different media has been inserted.
    //F_ST_WRPROTECT	The media is write protected.
    
    return 0;
}


/****************************************************************************
 *
 * fram_initfunc
 *
 * this init function has to be passed for highlevel to initiate the
 * driver functions
 *
 * INPUTS
 *
 * driver_param - driver parameter
 *
 * RETURNS
 *
 * driver structure pointer
 *
 ***************************************************************************/
F_DRIVER * fram_initfunc ( unsigned long driver_param )
{
  ( void ) driver_param;

    UART_UartPutString("Calling init\n");

  if( in_use )
    return NULL;

  (void)psp_memset( &t_driver, 0, sizeof( F_DRIVER ) );

  t_driver.readsector = fram_readsector;
  t_driver.writesector = fram_writesector;
  t_driver.getphy = fram_getphy;
  t_driver.release = fram_release;
  t_driver.getstatus = fram_getstatus;

  in_use = 1;

  return &t_driver;
} /* fram_initfunc */

/****************************************************************************
 *
 * printSector
 *
 * This function prints out the sector as rows of hex bytes followed by isprint() 
 * ASCII characters or "-" if it is not printable.  This function ASSUMES!!! that
 * it can call UART_UartPutString (this is not a very good assumption)
 *
 * INPUTS
 *
 * driver_param - driver parameter
 * data - unused
 * sector - which sector
 *
 * RETURNS
 *
 * void
 *
 ***************************************************************************/
void printSector( F_DRIVER * driver, void * data, unsigned long sector )
{
    int i;
    int count=0;
    (void)data;
    uint8_t sectorData[ F_SECTOR_SIZE];
    char buff[128]; // A scratch buffer for UART Printing
    fram_readsector(driver,sectorData,sector);
    
    sprintf(buff,"\nSector = %lu\n",sector);
    UART_UartPutString(buff);
    

    for(i=0;i<F_SECTOR_SIZE;i++)
    {
        sprintf(buff,"%2X ",sectorData[i]);
        UART_UartPutString(buff);
        if(++count>15)
        {
            count = 0;
            int j;
            for(j=-15 ; j<1 ; j++)
            {
                if(isprint(sectorData[i+j]))
                    UART_UartPutChar(sectorData[i+j]);
                else
                    UART_UartPutChar('-');
                       
            }
            UART_UartPutString("\n");
        } 
    }
    UART_UartPutString("\n");
}