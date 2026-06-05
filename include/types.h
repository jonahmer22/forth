#ifndef TYPES_H
#define TYPES_H

#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>

// sizes

#define MAX_TOKEN_LEN 256
#define LINE_SIZE     256
#define STACK_SIZE    256
#define BASE_SIZE     8
#define DATA_SIZE     65536

// cell types

typedef uintptr_t cell;
typedef intptr_t  scell;

#define P_UNS_ESC "%" PRIuPTR
#define P_SGN_ESC "%" PRIdPTR

#define UNSIGNED (uintptr_t)
#define SIGNED   (intptr_t)

// word flags

#define FLAG_IMMEDIATE    (1 << 0)
#define FLAG_COMPILE_ONLY (1 << 1)
#define FLAG_HIDDEN       (1 << 2)

#endif
