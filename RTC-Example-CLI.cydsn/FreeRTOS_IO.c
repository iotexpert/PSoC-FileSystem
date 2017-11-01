#include <project.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "FreeRTOS_IO.h"

SemaphoreHandle_t uartSemaphoreHandle;


size_t FreeRTOS_write( Peripheral_Descriptor_t const pxPeripheral, 
                       const void *pvBuffer, 
                       const size_t xBytes )
{
    
    (void) pxPeripheral; // ARH this is a bad bad idea
    
    for(unsigned int i=0;i<xBytes;i++)
        UART_UartPutChar(((char *)pvBuffer)[i]);
    
    return xBytes;
}

size_t FreeRTOS_read( Peripheral_Descriptor_t const pxPeripheral, 
                      void * const pvBuffer, 
                      const size_t xBytes )
{
    
    (void) pxPeripheral; // ARH this is a bad bad idea
    
    for(unsigned int i=0;i<xBytes;i++)
    {
        xSemaphoreTake(uartSemaphoreHandle,portMAX_DELAY);
        ((char *)pvBuffer)[i] = UART_UartGetChar();
        UART_ClearRxInterruptSource(UART_INTR_RX_NOT_EMPTY); 
		UART_SetRxInterruptMode(UART_INTR_RX_NOT_EMPTY);
    }
    return xBytes;
}


void uartISR()
{
    BaseType_t xHigherPriorityTaskWoken;
    // disable the interrupt
    UART_SetRxInterruptMode(0);
    xSemaphoreGiveFromISR(uartSemaphoreHandle,&xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR( xHigherPriorityTaskWoken ); 

}

Peripheral_Descriptor_t FreeRTOS_open( const int8_t *pcPath, const uint32_t ulFlags )
{
    (void)pcPath;
    (void)ulFlags;
    UART_Start();
    UART_SetCustomInterruptHandler(uartISR);
    
    UART_ClearRxInterruptSource(UART_INTR_RX_NOT_EMPTY); 
    UART_SetRxInterruptMode(UART_INTR_RX_NOT_EMPTY);

    uartSemaphoreHandle =   xSemaphoreCreateBinary(  );
    return 1;    
}
