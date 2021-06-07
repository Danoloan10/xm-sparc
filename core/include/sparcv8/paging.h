/*
 * $FILE: paging.h
 *
 * SPARC V8 paging version, empty so far, since LEON doesn't support paging
 *
 * $VERSION$
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_ARCH_PAGING_H_
#define _XM_ARCH_PAGING_H_

#define PTD_LEVELS 3

#define PAGE_SIZE 4096
#define LPAGE_SIZE PAGE_SIZE
#define PAGE_MASK (~(PAGE_SIZE-1))
#define PAGE_SHIFT 12

#define PTDL1_SHIFT 24
#define PTDL2_SHIFT 18
#define PTDL3_SHIFT 12
#define PTDL1SIZE 1024
#define PTDL2SIZE 256
#define PTDL3SIZE 256

#define PTDL1ENTRIES (PTDL1SIZE>>2)
#define PTDL2ENTRIES (PTDL2SIZE>>2)
#define PTDL3ENTRIES (PTDL3SIZE>>2)

#define XM_PCTRLTAB_ADDR (CONFIG_XM_OFFSET+16*1024*1024)

#ifdef _XM_KERNEL_

#ifndef __ASSEMBLY__
#define _VIRT2PHYS(x) ((xm_u32_t)(x)-CONFIG_XM_OFFSET+CONFIG_XM_LOAD_ADDR)
#define _PHYS2VIRT(x) ((xm_u32_t)(x)+CONFIG_XM_OFFSET-CONFIG_XM_LOAD_ADDR)

extern xmAddress_t contextTab[];
extern xmAddress_t _pgTables[], _ptdL1[], _ptdL2[], _ptdL3[];

#define VA2PtdL1(x) (((x)&0xff000000)>>PTDL1_SHIFT)
#define VA2PtdL2(x) (((x)&0xfc0000)>>PTDL2_SHIFT)
#define VA2PtdL3(x) (((x)&0x3f000)>>PTDL3_SHIFT)
#define PtdL1L2L32VA(l1, l2, l3) (((l1)<<PTDL1_SHIFT)|((l2)<<PTDL2_SHIFT)|((l3)<<PTDL3_SHIFT))
#else
#define _VIRT2PHYS(x) ((x)-CONFIG_XM_OFFSET+CONFIG_XM_LOAD_ADDR)
#define _PHYS2VIRT(x) ((x)+CONFIG_XM_OFFSET-CONFIG_XM_LOAD_ADDR)

#endif

#define _PG_ARCH_PTE_PRESENT 0x002
#define _PG_ARCH_PTD_PRESENT 0x001
#define _PG_ARCH_RW (7<<2)
#define _PG_ARCH_R (6<<2)
#define _PG_ARCH_RW_USER (3<<2)
#define _PG_ARCH_R_USER (2<<2)
#define _PG_ARCH_CACHE (1<<7)

#ifdef CONFIG_MMU
#define XM_VMAPSTART CONFIG_XM_OFFSET
#define XM_VMAPEND 0xffffffff
#define XM_VMAPSIZE ((XM_VMAPEND-XM_VMAPSTART)+1)
#define XM_PTDL1_START VA2PtdL1(CONFIG_XM_OFFSET)
#define XM_PTDL1_END 256

#define IS_PTD_PRESENT(x) (((x)&0x3)==_PG_ARCH_PTD_PRESENT)
#define IS_PTE_PRESENT(x) (((x)&0x3)==_PG_ARCH_PTE_PRESENT)
#define SET_PTD_NOT_PRESENT(x) ((x)&~0x3)
#define SET_PTE_NOT_PRESENT(x) ((x)&~0x3)
#define SET_PTE_RONLY(x) (((x)&~(7<<2))|(5<<2))
#define SET_PTE_UNCACHED(x) ((x)&~_PG_ARCH_CACHE)
#define GET_PTD_ADDR(x) (((x)<<4)&PAGE_MASK)
#define GET_PTE_ADDR(x) (((x)<<4)&PAGE_MASK)
#define GET_USER_PTD_ENTRIES(type) VA2PtdL1(CONFIG_XM_OFFSET)

#define GET_USER_PTE_ENTRIES(type) PTDL2ENTRIES
#define CLONE_XM_PTD_ENTRIES(type, vPtd, k) SetupPtdL1(vPtd, k)
#define IS_VALID_PTD_PTR(type, pAddr) \
    (((type)==PPAG_PTDL1)&&(((pAddr)>>2)&(PTDL1ENTRIES-1))<VA2PtdL1(CONFIG_XM_OFFSET))

#define IS_VALID_PTD_ENTRY(type) ((type)==PPAG_PTDL2)
#define IS_VALID_PTE_ENTRY(type) (1)

#endif

#endif
#endif
