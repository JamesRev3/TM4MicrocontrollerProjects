#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "clock.h"
#include "uart0.h"
#include "tm4c123gh6pm.h"
#include "wait.h"

//-----------------------------
// Global Variables
//-----------------------------


//-----------------------------
// Defines
//-----------------------------


// Port C defines, buncha stuff
#define PORT_C_DATA_R 0x400063FC
#define speaker (*((volatile uint32_t *)(0x42000000 + (PORT_C_DATA_R-0x40000000)*32 + 4*4)))
#define motion  (*((volatile uint32_t *)(0x42000000 + (PORT_C_DATA_R-0x40000000)*32 + 5*4)))

// led defines
#define PORT_F_DATA_R 0x400253FC
#define blue  (*((volatile uint32_t *)(0x42000000 + (PORT_F_DATA_R-0x40000000)*32 + 2*4)))
#define green (*((volatile uint32_t *)(0x42000000 + (PORT_F_DATA_R-0x40000000)*32 + 3*4)))

// mask defines
#define GREEN_LED_MASK    0b00001000
#define BLUE_LED_MASK     0b00000100


#define SPEAKER_MASK      0b00010000
#define MOTION_MASK       0b00100000


//-----------------------------
// InitHw
//-----------------------------

void gpioConfig(){

    GPIO_PORTC_IM_R |= MOTION_MASK;
    GPIO_PORTC_IBE_R |= MOTION_MASK;
    
    NVIC_EN0_R = 1 << (INT_GPIOC-16);
}

// Initialize Hardware
void initHw(){
    // Initialize system clock to 40 MHz
    initSystemClockTo40Mhz();

    // Enable clocks
    // 0=A, 1=B, 2=C, 3=D, 4=E, 5=F1

    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R2;
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5;
    _delay_cycles(3);

    initUart0();
    gpioConfig();

    GPIO_PORTC_DIR_R &= ~(MOTION_MASK);
    GPIO_PORTC_DIR_R |= SPEAKER_MASK;
    GPIO_PORTF_DIR_R |= BLUE_LED_MASK | GREEN_LED_MASK;

    GPIO_PORTC_DEN_R |= SPEAKER_MASK | MOTION_MASK;
    GPIO_PORTF_DEN_R |= BLUE_LED_MASK | GREEN_LED_MASK;

}


//-----------------------------
// Isr Functions
//-----------------------------

// state machine is an interrupt that goes into another interrupt and then goes back to the first
// is a sequence of n interrupts that can go back and forth between interrupts depending on conditions

// turn on a timer that triggers a second interrupt
// second is

void playSpeaker(){
    // to get time on and off
    // t = 1/2730
    // then play whatever t is
    // turn on speaker for half of of t
    // 2730  KHz = f
    // number is on the list
    // 2730 is the number
    // waitMicrosecond(500000);
    int i = 0;
    for(i = 0; i < 2000; i++){
        waitMicrosecond(185);
        speaker = 1;
        waitMicrosecond(185);
        speaker = 0;
    }
}
// isr is what happens when the interrupt is triggered
void motionIsr(){

    if(motion == 1){
        blue = 1;
        //playSpeaker();
    }
    else{
        blue = 0;
    }
    //playSpeaker();

//    waitMicrosecond(185);
//    speaker = 1;
//    waitMicrosecond(185);
//    speaker = 0;
    GPIO_PORTC_ICR_R |= MOTION_MASK;
}

void motionOffIsr(){
// maybe one day...
}

void detectMotion(){
    if(motion == 1){
        putsUart0("motion detected\n");
        blue = 1;
    }
    else{
        blue = 0;
    }
}

int main(){
    initHw();
	setUart0BaudRate(115200, 40e6);
    
    while(true){
        //detectMotion();
    }
}
