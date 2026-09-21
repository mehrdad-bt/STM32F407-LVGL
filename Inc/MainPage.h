#ifndef MAIN_PAGE_H
#define MAIN_PAGE_H

void MainPage_Init(void);
void MainPage_Run(void);

void MainPage_OnUartPacket(
    float voltage,
    float current
);

void MainPage_OnUartTimeout(void);

float MainPage_GetVoltage(void);
float MainPage_GetCurrent(void);

#endif
