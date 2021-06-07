/*
 * $FILE: rsw.c
 *
 * A boot rsw
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
  - [02/09/2015:SPR-280715-01] xmcBootPart removed
  - [02/10/2015:SPR-021015-01] XM_CF partition entry point is updated with 
       XEF value when CONFIG_USE_XEF_EPOINT is set
*/
#include <xm.h>
#include <container.h>
#include <xef.h>
#include <xm_inc/arch/arch_types.h>
#include <xm_inc/xmconf.h>
#include <xm_inc/compress.h>
#include <xm_inc/digest.h>
#include <rsw_stdc.h>

void HaltSystem(void) {
    extern void _halt_system(void);

    xprintf("[RSW] System Halted.\n");
    _halt_system();
}

void DoTrap(xm_u32_t noTrap) {
    xprintf("[RSW] Unexpected trap: %d\n", noTrap);
    HaltSystem();
}

static int XmcVAddr2PAddr(struct xmcMemoryArea *mAreas, xm_s32_t noAreas, xmAddress_t vAddr, xmAddress_t *pAddr) {
    xm_s32_t e;
    for (e=0; e<noAreas; e++) {
        if ((mAreas[e].mappedAt<=vAddr)&&(((mAreas[e].mappedAt+mAreas[e].size)-1)>=vAddr)) {
            *pAddr=vAddr-mAreas[e].mappedAt+mAreas[e].startAddr;
            return 0;
        }
    }
    return -1;
}

static int RId2Id(int partId, struct xmc *xmcTab, struct xmcPartition *xmcPartitionTab) {
    int n;
    //xmcPartitionTab=(struct xmcPartition *)&((char *)xmcTab)[RWORD(xmcTab->partitionTabOffset)];
    for (n=0; n<xmcTab->noPartitions; n++) {
        if (xmcPartitionTab[n].rId==partId)
            return xmcPartitionTab[n].id;
    }
    return xmcTab->noPartitions;
}

xmAddress_t hpvEntryPoint;
void RSwMain(void) {
    extern xm_u8_t *xmefContainerPtr;
    extern void start(void);
    struct xefContainerFile container;
    struct xefFile xefFile, xefCustomFile;
    struct xmHdr *xmHdr;
    struct xmc *xmc;
    struct xmcPartition *xmcPartitions;
    struct xmcMemoryArea *xmcMemArea;
    struct xefOps xefOps;
    //xmAddress_t hpvEntryPoint;
    xm_s32_t ret, min, e, i;

    xefOps.Read=memcpy;
    xefOps.Write=memcpy;
    xefOps.VAddr2PAddr=0;

    InitOutput();
    xprintf("[RSW] Start Resident Software\n");
    // Parse container
    if ((ret=XEF_parse_container(xmefContainerPtr, &container))!=CONTAINER_OK) {
	xprintf("[RSW] Error %d when parsing container file\n", ret);
	HaltSystem();
    }

    // Loading XM and the XML configuration file    
    if ((ret=XEF_parse_file((xm_u8_t *)(container.fileTab[container.partitionTab[0].file].offset+xmefContainerPtr), &xefFile, &xefOps))!=XEF_OK) {
	xprintf("[RSW] Error %d when parsing XEF file\n", ret);
	HaltSystem();
    }

    // Checking xml configuration file's digestion correctness
    if ((ret=XEF_parse_file((xm_u8_t *)(container.fileTab[container.partitionTab[0].customFileTab[0]].offset+xmefContainerPtr), &xefCustomFile, &xefOps))!=XEF_OK) {
	xprintf("[RSW] Error %d when parsing XEF file\n", ret);
	HaltSystem();
    }

    xmc=XEF_load_custom_file(&xefCustomFile, &xefFile.customFileTab[0]);

    if (xmc->signature!=XMC_SIGNATURE) {
        xprintf("[RSW] XMC signature not found\n");
	HaltSystem();
    }    

    xmHdr=XEF_load_file(&xefFile);
    hpvEntryPoint=xefFile.hdr->entryPoint;
    if ((xmHdr->sSignature!=XMEF_XM_MAGIC)||(xmHdr->eSignature!=XMEF_XM_MAGIC)) {
	xprintf("[RSW] XtratuM signature not found\n");
	HaltSystem();
    }

    // Loading additional custom files
    min=(container.partitionTab[0].noCustomFiles>xmHdr->noCustomFiles)?xmHdr->noCustomFiles:container.partitionTab[0].noCustomFiles;
    
    for (e=1; e<min; e++) {
        if ((ret=XEF_parse_file((xm_u8_t *)(container.fileTab[container.partitionTab[0].customFileTab[e]].offset+xmefContainerPtr), &xefCustomFile, &xefOps))!=XEF_OK) {
            xprintf("[RSW] Error %d when parsing XEF file\n", ret);
            HaltSystem();
        }
        XEF_load_custom_file(&xefCustomFile, &xefFile.customFileTab[e]);
    }

    xmcPartitions=(struct xmcPartition *)((xmAddress_t)xmc+xmc->partitionTabOffset);
    xmcMemArea=(struct xmcMemoryArea *)((xmAddress_t)xmc+xmc->physicalMemoryAreasOffset);
    // Loading partitions
    for (e=1; e<container.hdr->noPartitions; e++) {
        struct xmImageHdr *partHdr;
        int id = RId2Id(container.partitionTab[e].rId, xmc, xmcPartitions);
        xefOps.VAddr2PAddr=(void (*)(void *, xm_s32_t, xmAddress_t, xmAddress_t *))XmcVAddr2PAddr;
        xefOps.mAreas=&xmcMemArea[xmcPartitions[id].physicalMemoryAreasOffset];
        xefOps.noAreas=xmcPartitions[id].noPhysicalMemoryAreas;
        if ((ret=XEF_parse_file((xm_u8_t *)(container.fileTab[container.partitionTab[e].file].offset+xmefContainerPtr), &xefFile, &xefOps))!=XEF_OK) {
	    xprintf("[RSW] Error %d when parsing XEF file\n", ret);
	    HaltSystem();
	}
        partHdr=XEF_load_file(&xefFile);
        if ((partHdr->sSignature!=XMEF_PARTITION_MAGIC)&&
            (partHdr->eSignature!=XMEF_PARTITION_MAGIC)) {
	    xprintf("[RSW] Partition signature not found (0x%x)\n", partHdr->sSignature);
	    HaltSystem();
	}
#ifdef CONFIG_USE_XEF_EPOINT
        xmcPartitions[id].entryPoint = xefFile.hdr->entryPoint;
#endif
        // Loading additional custom files
        min=(container.partitionTab[e].noCustomFiles>partHdr->noCustomFiles)?partHdr->noCustomFiles:container.partitionTab[e].noCustomFiles;
        for (i=0; i<min; i++) {
            if ((ret=XEF_parse_file((xm_u8_t *)(container.fileTab[container.partitionTab[e].customFileTab[i]].offset+xmefContainerPtr), &xefCustomFile, &xefOps))!=XEF_OK) {
                xprintf("[RSW] Error %d when parsing XEF file\n", ret);
                HaltSystem();
            }
            xefOps.VAddr2PAddr=0;
            XEF_load_custom_file(&xefCustomFile, &xefFile.customFileTab[i]);
        }
    }

    xprintf("[RSW] Starting XM at 0x%x \n", hpvEntryPoint);
    ((void (*)(void))ADDR2PTR(hpvEntryPoint))();
}

