#include <stdio.h>
#include <stdlib.h>
#include "../include/stack.h"

// data stack

cell   dstack[STACK_SIZE] = {0};
size_t dsp = 0;

void dPush(cell d){
    if(dsp >= STACK_SIZE){
        fprintf(stderr, "\x1b[1;31m[FATAL 0x%04X]:\x1b[0m\tData stack overflow.\n\t\tMax stack size %d.\n", 0x8008, STACK_SIZE);
        exit(EXIT_FAILURE);
    }
    dstack[dsp++] = d;
}
cell dPop(){
    if(dsp <= 0){
        fprintf(stderr, "\x1b[1;31m[FATAL 0x%04X]:\x1b[0m\tData stack underflow.\n", 0x8005);
        exit(EXIT_FAILURE);
    }
    return dstack[--dsp];
}
void dClear(){
    for(size_t i = 0; i < STACK_SIZE; i++)
        dstack[i] = 0;

    dsp = 0;
}
void dStackPrint(){
    printf("<%zu> ", dsp);
    for(size_t i = 0; i < dsp; i++){
        printf(P_SGN_ESC " ", dstack[i]);
    }
}

// return stack

uint64_t rstack[STACK_SIZE] = {0};
size_t   rsp = 0;

void rPush(uint64_t r){
    if(rsp >= STACK_SIZE){
        fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tReturn stack overflow.\n\t\tMax stack size %d.\n", 0x8008, STACK_SIZE);
        exit(EXIT_FAILURE);
    }
    rstack[rsp++] = r;
}
uint64_t rPop(){
    if(rsp <= 0){
        fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tReturn stack underflow.\n", 0x8009);
        exit(EXIT_FAILURE);
    }
    return rsp == 0 ? 0 : rstack[--rsp];
}
void rClear(){
    for(size_t i = 0; i < STACK_SIZE; i++)
        rstack[i] = 0;
}
