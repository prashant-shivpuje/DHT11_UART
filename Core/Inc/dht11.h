/*
 * dht11.h
 *
 *  Created on: Oct 18, 2026
 *      Author: Prashant
 */

#ifndef DHT11_H_
#define DHT11_H_

#include "main.h"
#include<stdint.h>

#define DHT11_PORT GPIOB
#define DHT11_PIN  GPIO_PIN_0



typedef struct
{
	uint8_t temperature ;
	uint8_t humidity ;
}DHT11_Data ;

void DHT11_Init(TIM_HandleTypeDef *htim);
uint8_t DHT11_Read(uint8_t*data);

#endif /* INC_DHT11_H_ */
