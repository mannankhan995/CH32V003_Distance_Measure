#include "debug.h"
#include <stdio.h>

#define TX_PIN          GPIO_Pin_5                                // USART TX Pin PD5
#define RX_PIN          GPIO_Pin_6                                // USART RX Pin PD6
#define ECHO_PIN        GPIO_Pin_2                                // Sensor Echo Pin PD2
#define TRIG_PIN        GPIO_Pin_3                                // Sensor Trigger Pin PD3
#define baud_rate       115200                                    // set baudrate
#define Calibration     0                                         // sensor Calibration factor
#define Off_Set         0                                         // sensor error offset

volatile uint16_t t_rise = 0;
volatile uint16_t t_fall = 0;
volatile uint8_t  edge_captured = 0; // 0 = wait rise, 1 = wait fall, 2 = done


void USART_Initialization(void){
    GPIO_InitTypeDef  GPIO_InitStructure = {0};
    USART_InitTypeDef USART_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_USART1, ENABLE);

    GPIO_InitStructure.GPIO_Pin = TX_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = RX_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = baud_rate;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;

    USART_Init(USART1, &USART_InitStructure);
    USART_Cmd(USART1, ENABLE);
}

void TIM1_InputCapture_Init(void){
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure = {0};
    TIM_ICInitTypeDef TIM_ICInitStructure = {0};
    NVIC_InitTypeDef NVIC_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_TIM1, ENABLE);

    GPIO_InitStructure.GPIO_Pin = ECHO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    // Time Base Base Configuration: Prescaler yields 1MHz timer clock (24MHz / 24)
    TIM_TimeBaseStructure.TIM_Period = 0xFFFF;
    TIM_TimeBaseStructure.TIM_Prescaler = 24 - 1; 
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

    // Input Capture Channel 1 Settings
    TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;
    TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising; // Start by looking for Rising Edge
    TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
    TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
    TIM_ICInitStructure.TIM_ICFilter = 0x4; // Basic digital filtering to avoid switching noise
    TIM_ICInit(TIM1, &TIM_ICInitStructure);

    // Enable Interupt in Nested Vectored Interrupt Controller (NVIC)
    NVIC_InitStructure.NVIC_IRQChannel = TIM1_CC_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    TIM_ITConfig(TIM1, TIM_IT_CC1, ENABLE); // Enable Capture/Compare 1 Interrupt
    TIM_Cmd(TIM1, ENABLE);                  // Start Counter
}

// Timer 1 Capture/Compare Interrupt Handler
void TIM1_CC_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void TIM1_CC_IRQHandler(void) {
    if (TIM_GetITStatus(TIM1, TIM_IT_CC1) != RESET) {
        if (edge_captured == 0) {
            t_rise = TIM_GetCapture1(TIM1);
            
            // Switch polarity to capture the Falling Edge
            TIM_OC1PolarityConfig(TIM1, TIM_ICPolarity_Falling);
            edge_captured = 1;
        } 
        else if (edge_captured == 1) {
            t_fall = TIM_GetCapture1(TIM1);
            
            // Revert polarity back to Rising Edge for next loop
            TIM_OC1PolarityConfig(TIM1, TIM_ICPolarity_Rising);
            edge_captured = 2;
        }
        TIM_ClearITPendingBit(TIM1, TIM_IT_CC1);
    }
}


void Trig_GPIO_Initialization(void){
    
    // Enable peripheral clock for Trigger Port
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
    
    // Configure Trigger Pin
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    GPIO_InitStructure.GPIO_Pin = TRIG_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
}

void Trigger_Sensor(void) {
    edge_captured = 0;
    
    GPIO_WriteBit(GPIOD, TRIG_PIN, Bit_RESET);
    Delay_Us(2);
    GPIO_WriteBit(GPIOD, TRIG_PIN, Bit_SET);
    Delay_Us(10);
    GPIO_WriteBit(GPIOD, TRIG_PIN, Bit_RESET);
}


void SetSystemClockTo48MHz(void) {
    // 1. Configure 1 cycle of Flash Latency
    // This is mandatory! At 48MHz, the Flash memory is too slow to keep up with 
    // the CPU. Adding a flash wait-state prevents code glitches and corruption.
    FLASH->ACTLR &= (uint32_t)((uint32_t)~FLASH_ACTLR_LATENCY);
    FLASH->ACTLR |= (uint32_t)FLASH_ACTLR_LATENCY_1;

    // 2. Set AHB Prescaler (HCLK) to Divide-by-1
    // This ensures the main peripheral bus runs at the full speed of the core clock.
    RCC_HCLKConfig(RCC_SYSCLK_Div1);

    // 3. Configure the PLL source to use the internal HSI oscillator 
    // On the CH32V003, this implies: 24MHz HSI * 2 Multiplier = 48MHz
    RCC_PLLConfig(RCC_PLLSource_HSI_MUL2);

    // 4. Turn on the PLL hardware block
    RCC_PLLCmd(ENABLE);

    // 5. Poll the status register until the hardware confirms the PLL is locked and stable
    while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);

    // 6. Switch the main system clock source (SYSCLK) over to the active PLL output
    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

    // 7. Wait until the hardware registers confirm that the clock switch is complete
    // RCC_GetSYSCLKSource() returns 0x08 when the core is safely driven by the PLL
    while(RCC_GetSYSCLKSource() != 0x08);
}




int main(void){
    // SetSystemClockTo48MHz();
    SystemCoreClockUpdate();
    Delay_Init();
    Trig_GPIO_Initialization();
    USART_Initialization();
    TIM1_InputCapture_Init();


    while(1) {
        uint32_t duration_sum = 0;
        volatile uint8_t valid_samples = 0;

        for(uint8_t i = 0; i < 4; i++){
            Trigger_Sensor();

            uint32_t timeout = 0;
            while(edge_captured != 2 && timeout < 100000) {
            timeout++;
        }

            // Only accumulate valid measurements
            if (edge_captured == 2) {
                duration_sum += (uint16_t)(t_fall - t_rise);
                valid_samples++;
            }
            Delay_Ms(100);
        }

        if (valid_samples > 0) {

            // Math: Timer clock is 1MHz (1us per tick). Speed of sound = 0.0343 cm/us @20°„C. Distance = (time * 0.0343) / 2
            // Optimized Fixed-Point Math (Scales the calculation up by 100,000) 0.01715 * 100000 = 1715. Extract integer and fractional parts using integer division
            uint16_t avg_duration = duration_sum / valid_samples;
            uint32_t distance_scaled = (uint32_t)avg_duration * 1715;

            unsigned int distance_cm = distance_scaled / 100000;
            unsigned int distance_mm_frac = (distance_scaled % 100000) / 1000;

            printf("Distance: %u.%02u cm\r\n", distance_cm, distance_mm_frac);

        } else {
            printf("Measurement Timeout\r\n");
        }

        Delay_Ms(1000);
    }
}
