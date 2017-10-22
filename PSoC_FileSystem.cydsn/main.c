
#include "project.h"
#include "FreeRTOS.h"
#include "timers.h"
#include <stdio.h>

#include "extestfs.h"


// This function erases - write 0's into the FRAM... also known as NUKE the FRAM
void blankFRAM(void)
{
    int i;
    UART_UartPutString("Starting Erase 1st Block\n");
    I2C_I2CMasterSendStart(0x50,I2C_I2C_WRITE_XFER_MODE);
    I2C_I2CMasterWriteByte(0);
    I2C_I2CMasterWriteByte(0);
    for(i=0;i<0xFFFF;i++) // Write 64K of 0's to erase first bank
    {
        I2C_I2CMasterWriteByte(0);
    }
    I2C_I2CMasterSendStop();
    UART_UartPutString("Starting Erase 2nd Block\n");
    I2C_I2CMasterSendStart(0x51,I2C_I2C_WRITE_XFER_MODE);
    I2C_I2CMasterWriteByte(0);
    I2C_I2CMasterWriteByte(0);
    for(i=0;i<0xFFFF;i++) // write 64k of 0's to erase 2nd bank
    {
        I2C_I2CMasterWriteByte(0);
    }
    I2C_I2CMasterSendStop();
    
    UART_UartPutString("Erase Complete\n");

}
						
// Send VT100 Clear Screen and Home Escape Codes
inline void clearScreen()
{
    UART_UartPutString("\033[2J\033[H");
}

TaskHandle_t uartTaskHandle;
void uartISR()
{
    
    BaseType_t xHigherPriorityTaskWoken;
    // disable the interrupt
    UART_SetRxInterruptMode(0);
    vTaskNotifyGiveFromISR(uartTaskHandle,&xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
    
}

// This is the main task which processes commands from the UART and prints the results
// on the screen.
void uartTask(void *arg)
{
    (void)arg;
    UART_Start();
    I2C_Start();
    clearScreen();
    UART_UartPutString("Start Filesystem Demo\n");
    UART_SetCustomInterruptHandler(uartISR);
    
    int sectorCount=0;    
    while(1)
    {
        ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
        
        while(UART_SpiUartGetRxBufferSize()) // if there is data then read and process
        {
            char c;
            
            c= UART_UartGetChar();
			switch(c)
			{
				case 'i': // Initialize the RTOS
					exInit();
				break;
				case 'f': // Format the FRAM
					exFormat();
				break;
                    
				case 'q': // Return the drive information
					exDriveInfo();
				break;
			
                case 'w': // Create a file
				    exCreateFile();
				break;
					
				case 'r': // Read the file that was created (if it exists)
				    exReadFile();
				break;
					
				case '0': // Goto to sector 0 and print the data
				    sectorCount = 0;
					exPrintSector(0,0,0);
				break;
				
			    case '+': // Print the "current" sector and go to the next
				    exPrintSector(0,0,sectorCount);
				    sectorCount += 1;
			    break;
					
			    case '-': // print the current sector and go to the previous
				    exPrintSector(0,0,sectorCount);
				    sectorCount -= 1;
				    if(sectorCount<0)
					    sectorCount =0;
			    break;
                    
    			case 'd': // Do a directory
	    			exDirectory();
		    	break;
                    
                case 'c':
                    clearScreen();
                break;
                    
                case 'b': // Erase the FRAM (write all 0's)
                    blankFRAM();
                break;
			
                case '?': // Print out the list of commands
                    UART_UartPutString("b - Blank FRAM - write all 0's\n");
                    UART_UartPutString("i - Initalize Filesystem\n");
                    UART_UartPutString("f - Format FRAM\n");
                    UART_UartPutString("q - Print FileSystem Information\n");
                    UART_UartPutString("w - Create a file called \"afile.bin\"\n");
                    UART_UartPutString("r - Read the file called \"afile.bin\" and print contents\n");
                    UART_UartPutString("0 - Goto sector 0 and print contents\n");
                    UART_UartPutString("+ - Goto next sector and print contents\n");
                    UART_UartPutString("- - Goto previous sector and print contents\n");
                    UART_UartPutString("d - Print Directory\n");
                    UART_UartPutString("c - Clear Screen\n");
                    UART_UartPutString("? - Print help\n");        
                    
                break;
                    
			    default:
				    UART_UartPutString("Unknown :");
				    UART_UartPutChar(c);
				    UART_UartPutChar('\n');
			    break;
			}
        }
        // Turn the interrupts back on
        UART_ClearRxInterruptSource(UART_INTR_RX_NOT_EMPTY); 
        UART_SetRxInterruptMode(UART_INTR_RX_NOT_EMPTY);
    }
}

// The Blinking LED ... just to know we are alive
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

