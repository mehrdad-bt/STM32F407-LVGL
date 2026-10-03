#ifndef OSCOPE_PAGE_H
#define OSCOPE_PAGE_H

#ifdef __cplusplus
extern "C" {
#endif

void OscopePage_OnEnter(void);
void OscopePage_OnExit(void);

/* ==========================================================
 * Time / Division
 * ========================================================== */

void OscopePage_TimeIncrease(void);
void OscopePage_TimeDecrease(void);

/* ==========================================================
 * Volt / Division
 * ========================================================== */

void OscopePage_VoltIncrease(void);
void OscopePage_VoltDecrease(void);

/* ==========================================================
 * Trigger
 * ========================================================== */

void OscopePage_TriggerIncrease(void);
void OscopePage_TriggerDecrease(void);

/* ==========================================================
 * Run / Stop
 * ========================================================== */

void OscopePage_ToggleRunStop(void);

/* ==========================================================
 * Compatibility with older buttons
 * ========================================================== */

void OscopePage_IncreaseSpeed(void);
void OscopePage_DecreaseSpeed(void);

#ifdef __cplusplus
}
#endif

#endif /* OSCOPE_PAGE_H */
