#include <project.h>
#include <stdio.h>
#include "fat_sl.h"
#include "config_fat_sl.h"
#include "api_mdriver_fram.h"

// This function calls the FS printSector function 
void exPrintSector( void  * driver, void * data, unsigned long sector )
{
    printSector(driver,data,sector);
}

// This function initalize the filesystem
void exInit(void)
{
    unsigned char ucStatus;
    
	/* First create the volume. */
	ucStatus = f_initvolume( fram_initfunc );
    UART_UartPutString("Initialized sucessfully\n");
	/* It is expected that the volume is not formatted. */
	if( ucStatus == F_ERR_NOTFORMATTED )
	{
        UART_UartPutString("Filesystem Unformatted\n");
	}
    
    else
    {
        UART_UartPutString("Filesystem Formatted\n");
    }
}

#define FILENAME "afile.txt"
// exCreateFile - this function creates a file called "afile.txt" an
void exCreateFile(void)
{
    F_FILE *pxFile;
    UART_UartPutString("Attempting Create & Write File\n");
    pxFile = f_open(FILENAME, "w" );
  
    if(pxFile)
    {
        for(int i='0' ; i< '9' ; i++)
        {
            f_putc(i,pxFile);
        }
        f_close(pxFile);
    }
    else
    {
        UART_UartPutString("Failed File Create\n");
    }
}

void exReadFile(void)
{
    UART_UartPutString("Attempting Read\n");
    F_FILE *pxFile;
    pxFile = f_open( FILENAME, "r" );
    if(pxFile)
    {
        while(!f_eof(pxFile))
        {
            UART_UartPutChar(f_getc(pxFile));
        }
        f_close(pxFile);
    }
    else
    {
        UART_UartPutString("File not found\n");
    }
}



void exFormat(void)
{
    char buff[128];
	/* Format the created volume. */
    unsigned char ucStatus;
    //ucStatus = f_format( F_FAT12_MEDIA );
	ucStatus = f_format( F_FAT12_MEDIA );
	if( ucStatus == F_NO_ERROR )
	{
        UART_UartPutString("Formatted sucessfully\n");
    }
    else
    {
        sprintf(buff,"Error = %d\n",ucStatus);
        UART_UartPutString(buff);   
        UART_UartPutString("Format Error\n");
    }   
}
void exDriveInfo( void )
{
    char buff[128];
    F_SPACE xSpace;
    unsigned char ucReturned;

    /* Get space information on current embedded FAT file system drive. */
    ucReturned = f_getfreespace( &xSpace );
    if( ucReturned == F_NO_ERROR )
    {
        /* xSpace.total holds the total drive size, xSpace.free holds the
        free space on the drive, xSpace.used holds the size of the used space
        on the drive, xSpace.bad holds the size of unusable space on the
        drive. */
        sprintf(buff,"Free Space = %lu\nUsed Space = %lu\nTotal = %lu\n",xSpace.free,xSpace.used,xSpace.total);
        UART_UartPutString(buff);
    }
    else
    {
        /* xSpace could not be completed.  ucReturned holds the error code. */
        UART_UartPutString("Free space failed\n");
    }
}

void exDirectory( void )
{
    char buff[128];
    F_FIND xFindStruct;

    /* Print out information on every file in the subdirectory "subdir". */
    if( f_findfirst( "*.*", &xFindStruct ) == F_NO_ERROR )
    {
        do
        {
            sprintf(buff,"filename:%s ", xFindStruct.filename );
            UART_UartPutString(buff);

            if( ( xFindStruct.attr & F_ATTR_DIR ) != 0 )
            {
                UART_UartPutString ( "is a directory directory\n" );
            }
            else
            {
                sprintf ( buff, "is a file of size %lu\n", xFindStruct.filesize );
                UART_UartPutString(buff);
            }

        } while( f_findnext( &xFindStruct ) == F_NO_ERROR );
    }
}