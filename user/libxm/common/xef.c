/*
 * $FILE: xef.c
 *
 * XEF
 *
 * $VERSION$
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */
/*
  Changelog:
  - [02/09/2015:SPR-020915-02] 'continue' replaced with equivalent code.
  - [02/09/2015:SPR-020915-10] URead and UWrite moved outside of the scope 
                               of XEF_load_file and XEF_load_custom_file.
*/
#include <xef.h>
#include <xm_inc/digest.h>
#include <xm_inc/compress.h>
#include <xm_inc/xmef.h>

#include <endianess.h>

#undef OFFSETOF
#ifdef __compiler_offsetof
#define OFFSETOF(_type, _member) __compiler_offsetof(_type,_member)
#else
#ifdef HOST
#define OFFSETOF(_type, _member) (unsigned long)(&((_type *)0)->_member)
#else
#define OFFSETOF(_type, _member) PTR2ADDR(&((_type *)0)->_member)
#endif
#endif

xm_s32_t XEF_parse_file(xm_u8_t *img, struct xefFile *xefFile, struct xefOps *xefOps) {
#if 0
    xm_u8_t digest[XM_DIGEST_BYTES];
    struct digestCtx digestState;
    xm_s32_t e;
#endif
    xefFile->hdr=(struct xefHdr *)img;
    if (RWORD(xefFile->hdr->signature)!=XEF_SIGNATURE)
	return XEF_BAD_SIGNATURE;

    if ((XM_GET_VERSION(RWORD(xefFile->hdr->version))!=XEF_VERSION)&&
	(XM_GET_SUBVERSION(RWORD(xefFile->hdr->version))!=XEF_SUBVERSION))
	return XEF_INCORRECT_VERSION;

    xefFile->segmentTab=(struct xefSegment *)(img+RWORD(xefFile->hdr->segmentTabOffset));
    xefFile->customFileTab=(struct xefCustomFile *)(img+RWORD(xefFile->hdr->customFileTabOffset));
#ifdef CONFIG_IA32
    xefFile->relTab=(struct xefRel *)(img+RWORD(xefFile->hdr->relOffset));
    xefFile->relaTab=(struct xefRela *)(img+RWORD(xefFile->hdr->relaOffset));
#endif
    xefFile->image=img+RWORD(xefFile->hdr->imageOffset);
    xefFile->ops=xefOps;
#if 0
    if (RWORD(xefFile->hdr->flags)&XEF_DIGEST) {
	for (e=0; e<XM_DIGEST_BYTES; e++)
	    digest[e]=0;
	DigestInit(&digestState);
	DigestUpdate(&digestState, img, OFFSETOF(struct xefHdr, digest));
	DigestUpdate(&digestState, (xm_u8_t *)digest, XM_DIGEST_BYTES);
	DigestUpdate(&digestState, &img[(xm_u32_t)OFFSETOF(struct xefHdr, payload)], RWORD(xefFile->hdr->fileSize)-(xm_u32_t)OFFSETOF(struct xefHdr, payload));
	DigestFinal(digest, &digestState);

	for (e=0; e<XM_DIGEST_BYTES; e++) {
	    if (digest[e]!=xefFile->hdr->digest[e])
		return XEF_UNMATCHING_DIGEST;	
	}
    }
#endif
    return XEF_OK;
}

#ifndef HOST
static xm_s32_t URead(void *obj, void *b, xmSize_t s, void *d) {
    ((struct xefFile *)obj)->ops->Read(b, *(xm_u8_t **)d, s);
    *(xm_u8_t **)d+=s;
    return s;
}

static xm_s32_t UWrite(void *obj, void *b, xmSize_t s, void *d) {
    ((struct xefFile *)obj)->ops->Write(*(xm_u8_t **)d, b, s);
    *(xm_u8_t **)d+=s;
    return s;
}

void *XEF_load_file(struct xefFile *xefFile) {
    xm_s32_t e;
    xmAddress_t addr;
    if (xefFile->hdr->noSegments<=0)
        return 0;

    for (e=0; e<xefFile->hdr->noSegments; e++) {
        if (xefFile->segmentTab[e].fileSize){
            addr=xefFile->segmentTab[e].physAddr;
            if (xefFile->ops->VAddr2PAddr)
                xefFile->ops->VAddr2PAddr(xefFile->ops->mAreas, xefFile->ops->noAreas, addr, &addr);
            if (xefFile->hdr->flags&XEF_COMPRESSED) {
                xm_u8_t *rPtr, *wPtr;
                rPtr=(xm_u8_t *)&xefFile->image[xefFile->segmentTab[e].offset];
                wPtr=(xm_u8_t *)ADDR2PTR(addr);
                if (Uncompress(xefFile->segmentTab[e].deflatedFileSize, xefFile->segmentTab[e].fileSize, URead, &rPtr, UWrite, &wPtr, xefFile)<0) {
                    return 0;
                }
            } else {
                xefFile->ops->Write(ADDR2PTR(addr),
                                    &xefFile->image[xefFile->segmentTab[e].offset], xefFile->segmentTab[e].fileSize);
            }
        }
    }
    addr=xefFile->hdr->xmImageHdr;
    if (xefFile->ops->VAddr2PAddr)
        xefFile->ops->VAddr2PAddr(xefFile->ops->mAreas, xefFile->ops->noAreas, addr, &addr);
    return ADDR2PTR(addr);
}

void *XEF_load_custom_file(struct xefFile *xefCustomFile, struct xefCustomFile *customFile) {
    xm_s32_t e;
    if (xefCustomFile->hdr->noSegments<=0)
        return 0;

    for (e=0; e<xefCustomFile->hdr->noSegments; e++) {
	if (xefCustomFile->hdr->flags&XEF_COMPRESSED) {
            xm_u8_t *rPtr, *wPtr;
            rPtr=(xm_u8_t *)&xefCustomFile->image[xefCustomFile->segmentTab[e].offset];
            wPtr=(xm_u8_t *)ADDR2PTR(customFile->sAddr);
	    if (Uncompress(xefCustomFile->segmentTab[e].deflatedFileSize, xefCustomFile->segmentTab[e].fileSize, URead, &rPtr, UWrite, &wPtr, xefCustomFile)<0)
		return 0;
	} else {
	    xefCustomFile->ops->Write(ADDR2PTR(customFile->sAddr), &xefCustomFile->image[xefCustomFile->segmentTab[e].offset], xefCustomFile->segmentTab[e].fileSize);
        }
    }
  
    return ADDR2PTR(customFile->sAddr);
}
#endif

