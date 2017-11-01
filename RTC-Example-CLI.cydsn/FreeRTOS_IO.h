#ifndef FREERTOS_IO_H
#define FREERTOS_IO_H
#include "FreeRTOS.h"
typedef BaseType_t Peripheral_Descriptor_t;
    
    Peripheral_Descriptor_t FreeRTOS_open( const int8_t *pcPath, const uint32_t ulFlags );

    size_t FreeRTOS_read( Peripheral_Descriptor_t const pxPeripheral, 
                      void * const pvBuffer, 
                      const size_t xBytes );
    size_t FreeRTOS_write( Peripheral_Descriptor_t const pxPeripheral, 
                       const void *pvBuffer, 
                       const size_t xBytes );
    
    BaseType_t FreeRTOS_ioctl( Peripheral_Descriptor_t const xPeripheral, 
                              uint32_t ulRequest, 
                              void *pvValue );

#endif
    
