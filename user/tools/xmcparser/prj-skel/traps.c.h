/*
 * $FILE: traps.c.h
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

#ifndef _TRAPS_C_H_
#define _TRAPS_C_H_

char trapsFile[]="#include <xm.h>\n\n"

"typedef struct trapCtxt {\n"
    "\txm_u32_t y;\n"
    "\txm_u32_t g1;\n"
    "\txm_u32_t g2;\n"
    "\txm_u32_t g3;\n"
    "\txm_u32_t g4;\n"
    "\txm_u32_t g5;\n"
    "\txm_u32_t g6;\n"
    "\txm_u32_t g7;\n"
    "\txm_u32_t nPc;\n"
    "\txm_u32_t irqNr;\n"
    "\txm_u32_t flags;\n"
    "\txm_u32_t pc;\n"
"} trapCtxt_t;\n\n"

"void (*trapHandlersTab[256+32])(trapCtxt_t *);\n";

#endif
