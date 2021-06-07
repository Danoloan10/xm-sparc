/*
 * $FILE: math.c
 *
 * Math related functions
 *
 * $VERSION$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

/*
  Changelog:
  - [16/11/2015:SPR-290915-01] DivMod64 added
*/

xm_u64_t DivMod64(xm_u64_t numer, xm_u64_t denom, xm_u64_t *remain)
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
