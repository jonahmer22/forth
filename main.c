#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// input line

#define LINE_SIZE 256

char line[LINE_SIZE];

// stacks

#define STACK_SIZE 256

// data stack

double dstack[STACK_SIZE] = {0};
size_t dsp = 0;

void dPush(double d){
    dstack[dsp++] = d;
}
double dPop(){
    return dsp == 0 ? 0 : dstack[--dsp];
}
void dClear(){
    for(size_t i = 0; i < STACK_SIZE; i++)
        dstack[i] = 0;
}
void dStackPrint(){
    printf("<%zu> ", dsp);
    for(size_t i = 0; i < dsp; i++){
        printf("%g ", dstack[i]);
    }
}

// return stack

uint64_t rstack[STACK_SIZE] = {0};
size_t rsp = 0;

void rPush(uint64_t r){
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

#define SYS_CHAR '.'

typedef enum TokType{
    TOK_NUM,
    TOK_STR,
    TOK_SYS_WORD,
    TOK_WORD,
    TOK_NONE    // only for internals, should not show up anywhere and should be considered a fatal error if encountered in the wild
} TokType;

typedef struct TokenList{
    char *start;
    char *end;

    TokType type;

    struct TokenList *next;
} TokenList;

TokenList *tokenListInit(){
    TokenList *temp = malloc(sizeof(TokenList));

    temp->start = temp->end = temp->next = NULL;
    temp->type = TOK_NONE;

    return temp;
}

void tokenListAppend(TokenList *list, char *start, char *end, TokType type){
    TokenList *item = tokenListInit();

    item->start = start;
    item->end = end;
    item->type = type;

    if(list == NULL){
        list = item;
        return;
    }

    for(TokenList *temp = list; temp->next != NULL; temp = temp->next){
        temp->next = item;
        return;
    }
}

void tokenListPrint(TokenList *list){
    
}

int isWhiteSpace(char *p){
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

void skipWhiteSpace(char *p){
    for(; isWhiteSpace(p), p != '\0'; p++);
}

TokenList *tokenizeLine(const char *line){
    TokenList *list = NULL;

    char *p = line;
    while(*p != '\0'){
        skipWhiteSpace(p);

        char *start = p;

        if(*start == '"')
            for(; *p != '"', *p != '\0'; p++);
        else
            for(; !isWhiteSpace(p), *p != '\0'; p++);
        
        TokType type = TOK_WORD;
        if(*start >= '0' && *start <= '9'){
            type = TOK_NUM;
        }
        else if(*start == SYS_CHAR){
            type = TOK_SYS_WORD;
        }
        else if(*start == '"'){
            if(*p != '"'){
                fprintf(stderr, "\x1b[1;31m[FATAL 0x%04X]\x1b[0m:\tUnterminated string in line \" %s \".", line);
                exit(EXIT_FAILURE);
            }
            type = TOK_STR;
        }

        tokenListAppend(list, start, p, type);
    }

    return list;
}


int main(void){
    printf("\x1b[1;31mFORTHISH\x1b[0m\nA FORTH-like language made to be as dead simple as I can think of making a language.\n");
    // main loop
    for(; printf("\n\x1b[34m>\x1b[0m "), fgets(line, sizeof(line), stdin);){
        // exit case
        if(strcmp(line, "bye\n") == 0){
            break;
        }

        // TODO: switch to just whitespace seperated tokenization

        // parser head
        char *p = line;
        
        while(*p != '\0'){
            switch(*p){
                // skip whitespace
                case ' ':
                case '\t':
                case '\n':
                case '\r':
                case '\v':{
                    p++;
                    break;
                }
                // catch numbers
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':{
                    dPush(atof(p));
                    while(*p >= '0' && *p <= '9' || *p == '.')
                        p++;
                    break;
                }
                // arithmatic
                case '+':{
                    p++;
                    if(*p == '+'){
                        p++;
                        dPush(dPop()+1);
                        break;
                    }
                    double lhs = dPop();
                    double rhs = dPop();
                    dPush(lhs + rhs);
                    break;
                }
                case '-':{
                    p++;
                    if(*p == '-'){
                        p++;
                        dPush(dPop()-1);
                        break;
                    }
                    double lhs = dPop();
                    double rhs = dPop();
                    dPush(lhs - rhs);
                    break;
                }
                case '*':{
                    p++;
                    double lhs = dPop();
                    double rhs = dPop();
                    if(*p == '*'){
                        p++;
                        dPush(pow(lhs, rhs));
                        break;
                    }
                    dPush(lhs * rhs);
                    break;
                }
                case '/':{
                    p++;
                    double lhs = dPop();
                    double rhs = dPop();
                    if(*p == '/'){
                        p++;
                        dPush(floor(lhs / rhs));
                        break;
                    }
                    dPush(lhs / rhs);
                    break;
                }
                // system utils
                case '.':{
                    p++;
                    if(*p == 's'){
                        p++;
                        dStackPrint();
                        break;
                    }
                    else if(*p == '"'){
                        p++;    // skip the quote
                        char *tmp = p;
                        while(*tmp != '"' && *tmp != '\0')
                            tmp++;
                        
                        printf("%.*s", (int)(tmp-p), p);
                        p = ++tmp;
                        break;
                    }
                    printf("%g", dPop());
                    break;
                }
                default:{
                    // attempt to handle the users input

                    // error case
                    line[strcspn(line, "\n")] = '\0';
                    fprintf(stderr, "\x1b[1;31m[FATAL 0x%04X]\x1b[0m:\tParser error, undefined character '%c'\n\t\tin line: \" %s \".\n", 0x0167, *p, line);
                    p++;
                    continue;
                    break;
                }
            }
        }
    }

    return EXIT_SUCCESS;
}