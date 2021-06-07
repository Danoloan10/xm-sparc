/*
 * $FILE: audit.h
 *
 * Core trace events
 *
 * $VERSION$
 * 
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
  Changelog:
  - [17/09/2015:SPR-170915-01] audit events moved outside the _XM_KERNEL_ scope.
*/

#ifndef _XM_AUDIT_H_
#define _XM_AUDIT_H_

#ifdef CONFIG_AUDIT_EVENTS

#ifdef _XM_KERNEL_
#include <objects/trace.h>
#else
#include <xm_inc/objects/trace.h>
#endif

#ifdef _XM_KERNEL_
#include <assert.h>
extern void RaiseAuditEvent(xm_u8_t partId, xm_u8_t event);
#endif

/* <track id="xm-audit-events"> */
#define XM_AUDIT_START_WATCHDOG 0x1
#define XM_AUDIT_END_WATCHDOG 0x2
#define XM_AUDIT_BEGIN_PARTITION 0x3
#define XM_AUDIT_END_PARTITION 0x4
#define XM_AUDIT_BEGIN_CS 0x5
#define XM_AUDIT_END_CS 0x6
#define XM_AUDIT_BEGIN_IDLE 0x7
#define XM_AUDIT_END_IDLE 0x8
/* </track id="xm-audit-events"> */

#endif

#endif
