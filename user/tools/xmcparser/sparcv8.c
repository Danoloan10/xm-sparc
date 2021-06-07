/*
 * $FILE: sparcv8.c
 *
 * architecture dependent stuff
 *
 * $VERSION$
 *
 * Authors: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 *
 * [04/07/2016] Angel Esquinas
 * - SPR-160609-01 Improve page table memory reservation algorithm.
 *
 */

#define _RSV_IO_PORTS_
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <xm_inc/arch/paging.h>
#include <xm_inc/guest.h>
#include <xm_inc/arch/ginfo.h>

#include "conv.h"
#include "common.h"
#include "parser.h"
#include "xmc.h"

xm_u32_t ToRegionFlags(char *s) {
    xm_u32_t flags=0;
    if(!strcasecmp(s, "stram")) {
        flags=XMC_REG_FLAG_PGTAB;
    } else if(!strcasecmp(s, "sdram")) {
        flags=XMC_REG_FLAG_PGTAB;
    } else if(!strcasecmp(s, "rom")) {
        flags=XM_MEM_AREA_UNMAPPED|XMC_REG_FLAG_ROM;
    } else {
	EPrintF("Expected valid region type (%s)\n", s);
    }
    return flags;
}

static void BaseRange_AH(xmlNodePtr node, const xmlChar *val) {
    xmcIoPortTab[C_IOPORT].range.base=ToU32((char *)val, 16);
    xmcIoPortTabNoL[C_IOPORT].range.base=node->line;
    xmcIoPortTab[C_IOPORT].type=XM_IOPORT_RANGE;
}

static struct attrXml baseRange_A={BAD_CAST"base", BaseRange_AH};

static void NoPortsRange_AH(xmlNodePtr node, const xmlChar *val) {
    xmcIoPortTab[C_IOPORT].range.noPorts=ToU32((char *)val, 10);
    xmcIoPortTabNoL[C_IOPORT].range.noPorts=node->line;
}

static struct attrXml noPortsRange_A={BAD_CAST"noPorts", NoPortsRange_AH};

static void IoPort_NH0(xmlNodePtr node) {
    xmcPartitionTab[C_PARTITION].noIoPorts++;
    xmc.noIoPorts++;
    DO_REALLOC(xmcIoPortTab, xmc.noIoPorts*sizeof(struct xmcIoPort));
    DO_REALLOC(xmcIoPortTabNoL, xmc.noIoPorts*sizeof(struct xmcIoPortNoL));
    memset(&xmcIoPortTab[C_IOPORT], 0, sizeof(struct xmcIoPort));
    memset(&xmcIoPortTabNoL[C_IOPORT], 0, sizeof(struct xmcIoPortNoL));
}

static struct nodeXml rangeIoPort_N={BAD_CAST"Range", IoPort_NH0, 0, 0, (struct attrXml *[]){&baseRange_A, &noPortsRange_A, 0}, 0};

static void AddressRestricted_AH(xmlNodePtr node, const xmlChar *val) {
    xmcIoPortTab[C_IOPORT].restricted.address=ToU32((char *)val, 16);
    xmcIoPortTabNoL[C_IOPORT].restricted.address=node->line;
    xmcIoPortTab[C_IOPORT].type=XM_RESTRICTED_IOPORT;
}

static struct attrXml addressRestricted_A={BAD_CAST"address", AddressRestricted_AH};

static void MaskRestricted_AH(xmlNodePtr node, const xmlChar *val) {
    xmcIoPortTab[C_IOPORT].restricted.mask=ToU32((char *)val, 16);
    xmcIoPortTabNoL[C_IOPORT].restricted.mask=node->line;
}

static struct attrXml maskRestricted_A={BAD_CAST"mask", MaskRestricted_AH};

static struct nodeXml restrictedIoPort_N={BAD_CAST"Restricted", IoPort_NH0, 0, 0, (struct attrXml *[]){&addressRestricted_A, &maskRestricted_A, 0}, 0};

static void IoPorts_NH0(xmlNodePtr node) {
    xmcPartitionTab[C_PARTITION].ioPortsOffset=xmc.noIoPorts;
}

struct nodeXml ioPorts_N={BAD_CAST"IoPorts", IoPorts_NH0, 0, 0, 0, (struct nodeXml *[]){&rangeIoPort_N, &restrictedIoPort_N, 0}};

void GenerateIoPortTab(FILE *oFile) {
    int i;
    fprintf(oFile, "const struct xmcIoPort xmcIoPortTab[] = {\n");
    for (i=0; i<xmc.noIoPorts; i++) {
	fprintf(oFile, ADDNTAB(1, "[%d] = {\n"), i);
	fprintf(oFile, ADDNTAB(2, ".type = "));
	if (xmcIoPortTab[i].type==XM_IOPORT_RANGE) {
	    fprintf(oFile, "XM_IOPORT_RANGE,\n");
	    fprintf(oFile, ADDNTAB(2, "{.range.base = 0x%x,\n"), xmcIoPortTab[i].range.base);
	    fprintf(oFile, ADDNTAB(2, ".range.noPorts = %d, },\n"), xmcIoPortTab[i].range.noPorts);
	} else {
	    fprintf(oFile, "XM_RESTRICTED_IOPORT,\n");
	    fprintf(oFile, ADDNTAB(3, "{.restricted.address = 0x%x,\n"), xmcIoPortTab[i].restricted.address);
	    fprintf(oFile, ADDNTAB(3, ".restricted.mask = 0x%x,\n"), xmcIoPortTab[i].restricted.mask);
	    fprintf(oFile, ADDNTAB(2, "},\n"));
	}
	fprintf(oFile, ADDNTAB(1, "},\n"));
    }
    fprintf(oFile, "};\n\n");
}

void CheckIoPorts(void) {
    xmAddress_t a0, a1, b0, b1;
    xm_u32_t m0, m1;
    int e, line0, line1, iP;

    for (iP=0; iP<xmc.noIoPorts; iP++) {	
	if (xmcIoPortTab[iP].type==XM_IOPORT_RANGE) {
	    b0=xmcIoPortTab[iP].range.base;
	    b1=b0+xmcIoPortTab[iP].range.noPorts*sizeof(xm_u32_t);
	    line0=xmcIoPortTabNoL[iP].range.base;
            m0=~0;
	} else {
	    b0=xmcIoPortTab[iP].restricted.address;
            m0=xmcIoPortTab[iP].restricted.mask;
	    line0=xmcIoPortTabNoL[iP].restricted.address;
	    b1=b0+sizeof(xm_u32_t);
	}
	
	for (e=0; e<noRsvIoPorts; e++) {
	    a0=rsvIoPorts[e].base;
	    a1=a0+rsvIoPorts[e].offset*sizeof(xm_u32_t);
	    if (!((a0>=b1)||(a1<=b0))) {
		LineError(line0, "io-port [0x%lx:%d] reserved by XM",
			  rsvIoPorts[e].base, rsvIoPorts[e].offset);
	    }
	}
    
	for (e=iP+1; e<xmc.noIoPorts; e++) {
	    if (xmcIoPortTab[e].type==XM_IOPORT_RANGE) {
		a0=xmcIoPortTab[e].range.base;
		a1=a0+xmcIoPortTab[e].range.noPorts*sizeof(xm_u32_t);
		line1=xmcIoPortTabNoL[e].range.base;
                m1=~0;
	    } else {
		a0=xmcIoPortTab[e].restricted.address;
		a1=a0+sizeof(xm_u32_t);
                m1=xmcIoPortTab[e].restricted.mask;
		line1=xmcIoPortTabNoL[e].restricted.address;
	    }
	    if (!((a0>=b1)||(a1<=b0)))
                if (m0&m1)
                    LineError(line1, "io-port [0x%lx:%d] already assigned (line %d)", b0, b1-b0, line0);
	}
    }
}

#ifdef CONFIG_MPU
void ArchMpuRsvMem(FILE *oFile) {
}
#endif

#ifdef CONFIG_MMU
#define PTDL1_SHIFT 24
#define PTDL2_SHIFT 18
#define PTDL3_SHIFT 12
#define PTDL1SIZE 1024
#define PTDL2SIZE 256
#define PTDL3SIZE 256

#define LEVEL1_MAPPING_SIZE (1 << PTDL1_SHIFT)
#define LEVEL2_MAPPING_SIZE (1 << PTDL2_SHIFT)
#define LEVEL3_MAPPING_SIZE (1 << PTDL3_SHIFT)

#define PTDL1ENTRIES (PTDL1SIZE>>2)
#define PTDL2ENTRIES (PTDL2SIZE>>2)
#define PTDL3ENTRIES (PTDL3SIZE>>2)
#define XM_VMAPEND 0xfeffffff
#define VA2PtdL1(x) (((x)&0xff000000)>>PTDL1_SHIFT)
#define VA2PtdL2(x) (((x)&0xfc0000)>>PTDL2_SHIFT)
#define VA2PtdL3(x) (((x)&0x3f000)>>PTDL3_SHIFT)
#define ROUNDUP(r, v) ((((~(r)) + 1)&((v)-1))+(r))

#define PTE_MARK ((xm_u32_t *) (-1))

static void MapArea(
    xm_u32_t ***ptdL1, xmAddress_t vAddr, xmAddress_t pAddr, xmSize_t size,
    xm_s32_t *l2c, xm_s32_t *l3c)
{
    xm_u32_t **ptdL2, **ptdL3;
    xm_s32_t l1e, l2e, l3e;
    xmAddress_t addr;

    /* Check that memory area is aligned and multiple with the size */

    for (addr = vAddr; (addr >= vAddr) && addr < (vAddr + size - 1);)
    {
        l1e = VA2PtdL1(addr);
        l2e = VA2PtdL2(addr);
        l3e = VA2PtdL3(addr);

        xmAddress_t endVAddr;

        /* If no ptdl1 entries */
        if (ptdL1[l1e] == 0)
        {
            endVAddr = addr + LEVEL1_MAPPING_SIZE - 1;

            if ((l2e == 0) && (l3e == 0) && (endVAddr <= (vAddr + size - 1))
                && !(pAddr & ((16 * 1024 * 1024) - 1)) /* Check physical address is aligned */)
            {
                /* Map 16MB or less or virtual address to 16MB of physical address
                 * TODO: Error. The area is mapping more area
                 */
                ptdL1[l1e] = (xm_u32_t **) PTE_MARK; /* Set as used */

                /* Increment counters */
                pAddr += LEVEL1_MAPPING_SIZE;
                addr += LEVEL1_MAPPING_SIZE;
            }
            else
            {
                /* Create PTDL2 */
                l2c[0] += 1;
                DO_MALLOC(ptdL2, sizeof(xm_u32_t *) * PTDL2ENTRIES);
                memset(ptdL2, 0, sizeof(xm_u32_t *) * PTDL2ENTRIES);
                ptdL1[l1e] = ptdL2;
            }
        }
        else
        {
            ptdL2 = ptdL1[l1e];

            if (ptdL2 == (xm_u32_t **) PTE_MARK)
            {
                /* Must not be printed */
                EPrintF("Error Reserving PT memory: Already mapped\n");
            }
            else if (ptdL2[l2e] == 0)
            {
                endVAddr = addr + LEVEL2_MAPPING_SIZE - 1;

                if (!l3e /* Virtual address aligned to 256KB */
                && (endVAddr <= (vAddr + size))
                    && !(pAddr & ((256 * 1024) - 1)))
                {
                    if (ptdL2[l2e] != 0)
                    {
                        /*
                         * TODO: Raise internal error
                         */
                        /* Must not be printed */
                        EPrintF(
                            "Error mapping previously mapped area (256KB)\n");
                    }

                    ptdL2[l2e] = PTE_MARK;
                    pAddr += LEVEL2_MAPPING_SIZE;
                    addr += LEVEL2_MAPPING_SIZE;
                }
                else
                {
                    *l3c += 1;
                    DO_MALLOC(ptdL3, sizeof(xm_u32_t *) * PTDL3ENTRIES);
                    memset(ptdL3, 0, sizeof(xm_u32_t *) * PTDL3ENTRIES);
                    ptdL2[l2e] = (xm_u32_t *) ptdL3;
                }
            }
            else
            {
                /* Map physical address in page table of level 3
                 * 4KB of memory
                 */
                ptdL3 = (xm_u32_t **) (ptdL2[l2e]);
                if ( ptdL3 == (xm_u32_t **) PTE_MARK)
                {
                    /* Must not be printed */
                    EPrintF("Error Reserving PT memory: Already mapped\n");
                }

                /* Check if previous area was mapped in this page
                 * New areas must be mapped only in unmapped areas
                 */
                if (ptdL3[l3e] != 0)
                {
                    EPrintF("Error mapping area previously mapped\n");
                }
                ptdL3[l3e] = PTE_MARK;

                /* Increment counters */
                pAddr += PAGE_SIZE;
                addr += PAGE_SIZE;
            }
        }
    }
}

void ArchMmuRsvMem(FILE *oFile)
{
    xmAddress_t a, b, addr = 0, c;
    unsigned int **ptdL1[PTDL1ENTRIES];
    xm_s32_t e, i, l2c = 0, l3c = 0;

    /* Reserve page table memory for each partition */
    for (e = 0; e < xmc.noPartitions; e++)
    {
        RsvBlock(PTDL1SIZE, PTDL1SIZE, "ptdL1");
        memset(ptdL1, 0, sizeof(unsigned int **) * PTDL1ENTRIES);
        for (i = 0; i < xmcPartitionTab[e].noPhysicalMemoryAreas; i++)
        {
            if (xmcMemAreaTab[i + xmcPartitionTab[e].physicalMemoryAreasOffset].flags
                & XM_MEM_AREA_UNMAPPED)
            {
                continue;
            }

            a =
                xmcMemAreaTab[i + xmcPartitionTab[e].physicalMemoryAreasOffset].mappedAt;

            b = xmcMemAreaTab[i
                        + xmcPartitionTab[e].physicalMemoryAreasOffset].size;
            c =
                xmcMemAreaTab[i + xmcPartitionTab[e].physicalMemoryAreasOffset].startAddr;

            MapArea(ptdL1, a, c, b, &l2c, &l3c);
        }
    }

    /* Memory reserved for XM */
    memset(ptdL1, 0, sizeof(unsigned int **) * PTDL1ENTRIES);
    for (i = 0; i < xmc.hpv.noPhysicalMemoryAreas; i++)
    {
        a = xmcMemAreaTab[i + xmc.hpv.physicalMemoryAreasOffset].mappedAt;
        b = xmcMemAreaTab[i + xmc.hpv.physicalMemoryAreasOffset].size;
        c = xmcMemAreaTab[i + xmc.hpv.physicalMemoryAreasOffset].startAddr;
        MapArea(ptdL1, a, c, b, &l2c, &l3c);
    }

    for (i = 0; i < xmc.noCommChannels; i++)
    {
        xmAddress_t physAddress = xmcCommChannelTab[i].q.address;
        xmSize_t size = xmcCommChannelTab[i].q.size;
        if (xmcCommChannelTab[i].type == XM_SAMPLING_CHANNEL)
        {
            physAddress = xmcCommChannelTab[i].s.address;
            size = xmcCommChannelTab[i].s.size;
        }
        if (physAddress && size)
        {
            a = addr;
            b = size;
            c = physAddress;
            MapArea(ptdL1, a, c, b, &l2c, &l3c);
        }
    }

    printf("Memory for %d PTDTL2 and %d PTDTL3 has been reserved\n", l2c, l3c);
    for (e = 0; e < l2c; e++)
        RsvBlock(PTDL2SIZE, PTDL2SIZE, "ptdL2");
    for (e = 0; e < l3c; e++)
        RsvBlock(PTDL3SIZE, PTDL3SIZE, "ptdL3");

    for (e = 0; e < xmc.noPartitions; e++)
    {
        RsvBlock(PTDL2SIZE, PTDL2SIZE, "partition control tables's PTDL2");
        RsvBlock(PTDL3SIZE, PTDL3SIZE, "partition control tables's PTDL3");
    }
}

#endif
