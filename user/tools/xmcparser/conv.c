/*
 * $FILE: conv.c
 *
 * conversion helper functions
 *
 * $VERSION$
 *
 * Authors: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "limits.h"
#include "common.h"
#include "parser.h"
#include "checks.h"
#include "xmc.h"

xm_u32_t ToVersion(char *s) {
    xm_u32_t version, subversion, revision;

    sscanf(s, "%u.%u.%u", &version, &subversion, &revision);
    return XMC_SET_VERSION(version, subversion, revision);
}

xm_u32_t ToU32(char *s, int base) {
    xm_u32_t u;

    u=strtoul(s, 0, base);
    return u;
}

int ToYNTF(char *s, int line) {
     if (!strcasecmp(s, "yes")||!strcasecmp(s, "true"))
         return 1;
     
     if (!strcasecmp(s, "no")||!strcasecmp(s, "false"))
         return 0;
     
     LineError(line, "expected yes|no|true|false");
     return 0;
}

xm_u32_t ToFreq(char *s) {
    double d;
    sscanf(s, "%lf", &d);
    if(strcasestr(s, "MHz")) {
	d*=1000.0;
    } /*else if(strcasestr(b, "KHz")) {
	d*=1000.0;
	} */
    if (d>=UINT_MAX)
        EPrintF("Frequency exceeds MAX_UINT");
    return (xm_u32_t)d;
}

xm_u32_t ToTime(char *s) {
    double d;
    sscanf(s, "%lf", &d); 
    if(strcasestr(s, "us")) {
    } else if(strcasestr(s, "ms")) {
	d*=1000.0;
    } else  if(strcasestr(s, "s")) {
	d*=1000000.0;
    }
    if (d>=UINT_MAX)
        EPrintF("Time exceeds MAX_UINT");
    return (xm_u32_t)d;
}

xmSize_t ToSize(char *s) {
    double d;
    sscanf(s, "%lf", &d);  
    if(strcasestr(s, "MB")) {
	d*=(1024.0*1024.0);
    } else if(strcasestr(s, "KB")) {
	d*=1024.0;
    } /*else  if(strcasestr(s, "B")) {
	} */

    if (d>=UINT_MAX)
	EPrintF("Size exceeds MAX_UINT");
    
    return (xmSize_t)d;
}

xm_u32_t ToCommPortType(char *s, int line) {
    if (!strcmp(s, "queuing"))
	return XM_QUEUING_PORT;

    if (!strcmp(s, "sampling"))
	return XM_SAMPLING_PORT;

    LineError(line, "expected a valid communication port type");
    return 0;
}

struct attr2flags {
    char attr[32];
    xm_u32_t flag;
};

#define NO_PARTITION_FLAGS 6
static struct attr2flags partitionFlagsTab[NO_PARTITION_FLAGS]={
    [0]={
	.attr="system",
	.flag=XM_PART_SYSTEM,
    },
    [1]={
	.attr="fp",
	.flag=XM_PART_FP,
    },
    [2]={
	.attr="boot",
	.flag=XM_PART_BOOT,
    }, 
    [3]={
	.attr="icache_disabled",
	.flag=XM_PART_ICACHE_DISABLED,
    },    
    [4]={
	.attr="dcache_disabled",
	.flag=XM_PART_DCACHE_DISABLED,
    }, 
    [5]={
	.attr="none",
	.flag=0x0,
    },
};

#define NO_PHYMEM_AREA_FLAGS 10
static struct attr2flags physMemAreaFlagsTab[NO_PHYMEM_AREA_FLAGS]={
    [0]={
	.attr="unmapped",
	.flag=XM_MEM_AREA_UNMAPPED,
    },
    [1]={
	.attr="read-only",
	.flag=XM_MEM_AREA_READONLY,
    },
    [2]={
	.attr="uncacheable",
	.flag=XM_MEM_AREA_UNCACHEABLE,
    },
    [3]={
	.attr="rom",
	.flag=XM_MEM_AREA_ROM,
    },
    [4]={
	.attr="flag0",
	.flag=XM_MEM_AREA_FLAG0,
    },
    [5]={
	.attr="flag1",
	.flag=XM_MEM_AREA_FLAG1,
    },
    [6]={
	.attr="flag2",
	.flag=XM_MEM_AREA_FLAG2,
    },
    [7]={
	.attr="flag3",
	.flag=XM_MEM_AREA_FLAG3,
    },
    [8]={
	.attr="none",
	.flag=0x0,
    },
};

#define NO_SLOTS_FLAGS 10
static struct attr2flags slotsFlagsTab[NO_PHYMEM_AREA_FLAGS]={
    [0]={
	.attr="periodStart",
	.flag=XM_PERIOD_START_SLOT,
    },
};

void ProcessIdList(char *s, void (*CallBack)(int, char *), int line) {
    char *tmp, *tmp1;

    for (tmp=s, tmp1=strstr(tmp, " "); *tmp;) {
        if (tmp1) *tmp1=0;
        
        CallBack(line, tmp);

        if (tmp1)
            tmp=tmp1+1;
        else
            break;
	tmp1=strstr(tmp, " ");
     }
}

static xm_u32_t ToFlags(char *s, struct attr2flags *attr2flags, int noElem, int line) {
    xm_u32_t flags=0, found=0;
    char *tmp, *tmp1;
    int e;

    for (tmp=s, tmp1=strstr(tmp, " "); *tmp;) {
        if (tmp1) *tmp1=0;
	for (e=0; e<noElem; e++)
	    if (!strcasecmp(tmp, attr2flags[e].attr)) {
		flags|=attr2flags[e].flag;
		found=1;
	    }
	if (!found)
	    LineError(line, "expected valid flag (%s)\n", tmp);
	found=0;
	
	if (tmp1)
            tmp=tmp1+1;
        else
            break;
	tmp1=strstr(tmp, " ");
    }

    return flags;
}

xm_u32_t ToPartitionFlags(char *s, int line) {
    return ToFlags(s, partitionFlagsTab, NO_PARTITION_FLAGS, line);
}

xm_u32_t ToPhysMemAreaFlags(char *s, int line) {
    return ToFlags(s, physMemAreaFlagsTab, NO_PHYMEM_AREA_FLAGS, line);
}

xm_u32_t ToSlotsFlags(char *s, int line) {
    return ToFlags(s, slotsFlagsTab, NO_SLOTS_FLAGS, line);
}

xm_u32_t ToCommPortDirection(char *s, int line) {
    if (!strcmp(s, "source"))
	return XM_SOURCE_PORT;

    if (!strcmp(s, "destination"))
	return XM_DESTINATION_PORT;
	    
    LineError(line, "expected a valid communication port direction");
    return 0;
}

void ToHwIrqLines(char *s, int lineNo) {
    char *tmp, *tmp1;
    int line;
    
    for (tmp=s, tmp1=strstr(tmp, " "); *tmp;) {
        if (tmp1) *tmp1=0;

	line=atoi(tmp);
	if (line>=CONFIG_NO_HWIRQS)
	    LineError(lineNo, "invalid hw interrupt line (%d)", line);
	if (xmc.hpv.hwIrqTab[line].owner!=XM_IRQ_NO_OWNER)
	    LineError(lineNo, "hw interrupt line (%d) already assigned (line %d)", line, xmcNoL.hpv.hwIrqTab[line]);
	
	CheckHwIrq(line, lineNo);
	xmc.hpv.hwIrqTab[line].owner=xmc.noPartitions-1;
	xmcNoL.hpv.hwIrqTab[line]=lineNo;
	if (tmp1)
            tmp=tmp1+1;
        else
            break;
	tmp1=strstr(tmp, " ");
    }
}

