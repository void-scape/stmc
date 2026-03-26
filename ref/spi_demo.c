#include "stm32l552xx.h"

// Some helper macros
#define bitset(word,   idx)  ((word) |=  (1<<(idx))) //Sets the bit number <idx> -- All other bits are not affected.
#define bitclear(word, idx)  ((word) &= ~(1<<(idx))) //Clears the bit number <idx> -- All other bits are not affected.
#define bitflip(word,  idx)  ((word) ^=  (1<<(idx))) //Flips the bit number <idx> -- All other bits are not affected.
#define bitcheck(word, idx)  ((word>>idx) &    1   ) //Checks the bit number <idx> -- 0 means clear; !0 means set.

// Loopback configuration to demo using SPI as a master or slave
//
// SPI2 (Master):
//     MOSI --> PD4  (Green) --------------+
//     MISO <-- PC2 (Blue) ------------+   |
//     SS   --> PD0 (Yellow) ------+   |   |
//     SCLK --> PD3 (Red) ------+  |   |   |
//                              |  |   |   |
// SPI3 (Slave):                |  |   |   |
//     SCLK <-- PC10  ----------+  |   |   |
//     SS   <-- PA4  --------------+   |   |
//     MISO --> PC11 ------------------+   |
//     MOSI <-- PD6  ----------------------+
//

void initSPI2M(); // Initialize SPI2 as master
void initSPI3S(); // Initialize SPI3 as slave
void initLED();
void RLEDset();
void GLEDtoggle();

int main(){
	RCC->AHB2ENR  |= 0b1111111;   // Enable the clock of the GPIOs ABCDEFG
	RCC->APB1ENR1 |=(3<<14);      // Enable the clock of SPI2 and SPI3
	initLED();

	// Configure SPI2 as MASTER using MOTOROLA frame format
	initSPI2M();

	// Configure SPI3 as SLAVE using MOTOROLA frame format
	initSPI3S();

	uint16_t tx_data=0x1000;
	uint16_t rx_data;


	while(1){
		SPI2->DR = tx_data;
		for(int i =0; i<100;i++); //some delay to allow SPI2 to send the data

		//SPI3 should have received the data by now
		if (SPI3->SR & 0x1){
			rx_data=SPI3->DR;
			if (rx_data!=tx_data) RLEDset();   // Check the data, RED is not match
			else                  GLEDtoggle();   // Toggle GREEN if matches
		}
		tx_data+=1;

	}

	return 0;
}

// Initialize SPI2 as master
void initSPI2M(){
	//////  MOSI --> PD4
	bitclear(GPIOD->MODER,  4*2  );  // 0b10 for AF
	bitset(  GPIOD->MODER,  4*2+1);
	bitset(  GPIOD->AFR[0], 4*4  ); // Use AF5 for SPI2_MOSI 0b0101
	bitclear(GPIOD->AFR[0], 4*4+1);
	bitset(  GPIOD->AFR[0], 4*4+2);
	bitclear(GPIOD->AFR[0], 4*4+3);
	bitset(  GPIOD->OSPEEDR,4*2);  // OSPEEDR 0b11  -- SPI runs fast, so you need to run
	bitset(  GPIOD->OSPEEDR,4*2+1);// GPIO drivers

	//////  MISO <-- PC2
	bitclear(GPIOC->MODER,  2*2  );  // 0b10 for AF
	bitset(  GPIOC->MODER,  2*2+1);
	bitset(  GPIOC->AFR[0], 2*4  ); // Use AF5 for SPI2_MISO 0b0101
	bitclear(GPIOC->AFR[0], 2*4+1);
	bitset(  GPIOC->AFR[0], 2*4+2);
	bitclear(GPIOC->AFR[0], 2*4+3);
	bitset(  GPIOC->OSPEEDR,2*2);  // OSPEEDR 0b11
	bitset(  GPIOC->OSPEEDR,2*2+1);

	//////  SS   --> PD0
	bitclear(GPIOD->MODER,  0*2  );  // 0b10 for AF
	bitset(  GPIOD->MODER,  0*2+1);
	bitset(  GPIOD->AFR[0], 0*4  ); // Use AF5 for SPI2_SS 0b0101
	bitclear(GPIOD->AFR[0], 0*4+1);
	bitset(  GPIOD->AFR[0], 0*4+2);
	bitclear(GPIOD->AFR[0], 0*4+3);
	bitset(  GPIOD->OSPEEDR,0*2);  // OSPEEDR 0b11
	bitset(  GPIOD->OSPEEDR,0*2+1);

	//////  SCLK --> PD3
	bitclear(GPIOD->MODER,  3*2  );  // 0b10 for AF
	bitset(  GPIOD->MODER,  3*2+1);
	bitset(  GPIOD->AFR[0], 3*4  ); // Use AF3 for SPI2_SS 0b0011
	bitset(  GPIOD->AFR[0], 3*4+1);
	bitclear(GPIOD->AFR[0], 3*4+2);
	bitclear(GPIOD->AFR[0], 3*4+3);
	bitset(  GPIOD->OSPEEDR,3*2);  // OSPEEDR 0b11
	bitset(  GPIOD->OSPEEDR,3*2+1);

	SPI2->CR2 |= (0xf<<8)|(3<<2); // CR2[11:8]= 0b1111    16-bit data size
                                  // CR2[2] = 0b1         SSOE: SS output enable
	                              // CR2[3] = 0b1         NSSP: NSS pulse management. In the case of a single data transfer, it forces the NSS pin high level after the transfer.

	SPI2->CR1 |= (1<<2)|(1<<6); // CR1[2] Master
                                // CR1[6] Enable SPI

}

// Initialize SPI3 as slave
void initSPI3S(){

	//     SCLK <-- PC10
	bitclear(GPIOC->MODER,  10*2  );  // 0b10 for AF
	bitset(  GPIOC->MODER,  10*2+1);
	bitclear(GPIOC->AFR[1], 2*4  ); // Use AF6 for SPI3_SCLK 0b0110
	bitset(  GPIOC->AFR[1], 2*4+1);
	bitset(  GPIOC->AFR[1], 2*4+2);
	bitclear(GPIOC->AFR[1], 2*4+3);
	bitset(  GPIOC->OSPEEDR,10*2);  // OSPEEDR 0b11
	bitset(  GPIOC->OSPEEDR,10*2+1);

	//     SS   <-- PA4
	bitclear(GPIOA->MODER,  4*2  );  // 0b10 for AF
	bitset(  GPIOA->MODER,  4*2+1);
	bitclear(GPIOA->AFR[0], 4*4  ); // Use AF6 for SPI3_SS 0b0110
	bitset(  GPIOA->AFR[0], 4*4+1);
	bitset(  GPIOA->AFR[0], 4*4+2);
	bitclear(GPIOA->AFR[0], 4*4+3);
	bitset(  GPIOA->OSPEEDR,4*2);  // OSPEEDR 0b11
	bitset(  GPIOA->OSPEEDR,4*2+1);

	//     MOSI <-- PD6
	bitclear(GPIOD->MODER,  6*2  );  // 0b10 for AF
	bitset(  GPIOD->MODER,  6*2+1);
	bitset(  GPIOD->AFR[0], 6*4  ); // Use AF5 for SPI2_SS 0b0101
	bitclear(GPIOD->AFR[0], 6*4+1);
	bitset(  GPIOD->AFR[0], 6*4+2);
	bitclear(GPIOD->AFR[0], 6*4+3);
	bitset(  GPIOD->OSPEEDR,6*2);  // OSPEEDR 0b11
	bitset(  GPIOD->OSPEEDR,6*2+1);

	//     MISO --> PC11
	bitclear(GPIOC->MODER,  11*2  ); // 0b10 for AF
	bitset(  GPIOC->MODER,  11*2+1);
	bitclear(GPIOC->AFR[1], 3*4  );  // Use AF6 for SPI3_SCLK 0b0110
	bitset(  GPIOC->AFR[1], 3*4+1);
	bitset(  GPIOC->AFR[1], 3*4+2);
	bitclear(GPIOC->AFR[1], 3*4+3);
	bitset(  GPIOC->OSPEEDR,11*2);  // OSPEEDR 0b11
	bitset(  GPIOC->OSPEEDR,11*2+1);

	SPI3->CR2 |= (0xf<<8)|(3<<2); // CR2[11:8]= 0b1111    16-bit data size
                                  // CR2[2] = 0b1         SSOE: SS output enable
	                              // CR2[3] = 0b1         NSSP: NSS pulse management. In the case of a single data transfer, it forces the NSS pin high level after the transfer.

	SPI3->CR1 |= (1<<6);          // Slave, Enable SPI
}

void initLED(){
	// Set up the mode -- RED -- PA9
	GPIOA->MODER |= 1<<(9*2); // setting bit 18
	GPIOA->MODER &= ~(1<<(9*2+1));
	// Set up the mode -- GREEN  -- PC07 (LED1)
	GPIOC->MODER |= 1<<(7*2); // setting bit 18
	GPIOC->MODER &= ~(1<<(7*2+1));
}
void RLEDset(){	    GPIOA->ODR |= 1<<9;}
void GLEDtoggle(){	GPIOC->ODR ^= 1<<7;}
