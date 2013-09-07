
#include "temperature.h"
#include <miosix.h>

unsigned short getRawTemperature()
{
    {
        miosix::FastInterruptDisableLock dLock;
        RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    }
    ADC1->CR1=0;
    ADC1->SMPR1=ADC_SMPR1_SMP16_2
              | ADC_SMPR1_SMP16_1
              | ADC_SMPR1_SMP16_0; //239.5 cycles sample time
    ADC1->SQR1=0; //Do only one conversion
    ADC1->SQR2=0;
    ADC1->SQR3=16; //Convert channel 16 (temperature)
    ADC1->CR2=ADC_CR2_ADON | ADC_CR2_TSVREFE;
    ADC1->CR2|=ADC_CR2_CAL;
    miosix::delayUs(10); //Wait fot tempsensor to stabilize
    while(ADC1->SR & ADC_CR2_CAL) ;
    ADC1->CR2|=ADC_CR2_ADON; //Setting ADON twice starts a conversion
    while((ADC1->SR & ADC_SR_EOC)==0) ;
    unsigned short result=ADC1->DR;
    ADC1->CR2=0; //Turn ADC OFF
    return result;
}
