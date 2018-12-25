//this make the noise

#include <proxmark3.h>
#include "apps.h"
#include "util.h"

#define PWMC_EnableChannel(x) AT91C_BASE_PWMC->PWMC_ENA = (1 << x)
#define PWMC_DisableChannel(x) AT91C_BASE_PWMC->PWMC_DIS = (1 << x)

//Duty cycle value 0 is not permitted on SAM7S chips.
//Duty cycle value 1 is not permitted in left-aligned mode on SAM7S chips.
void PWMC_SetDutyCycle(uint8_t channel, uint16_t duty)
{
    // If channel is disabled, write to CDTY
    if ((AT91C_BASE_PWMC->PWMC_SR & (1 << channel)) == 0) {
        AT91C_BASE_PWMC->PWMC_CH[channel].PWMC_CDTYR = duty;
    }
    // Otherwise use update register
    else {
        AT91C_BASE_PWMC->PWMC_CH[channel].PWMC_CMR &= ~AT91C_PWMC_CPD;
        AT91C_BASE_PWMC->PWMC_CH[channel].PWMC_CUPDR = duty;
    }
}

void PWMC_SetPeriod(uint8_t channel, uint16_t period)
{
    // If channel is disabled, write to CPRD
    if ((AT91C_BASE_PWMC->PWMC_SR & (1 << channel)) == 0) {
        AT91C_BASE_PWMC->PWMC_CH[channel].PWMC_CPRDR = period;
    }
    // Otherwise use update register
    else {
        AT91C_BASE_PWMC->PWMC_CH[channel].PWMC_CMR |= AT91C_PWMC_CPD;
        AT91C_BASE_PWMC->PWMC_CH[channel].PWMC_CUPDR = period;
    }
}

void PWMC_ConfigureChannel(uint8_t channel, uint8_t prescaler, uint8_t alignment, uint8_t polarity, uint32_t mode)
{
	AT91C_BASE_PIOA->PIO_ASR = AT91C_PA1_PWM1;
	AT91C_BASE_PIOA->PIO_PDR = AT91C_PA1_PWM1;

	// Disable channel (effective at the end of the current period)
    if ((AT91C_BASE_PWMC->PWMC_SR & (1 << channel)) != 0) {
        AT91C_BASE_PWMC->PWMC_DIS = 1 << channel;
        while ((AT91C_BASE_PWMC->PWMC_SR & (1 << channel)) != 0);
    }

    // Configure channel
    AT91C_BASE_PWMC->PWMC_CH[channel].PWMC_CMR = prescaler | alignment | polarity;

    // Configure clocks
    AT91C_BASE_PWMC->PWMC_MR = mode;
}

// frequency range 190Hz to 48Mhz
void PWMC_Beep(uint8_t channel, uint32_t frequency, uint32_t duration)
{
	uint32_t i = 31, result;

	result = 48000000/frequency;
	while ((i>7) && ((result>>i)==0)) i--;

	// cannot synthesise required frequency
	if (i>19) return;
	// create prescaler/divisor pair and pack it in required register format
	if (i>7) {
		i-=7;
		result = (i<<8) | (result>>i);
	}

	PWMC_ConfigureChannel(channel,AT91C_PWMC_CPRE_MCKA,0,0,result);
	PWMC_SetPeriod(channel, 5);
	PWMC_SetDutyCycle(channel, 50);
	PWMC_EnableChannel(channel);
	SpinDelay(duration);
	PWMC_DisableChannel(channel);
}
