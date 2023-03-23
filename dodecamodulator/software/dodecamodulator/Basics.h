#ifndef BASICS_H
#define BASICS_H

#include "Arduino.h"

typedef long FIXPT;
#define PRES             16384
#define PSHIFT           14
#define PROUNDBIT        (1 << (PSHIFT-1))
#define FROM_FLOAT(a) (long(a*PRES))
#define FROM_INT(a) (a << PSHIFT)
#define TO_INT(a) ((a + PROUNDBIT)>> PSHIFT)

long SIN(unsigned int angle);

#endif
