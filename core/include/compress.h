/*
 * $FILE: compress.h
 *
 * compression functions
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
  - [02/09/2015:SPR-020915-10] Additional data is passed to URead and UWrite
*/
#ifndef _LIBXEF_COMPRESS_H_
#define _LIBXEF_COMPRESS_H_

#define COMPRESS_BAD_MAGIC -1
#define COMPRESS_BUFFER_OVERRUN -2
#define COMPRESS_ERROR_LZ -3
#define COMPRESS_READ_ERROR -4
#define COMPRESS_WRITE_ERROR -5

typedef xm_s32_t (*CFunc_t)(void *obj, void *buffer, xmSize_t size, void *data);

extern xm_s32_t Compress(xm_u32_t inSize, xm_u32_t outSize, CFunc_t Read, void *rData, CFunc_t Write, void *wData, void (*SeekW)(xmSSize_t offset, void *wData), void *obj);
extern xm_s32_t Uncompress(xm_u32_t inSize, xm_u32_t outSize, CFunc_t Read, void *rData, CFunc_t Write, void *wData, void *obj);

#endif
