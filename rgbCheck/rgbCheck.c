// RGB PWM Example
// Jason Losh
// Modified by James Revette

// inputs in order are "rgb", "eeprom block", "red val", "green val", "blue val"
// rgb is simply a string with the letters r, g, and b
// eeprom block is an integer, same with red, green, and blue val of the eeporm block your combination is stored to
// red, green, and blue val are set to 0-100 at base but can be modified to 0-1023


//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------
// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

// Hardware configuration:
// Red LED:
//   M1PWM5 (PF1) drives an NPN transistor that powers the red LED
// Green LED:
//   M1PWM7 (PF3) drives an NPN transistor that powers the green LED
// Blue LED:
//   M1PWM6 (PF2) drives an NPN transistor that powers the blue LED

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "clock.h"
#include "wait.h"
#include "tm4c123gh6pm.h"
#include "eeprom.h"
#include "uart0.h"

// PortF masks
#define RED_LED_MASK 2
#define BLUE_LED_MASK 4
#define GREEN_LED_MASK 8

//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

USER_DATA data;


//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Initialize RGB
void initRgb(){
    // Enable clocks
    SYSCTL_RCGCPWM_R |= SYSCTL_RCGCPWM_R1;
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5;
    _delay_cycles(3);

    // Configure three LEDs
    GPIO_PORTF_DEN_R |= RED_LED_MASK | GREEN_LED_MASK | BLUE_LED_MASK;

    GPIO_PORTF_AFSEL_R |= RED_LED_MASK | GREEN_LED_MASK | BLUE_LED_MASK;

    GPIO_PORTF_PCTL_R &= ~(GPIO_PCTL_PF1_M | GPIO_PCTL_PF2_M | GPIO_PCTL_PF3_M);
    GPIO_PORTF_PCTL_R |= GPIO_PCTL_PF1_M1PWM5 | GPIO_PCTL_PF2_M1PWM6 | GPIO_PCTL_PF3_M1PWM7;
    
    //GPIO_PORTA_PCTL_R |= GPIO_PCTL_PA6_M1PWM2 | GPIO_PCTL_PA7_M1PWM3;

    // Configure PWM module 1 to drive RGB LED
    // RED   on M1PWM5 (PF1), M1PWM2b
    // BLUE  on M1PWM6 (PF2), M1PWM3a
    // GREEN on M1PWM7 (PF3), M1PWM3b
    SYSCTL_SRPWM_R = SYSCTL_SRPWM_R1;                // reset PWM1 module
    SYSCTL_SRPWM_R = 0;                              // leave reset state
    PWM1_2_CTL_R = 0;                                // turn-off PWM1 generator 2 (drives outs 4 and 5)
    PWM1_3_CTL_R = 0;                                // turn-off PWM1 generator 3 (drives outs 6 and 7)
    PWM1_2_GENB_R = PWM_1_GENB_ACTCMPBD_ONE | PWM_1_GENB_ACTLOAD_ZERO;
                                                     // output 5 on PWM1, gen 2b, cmpb
    PWM1_3_GENA_R = PWM_1_GENA_ACTCMPAD_ONE | PWM_1_GENA_ACTLOAD_ZERO;
                                                     // output 6 on PWM1, gen 3a, cmpa
    PWM1_3_GENB_R = PWM_1_GENB_ACTCMPBD_ONE | PWM_1_GENB_ACTLOAD_ZERO;
                                                     // output 7 on PWM1, gen 3b, cmpb

    PWM1_2_LOAD_R = 1024;                            // set frequency to 40 MHz sys clock / 2 / 1024 = 19.53125 kHz
    PWM1_3_LOAD_R = 1024;                            // (internal counter counts down from load value to zero)

    PWM1_2_CMPB_R = 0;                               // red off (0=always low, 1023=always high)
    PWM1_3_CMPB_R = 0;                               // green off
    PWM1_3_CMPA_R = 0;                               // blue off

    PWM1_2_CTL_R = PWM_1_CTL_ENABLE;                 // turn-on PWM1 generator 2
    PWM1_3_CTL_R = PWM_1_CTL_ENABLE;                 // turn-on PWM1 generator 3
    PWM1_ENABLE_R = PWM_ENABLE_PWM5EN | PWM_ENABLE_PWM6EN | PWM_ENABLE_PWM7EN;
                                                     // enable outputs
}


void setRgbColor(uint16_t red, uint16_t green, uint16_t blue){

    // divide pwm input by 100
    // multiply by load register
    // create intermediary variable
    // then set that equal to the compare register

    // If you want to run the code with 0-100, leave as is
    // If you want to run the code with 0-1023, comment out lines 112-126
    // then uncomment 129-131

    float redFloat = 0;
    float greenFloat = 0;
    float blueFloat = 0;

    redFloat = (float)red / 100;
    greenFloat = (float)green / 100;
    blueFloat = (float)blue / 100;

    redFloat = redFloat * 1023;
    greenFloat = greenFloat * 1023;
    blueFloat = blueFloat * 1023;

    PWM1_2_CMPB_R = redFloat;
    PWM1_3_CMPA_R = blueFloat;
    PWM1_3_CMPB_R = greenFloat;

    // PWM1_2_CMPB_R = red;
    // PWM1_3_CMPA_R = blue;
    // PWM1_3_CMPB_R = green;

    // Do not have lines 113-131 all uncommented at the same time.
    // If you do, there may be some wonky unexpected results.
    // Try it if you feel like it, it won't explode your board or anything.
    // but... it does explode, I am not at fault for any damages.
}



// Initialize Hardware
void initHw(){
    // Initialize system clock to 40 MHz
    initSystemClockTo40Mhz();
    initEeprom();
    initUart0();
}

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

int main(void){
	// Initialize hardware
    initHw();
	initRgb();
	setUart0BaudRate(115200, 40e6);
    int blockNum = 0;
    uint16_t redVal = 0;
    uint16_t greenVal = 0;
    uint16_t blueVal = 0;
    char buffer[40];

    // Cycle through colors
	while(true)
	{
        getsUart0(&data);
        parseFields(&data);

        if(isCommand(&data, "rgb", 4)){
            // "rgb", "eeprom block", "red val", "green val", "blue val"
            blockNum = getFieldInteger(&data, 1);
            redVal = getFieldInteger(&data, 2);
            greenVal = getFieldInteger(&data, 3);
            blueVal = getFieldInteger(&data, 4);

            writeEeprom(16*blockNum+0, redVal);
            writeEeprom(16*blockNum+1, greenVal);
            writeEeprom(16*blockNum+2, blueVal);
            putcUart0('\n');
        }
    
        else if(isCommand(&data, "rgb", 1)){
            blockNum = getFieldInteger(&data, 1);
            redVal =  readEeprom(16 * blockNum + 0);
            greenVal =       readEeprom(16 * blockNum + 1);
            blueVal =   readEeprom(16 * blockNum + 2);

            setRgbColor(redVal, greenVal, blueVal); 
        
            snprintf (buffer, sizeof(buffer), "event: %d, redVal: %d\n", blockNum, redVal);
            putsUart0(buffer);
            snprintf (buffer, sizeof(buffer), "event: %d, greenVal: %d\n", blockNum, greenVal);
            putsUart0(buffer);
            snprintf (buffer, sizeof(buffer), "event: %d, blueVal: %d\n", blockNum, blueVal);
            putsUart0(buffer);
            putcUart0('\n');
        }
    }
}
