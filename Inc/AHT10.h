#ifndef AHT10_H
#define AHT10_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    float temperature;
    float humidity;
    bool valid;

} AHT10_Data_t;


/* Initialize AHT10 */
bool AHT10_Init(I2C_HandleTypeDef *hi2c);


/* Must be called periodically from main loop */
void AHT10_Task(void);


/* Get latest sensor data */
bool AHT10_GetData(AHT10_Data_t *data);

#endif
