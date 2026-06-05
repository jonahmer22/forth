#include <stdio.h>
#include <stdlib.h>
#include "../include/token.h"
#include "../arena/arena.h"

// parsing / tokenizing, whatever the fuck you wanna call it

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

            if(*p == '"'){
                tokenListAppend(list, start, start);

                p++;

                continue;
            }

            // get the string to be
            start = p;
            for(; *p != '\"' && *p != '\0'; p++);

            if(start == p){
                continue;
            }

            if(*p == '\0'){
                fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tPossibly unterminated string.\n\t\tLine contents: \" %.*s \".\n", 0x5321, (int)(p-start), start);
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
