// This template has heap_4.c included in the project.  If you want a 
// different heap scheme then remove heap_4 from the project and add 
// the other scheme from FreeRTOS/Source/portable/memmag

// TraceRecorder
// There are methods
// 1. Snapshot mode
// 2. Streaming UART/DMA
// 3. Streaming JLINK RTT
//
// To use method 1: 
// - in FreeRTOSConfig.h make this line be 1 (currently line 45)
//#define configUSE_TRACE_FACILITY                1
// - in trcConfig.h configure the TRC_CFG_RECORDER 
// #define TRC_CFG_RECORDER_MODE TRC_RECORDER_MODE_SNAPSHOT
//
// To use method 2:
// - in FreeRTOSConfig.h make this line be 1 (currently line 45)
//#define configUSE_TRACE_FACILITY                1
// - in trcConfig.h configure the TRC_CFG_RECORDER
//#define TRC_CFG_RECORDER_MODE TRC_RECORDER_MODE_STREAMING
//
// This project currently has the PSoC UART Streaming Port
// add the TraceRecorder/streamports/PSoC_Serial/trcStreamingPort.c to the project
// add the TraceRecorder/streamports/PSoC_Serial/include/trcStreamingPort.h
// add the TraceRecorder/streamports/PSoC_Serial/include to the compiler include directories
// Enable the Trace_DMA schematic sheet and make sure UART pins are assigned correctly
// This port depends on the UART being named UART and the DMA being named "DMA"
//
// To use method 3: JLINK RTT
// Remove the previous streamport files from the project
// Remove the Streamport include path
// add the TraceRecorder/streamports/JLink_RTT/Segger_RTT.c to the project
// add the TraceRecorder/streamports/JLink_RTT/Segger_RTT_Printf.c to the project
// add the TraceRecorder/streamports/JLink_RTT/include/trcStreamingPort.h
// add the TraceRecorder/streamports/JLink_RTT/include/Segger_RTT.h
// add the TraceRecorder/streamports/JLink_RTT/include/Segger_RTT_Conf.h
// add the TraceRecorder/streamports/JLink_RTT/include to the compiler include directories


#include "project.h"
#include "FreeRTOS.h"
#include "timers.h"
#include "fat_sl.h"
#include "config_fat_sl.h"
#include "api_mdriver_fram.h"
#include <stdio.h>

TaskHandle_t uartTaskHandle;
void uartISR()
{
    
    BaseType_t xHigherPriorityTaskWoken;
    // disable the interrupt
    UART_SetRxInterruptMode(0);
    vTaskNotifyGiveFromISR(uartTaskHandle,&xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
    
}

// Should only be used in UART Task
char buff[128];

void exInit()
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

void exCreateFile()
{
    F_FILE *pxFile;
    UART_UartPutString("Attempting Create & Write File\n");
     pxFile = f_open( "afile.bin", "w" );
    if(pxFile)
    {
    f_putc('1',pxFile);
    f_putc('2',pxFile);
    f_putc('3',pxFile);
    f_putc('4',pxFile);
    f_putc('5',pxFile);
    f_close(pxFile);
    }
    else
    {
        UART_UartPutString("Failed File Create\n");
    }
}

void exReadFile()
{
    UART_UartPutString("Attempting Read\n");
    F_FILE *pxFile;
    pxFile = f_open( "afile.bin", "r" );
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



void exFormat()
{
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
void vDriveInfo( void )
{
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
        UART_UartPutString("free space failed\n");
    }
}

void exDirectory( void )
{
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
						




void uartTask(void *arg)
{
    (void)arg;
    UART_Start();
    I2C_Start();

    UART_SetCustomInterruptHandler(uartISR);
    
    int sectorCount=128;
    
    while(1)
    {
        ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
        
        while(UART_SpiUartGetRxBufferSize())
        {
            char c;
            
            c= UART_UartGetChar();
            switch(c)
            {
                case 'i':
                    exInit();
                break;
                case 'f':
                    exFormat();
                break;
                case 'q':
                    vDriveInfo();
                break;
                    case 'w':
                    exCreateFile();
                    break;
                    
                    case 'r':
                    exReadFile();
                    break;
                    
                    case '0':
                        sectorCount = 0;
                        printSector(0,0,0);
                    break;
                   
            case '+':
                printSector(0,0,sectorCount);
                sectorCount += 1;
                
            break;
                    
            case '-':
                printSector(0,0,sectorCount);
                sectorCount -= 1;
                if(sectorCount<0)
                    sectorCount =0;
                
            break;
            case 'd':
                exDirectory();
            break;
            
                case 'z':
                printSector(0,0,127);
                break;
                
             case 'x':
                printSector(0,0,128);
                break;
                 case 'c':
                printSector(0,0,129);
                break;
         
            default:
                UART_UartPutString("Unknown :");
                UART_UartPutChar(c);
                UART_UartPutChar('\n');
            break;
            }
            
        }
        UART_ClearRxInterruptSource(UART_INTR_RX_NOT_EMPTY);
        UART_SetRxInterruptMode(UART_INTR_RX_NOT_EMPTY);
        
    }

}

// An example Task
void ledTask(void *arg)
{
    (void)arg;
    while(1)
    {
        RED_Write(~RED_Read());
        vTaskDelay(500);  
    }
}


int main(void)
{
    CyGlobalIntEnable;
    FreeRTOS_Start();
    
    #if ( configUSE_TRACE_FACILITY == 1 )
    vTraceEnable(TRC_START);
    #endif
    
    xTaskCreate(ledTask,"LED Task",configMINIMAL_STACK_SIZE,0,1,0);
    xTaskCreate(uartTask,"UART Task",configMINIMAL_STACK_SIZE*2,0,1,&uartTaskHandle);
    vTaskStartScheduler();  // Will never return
    while(1);               // Eliminate compiler warning
}

