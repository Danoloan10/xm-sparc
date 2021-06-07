/*
 * $FILE: console.c
 *
 * Object console
 *
 * $VERSION$
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */
/*
  Changelog:
  - [02/09/2015:SPR-020915-04] Remove VBOOT macro and boot.h
*/
#include <assert.h>
#include <rsvmem.h>
#include <hypercalls.h>
#include <sched.h>
#include <objdir.h>
#include <stdc.h>

#include <objects/console.h>
#include <spinlock.h>

static struct console xmCon[CONFIG_NO_CPUS], *partitionConTab;
#ifdef CONFIG_SMP_SUPPORT
static spinLock_t spinlock=SPINLOCK_INIT;
#endif

void ConsolePutChar(xm_u8_t c) {
    if (xmCon[GET_CPU_ID()].dev) {
	if (KDevWrite(xmCon[GET_CPU_ID()].dev, &c, 1)!=1) {
	    KDevSeek(xmCon[GET_CPU_ID()].dev, 0, DEV_SEEK_START);
	    KDevWrite(xmCon[GET_CPU_ID()].dev, &c, 1);
	}
    }
}

static inline xm_s32_t WriteMod(struct console *con, xm_u8_t *b) {
    if (KDevWrite(con->dev, b, 1)!=1) {
        KDevSeek(con->dev, 0, DEV_SEEK_START);
        if (KDevWrite(con->dev, b, 1)!=1)
            return 0;
    }
    return 1;
}

static xm_s32_t WriteConsoleObj(xmObjDesc_t desc, xm_u8_t *__gParam buffer, xmSize_t length) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    xmId_t partId=OBJDESC_GET_PARTITIONID(desc), pId=PartitionId2PId(partId);
    struct console *con;
    xm_s32_t e;
    if (partId!=GetPartitionCfg(info->sched.cKThread)->rId)
	return XM_PERM_ERROR;
    if (CheckGParam(buffer, length, 1, PFLAG_NOT_NULL)<0)
        return XM_INVALID_PARAM;
    con=(pId==XM_HYPERVISOR_ID)?&xmCon[GET_CPU_ID()]:&partitionConTab[pId];
#ifdef CONFIG_SMP_SUPPORT
    SpinLock(&spinlock);
#endif
    for (e=0; e<length; e++) {
	PreemptionOn();
	PreemptionOff();
        if (!WriteMod(con, &buffer[e])) {
#ifdef CONFIG_SMP_SUPPORT
            SpinUnlock(&spinlock);
#endif
            return e;
        }
    }
#ifdef CONFIG_SMP_SUPPORT
    SpinUnlock(&spinlock);
#endif
    return length;    
}

static const struct object consoleObj={
    .Write=(writeObjOp_t)WriteConsoleObj,
    .Read=NULL,
    .Seek=NULL,
};

xm_s32_t SetupConsole(void) {
    xm_s32_t e;

    GET_MEMZ(partitionConTab, sizeof(struct console)*xmcTab.noPartitions);
    for (e=0; e<CONFIG_NO_CPUS; e++)
        xmCon[e].dev=LookUpKDev(&xmcTab.hpv.cpuTab[e].consoleDev);
    objectTab[OBJ_CLASS_CONSOLE]=&consoleObj;
    for (e=0; e<xmcTab.noPartitions; e++) {
	partitionConTab[e].dev=LookUpKDev(&xmcPartitionTab[e].consoleDev);
    }

    return 0;
}

REGISTER_OBJ(SetupConsole);
