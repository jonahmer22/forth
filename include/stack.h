#ifndef STACK_H
#define STACK_H

#include <stddef.h>
#include <stdint.h>
#include "types.h"

// data stack

extern cell   dstack[STACK_SIZE];
extern size_t dsp;

void dPush(cell d);
cell dPop(void);
void dClear(void);
void dStackPrint(void);

// return stack

extern uint64_t rstack[STACK_SIZE];
extern size_t   rsp;

void     rPush(uint64_t r);
uint64_t rPop(void);
void     rClear(void);

#endif
