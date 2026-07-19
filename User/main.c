
#include "debug.h"

#define TX_PIN GPIO_Pin_5                                   // USART TX Pin PD5
#define RX_PIN GPIO_Pin_6                                   // USART RX Pin PD6
#define ECHO_PIN GPIO_Pin_2                                 // Sensor Echo Pin PD2
#define TRIG_PIN GPIO_Pin_3                                 // Sensor Trigger Pin PD3
#define baud_rate 115200                                    // set baudrate

volatile uint16_t t_rise = 0;
volatile uint16_t t_fall = 0;
volatile uint8_t  edge_captured = 0; // 0 = wait rise, 1 = wait fall, 2 = done


void USART_Initialization(void)
{
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

void TIM1_InputCapture_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure = {0};
    TIM_ICInitTypeDef TIM_ICInitStructure = {0};
    NVIC_InitTypeDef NVIC_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_TIM1, ENABLE);

    GPIO_InitStructure.GPIO_Pin = ECHO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    // Time Base Base Configuration: Prescaler yields 1MHz timer clock (48MHz / 48)
    TIM_TimeBaseStructure.TIM_Period = 0xFFFF;
    TIM_TimeBaseStructure.TIM_Prescaler = 48 - 1; 
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

int main(void)
{
    SystemCoreClockUpdate();
    Delay_Init();
    Trig_GPIO_Initialization();
    USART_Initialization();
    TIM1_InputCapture_Init();

    while(1)
    {
    }
}
