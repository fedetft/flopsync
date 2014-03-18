
#ifndef TEMPERATURE_H
#define TEMPERATURE_H

/**
 * \return the raw value of the stm32f100c8 internal temperature sensor
 * | min | avg | max |
 * | 4.0 | 4.3 | 4.6 | mV/°C temperature coefficient is around
 * |1.32 |1.41 |1.50 | V @ 25°C
 * Adc resolution is 12 bit, and reference voltage is ~3V on battery
 * powered nodes, and 3.3V on USB powered ones
 */
unsigned short getRawTemperature();

/**
 * \return the crystal oscillator temperature
 * 10 mv/k LM335
 * 
 * Adc resolution is 12 bit, and reference voltage is ~3V on battery
 * powered nodes, and 3.3V on USB powered ones
 */
unsigned short getADCTemperature();

#endif //TEMPERATURE_H
