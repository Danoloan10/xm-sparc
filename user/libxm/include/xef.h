/*
 * $FILE: xef.h
 *
 * XM's executable format helper functions
 *
 * $VERSION$
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _LIB_XM_XEF_H_
#define _LIB_XM_XEF_H_

#include <xm_inc/xmef.h>

#define XEF_OK 0
#define XEF_BAD_SIGNATURE -1
#define XEF_INCORRECT_VERSION -2
#define XEF_UNMATCHING_DIGEST -3

struct xefOps {
    void *(*Read)(void *dest, const void *src, unsigned long n);
    void *(*Write)(void *dest, const void *src, unsigned long n);
    void (*VAddr2PAddr)(void *, xm_s32_t, xmAddress_t, xmAddress_t *);
    void *mAreas;
    xm_s32_t noAreas;
};

struct xefFile {
    struct xefHdr *hdr;
    struct xefSegment *segmentTab;
    struct xefCustomFile *customFileTab;
    xm_u8_t *image;
    struct xefOps *ops;
};

extern xm_s32_t XEF_parse_file(xm_u8_t *img, struct xefFile *xefFile, struct xefOps *xefOps);
extern void *XEF_load_file(struct xefFile *xefFile);
extern void *XEF_load_custom_file(struct xefFile *xefCustomFile, struct xefCustomFile *customFile);

#endif
