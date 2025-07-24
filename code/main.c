#include <stm32f1xx.h>

// Servo angle PWM values
#define Servo1MinAngle  600   // PWM for 0°
#define Servo1MaxAngle  3000  // PWM for 180°
#define Servo2MinAngle  500   // PWM for 0°
#define Servo2MaxAngle  3000  // PWM for 180°

void port_init(); 
void timer2_init(); 
void servo1_angle(uint32_t angle); 
void servo2_angle(uint32_t angle); 
void _delay_ms(int time); 
void init_ADC(); 
uint16_t ADC_read(int channel_num); 

// Global variables
int s_mode = 0; 
unsigned int data = 0; 
uint16_t value = 0;

int main(void) 
{ 
    port_init(); 
    timer2_init(); 
    init_ADC(); 

    while(1) 
    { 
        switch(s_mode) 
        { 
            case 0:
                servo1_angle(180);       // Move Servo1 to 180°
                _delay_ms(2000); 
                servo1_angle(97);        // Move Servo1 to ~97°
                _delay_ms(1000); 
                s_mode++;                // Proceed to next state
                break; 

            case 1:
                value = ADC_read(3);     // Read analog value from channel 3
                if(value > 3400 && value < 3900)
                {
                    s_mode = 2;
                }
                else if(value > 3950)
                {
                    s_mode = 3;
                }
                else if(value > 3100 && value < 3400)
                {
                    s_mode = 4;
                }
                else
                {
                    s_mode = 5;
                }
                break;

            case 2:
                servo2_angle(0);         // Move Servo2 to 0°
                _delay_ms(2000); 
                servo1_angle(85);        // Move Servo1
                _delay_ms(1000); 
                servo1_angle(90); 
                _delay_ms(1000); 
                break; 

            case 3:
                servo2_angle(60); 
                _delay_ms(2000); 
                servo1_angle(85); 
                _delay_ms(1000); 
                servo1_angle(90); 
                _delay_ms(1000); 
                break; 

            case 4:
                servo2_angle(90); 
                _delay_ms(2000); 
                servo1_angle(85); 
                _delay_ms(1000); 
                servo1_angle(90); 
                _delay_ms(1000); 
                break; 
        } 
    }
} 

// Initialize GPIO ports
void port_init() 
{
    RCC->CR |= (1<<0); // Enable HSE (High-Speed External clock)

    // Set ADC prescaler to divide by 6 -> 72 MHz / 6 = 12 MHz
    RCC->CFGR = (1<<15) | (1<<14); 

    RCC->APB2ENR |= (1<<2) | (1<<9);   // Enable GPIOA and ADC1 clocks
    RCC->APB1ENR |= (1<<0);            // Enable TIM2 clock

    // Clear configuration bits for PA1 (ADC input)
    GPIOA->CRL &= ~((1<<12) | (1<<13) | (1<<14) | (1<<15)); 
} 

// Initialize Timer 2 for PWM generation
void timer2_init() 
{
    TIM2->CCER |= (1<<8);   // Enable channel 3 output (CCR3)
    TIM2->CCER |= (1<<4);   // Enable channel 2 output (CCR2)

    TIM2->CR1 |= (1<<0) | (1<<7); // Enable counter, enable auto-reload preload
    TIM2->ARR = 8399;             // Auto-reload value

    TIM2->CR2 |= (1<<10) | (1<<11); // Master mode selection
    TIM2->CR2 |= (1<<12) | (1<<13); // Capture/Compare DMA requests

    TIM2->PSC = 10; // Prescaler -> Timer clock = 72MHz / (10+1) = ~6.5MHz

    // Channel 2 configuration: PWM Mode 1
    TIM2->CCMR2 |= (1<<7) | (1<<6) | (1<<5) | (1<<3) | (1<<2); 

    // Channel 3 configuration: PWM Mode 1
    TIM2->CCMR1 |= (1<<15) | (1<<14) | (1<<13) | (1<<11) | (1<<10); 

    TIM2->BDTR |= (1<<15); // Main output enable
} 

// Set angle for Servo 1 (connected to CCR3)
void servo1_angle(uint32_t angle) 
{
    // Configure PA1 as Alternate Function Output Push-Pull
    GPIOA->CRL |= (1<<8) | (1<<9) | (1<<11); 

    // Convert angle to PWM pulse width
    uint32_t step = ((Servo1MaxAngle - Servo1MinAngle)/180) * angle; 

    if(step == 0)
        step = Servo1MinAngle; 
    if(step > Servo1MaxAngle)
        step = Servo1MaxAngle; 

    TIM2->CCR3 = step; // Set compare value for Servo 1
} 

// Set angle for Servo 2 (connected to CCR2)
void servo2_angle(uint32_t angle) 
{
    // Configure PA0 as Alternate Function Output Push-Pull
    GPIOA->CRL |= (1<<4) | (1<<5) | (1<<7); 

    // Convert angle to PWM pulse width
    uint32_t step = ((Servo2MaxAngle - Servo2MinAngle)/180) * angle; 

    if(step == 0)
        step = Servo2MinAngle; 
    if(step > Servo2MaxAngle)
        step = Servo2MaxAngle; 

    TIM2->CCR2 = step; // Set compare value for Servo 2
} 

// Initialize ADC1
void init_ADC() 
{
    ADC1->CR2 = (1<<0) | (1<<2) | (1<<22) | (1<<17); 
    // ADC ON, continuous conversion, external trigger, calibration

    ADC1->SQR1 = 0x00100000; // 1 conversion
    ADC1->SMPR2 = (1<<0) | (1<<1) | (1<<2); // Set sampling time
} 

// Read ADC value from a specific channel
uint16_t ADC_read(int channel_num) 
{
    ADC1->SQR3 = channel_num; // Select channel
    while((ADC1->CR2 & 0x00000004) != 0); // Wait if conversion in progress

    ADC1->CR2 |= (1<<0); // Start conversion
    while((ADC1->SR & 0x00000002) == 0); // Wait for EOC (End Of Conversion)

    data = ADC1->DR; // Read data
    ADC1->SR = 0x00000000; // Clear status flags
    ADC1->CR2 |= (1<<22);  // Re-start conversion

    return data; 
} 

// Simple delay function
void _delay_ms(int time) 
{ 
    for(int i = 0; i < time; i++) 
    { 
        for(int j = 0; j < 1000; j++); 
    } 
}
