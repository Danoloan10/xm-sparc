/*
 * $FILE: main.c.h
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

#ifndef _MAIN_C_H_
#define _MAIN_C_H_

char mainFile[]="#include <xm.h>\n\n"

"partitionControlTable_t partitionControlTable __attribute__ ((section (\".xm_ctrl\"), aligned(0x1000)));\n"
"partitionInformationTable_t partitionInformationTable __attribute__ ((section (\".xm_ctrl\"), aligned(0x1000)));\n\n"

"void PartitionMain(void) {\n"
    "\tXM_halt_partition(XM_PARTITION_SELF);\n"
"}\n";

#endif
