#include <stdint.h>
#include <string.h>

#include "ieee754_clf.h"


float_class_t classify(double x) {

    void *tmp_pointer = &x;
    uint64_t a_value = *((uint64_t *) tmp_pointer);

    uint64_t sign = (a_value >> 63);
    uint64_t exp = (~(1ULL << 11)) & (a_value >> 52);
    uint64_t mantissa = a_value &
                 (~((~(1ULL << 12)) << 52));

    if (exp == 0x7FF) {
        if (mantissa == 0) {
            return sign ? MinusInf : Inf;
        } else {
            return NaN;
        }
    } else if (exp == 0) {
        if (mantissa == 0) {
            return sign ? MinusZero : Zero;
        } else {
            return sign ? MinusDenormal : Denormal;
        }
    } else {
        return sign ? MinusRegular : Regular;
    }
}