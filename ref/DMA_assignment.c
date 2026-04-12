#include "main.h"

// HARDWARE SETUP:
//================
//  Connect PA0 to 100ohm resistor then to the buzzer
//                       100ohm
//              [PA0]__/\/\/\/\/\_________
//                                        |
//        < M C U >                      _|_
//         =======                      |   |
//                                      |___|  8ohm buzzer
//                                        |
//                                      __|__
//                                       ___ 
//                                        -  
// ** NOTE: 
//      Use a small resistor to limit current. 100 Ohm is ideal! Ask for one of try connecting two 330ohm resistors from the resistor array
// you have.
//
/////////////

uint16_t melody[]={659,494,523,587,523,494,440,440,523,659,587,523,494,523,587,659,523,440,440,440,494,523,587,698,880,784,698,659,523,659,587,523,494,494,523,587,659,523,440,440,0	,659,494,523,587,523,494,440,440,523,659,587,523,494,523,587,659,523,440,440,440,494,523,587,698,880,784,698,659,523,659,587,523,494,494,523,587,659,523,440,440,0,659,523,587,494,523,440,415,494,0,659,523,587,494,523,659,880,831,0};
uint16_t duration[]={416,208,208,416,208,208,416,208,208,416,208,208,624,208,416,416,416,416,208,416,208,208,624,208,416,208,208,624,208,416,208,208,416,208,208,416,416,416,416,416,416,416,208,208,416,208,208,416,208,208,416,208,208,624,208,416,416,416,416,208,416,208,208,624,208,416,208,208,624,208,416,208,208,416,208,208,416,416,416,416,416,416,833,833,833,833,833,833,833,416,208,833,833,833,833,416,416,833,833,0};
num_notes=99;

/// ----  functions are declared here ---- ///
void setClks();
void melody_timer_enable();
void duration_timer_enable();
void DMA1_init(void);
void DMA2_init(void);



void main(){
	setClks();
    melody_timer_enable();
    duration_timer_enable();
    DMA1_init(); // DMA to setup the melody
    DMA2_init(); // DMA to setup the duration

    // DMA1 and DMA2 are configured and enabled
    TIM2->CR1 |=1;  // Enable timer 2 that will be used to generate the melody by adjusting the prescaler
    TIM3->CR1 |=1;  // Enable timer 3 that will be used to keep the melody running for certain durations
    while(1){}
}


void DMA1_init(void) {
	// Configure DMA1 to use memory to peripheral mode.
	// The data need to be moved from "melody" array to the prescalar of TIM2
	// Data size is 16-bit, DMA should operate in a circular mode.
	// Configure DMAMUX1_Channel0 so requests for DMA are routed from TIM3_UP

    // *** PLEASE UPDATE ***
}

void DMA2_init(void) {
	// Configure DMA2 to use memory to peripheral mode.
	// The data need to be moved from "duration" array to the ARR of TIM3
	// Data size is 16-bit, DMA should operate in a circular mode.
	// Configure DMAMUX1_Channel8 so requests for DMA are routed from TIM3_UP

    // *** PLEASE UPDATE ***
}





void melody_timer_enable(){
	RCC->AHB2ENR |=1; //Enable GPIOA
	GPIOA->MODER  &= ~(0x3); // Clear the two bits for PA0
	GPIOA->MODER  |=  (0x2); // Set the mode to AF (10--> 0x2)
	// Set the AF=8
	GPIOA->AFR[0] &= ~(0xF); // Clear the 4 bits for PA0
    GPIOA->AFR[0] |=  (0x1); // Set the 4 bits to (AF1) to connect to TIM2_CH1

	// Use Case 3: PWM mode 1 - Center Aligned
	RCC->APB1ENR1 |= 1;     // enable TIM2 clock
	TIM2->ARR   = 10 - 1;   // divided by 10
	TIM2->CCMR1 = 0x60;     // PWM, High if CNT<CCR1 if count down
							//          or if CNT>CCR1 if count up
	TIM2->CCR1  = 5;        // set match value (always 50% duty cycle)
	TIM2->CCER |= 1;        // enable CH1 compare mode
	TIM2->CNT   = 0;        // clear counter
	TIM2->CR1   = 0x60;     // enable TIM2 , Center-aligned
    // NOT ENABLED YET
   // TIM2->PSC   = 0;        //      <----- Changing this will change the melody tone
}
void duration_timer_enable(){
	RCC->APB1ENR1 |= 2;      // enable TIM3 clock
	TIM3->PSC   = 16000;     // 1msec
    TIM3->CNT   = 0;         /* clear timer counter */
    TIM3->CR1   = (1<<2);    // URS=1 bit 2 of CR1  -- DMA req on under/overflow
    TIM3->DIER  = 1<<8;      // Update DMA request enable
    // NOT ENABLED YET
   // TIM3->ARR   = 50 - 1;   //      <----- Set to duration to call DMA
}



void setClks(){
	RCC->APB1ENR1 |=1<<28;   // Enable the power interface clock by setting the PWREN bits
	RCC->APB1ENR2 |=0x1;     // Enable LPUART1EN clock
	RCC->CCIPR1   |=0x800;   // 01 for HSI16 to be used for LPUART1
	RCC->CCIPR1   &= ~(0x400);
	RCC->CFGR     |=0x1;     // Use HSI16 as SYSCLK
	RCC->CR       |=0x161;   // MSI clock enable; MSI=4 MHz; HSI16 clock enable
}
