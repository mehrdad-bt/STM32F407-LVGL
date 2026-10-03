#ifndef OSCOPE_PAGE_H
#define OSCOPE_PAGE_H

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * OSCILLOSCOPE PAGE API
 * ============================================================
 */

void OscopePage_OnEnter(void);
void OscopePage_OnExit(void);

void OscopePage_TimeIncrease(void);
void OscopePage_TimeDecrease(void);

void OscopePage_VoltIncrease(void);
void OscopePage_VoltDecrease(void);

void OscopePage_TriggerIncrease(void);
void OscopePage_TriggerDecrease(void);

void OscopePage_ToggleRunStop(void);

void OscopePage_SweepIncrease(void);
void OscopePage_SweepDecrease(void);

void OscopePage_Task(void);

#ifdef __cplusplus
}
#endif

#endif /* OSCOPE_PAGE_H */
