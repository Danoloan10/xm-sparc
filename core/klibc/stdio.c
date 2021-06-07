/*
 * $FILE: stdio.c
 *
 * Standard buffered input/output
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
 * - printf() based on sprintf() from gcctest9.c Volker Oth
 * - Changes made by Holger Klabunde
 * - Changes made by Martin Thomas for the efsl debug output
 * - Changes made by speiro for lpc2000 devices for PaRTiKle
 */
/*
  Changelog:
  - [02/09/2015:SPR-020915-01] MISRA rules.9.1 violation fixed.
  - [02/09/2015:SPR-120515-03] Removed commented defines and their 
                               associated code.
  - [02/09/2015:SPR-020915-02] 'continue' replaced with equivalent code.
  - [16/11/2015:SPR-290915-01] DivMod64 replaced a/b operation for 64-bit 
     integers
*/
#include <stdc.h>
#include <spinlock.h>
#include <objects/console.h>

#define SCRATCH 20
#define USE_LONG	// %lx, %Lu and so on, else only 16 bit
			// %integer is allowed

#define USE_STRING			// %s, %S Strings as parameters
#define USE_CHAR	// %c, %C Chars as parameters
#define USE_INTEGER // %i, %I Remove this format flag. %d, %D does the same
#define USE_HEX		// %x, %X Hexadecimal output

#define PADDING					//SPACE and ZERO padding

typedef struct{
    xm_s32_t (*_putC)(xm_s32_t c, void *a);
    void *a;
} fPrint_t;

static xm_u8_t *ConversionLoop(fPrint_t *fp, va_list *args, 
                           xm_u8_t issigned, xm_u16_t base,
                           xm_u8_t *ptr, 
#ifdef USE_LONG
                           xm_u8_t islong, xm_u8_t isvlong, 
                           xm_u64_t u_val, xm_s64_t s_val, 
#else
                           xm_u32_t u_val, xm_s32_t s_val, 
#endif
                           xm_u8_t width, xm_u8_t fill, xm_u8_t *scratch) {
    if(issigned) { //Signed types
        
#ifdef USE_LONG
        if(isvlong) { s_val = va_arg((*args), xm_s64_t); }
        else if(islong) { s_val = va_arg((*args), xm_s32_t); }
        else { s_val = va_arg((*args), xm_s32_t); }
#else
        s_val = va_arg((*args), xm_s32_t);
#endif
        
        if(s_val < 0) { //Value negativ ?
            s_val = - s_val; //Make it positiv
            fp->_putC('-', fp->a);		 //Output sign
        }
	
        if(!isvlong)
            u_val = (xm_u32_t)s_val;
        else
            u_val = (xm_u64_t)s_val;
    } else {//Unsigned types
#ifdef USE_LONG
        if(isvlong) {u_val = va_arg((*args), xm_u64_t); }
        else if(islong) { u_val = va_arg((*args), xm_u32_t); }
        else { u_val = va_arg((*args), xm_u32_t); }
#else
        u_val = va_arg((*args), xm_u32_t);
#endif
    }
    
    ptr = scratch + SCRATCH;
    *--ptr = 0;
    do {
        xm_u64_t r;
        DivMod64(u_val, base, &r);
        char ch = r + '0';
#ifdef USE_HEX
        if (ch > '9') {
            ch += 'a' - '9' - 1;
        }
#endif					
        *--ptr = ch;
        u_val = DivMod64(u_val, base, 0);
	
#ifdef PADDING
        if(width) width--; //calculate number of padding chars
#endif
    } while (u_val);
    
#ifdef PADDING
    while(width--) *--ptr = fill; //insert padding chars					
#endif
    
    while(*ptr) { fp->_putC(*ptr, fp->a); ptr++; }
    return ptr;
}

static void __PrintFmt(fPrint_t *fp, const char *fmt, va_list args) {
    xm_u8_t scratch[SCRATCH];
    xm_u8_t fmtFlag;
    xm_u16_t base;
    xm_u8_t *ptr = NULL;
    xm_u8_t issigned=0;
  
#ifdef USE_LONG
    xm_u8_t islong=0;
    xm_u8_t isvlong=0;
    xm_u64_t u_val=0;
    xm_s64_t s_val=0;
#else
    xm_u32_t u_val=0;
    xm_s32_t s_val=0;
#endif
  
    xm_u8_t fill;
    xm_u8_t width;

    for (;;) {
	while ((fmtFlag = *(fmt++)) != '%') {			 // Until '%' or '\0' 
	    if (!fmtFlag){ return; }
	    fp->_putC(fmtFlag, fp->a);
	}
    
	issigned=0; //default unsigned
	base = 10;
    
	fmtFlag = *fmt++; //get char after '%'
    
#ifdef PADDING
	width=0; //no formatting
	fill=0;	 //no formatting
    
	if(fmtFlag=='0' || fmtFlag==' ') { //SPACE or ZERO padding	?      
	    fill=fmtFlag;
	    fmtFlag = *fmt++; //get char after padding char
	    while(fmtFlag>='0' && fmtFlag<='9') {
		width = 10*width + (fmtFlag-'0');
		fmtFlag = *fmt++; //get char after width char
	    }
	}
#endif
    
#ifdef USE_LONG
	islong=0; //default int value
	isvlong=0;
        if(fmtFlag=='l') //Long value 
        {
            islong=1;
            fmtFlag = *fmt++; //get char after 'l' or 'L'
            if (fmtFlag=='l'){
                isvlong = 1;
                fmtFlag = *fmt++; //get char after 'l' or 'L'
            }
        }
#endif
    
	switch (fmtFlag) {
#ifdef USE_CHAR
	case 'c':
	    fmtFlag=va_arg(args, xm_s32_t);
	    // no break -> run into default
#endif
      
	default:
	    fp->_putC(fmtFlag, fp->a);
	    break;
      
#ifdef USE_STRING
	case 's':
	    ptr=(xm_u8_t*)va_arg(args, char *);
	    while(*ptr) { fp->_putC(*ptr, fp->a); ptr++; }
	    break;
#endif
      
#ifdef USE_INTEGER //don't use %i, is same as %d
	case 'i':
#endif
	case 'd':
	    issigned=1;
	    // no break -> run into next case
	case 'u':
            ptr = ConversionLoop(fp, &args, issigned, base, ptr, 
#ifdef USE_LONG
                           islong, isvlong, u_val, s_val, 
#else
                           u_val, s_val, 
#endif
                           width, fill, scratch);
            break;
#ifdef USE_HEX
	case 'x':
	    base = 16;
            ptr = ConversionLoop(fp, &args, issigned, base, ptr, 
#ifdef USE_LONG
                                 islong, isvlong, u_val, s_val, 
#else
                                 u_val, s_val, 
#endif
                                 width, fill, scratch);            
            break;
#endif
	}
    }
}

static xm_s32_t PrintFPutC(xm_s32_t c, void *a){
    xm_u32_t *nc=(xm_u32_t*)a;
    ConsolePutChar(c);
    (*nc)++;
    return 1;
}

static spinLock_t vpSpin=SPINLOCK_INIT;
xm_s32_t vprintf(const char *fmt, va_list args) {
    xm_s32_t nc=0;
    fPrint_t fp = {PrintFPutC, (void*)&nc};
    xmWord_t flags;
    SpinLockIrqSave(&vpSpin, flags);
    __PrintFmt(&fp, fmt, args);
    SpinUnlockIrqRestore(&vpSpin, flags);
    return nc;
}

xm_s32_t kprintf(const char *format,...) {
    va_list argPtr = 0;
    xm_s32_t n;

    va_start(argPtr, format);
    n=vprintf(format, argPtr);
    va_end(argPtr);
    
    return n;
}

#ifdef CONFIG_EARLY_OUTPUT

static xm_s32_t EarlyPrintFPutC(xm_s32_t c, void *a){
    extern void EarlyPutChar(xm_u8_t c);
    xm_u32_t *nc=(xm_u32_t*)a;
    EarlyPutChar(c);
    (*nc)++;
    return 1;
}

static xm_s32_t EarlyVPrintf(const char *fmt, va_list args) {
    xm_s32_t nc=0;
    fPrint_t fp = {EarlyPrintFPutC, (void*)&nc};
    xmWord_t flags;
    
    SpinLockIrqSave(&vpSpin, flags);
    __PrintFmt(&fp, fmt, args);
    SpinUnlockIrqRestore(&vpSpin, flags);
    return nc;
}

#endif

xm_s32_t eprintf(const char *format,...) {
#ifdef CONFIG_EARLY_OUTPUT
    va_list argPtr = 0;
    xm_s32_t n;

    va_start(argPtr, format);
    n=EarlyVPrintf(format, argPtr);
    va_end(argPtr);
    return n;
#else
    return 0;
#endif
}
