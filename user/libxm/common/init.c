/*
 * $FILE: init.c
 *
 * Initialisation of the libxm
 *
 * $VERSION$
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#include <xm.h>

struct libXmParams libXmParams;

void init_libxm(partitionControlTable_t *partCtrlTab){
    libXmParams.partCtrlTab=partCtrlTab;
    libXmParams.partMemMap=(struct xmPhysicalMemMap *)((xmAddress_t)partCtrlTab+sizeof(partitionControlTable_t));
    libXmParams.commPortBitmap=(xmWord_t *)((xmAddress_t)libXmParams.partMemMap+partCtrlTab->noPhysicalMemAreas*sizeof(struct xmPhysicalMemMap));
    init_arch_libxm();
}

