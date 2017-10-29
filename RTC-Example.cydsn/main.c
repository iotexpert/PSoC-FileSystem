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
#include <stdio.h>
#include "FreeRTOS.h"
#include "timers.h"
TaskHandle_t uartTaskHandle;
void uartISR()
{
    
    BaseType_t xHigherPriorityTaskWoken;
    // disable the interrupt
    UART_SetRxInterruptMode(0);
    vTaskNotifyGiveFromISR(uartTaskHandle,&xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
    
}

// Send VT100 Clear Screen and Home Escape Codes
inline void clearScreen()
{
    UART_UartPutString("\033[2J\033[H");
}

// This is the main task which processes commands from the UART and prints the results
// on the screen.
void uartTask(void *arg)
{
    (void)arg;
    UART_Start();
    clearScreen();
    UART_UartPutString("Start Real Time Clock Demo\n");
    UART_SetCustomInterruptHandler(uartISR);
    uint32 timeBCD;
    char buff[32];
    
    RTC_Start();

    
    while(1)
    {
        ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
        
        while(UART_SpiUartGetRxBufferSize()) // if there is data then read and process
        {
            char c;
            
            c= UART_UartGetChar();
			switch(c)
			{
                case 't':
                    
                    timeBCD = RTC_GetTime();
                    sprintf(buff,"%d:%d:%d\n",(int)RTC_GetHours(timeBCD),(int)RTC_GetMinutes(timeBCD),(int)RTC_GetSecond(timeBCD));
                    UART_UartPutString(buff);
                    
                break;
                
			
                case '?': // Print out the list of commands
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

