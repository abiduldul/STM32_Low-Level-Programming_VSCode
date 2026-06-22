#include "stm32l4p5xx.h"

void systick_init(void) {
    // The chip runs at 4,000,000 cycles per second.
    // To wait exactly 1 millisecond, we need to count down 4,000 cycles.
    // We subtract 1 because counting down to 0 takes one extra cycle.
    SysTick->LOAD = 4000 - 1; 

    // Clear the current value just to start with a clean slate
    SysTick->VAL = 0;

    // Enable SysTick (Bit 0) and use the Processor Clock (Bit 2)
    SysTick->CTRL |= (1 << 0) | (1 << 2);
}

void delay_ms(uint32_t ms) {
    for (uint32_t i = 0; i < ms; i++) {
        // Reset the counter value to zero so it triggers a reload
        SysTick->VAL = 0; 
        
        // Wait until Bit 16 (the COUNTFLAG) turns into a 1
        while ((SysTick->CTRL & (1 << 16)) == 0) {
            // Just wait here for 1 millisecond
        }
    }
}

void uart_write(char ch) {
    // Wait until the Transmit Data Register is Empty (TXE flag is bit 7 in ISR)
    while (!(LPUART1->ISR & (1 << 7))) {
        // Just wait
    }
    // Drop the character into the transmit register
    LPUART1-> TDR = ch;
}

int main(void) {
    // On the STM32L4, GPIO ports are connected to the AHB2 bus.
    // Bit 0 of AHB2ENR enables the clock for GPIO Port A.
    RCC->AHB2ENR |= (1 << 0) | (1 << 2); 
    // Enable the clock for LPUART1. It lives on the APB1 Bus (Register 2), Bit 0.
    RCC->APB1ENR2 |= (1 << 0);

    // The MODER register controls pin modes. Each pin uses 2 bits.
    // Pin 8 uses bits 16 and 17 (8 * 2 = 16).
    
    // Set Pin 8 of Port A to General Purpose Output Mode (01)
    GPIOA->MODER &= ~(0x3 << 16);
    GPIOA->MODER |= (0x1 << 16);  

    // For LPUART1, we need to use Pin 0 and Pin 1 of Port C in Alternate Function mode.
    GPIOC->MODER &= ~(0xF << 0); // Clear bits 0, 1, 2, and 3
    GPIOC->MODER |= (0xA << 0);  // 0xA is '1010' in binary, setting both pins to AF mode
    
    // Tell the pins WHICH Alternate Function to use. LPUART1 is AF8 (binary '1000').
    // We modify the Alternate Function Low Register (AFR[0]).
    GPIOC->AFR[0] &= ~(0xFF << 0); // Clear the AF slots for Pin 0 and Pin 1
    GPIOC->AFR[0] |= (0x88 << 0);  // Set AF8 (1000) into both slots (1000 1000 = 0x88)

    // Set the Baud Rate to 115200. 
    // The STM32L4 default boot clock is the MSI at 4 MHz.
    // LPUART formula: BRR = (256 * ClockSpeed) / BaudRate -> (256 * 4,000,000) / 115200 = 8889
    LPUART1->BRR = 8889;
    
    // Enable the Peripheral (UE=bit 0), Receiver (RE=bit 2), and Transmitter (TE=bit 3)
    LPUART1->CR1 |= (1 << 0) | (1 << 2) | (1 << 3);

    systick_init();
    while(1) {
        // Toggle bit 8 in the Output Data Register (ODR)
        GPIOA->ODR ^= (1 << 8);
        
        // Send a message via UART
        uart_write('H');
        uart_write('i');
        uart_write('\r'); 
        uart_write('\n');

        // Wait for a moment so we can see the blink
        delay_ms(100); 
    }
}