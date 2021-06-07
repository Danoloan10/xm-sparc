/*
 * $FILE: lds.h
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


/*OUTPUT_FORMAT("binary")*/
/*OUTPUT_FORMAT("elf32-sparc", "elf32-sparc", "elf32-sparc")
OUTPUT_ARCH(sparc)*/

#ifndef _LDS_H_
#define _LDS_H_

char ldsFile[]=
"OUTPUT_FORMAT(\"elf32-sparc\", \"elf32-sparc\", \"elf32-sparc\")\n"
"OUTPUT_ARCH(sparc)\n"
"ENTRY(start)\n\n"
"START_ADDR = 0x%lx;\n\n"
"SECTIONS\n"
"{\n"
  "\t. = START_ADDR;\n"
  "\t.text ALIGN (4): {\n"
	"\t\t. = ALIGN(4K);\n"
	"\t\t_sguest = .;\n"
  	"\t\t*(.text.init)\n"
  	"\t\t*(.text)\n"
  "\t}\n\n"

  "\t.rodata ALIGN (4) : {\n"
	"\t\t*(.rodata)\n"
  	"\t\t*(.rodata.*)\n"
	"\t\t*(.rodata.*.*)\n"
  "\t}\n\n"
  
  "\t.data ALIGN (4) : {\n"
	"\t\t_sdata = .;\n"
	"\t\t*(.data)\n"
	"\t\t_edata = .;\n"
  "\t}\n\n"

  "\t.bss : {\n"
  	"\t\t*(.xm_ctrl)\n"
	"\t\t_sbss = .;\n"
	"\t\t*(COMMON)\n"
	"\t\t*(.bss)\n"
	"\t\t_ebss = .;\n"
  "\t}\n\n"
  
  "\t_eguest = .;\n\n"

  "\t/DISCARD/ :\n"
  "\t{\n"
	"\t\t*(.note)\n"
	"\t\t*(.comment*)\n"
  "\t}\n"
"}\n";

#endif
