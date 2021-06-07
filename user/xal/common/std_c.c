/*
 * rprintf.c
 * - printf() based on sprintf() from gcctest9.c Volker Oth
 * - Changes made by Holger Klabunde
 * - Changes made by Martin Thomas for the efsl debug output
 * - Changes made by speiro for lpc2000 devices for PaRTiKle
 */
/*
  Changelog:
  - [02/09/2015:SPR-020915-01] MISRA rules.9.1 violation fixed.
  - [02/09/2015:SPR-120515-03] Removed commented code, defines and their 
                               associated code.
  - [02/09/2015:SPR-020915-02] 'continue' replaced with equivalent code.
  - [02/09/2015:SPR-020915-03] 'goto' replaced with equivalent code
  - [16/11/2015:SPR-290915-01] mul replaced by Mult32
  - [21/03/2017:CP-170206-01]  Add support for print float %[.d]f.
*/

#include <stdarg.h>
#include <string.h>
#include <xm.h>

#define SCRATCH 20	//32Bits go up to 4GB + 1 Byte for \0

//Spare some program space by making a comment of all not used format flag lines
#define USE_LONG	// %lx, %Lu and so on, else only 16 bit integer is allowed
#define USE_STRING			// %s, %S Strings as parameters
#define USE_CHAR	// %c, %C Chars as parameters
#define USE_INTEGER // %i, %I Remove this format flag. %d, %D does the same
#define USE_HEX		// %x, %X Hexadezimal output
#define USE_UPPERHEX	// %x, %X outputs A,B,C... else a,b,c...
#ifndef USE_HEX
# undef USE_UPPERHEX		// ;)
#endif
#define USE_UPPER // uncommenting this removes %C,%D,%I,%O,%S,%U,%X and %L..
// only lowercase format flags are used
#define PADDING					//SPACE and ZERO padding

static xm_u64_t divmod64(xm_u64_t numer, xm_u64_t denom, xm_u64_t *remain)
{
    xm_u64_t quotient = 0, quotbit = 1;

    if (denom == 0) {
        return 0;
    }

    while (((xm_s64_t) denom >= 0) && (denom < numer)) {
        denom <<= 1;
        quotbit <<= 1;
    }

    while (quotbit && (numer != 0)) {
        if (denom <= numer) {
            numer -= denom;
            quotient += quotbit;
        }
        denom >>= 1;
        quotbit >>= 1;
    }
    
    if (remain)
        *remain = numer;
    
    return quotient;
}

static unsigned int Mult32(unsigned int v, unsigned base) {
    unsigned int r = 0;
    int e;
    for (e = 0; e<32; e++) {
        if ((base & (1<<e)) == (1<<e)) {
            r += (v << e);
        }
    }
    return r;
}

xm_s32_t atoi(const char* s) {
    long int v=0;
    int sign=1;
    unsigned base;

    while ( *s == ' '  ||  (unsigned int)(*s - 9) < 5u){
        s++;
    }

    switch (*s) {
        case '-':
            sign=-1;
        case '+':
            ++s;
    }
	
    base=10;
    if (*s == '0') {
        if (*(s+1) == 'x') { /* WARNING: assumes ascii. */
            s++;
            base = 16;
        }
        else {
            base = 8;
        }
        s++;
    }

    while ((unsigned int) (*s - '0') < base){
        v = Mult32(v, base)+*s-'0';//v = v * base + *s - '0'; 
        ++s;
    }

    if (sign==-1){
        return -v;
    }
    return v;
}

char *strchr(const char *t, int c) {
    register char ch;

    ch = c;
    for (;;) {
        if (*t == ch) break; if (!*t) return 0; ++t;
    }
    return (char*)t;
}


char *strcat(char *s, const char* t) {
    char *dest=s;
    s+=strlen(s);
    for (;;) {
        if (!(*s=*t)) break; ++s; ++t;
    }
    return dest;
}

char *strncat(char *s, const char *t, xmSize_t n) {
    char *dest=s;
    register char *max;
    s+=strlen(s);
    if ((max=s+n)!=s) {
        for (;;) {
            if (!(*s = *t)) break; if (++s==max) break; ++t;
        }
        *s=0;
    }
    return dest;
}

void *memset(void *dst, int s, unsigned int count) {
    register char * a = dst;
    count++;
    while (--count)
        *a++ = s;
    return dst;
}

void *memcpy(void* dst, const void* src, unsigned int count) {
    register char *d=dst;
    register const char *s=src;
    ++count;
    while (--count) {
        *d = *s;
        ++d; ++s;
    }
    return dst;
}

char *strncpy(char *dest, const char *src, xmSize_t n) {
    xm_s32_t j;

    memset(dest,0,n);

    for (j=0; j<n && src[j]; j++)
	dest[j]=src[j];

    if (j>=n)
	dest[n-1]=0;

    return dest;
}

int strncmp(const char *s1, const char *s2, xmSize_t n) {
  register const unsigned char *a=(const unsigned char*)s1;
  register const unsigned char *b=(const unsigned char*)s2;
  register const unsigned char *fini=a+n;

  while (a < fini) {
    register int res = *a-*b;
    if (res) return res;
    if (!*a) return 0;
    ++a; ++b;
  }
  return 0;
}
int strcmp(const char *s, const char *t) {
    char x;

    for (;;) {
	x = *s; if (x != *t) break; if (!x) break; ++s; ++t;
    }
    return ((int)x)-((int)*t);
}

int memcmp(const void *dst, const void *src, unsigned int count) {
    int r;
    const char *d=dst;
    const char *s=src;
    ++count;
    while (--count) {
        if ((r=(*d - *s)))
            return r;
        ++d;
        ++s;
    }
    return 0;
}

unsigned int strlen(const char *s) {
    unsigned int i;
    if (!s) return 0;
    for (i = 0; *s; ++s) ++i;
    return i;
}


typedef struct Fprint Fprint;
struct Fprint{
    int (*putc)(int c, void *a);
    void *a;
};

static unsigned char *ConversionLoop(
    Fprint *fp, va_list *args, 
    unsigned char issigned, unsigned short base,
    unsigned char *ptr, 
#ifdef USE_LONG
    unsigned char islong, unsigned char isvlong, 
    unsigned long long u_val, long long s_val, 
#else
    unsigned int u_val, int s_val, 
#endif
    unsigned char width, unsigned char fill, unsigned char *scratch) {
    if(issigned) //Signed types
    {
#ifdef USE_LONG
        if(isvlong) { s_val = va_arg((*args), long long); }
        else if(islong) { s_val = va_arg((*args), long); }
        else { s_val = va_arg((*args), int); }
#else
        s_val = va_arg((*args), int);
#endif
        
        if(s_val < 0) //Value negativ ?
        {
            s_val = - s_val; //Make it positiv
            if (fp->putc) fp->putc('-', fp->a);		 //Output sign
        }
        
        if(!isvlong)
            u_val = (unsigned long)s_val;
        else
            u_val = (unsigned long long)s_val;
    }
    else //Unsigned types
    {
#ifdef USE_LONG
        if(isvlong) {u_val = va_arg((*args), unsigned long long); }
        else if(islong) { u_val = va_arg((*args), unsigned long); }
        else { u_val = va_arg((*args), unsigned int); }
#else
        u_val = va_arg((*args), unsigned int);
#endif
    }
    
    ptr = scratch + SCRATCH;
    *--ptr = 0;
    do
    {
        unsigned long long r = 0;
        divmod64(u_val, base, &r);
        char ch = (char) r + '0';
#ifdef USE_HEX
        if (ch > '9')
        {
            ch += 'a' - '9' - 1;
#ifdef USE_UPPERHEX
            ch-=0x20;
#endif
        }
#endif					
        *--ptr = ch;
        u_val = divmod64(u_val, base, 0);
        
#ifdef PADDING
        if(width) width--; //calculate number of padding chars
#endif
        } while (u_val);
    
#ifdef PADDING
    while(width--) *--ptr = fill; //insert padding chars					
#endif
    
    while(*ptr) { if (fp->putc) fp->putc(*ptr, fp->a); ptr++; }
    return ptr;
}

static const double round_nums[8] =
    { 0.5, 0.05, 0.005, 0.0005, 0.00005, 0.000005, 0.0000005, 0.00000005 };

void dbl2stri(Fprint *fp, double dbl, unsigned dec_digits, int always_decimal)
{
    static char local_bfr[128];
    char *output = local_bfr;

    //*******************************************
    //  extract negative info
    //*******************************************
    if (dbl < 0.0)
    {
        *output++ = '-';
        dbl *= -1.0;
    }

    //  handling rounding by adding .5LSB to the floating-point data
    if (dec_digits < 8)
    {
        dbl += round_nums[dec_digits];
    }

    //**************************************************************************
    //  construct fractional multiplier for specified number of digits.
    //**************************************************************************
    unsigned int mult = 1;
    unsigned int idx;
    for (idx = 0; idx < dec_digits; idx++)
        mult *= 10;

    unsigned int wholeNum = (unsigned int) dbl;
    unsigned int decimalNum = (unsigned int) ((dbl - wholeNum) * mult);

    /*******************************************
     *  convert integer portion
     *******************************************/
    char tbfr[40];
    idx = 0;
    while (wholeNum != 0)
    {
        tbfr[idx++] = '0' + (wholeNum % 10);
        wholeNum /= 10;
    }
    // printf("%.3f: whole=%s, dec=%d\n", dbl, tbfr, decimalNum) ;
    if (idx == 0)
    {
        *output++ = '0';
    }
    else
    {
        while (idx > 0)
        {
            *output++ = tbfr[idx - 1];  //lint !e771
            idx--;
        }
    }
    if (dec_digits > 0 || (always_decimal == 1))
    {
        *output++ = '.';

        //*******************************************
        //  convert fractional portion
        //*******************************************
        idx = 0;
        while (decimalNum != 0)
        {
            tbfr[idx++] = '0' + (decimalNum % 10);
            decimalNum /= 10;
        }
        /*  pad the decimal portion with 0s as necessary; */
        /*  We wouldn't want to report 3.093 as 3.93, would we?? */
        while (idx < dec_digits)
        {
            tbfr[idx++] = '0';
        }
        // printf("decimal=%s\n", tbfr) ;
        if (idx == 0)
        {
            *output++ = '0';
        }
        else
        {
            while (idx > 0)
            {
                *output++ = tbfr[idx - 1];
                idx--;
            }
        }
    }
    *output = '\0';

    //  prepare output
    output = local_bfr;
    while (*output != '\0')
    {
        fp->putc(*output++, fp->a);
    }
}

void vrprintf(Fprint *fp, char const *fmt, va_list args)
{
    unsigned char scratch[SCRATCH];
    unsigned char fmt_flag;
    unsigned short base;
    unsigned char *ptr = 0;
    unsigned char issigned = 0;
    /* Precision is 6 by default */
    unsigned precision = 6;
    double dbl = 0;

#ifdef USE_LONG
    unsigned char islong = 0;
    unsigned char isvlong = 0;
    unsigned long long u_val = 0;
    long long s_val = 0;
#else
    unsigned int u_val=0;
    int s_val=0;
#endif

    unsigned char fill;
    unsigned char width;

    for (;;)
    {
        while ((fmt_flag = *(fmt++)) != '%')
        {			 // Until '%' or '\0'
            if (!fmt_flag)
            {
                return;
            }
            if (fp->putc)
                fp->putc(fmt_flag, fp->a);
        }

        issigned = 0; //default unsigned
        base = 10;

        fmt_flag = *fmt++; //get char after '%'

#ifdef PADDING
        width = 0; //no formatting
        fill = 0;	 //no formatting

        if (fmt_flag == '0' || fmt_flag == ' ') //SPACE or ZERO padding	?
        {
            fill = fmt_flag;
            fmt_flag = *fmt++; //get char after padding char
            while (fmt_flag >= '0' && fmt_flag <= '9')
            {
                width = 10 * width + (fmt_flag - '0');
                fmt_flag = *fmt++; //get char after width char
            }
        }
#endif

        /* Obtain precision, or padding for integer specifiers */
        if (fmt_flag == '.')
        {
            precision = 0;
            fmt_flag = *fmt++; /* Get char after precision char */
            while (fmt_flag >= '0' && fmt_flag <= '9')
            {
                precision = 10 * precision + (fmt_flag - '0');
                fmt_flag = *fmt++;
            }
            /* The precision has been limited to 9 digits */
            if (precision > 9)
            {
                precision = 9;
            }
        }
        else
        {
            precision = 6;
        }

#ifdef USE_LONG
        islong = 0; //default int value
        isvlong = 0;
#ifdef USE_UPPER
        if (fmt_flag == 'l' || fmt_flag == 'L') //Long value
#else
            if(fmt_flag=='l') //Long value
#endif
        {
            islong = 1;
            fmt_flag = *fmt++; //get char after 'l' or 'L'
            if (fmt_flag == 'l')
            {
                isvlong = 1;
                fmt_flag = *fmt++; //get char after 'l' or 'L'
            }
        }
#endif

        switch (fmt_flag)
        {
#ifdef USE_CHAR
        case 'c':
#ifdef USE_UPPER
        case 'C':
#endif
            fmt_flag = va_arg(args, int);
            // no break -> run into default
#endif

        default:
            if (fp->putc)
                fp->putc(fmt_flag, fp->a);
            break;

#ifdef USE_STRING
#ifdef USE_UPPER
        case 'S':
#endif
        case 's':
            ptr = (unsigned char*) va_arg(args, char *);
            while (*ptr)
            {
                if (fp->putc)
                    fp->putc(*ptr, fp->a);
                ptr++;
            }
            break;
#endif

#ifdef USE_INTEGER //don't use %i, is same as %d
        case 'i':
#ifdef USE_UPPER
        case 'I':
#endif
#endif
        case 'd':
#ifdef USE_UPPER
        case 'D':
#endif
            issigned = 1;
            // no break -> run into next case
        case 'u':
#ifdef USE_UPPER
        case 'U':
#endif
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
#ifdef USE_UPPER
        case 'X':
#endif
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
        case 'f':
            dbl = (double) va_arg(args, double);
            dbl2stri(fp, dbl, precision, 0);
            break;
        }
    }
}

int putchar(int c) {
    static char buff[512];
    static int i=0;
    if (i > (sizeof(buff)-2)) {
        XM_write_console(&buff[0], i);
        i=0;
    }
    buff[i++]=c;
  
    if (c == '\n' || c == '\r') {
        if (i)
            XM_write_console(&buff[0], i);
        i=0;
    }
    return c;
}

static int printfputc(int c, void *a){
    unsigned long * nc = (unsigned long*) a;
    putchar(c);
    (*nc)++;
    return 1;
}

int printf(char const *fmt, ...){
    int nc=0;
    Fprint fp = {printfputc, (void*)&nc};
    va_list args = 0;
    va_start(args, fmt);
    vrprintf(&fp, fmt, args);
    va_end(args);

    return nc;
}

typedef struct Sdata Sdata;
struct Sdata{
    char *s;
    int *nc;
};

static int sprintfputc(int c, void *a){
    Sdata *sd = (Sdata*) a;
	
    (*sd->s++) = c;
    (*sd->nc)++;
    return 1;
}

int sprintf(char *s, char const *fmt, ...){
    int nc=0;
    Sdata sd = {s, &nc};
    Fprint fp = {sprintfputc, (void*)&sd};
    va_list args = 0;
	
    va_start(args, fmt);
    vrprintf(&fp, fmt, args);
    va_end(args);
    return nc;
}

typedef struct Sndata Sndata;
struct Sndata {
    char *s;
    int *n;	// s size
    int *nc;
};

static int snprintfputc(int c, void *a){
    Sndata *snd = (Sndata*) a;

    if (*snd->n > *snd->nc){
        if (snd->s) {
            snd->s[(*snd->nc)] = c;
            (*snd->nc)++;
            snd->s[(*snd->nc)]='\0';
        }
    }
    return 1;
}

int snprintf(char *s, int n, const char *fmt, ...){
    int nc=0;
    Sndata snd = {s, &n, &nc};
    Fprint fp = {snprintfputc, (void*)&snd};
    va_list args = 0;
	
    va_start(args, fmt);
    vrprintf(&fp, fmt, args);
    va_end(args);
    return nc;
}

#define MAX_VSPRINTF_SIZE 1024

int vsprintf(char *str, const char *format, va_list ap){
    int nc=0;
    int maxNc=MAX_VSPRINTF_SIZE;

    Sndata snd = {str, &maxNc, &nc};
    Fprint fp = {snprintfputc, (void*)&snd};
    vrprintf(&fp, format, ap);
    return nc;
}
