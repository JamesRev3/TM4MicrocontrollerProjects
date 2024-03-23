// UART0 Library
// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL
// Target uC:       TM4C123GH6PM
// System Clock:    -

// Hardware configuration:
// UART Interface:
//   U0TX (PA1) and U0RX (PA0) are connected to the 2nd controller
//   The USB on the 2nd controller enumerates to an ICDI interface and a virtual COM port

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "uart0.h"

// PortA masks
#define UART_TX_MASK 2
#define UART_RX_MASK 1

//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Initialize UART0
void initUart0(){
    // Enable clocks
    SYSCTL_RCGCUART_R |= SYSCTL_RCGCUART_R0;
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R0;
    _delay_cycles(3);

    // Configure UART0 pins
    GPIO_PORTA_DR2R_R |= UART_TX_MASK;                  // set drive strength to 2mA (not needed since default configuration -- for clarity)
    GPIO_PORTA_DEN_R  |= UART_TX_MASK  | UART_RX_MASK;    // enable digital on UART0 pins
    GPIO_PORTA_AFSEL_R |= UART_TX_MASK | UART_RX_MASK;  // use peripheral to drive PA0, PA1
    GPIO_PORTA_PCTL_R &= ~(GPIO_PCTL_PA1_M  | GPIO_PCTL_PA0_M); // clear bits 0-7
    GPIO_PORTA_PCTL_R |= GPIO_PCTL_PA1_U0TX | GPIO_PCTL_PA0_U0RX;
                                                        // select UART0 to drive pins PA0 and PA1: default, added for clarity

    // Configure UART0 to 115200 baud (assuming fcyc = 40 MHz), 8N1 format
    UART0_CTL_R = 0;                                    // turn-off UART0 to allow safe programming
    UART0_CC_R = UART_CC_CS_SYSCLK;                     // use system clock (40 MHz)
    UART0_IBRD_R = 21;                                  // r = 40 MHz / (Nx115.2kHz), set floor(r)=21, where N=16
    //IBRD means integer baud rate
    UART0_FBRD_R = 45;                                  // round(fract(r)*64)=45
    //FBRD means fractional baud rate
    UART0_LCRH_R = UART_LCRH_WLEN_8/*0x00000060*/ | UART_LCRH_FEN/*0x0000010*/;    // configure for 8N1 w/ 16-level FIFO
    UART0_CTL_R = UART_CTL_TXE | UART_CTL_RXE | UART_CTL_UARTEN;
                                                        // enable TX, RX, and module
}

void parseFields(USER_DATA *data){
    bool isDelimiter = true;
    data->fieldCount = 0;
    int i = 0;
    for(i = 0; *((data->bufferString)+i) != '\0'; i++){
        // if incoming data is between set values then set it to num, alp, or delim
            if(data->bufferString[i] >='0' &&  data->bufferString[i] <= '9'){
                //set to a delimiter first
                data->fieldType[data->fieldCount] = 'N';
                // changing the field type to a letter or number because that's where we find it
                data->fieldPosition[data->fieldCount] = i;
                if(isDelimiter == true){
                    data->fieldCount++;}
                isDelimiter = false;}
                // another if for if it's a delimiter to stop it from adding to fieldcount everytime
            else if((data->bufferString[i] >= 'A' && data->bufferString[i] <= 'Z') 
            || (data->bufferString[i] >= 'a' && data->bufferString[i] <= 'z')){
                data->fieldType[data->fieldCount] = 'A';
                data->fieldPosition[data->fieldCount] = i;
                if(isDelimiter == true){
                    data->fieldCount++;}
                isDelimiter = false;
            }
            else if(data->bufferString[i] == 8 || data->bufferString[i] == 127){
                isDelimiter = false;
            }
            else{
                *(data->bufferString+i) = '\0'; 
                isDelimiter = true;}

        if(data->fieldCount == MAX_FIELDS){
            return;
        }
    }
}

char * getFieldString(USER_DATA* data, uint8_t fieldNumber){
// returning a string assuming it is a string we can return, else return null
     // int i = 0;
     // for(i = 0; *((data->bufferString)+i) != '\0'; i++ ){
        if(data->fieldType[fieldNumber] != 'A'){
            return "N";
            // return &data->bufferString[data->fieldPosition[fieldNumber]];
        }else{
            return &data->bufferString[data->fieldPosition[fieldNumber]];
        }
    // }
}

int32_t getFieldInteger(USER_DATA* data, uint8_t fieldNumber){
// returning a number assuming it is a number we can return, else return null

    int i = 0;
    if(fieldNumber < data->fieldCount && data->fieldType[fieldNumber] == 'N'){
        int count = 0;
        // integer to count up our for loop
        char* stringNum = &data->bufferString[data->fieldPosition[fieldNumber]];
        // character to go through our string and check for a number
        for(count = 0; stringNum[count] != 0; count++){
            // first get field number from field type
            i = i *10 +((stringNum[count])- 48);
            // parsing through the string to find our variable
            // then subtracting 48 to get the integer value from ascii value
            // multiplying i in order to shift it so we can continually read right to left
            //return i;
        }
    }
    return i;
} 

bool isCommand(USER_DATA* data, const char strCommand[], uint8_t minArguments){
    // checking if the string we pass in is the same as the one in parseFields
    // one string with two substring, bufferString is the whole string
    // isCommand is looking for the 0th sub string
    bool valid = false;
    uint32_t i;

    if(data->fieldCount - 1 >= minArguments){
        valid = true;
        for(i = 0; data->bufferString[data->fieldPosition[0] + i] != 0; i++){
            if(strCommand[i] != data->bufferString[data->fieldPosition[0] + i]){
                valid = false;
            }
        }
    }
    return valid;
}

void getsUart0(USER_DATA *data){
    //char character;
    int count = 0;
    char character;
    while(true){
       character = getcUart0();

// If the character is a backspace (ASCII code 8 or 127) and count > 0, then 
// decrement the count so that the last character will be overwritten

        if (( character == 8 || character == 127) && count > 0){ // 8 is backspace in ascii
            count--;
        }

// If the character is a carriage return (ASCII code 13) (this is sent when you
// type <enter_key> indicating the end of the string), then add a null
// terminator (ASCII code 0) to the end of the string and return from the
// function.

        else if (character == 13){ // 13 is carriage return in ascii
            data->bufferString[count] = '\0';
            return;
        }
// If the character is a space (ASCII code 32) or another printable character
// (ASCII code >= 32), then store the character in
// data->buffer[count]. Increment the count.

        else if (character >= 32 ){ // 32 is space in ascii
            data->bufferString[count] = character;
            count++;   
        }

// If count == maxChars (no more room in the str[])
// then add a null terminator to the end of the string and
// return from the function.
        if(count == MAX_CHARS){
              data->bufferString[count] = '\0';
              return;
            }             
        }
       return;
}

// Set baud rate as function of instruction cycle frequency
 void setUart0BaudRate(uint32_t baudRate, uint32_t fcyc){
     uint32_t divisorTimes128 = (fcyc * 8) / baudRate;   // calculate divisor (r) in units of 1/128,
                                                         // where r = fcyc / 16 * baudRate
     divisorTimes128 += 1;                               // add 1/128 to allow rounding
     UART0_CTL_R = 0;                                    // turn-off UART0 to allow safe programming
     UART0_IBRD_R = divisorTimes128 >> 7;                // set integer value to floor(r)
     UART0_FBRD_R = ((divisorTimes128) >> 1) & 63;       // set fractional value to round(fract(r)*64)
     UART0_LCRH_R = UART_LCRH_WLEN_8 | UART_LCRH_FEN;    // configure for 8N1 w/ 16-level FIFO
     UART0_CTL_R = UART_CTL_TXE | UART_CTL_RXE | UART_CTL_UARTEN;
                                                         // turn-on UART0
 }

// Blocking function that writes a serial character when the UART buffer is not full
void putcUart0(char c){
    while (UART0_FR_R & UART_FR_TXFF);               // wait if uart0 tx fifo full
    UART0_DR_R = c;                                  // write character to fifo
}

// Blocking function that writes a string when the UART buffer is not full
void putsUart0(char* str){
    uint8_t i = 0;
    while (str[i] != '\0')
        putcUart0(str[i++]);
}

// Blocking function that returns with serial data once the buffer is not empty
char getcUart0(){
    while (UART0_FR_R & UART_FR_RXFE);               // wait if uart0 rx fifo empty
    return UART0_DR_R & 0xFF;                        // get character from fifo
}

// Returns the status of the receive buffer
bool kbhitUart0(){
    return !(UART0_FR_R & UART_FR_RXFE);
}
