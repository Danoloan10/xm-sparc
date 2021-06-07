/*
 * $FILE: hm.c
 *
 * Health Monitor
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
  - [18/11/2015:SPR-290915-01] '/' replaced by Div32
*/
#include <xm.h>
#include <hm.h>
#include <xm_inc/objects/hm.h>

static xm_u32_t Div32(xm_u32_t numer, xm_u32_t denom) {
    xm_u32_t quotient = 0, quotbit = 1;

    if (denom == 0) {
        return 0;
    }

    while (((xm_s32_t) denom >= 0) && (denom < numer)) {
        denom <<= 1;
        quotbit <<= 1;
    }

    while (quotbit && (numer != 0)) {
        if (denom <= numer) {
            numer -= denom;
            quotient += quotbit;
        }
        denom >>= 1;
        quotbit >>= 1;
    }
    
    return quotient;
}

xm_s32_t XM_hm_read(xmHmLog_t *hmLogPtr, xm_s32_t noLogs) {
    xm_s32_t ret;

    if (!hmLogPtr){
        return XM_INVALID_PARAM;
    }
    ret=XM_read_object(OBJDESC_BUILD(OBJ_CLASS_HM, XM_HYPERVISOR_ID, 0), hmLogPtr, noLogs*sizeof(xmHmLog_t), 0);
    return (ret>0) ? (Div32(ret, sizeof(xmHmLog_t))) : ret;
}

//@ \void{<track id="using-proc">}
xm_s32_t XM_hm_status(xmHmStatus_t *hmStatusPtr) {

    if (!hmStatusPtr) {
        return XM_INVALID_PARAM;
    }
    return XM_ctrl_object(OBJDESC_BUILD(OBJ_CLASS_HM, XM_HYPERVISOR_ID, 0), XM_HM_GET_STATUS, hmStatusPtr);
}
//@ \void{</track id="using-proc">}

xm_s32_t XM_hm_raise_event(xm_u32_t event) {
    /* SPR-130115-08 */
    xm_s32_t ret = XM_write_object(OBJDESC_BUILD(OBJ_CLASS_HM, XM_PARTITION_SELF, 0), &event, 1, 0);
    return (ret >= 0)?XM_OK: ret;
}
