#ifndef OSCOPE_PAGE_H
#define OSCOPE_PAGE_H

#ifdef __cplusplus
extern "C" {
#endif

void OscopePage_OnEnter(void);
void OscopePage_OnExit(void);

void OscopePage_TimeIncrease(void);
void OscopePage_TimeDecrease(void);

void OscopePage_VoltIncrease(void);
void OscopePage_VoltDecrease(void);

void OscopePage_TriggerIncrease(void);
void OscopePage_TriggerDecrease(void);

void OscopePage_ToggleRunStop(void);

void OscopePage_IncreaseSpeed(void);
void OscopePage_DecreaseSpeed(void);

#ifdef __cplusplus
}
#endif

#endif /* OSCOPE_PAGE_H */
