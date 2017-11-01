#include "project.h"
#include <stdio.h>
#include "FreeRTOS.h"
#include "FreeRTOS_CLI.h" 
#include "FreeRTOS_IO.h"
#include "timers.h"

// Send VT100 Clear Screen and Home Escape Codes
#define VT100_CLEARSCREEN "\033[2J\033[H" 
inline void clearScreen()
{
    FreeRTOS_write(0,VT100_CLEARSCREEN,strlen(VT100_CLEARSCREEN));
}

#define MAX_INPUT_LENGTH    50
//#define MAX_OUTPUT_LENGTH   configCOMMAND_INT_MAX_OUTPUT_SIZE
#define MAX_OUTPUT_LENGTH   128

/*****************************************************************************\
 * Function:    clearCommand
 * Input:       char *pcWriteBufer,size_t xWriteBufferLen,const char *pcCommandString
 * Returns:     BaseType_t
 * Description: 
 *     This function clears the screen. It is run by the CLI interpreter
\*****************************************************************************/

BaseType_t clearCommand( char *pcWriteBuffer,size_t xWriteBufferLen, const char *pcCommandString )
{
    (void)pcCommandString;
    static int processed = 0;
    
    char *clearScreen = VT100_CLEARSCREEN;
    // Only allowed to write up top xWriteBufferLen bytes ... 
    strncpy(pcWriteBuffer,&clearScreen[processed],xWriteBufferLen-1);
    pcWriteBuffer[xWriteBufferLen-1]=0;

    processed = processed + xWriteBufferLen-1;
    if(processed < (int)strlen(clearScreen))
        return pdTRUE;
    
    processed = 0;
    return pdFALSE;
}
		
    static const CLI_Command_Definition_t clearCommandStruct =
    {
        "clear",
        "clear: Clear Screen by sending VT100 Escape Code\n",
        clearCommand,
        0
    };

/*****************************************************************************\
 * Function:    timeCommand
 * Input:       char *pcWriteBufer,size_t xWriteBufferLen,const char *pcCommandString
 * Returns:     BaseType_t
 * Description: 
 *     This function gets the RTC Time and returns. It is run by the CLI interpreter
\*****************************************************************************/
BaseType_t timeCommand(char *pcWriteBuffer,size_t xWriteBufferLen,const char *pcCommandString )
{
    (void)xWriteBufferLen;
    (void)pcCommandString;
    uint32 time = RTC_GetTime();
    
    uint32_t hours = RTC_GetHours(time);
    uint32_t minutes = RTC_GetMinutes(time);
    uint32_t seconds = RTC_GetSecond(time);
    uint32_t ampm = RTC_GetAmPm(time);
    sprintf(pcWriteBuffer,"%02u:%02u:%02u %s\n",(unsigned int)hours,(unsigned int)minutes,(unsigned int)seconds,(ampm == RTC_AM)?"AM":"PM");
        
    return pdFALSE;
}
		
static const CLI_Command_Definition_t timeCommandStruct =
{
    "time",
	"time: Print out current time\n",
	timeCommand,
	0 // No arguments
};
	
/*****************************************************************************\
 * Function:    setTimeCommand
 * Input:       char *pcWriteBufer,size_t xWriteBufferLen,const char *pcCommandString
 * Returns:     BaseType_t
 * Description: 
 *     This function process the 2nd argument in the CLI then
 *     calls the RTC set time function. It is run by the CLI interpreter
 *     The argument must be of the form of hh:mm (2 numbers, colon, 2 numbers
\*****************************************************************************/
BaseType_t setTimeCommand( char *pcWriteBuffer, size_t xWriteBufferLen,const char *pcCommandString )
{
    (void)xWriteBufferLen;
    (void)pcCommandString;
    BaseType_t numChar;
    const char *numbers= FreeRTOS_CLIGetParameter(pcCommandString,1,&numChar);
    
    // This block of code error checks the input argument
    if(numChar != 5) 
    {
        strcpy(pcWriteBuffer,"setTime: wrong number of chars setTime hh:mm\n");
        return pdFALSE;
    }
    if(numbers[2] != ':' || (!isdigit(numbers[0])) || (!isdigit(numbers[1]) || (!isdigit(numbers[3])) || (!isdigit( numbers[4]))))
    {
        strcpy(pcWriteBuffer,"setTime: missing : or not digits setTime hh:mm\n");
        return pdFALSE;
    }
    
    int hour, minute;
    sscanf(numbers,"%2d",&hour); // Yes I know it is cheap... but it was also easy
    sscanf(&numbers[3],"%2d",&minute);
    
    if(hour>23 || minute > 59)
    {
        strcpy(pcWriteBuffer,"setTime: hours>23 or minutes>59 hh:mm\n");
        return pdFALSE;
    }
   
    // Build up a legal time string to send to the API
    RTC_SetDateAndTime(RTC_ConstructTime( RTC_24_HOURS_FORMAT, 0 ,hour,minute,0), RTC_GetDate());
    // Format the output string 
    sprintf((char *)pcWriteBuffer,"Time =%02d:%02d\n",hour,minute); 
    
    return pdFALSE;
}
		
    static const CLI_Command_Definition_t setTimeCommandStruct =
    {
        "setTime",
        "setTime: setTime hh:mm\n",
        setTimeCommand,
        1
    };
    
/*****************************************************************************\
 * Function:    cliTask
 * Input:       void *arg  ... unused
 * Returns:     void
 * Description: 
 *     This function is the inifite loop for the command line intepreter.. it
 *     reads characters using the FreeRTOS_read function then sends them to the
 *     cli when there is a \r 
\*****************************************************************************/
void cliTask(void *arg)
{
    (void)arg;

    char pcOutputString[ MAX_OUTPUT_LENGTH ], pcInputString[ MAX_INPUT_LENGTH ];
    int8_t cRxedChar, cInputIndex = 0;
    BaseType_t xMoreDataToFollow;    

    FreeRTOS_open( (const int8_t *)"/uart",0 );
    clearScreen();
    #define INTRO_STRING "Command Line & RTC Demo\n"
    FreeRTOS_write(0,INTRO_STRING,strlen(INTRO_STRING));

    RTC_Start();

    FreeRTOS_CLIRegisterCommand( &clearCommandStruct );	
    FreeRTOS_CLIRegisterCommand( &setTimeCommandStruct );	
    FreeRTOS_CLIRegisterCommand( &timeCommandStruct );	
    while(1)
    {
        FreeRTOS_read( 0, &cRxedChar, sizeof( cRxedChar ) );
        if( cRxedChar == '\r' )
        {
            /* A newline character was received, so the input command string is
            complete and can be processed.  Transmit a line separator, just to
            make the output easier to read. */

            FreeRTOS_write(0,&cRxedChar,1);

            /* The command interpreter is called repeatedly until it returns
            pdFALSE.  See the "Implementing a command" documentation for an
            exaplanation of why this is. */
            do
            {
                /* Send the command string to the command interpreter.  Any
                output generated by the command interpreter will be placed in the
                pcOutputString buffer. */
                
                xMoreDataToFollow = FreeRTOS_CLIProcessCommand
                              (
                                  pcInputString,   /* The command string.*/
                                  pcOutputString,  /* The output buffer. */
                                  MAX_OUTPUT_LENGTH/* The size of the output buffer. */
                              );

                /* Write the output generated by the command interpreter to the
                console. */
                    
                FreeRTOS_write( 0, pcOutputString, strlen( pcOutputString ) );

            } while( xMoreDataToFollow != pdFALSE );

            /* All the strings generated by the input command have been sent.
            Processing of the command is complete.  Clear the input string ready
            to receive the next command. */
            cInputIndex = 0;
            memset( pcInputString, 0x00, MAX_INPUT_LENGTH );
        }
        else
        {
            /* The if() clause performs the processing after a newline character
            is received.  This else clause performs the processing if any other
            character is received. */

            if( cRxedChar == 127 ) // delete character
            {
                FreeRTOS_write(0,&cRxedChar,1);
                /* Backspace was pressed.  Erase the last character in the input
                buffer - if there are any. */
                if( cInputIndex > 0 )
                {
                    cInputIndex--;
                    pcInputString[ cInputIndex ] = '\0';
                }
            }
            else
            {
                /* A character was entered.  It was not a new line, backspace
                or carriage return, so it is accepted as part of the input and
                placed into the input buffer.  When a \n is entered the complete
                string will be passed to the command interpreter. */
                if( cInputIndex < MAX_INPUT_LENGTH )
                {
                    FreeRTOS_write(0,&cRxedChar,1);
                    pcInputString[ cInputIndex ] = cRxedChar;
                    cInputIndex++;
                }
            }
        }
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
    xTaskCreate(cliTask,"CLI Task",configMINIMAL_STACK_SIZE*3,0,1,0);
    vTaskStartScheduler();  // Will never return
    while(1);               // Eliminate compiler warning
}

