/*
 * $FILE: checks.h
 *
 * checks definitions
 *
 * $VERSION$
 *
 * Authors: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

/*
  Changelog:
  - [02/09/2015:SPR-280715-01] added CheckPartitionEntryPoint.
*/

#ifndef _CHECKS_H_
#define _CHECKS_H_
#include "xmc.h"

extern void CheckHwIrq(int line, int lineNo);
extern void CheckPortName(int port, int partition);
extern void CheckMemoryRegion(int region);
#if defined(CONFIG_MPU) && defined(CONFIG_AT697F)
extern void CheckSharedMemoryAreaAlignment(int area);
extern void CheckSharedMemoryAreaOverlapping();
#endif
extern int CheckPhysMemArea(int memArea);
extern void CheckMemAreaPerPart(void);
extern void CheckMemBlockReg(void);
extern void CheckMemBlock(int mB);
extern void CheckHpvMemAreaFlags(void);
extern void CheckUartId(int uartId, int line);
extern void CheckSchedCyclicPlan(struct xmcSchedCyclicPlan *plan, struct xmcSchedCyclicPlanNoL *planNoL);
extern void CheckCyclicPlanPartitionId(void);
extern void CheckPartitionName(char *name, int line);
extern void HmHpvIsActionPermittedOnEvent(int event, int action, int line);
extern void HmPartIsActionPermittedOnEvent(int event, int action, int line);
extern void CheckIoPorts(void);
extern void HmCheckExistMaintenancePlan(void);
extern void CheckIpviTab(void);
extern void CheckChannelSizes(void);
extern void CheckPartitionEntryPoint(void);

#endif
