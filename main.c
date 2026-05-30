#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include "arena/arena.h"

// input line

#define LINE_SIZE 256

char line[LINE_SIZE];

// stacks

#define STACK_SIZE 256

// data stack

typedef uintptr_t cell;
typedef intptr_t scell;

#define P_UNS_ESC "%" PRIuPTR
#define P_SGN_ESC "%" PRIdPTR

#define UNSIGNED (uintptr_t)
#define SIGNED (intptr_t)

cell dstack[STACK_SIZE] = {0};
size_t dsp = 0;

void dPush(cell d){
    if(dsp >= STACK_SIZE){
        fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tData stack overflow.\n\t\tMax stack size %d.\n", 0x8008, STACK_SIZE);
        exit(EXIT_FAILURE);
    }
    dstack[dsp++] = d;
}
cell dPop(){
    return dsp == 0 ? 0 : dstack[--dsp];
}
void dClear(){
    for(size_t i = 0; i < STACK_SIZE; i++)
        dstack[i] = 0;
}
void dStackPrint(){
    printf("<%zu> ", dsp);
    for(size_t i = 0; i < dsp; i++){
        printf(P_SGN_ESC " ", dstack[i]);
    }
}

// return stack

uint64_t rstack[STACK_SIZE] = {0};
size_t rsp = 0;

void rPush(uint64_t r){
    if(rsp >= STACK_SIZE){
        fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tReturn stack overflow.\n\t\tMax stack size %d.\n", 0x8008, STACK_SIZE);
        exit(EXIT_FAILURE);
    }
    rstack[rsp++] = r;
}
uint64_t rPop(){
    return rsp == 0 ? 0 : rstack[--rsp];
}
void rClear(){
    for(size_t i = 0; i < STACK_SIZE; i++)
        rstack[i] = 0;
}

// parsing / tokenizing, whatever the fuck you wanna call it

typedef struct TokenList{
    const char *start;
    const char *end;

    struct TokenList *next;
} TokenList;

TokenList *tokenListInit(){
    TokenList *temp = arenaAlloc(sizeof(TokenList));

    temp->start = temp->end = NULL;
    temp->next = NULL;

    return temp;
}

TokenList *tokenListAppend(TokenList *list, const char *start, const char *end){
    TokenList *item = tokenListInit();

    item->start = start;
    item->end = end;

    if(list == NULL){
        return item;
    }

    TokenList *temp = list;
    while(temp->next != NULL)
        temp = temp->next;
    temp->next = item;

    return list;
}

void tokenListPrint(TokenList *list){
    for(TokenList *tmp = list; tmp != NULL; tmp = tmp->next){
        printf("-[\" %.*s \"]-", (int)(tmp->end - tmp->start), tmp->start);
    }
    printf("\n");
}

int isWhiteSpace(const char *p){
    switch(*p){
        case ' ':
        case '\t':
        case '\r':
        case '\n':
        case '\v':{
            return 1;
            break;
        }
        default:{
            return 0;
            break;
        }
    }
}

const char *skipWhiteSpace(const char *p){
    for(; isWhiteSpace(p) && *p != '\0'; p++);
    return p;
}

int isDigit(const char p){
    return p >= '0' && p <= '9';
}

TokenList *tokenizeSrc(const char *line){
    TokenList *list = NULL;

    const char *p = line;
    while(*p != '\0'){
        p = skipWhiteSpace(p);
        if(*p == '\0')
            break;

        const char *start = p;

        for(; !isWhiteSpace(p) && (*p != '\0'); p++);

        // special strings cases
        if((*start == '.' && *(p-1) == '\"') && (p-start == 2)){
            list = tokenListAppend(list, start, p);

            p = skipWhiteSpace(p);
            
            // get the string to be
            start = p;
            for(; *p != '\"' && *p != '\0'; p++);

            if(start == p){
                fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tThere appears to be an unterminated print statement.\n", 0x0690);
                exit(EXIT_FAILURE);
            }

            list = tokenListAppend(list, start, p);
            p++;

            continue;
        }
        
        list = tokenListAppend(list, start, p);
    }

    return list;
}


int main(void){
    arenaInit();

    printf("\x1b[1;31mFORTHISH\x1b[0m\nA FORTH-like language made to be as dead simple as I can think of making a language.\n");
    // main loop
    for(; printf("\n\x1b[34m>\x1b[0m "), fgets(line, sizeof(line), stdin);){
        // exit case
        line[strcspn(line, "\n")] = '\0';
        if(strcmp(line, "bye") == 0){
            break;
        }

        TokenList *list = tokenizeSrc(line);

        tokenListPrint(list);

        for(TokenList *curr = list; curr != NULL; curr = curr->next){
            #define BUFF_LEN 256
            char temp[BUFF_LEN] = {0};
            size_t len = (curr->end-curr->start);

            if(len >= BUFF_LEN){
                fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tToken is longer than buffer length of (%d).\nToken contents: \" %.*s \"", 0x0670, BUFF_LEN, (int)len, curr->start);
                exit(EXIT_FAILURE);
            }

            memcpy(temp, curr->start, len >= BUFF_LEN ? BUFF_LEN-1 : len);
            #undef BUFF_LEN

            char *p;
            scell num = SIGNED strtoimax(temp, &p, 0);
            
            if(((p-temp) == (curr->end-curr->start)) && (p != temp)){   // seee if it's a number
                printf("number happened: " P_SGN_ESC "\n", num);
            }
            else if(0){  // try to do a dictionary lookup

            }
            else{   // error for undefined word

            }
        }
    }

    arenaDestroy();
    return EXIT_SUCCESS;
}
