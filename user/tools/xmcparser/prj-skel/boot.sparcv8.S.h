/*
 * $FILE: boot.sparcv8.S.h
 *
 * Generates the project skeleton
 *
 * $VERSION$
 *
 * Authors: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _BOOT_SPARCV8_S_H_
#define _BOOT_SPARCV8_S_H_

char bootFile[]="#include <xm.h>\n"
"#include <xm_inc/arch/asm_offsets.h>\n\n"

"#define STACK_SIZE 8192\n"
"#define MIN_STACK_FRAME 0x60\n\n"
	
".section .text.init, \"ax\"\n"
".align 8\n"
".global __xmImageHdr\n"
"__xmImageHdr:\n"
	"\t.long XMEF_PARTITION_MAGIC\n"
	"\t.long XM_SET_VERSION(XM_ABI_VERSION, XM_ABI_SUBVERSION, XM_ABI_REVISION) // XM's ABI version\n"
	"\t.long XM_SET_VERSION(XM_API_VERSION, XM_API_SUBVERSION, XM_API_REVISION) // XM's API version\n"
	"\t.long 0\n"
	"\t.long _sguest\n"
	"\t.long _eguest\n"
	"\t.long __xmPartitionHdr\n"
	"\t.long 0\n"
	"\t.long 0\n\n"

".align 8\n"
".global __xmPartitionHdr\n"
"__xmPartitionHdr:\n"
	"\t.long XMEF_PARTITION_HDR_MAGIC\n"
	"\t.long start\n"
	"\t.long __xmImageHdr\n"
	"\t.long partitionControlTable\n"
	"\t.long partitionInformationTable\n"
"#ifdef CONFIG_MMU\n"
	"\t.long _pgdAddr\n"
	"\t.long (_pgdAddrEnd-_pgdAddr)\n"
"#endif\n\n"
".text\n"
".align 8\n"

".global start, _start\n\n"

"_start:\n"
"start:\n"
	"\tset _sbss, %%o0\n"
	"\tset _ebss, %%o1\n"

"1:	st %%g0, [%%o0]\n"
	"\tsubcc %%o0, %%o1, %%g0\n"
	"\tbl 1b\n"
	"\tadd %%o0, 0x4, %%o0\n"
	
	"\tset __stack_top, %%fp\n"

	"\tset __xmPartitionHdr, %%o0\n"
	"\tld [%%o0+_PARTITIONINFORMATIONTABLE_OFFSET], %%o1\n"
	"\tld [%%o0+_PARTITIONCONTROLTABLE_OFFSET], %%o0\n"
	"\tcall init_libxm\n"
	"\tsub %%fp, MIN_STACK_FRAME, %%sp\n"

	"\tset write_register32_nr, %%o0\n"
	"\tmov %%g0, %%o1\n"
	"\tset _traptab, %%o2\n"
	"\t__XM_HC\n"

        "\tcall PartitionMain\n"

	"\tsub %%fp, MIN_STACK_FRAME, %%sp\n"

	"\tsethi %%hi(partitionInformationTable), %%o1\n"
	"\tor %%o1, %%lo(partitionInformationTable), %%o1\n"
	"\tld [%%o1+ _ID_OFFSET], %%o1\n"
	"\tmov halt_partition_nr, %%o0\n"
	"\t__XM_HC\n"

"1:	b 1b\n"
	"\tnop\n\n"

"CommonTrapHandler:\n"
	"\tset sparcv8_flush_regwin_nr, %%o0\n"
	"\t__XM_AHC\n"
	"\tsub %%fp, 48, %%fp\n"
	"\tstd %%l0, [%%fp+40]\n"
	"\tstd %%l2, [%%fp+32]\n"
	"\tstd %%g6, [%%fp+24]\n"
	"\tstd %%g4, [%%fp+16]\n"
	"\tstd %%g2, [%%fp+8]\n"
	"\tst %%g1, [%%fp+4]\n"
	"\trd %%y, %%g5\n"
	"\tst %%g5, [%%fp]\n"
	
	"\tset trapHandlersTab, %%g1\n"
	"\tsll %%l3, 2, %%g2\n"
	"\tld [%%g1+%%g2], %%g1\n"
	
	"\tcmp %%g1, 0\n"
	"\tbe 1f\n"
	"\tnop\n"

	"\tmov %%fp, %%o0\n"
	
	"\tcall %%g1\n"
	"\tsub %%fp, MIN_STACK_FRAME, %%sp\n"

"1:	 call XM_exec_pendirqs\n"
	"\tsub %%fp, MIN_STACK_FRAME, %%sp\n"

	"\tld [%%fp], %%g1\n"
        "\twr %%g1, %%y\n"
        "\tld [%%fp+4], %%g1\n"
        "\tldd [%%fp+8], %%g2\n"
        "\tldd [%%fp+16], %%g4\n"
        "\tldd [%%fp+24], %%g6\n"
        "\tldd [%%fp+32], %%l2\n"
        "\tldd [%%fp+40], %%l0\n"
	
        "\tadd %%fp, 48, %%fp\n"
	"\tset sparcv8_iret_nr, %%o0\n"
	"\t__XM_AHC\n\n"

".macro BUILD_TRAP trapNr\n"
	"\tb CommonTrapHandler\n"
	"\tmov \\trapNr\\(), %%l3\n"
	"\tnop\n"
	"\tnop\n"
".endm\n\n"
	
".macro BUILD_TRAP_BLOCK stTrapNr endTrapNr\n"
"vector=\\stTrapNr\\()\n"
".rept \\endTrapNr\\()-\\stTrapNr\\()+1\n"
	"\tBUILD_TRAP vector\n"
"vector=vector+1\n"
".endr\n"
".endm\n\n"
	
".align 4096\n"
".global _traptab\n"
"_traptab:\n"
"! + 0x00: reset\n"
"t_reset: b start\n"
	"\tnop\n"
	"\tnop\n"
	"\tnop\n"

	"\tBUILD_TRAP_BLOCK 0x1 0xff\n\n"

".data\n"
".align 8\n"
"__stack:\n"
        "\t.fill (STACK_SIZE/4),4,0\n"
"__stack_top:\n\n"

".previous\n"
"#ifdef CONFIG_MMU\n"
".section .xm_ctrl, \"w\"\n"
".align 4096\n"
"_pgdAddr:\n"
	"\t.zero 1024 // ptdL1\n"
".align 4096\n"
	"\t.zero 4096*10 // ptdL2&ptdL3\n"
"_pgdAddrEnd:\n"
".previous\n"
"#endif\n";

#endif
