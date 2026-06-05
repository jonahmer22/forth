#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include "arena/arena.h"

// max token length

#define MAX_TOKEN_LEN 256

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
    if(dsp <= 0){
        fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tData stack underflow.\n", 0x8005);
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
size_t rsp = 0;

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

// compiling stuff

typedef struct Entry Entry;

#define BASE_SIZE 8

typedef enum ActionType{
    ACT_NUM = 0,
    ACT_PRINT,
    ACT_WORD,
    ACT_BRANCH,
    ACT_EOF
} ActionType;

typedef struct Action{
    ActionType type;

    cell num; // also used for storing encoded branch offsets
    char *str;
    Entry *word;
} Action;

typedef struct Body{
    Action *actions;

    size_t used;
    size_t cap;
} Body;

Body *bodyInit(){
    Body *body = malloc(sizeof(Body));

    body->actions = malloc(sizeof(Action)*BASE_SIZE);
    body->cap = BASE_SIZE;
    body->used = 0;

    return body;
}

void bodyEnsureSize(Body *body, size_t size){
    if(body->cap < size){
        for(; body->cap < size; body->cap <<= 1);

        body->actions = realloc(body->actions, sizeof(Action)*size);

        body->cap = size;
    }
}

void bodyAppend(Body *body, Action action){
    bodyEnsureSize(body, body->used + 1);

    body->actions[body->used++] = action;
}

// TODO: void bodyDestroy()

// dictionary bs

typedef enum WordType{
    WORD_BUILTIN = 0,
    WORD_USERDEF
} WordType;

typedef struct Entry{
    char name[MAX_TOKEN_LEN];
    size_t nLen;

    // marks whether or not it's a builtin or user defined
    WordType type;

    // for builtins they should just point to a C function I guess
    void (*behavior)(void *);

    // for user defined words, this should be compiled
    // idk what the fuck to do here, I need to implement a basic VM for this first?
    // apparently the answer is this
    Body *body;

    // immediate, hidden, compileonly, etc. later
    uint8_t flags;

    struct Entry *prev;
} Entry;

Entry *entryInit(const char *name, size_t nLen, WordType type, void (*behavior)(void *), uint8_t flags, Body *body){
    Entry *temp = arenaAlloc(sizeof(Entry));

    memcpy(temp->name, name, nLen);
    temp->name[nLen] = '\0';
    temp->nLen = nLen;

    temp->type = type;

    temp->behavior = behavior;

    temp->body = body;

    temp->flags = flags;

    temp->prev = NULL;  // this should be set by the dictionary appending function

    return temp;
}

typedef struct Dictionary{
    Entry *head;

    size_t len;
} Dictionary;

Dictionary *dictionaryInit(){
    Dictionary *temp = arenaAlloc(sizeof(Dictionary));

    temp->head = NULL;
    temp->len = 0;

    return temp;
}

void dictionaryAdd(Dictionary *dict, Entry *entry){
    entry->prev = dict->head;
    
    dict->head = entry;
    dict->len++;
}

Entry *dictionaryLookup(Dictionary *dict, const char *start, size_t nLen){
    for(Entry *temp = dict->head; temp != NULL; temp = temp->prev){
        if((memcmp(temp->name, start, temp->nLen) == 0) && (nLen == temp->nLen)){
            // we found it
            return temp;
        }
    }
    return NULL;
}

// universal state struct

typedef struct State{
    Dictionary *dict;

    Entry *compiling;   // the word that is currently being compiled
    Body *compBody;     // the body of the word that is currently being compiled

    TokenList *list;
    TokenList *curr;

    cell *dstack;
    size_t *dsp;

    uint64_t *rstack;
    size_t *rsp;

    int compileMode;

    uint8_t status;
} State;

State *stateInit(Dictionary *dict, TokenList* list, cell *dstack, size_t *dsp, uint64_t *rstack, size_t *rsp){
    State *state = arenaAlloc(sizeof(State));

    {
        state->dict = dict;

        state->compiling = NULL;
        state->compBody = NULL;

        state->list = state->curr = list;

        state->dstack = dstack;
        state->dsp = dsp;

        state->rstack = rstack;
        state->rsp = rsp;

        state->compileMode = 0;

        state->status = 0;
    }

    return state;
}

// builtins functions

void dot(void *na){     // .
    printf(P_SGN_ESC, dPop());
}
void dotS(void *na){    // .s
    dStackPrint();
}
void dotQuote(void *na){// ."
    State *state = ((State *)na);

    if(state->curr->next == NULL){
        fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tPrint statement missing a string.\n", 0x0743);
        exit(EXIT_FAILURE);
    }
    const char *start = state->curr->next->start;
    const char *end = state->curr->next->end;

    state->curr = state->curr->next;

    printf("%.*s", (int)(end-start), start);
}
void dup(void *na){     // dup
    cell temp = dPop();
    dPush(temp); dPush(temp);
}
void drop(void *na){    // drop
    dPop();
}
void swap(void *na){    // swap
    if(dsp < 2){
        fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tData stack underflow while trying to swap.\n", 0x8318);
        exit(EXIT_FAILURE);
    }
    cell temp = dstack[dsp-2];
    dstack[dsp-2] = dstack[dsp-1];
    dstack[dsp-1] = temp;
}
void over(void *na){    // over
    if(dsp < 2){
        fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tData stack underflow while trying to over.\n", 0x8312);
        exit(EXIT_FAILURE);
    }
    cell temp = dstack[dsp-2];
    dPush(temp);
}
void rot(void *na){     // rot
    if(dsp < 3){
        fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tData stack underflow while trying to rot.\n", 0x8332);
        exit(EXIT_FAILURE);
    }
    cell temp = dstack[dsp-3];
    dstack[dsp-3] = dstack[dsp-2];
    dstack[dsp-2] = dstack[dsp-1];
    dstack[dsp-1] = temp;
}
void nip(void *na){     // nip
    if(dsp < 2){
        fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tData stack underflow while trying to nip.\n", 0x8132);
        exit(EXIT_FAILURE);
    }
    swap(NULL); dPop();
}
void tuck(void *na){    // tuck
    if(dsp < 2){
        fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tData stack underflow while trying to tuck.\n", 0x8732);
        exit(EXIT_FAILURE);
    }
    swap(NULL);
    cell temp = dstack[dsp-2];
    dPush(temp);
}
void twodup(void *na){  // 2dup
    if(dsp < 2){
        fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tData stack underflow while trying to 2dup.\n", 0x8534);
        exit(EXIT_FAILURE);
    }
    cell temp1 = dstack[dsp-2];
    cell temp2 = dstack[dsp-1];
    dPush(temp1); dPush(temp2);
}
void twodrop(void *na){ // 2drop
    if(dsp < 2){
        fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tData stack underflow while trying to 2drop.\n", 0x8514);
        exit(EXIT_FAILURE);
    }
    drop(NULL); drop(NULL);
}
void depth(void *na){   // depth
    dPush(UNSIGNED dsp);
}
void clear(void *na){   // clear
    dClear();
}
void mod(void *na){     // mod
    scell rhs = SIGNED dPop();
    scell lhs = SIGNED dPop();

    if(rhs == 0){
        fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tAttempting to divide by zero.\n\t\tStack has been reset.\n", 0x0610);

        // consume all the tokens so that execution stops
        State *state = ((State *)na);
        for(; state->curr->next != NULL; state->curr = state->curr->next);

        // clear the stacks
        dClear();
        rClear();

        return;
    }

    dPush(UNSIGNED (lhs % rhs));
}
void slashmod(void *na){// /mod
    scell rhs = SIGNED dPop();
    scell lhs = SIGNED dPop();
    
    if(rhs == 0){
        fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tAttempting to divide by zero.\n\t\tStack has been reset.\n", 0x0610);

        // consume all the tokens so that execution stops
        State *state = ((State *)na);
        for(; state->curr->next != NULL; state->curr = state->curr->next);

        // clear the stacks
        dClear();
        rClear();

        return;
    }

    dPush(UNSIGNED (lhs % rhs));
    dPush(UNSIGNED (lhs / rhs));
}
void negate(void *na){  // negate
    dPush(-(SIGNED dPop()));
}
void fsabs(void *na){   // abs
    scell temp = SIGNED dPop();
    dPush(UNSIGNED (labs(temp)));
}
void oneplus(void *na){ // 1+
    dPush(dPop()+1);
}
void onemin(void *na){  // 1-
    dPush(dPop()-1);
}
void twotimes(void *na){// 2*
    dPush(UNSIGNED ((SIGNED dPop())*2));
}
void twodiv(void *na){  // 2/
    dPush(UNSIGNED ((SIGNED dPop())/2));
}
void fadd(void *na){     // +
    dPush(dPop() + dPop());
}
void fsub(void *na){     // -
    cell rhs = dPop();
    cell lhs = dPop();
    dPush(lhs - rhs);
}
void fmul(void *na){     // *
    dPush(UNSIGNED (SIGNED dPop() * SIGNED dPop()));
}
void fdiv(void *na){     // /
    scell rhs = SIGNED dPop();
    scell lhs = SIGNED dPop();
    
    if(rhs == 0){
        fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tAttempting to divide by zero.\n\t\tStack has been reset.\n", 0x0610);

        // consume all the tokens so that execution stops
        State *state = ((State *)na);
        for(; state->curr->next != NULL; state->curr = state->curr->next);

        // clear the stacks
        dClear();
        rClear();

        return;
    }

    dPush(UNSIGNED (lhs / rhs));
}
void eq(void *na){      // =
    cell rhs = dPop();
    cell lhs = dPop();
    dPush(UNSIGNED (lhs == rhs ? -1 : 0));
}
void neq(void *na){     // <>
    cell rhs = dPop();
    cell lhs = dPop();
    dPush(UNSIGNED (lhs != rhs ? -1 : 0));
}
void lt(void *na){     // <
    scell rhs = SIGNED dPop();
    scell lhs = SIGNED dPop();
    dPush(UNSIGNED (lhs < rhs ? -1 : 0));
}
void gt(void *na){     // >
    scell rhs = SIGNED dPop();
    scell lhs = SIGNED dPop();
    dPush(UNSIGNED (lhs > rhs ? -1 : 0));
}
void lte(void *na){     // <=
    scell rhs = SIGNED dPop();
    scell lhs = SIGNED dPop();
    dPush(UNSIGNED (lhs <= rhs ? -1 : 0));
}
void gte(void *na){     // >=
    scell rhs = SIGNED dPop();
    scell lhs = SIGNED dPop();
    dPush(UNSIGNED (lhs >= rhs ? -1 : 0));
}
void zeq(void *na){     // 0=
    scell temp = SIGNED dPop();
    dPush(UNSIGNED (temp == 0 ? -1 : 0));
}
void zlt(void *na){     // 0<
    scell temp = SIGNED dPop();
    dPush(UNSIGNED (temp < 0 ? -1 : 0));
}
void zgt(void *na){     // 0>
    scell temp = SIGNED dPop();
    dPush(UNSIGNED (temp > 0 ? -1 : 0));
}
void and(void *na){     // and
    dPush(dPop() & dPop());
}
void or(void *na){      // or
    dPush(dPop() | dPop());
}
void xor(void *na){     // xor
    dPush(dPop() ^ dPop());
}
void invert(void *na){  // invert
    dPush(~dPop());
}
void lshift(void *na){  // lshift
    cell amt = dPop();
    cell val = dPop();
    if(amt > (sizeof(cell)*8)){
        amt = (sizeof(cell)*8);
    }
    dPush(val<<amt);
}
void rshift(void *na){  // rshift
    cell amt = dPop();
    cell val = dPop();
    if(amt > (sizeof(cell)*8)){
        amt = (sizeof(cell)*8);
    }
    dPush(val>>amt);
}
void cr(void *na){      // cr
    putc('\n', stdout);
}
void space(void *na){   // space
    putc(' ', stdout);
}
void spaces(void *na){  // spaces
    cell amt = dPop();
    for(uintptr_t i = 0; i < amt; i++)
        space(NULL);
}
void emit(void *na){    // emit
    cell val = dPop();
    putc(val, stdout);
}
void unsigndot(void *na){   // u.
    printf(P_UNS_ESC, dPop());
}
void words(void *na){   // words
    Dictionary *dict = ((State *)na)->dict;
    for(Entry *temp = dict->head; temp != NULL; temp = temp->prev){
        printf("%s ", temp->name);
    }
    putc('\n', stdout);
}
void bye(void *na){     // bye
    exit(EXIT_SUCCESS);
}
void runUserWord(void *na){
    // should execute a user word based on the name curr (do a dict search then execute the body)
    State *state = (State *)na;

    // find our word
    Entry *word = dictionaryLookup(state->dict, state->curr->start, (size_t)(state->curr->end-state->curr->start));

    // walk the actions in the body until ACT_EOF
    // TODO: a good idea for branching is to use a scell to encode the offset and then subtract that from any index counter used to walk the list
    Action *action = word->body->actions;
    for(size_t i = 0; action[i].type != ACT_EOF; i++){
        switch(action[i].type){
            case ACT_NUM:{
                dPush(action[i].num);
                break;
            }
            case ACT_PRINT:{
                printf("%s", action[i].str);
                break;
            }
            case ACT_WORD:{
                action[i].word->behavior(na);
                break;
            }
            default:{
                // TODO: should fatal error here
            }
        }
    }

}
void userword(void *na){// : and ;
    State *state = (State *)na;

    // move the head token past ':'
    state->curr = state->curr->next;

    // setup vars
    state->compileMode = 1;
    char name[MAX_TOKEN_LEN] = {0};
    size_t nameLen = (size_t)(state->curr->end-state->curr->start);
    Body *body = bodyInit();
    state->compBody = body;

    // should check the name exists and is not just ';' which should be a soft error
    if(memcmp(";", state->curr->start, nameLen) == 0){
        fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tEmpty word definition.", 0x5331);
        return;
    }
    // set the name
    memcpy(name, state->curr->start, nameLen);
    state->curr = state->curr->next;    // move current token past the name

    printf("name: %s\n", name);

    printf("body: ");
    while(state->curr != NULL && *(state->curr->start) != ';'){
        TokenList *curr = state->curr;
        size_t len = (curr->end-curr->start);
        printf("%.*s ", (int)len, curr->start);

        // find out whether it's a number, then word (handle specials like string etc)
        char *p;
        scell num = SIGNED strtoimax(curr->start, &p, 0);

        // spoilers
        Entry *word;
        Action temp = {0};
        
        if(((p-curr->start) == (len)) && (p != curr->start)){   // seee if it's a number
            // we have a number
            temp.num = num;
            temp.type = ACT_NUM;
            // make an action that is just a number and append it
            bodyAppend(state->compBody, temp);
        }
        else if((len == 2) && !memcmp(".\"", curr->start, len)){  // do a check for (.")
            if(curr->next == NULL){
                fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tPrint statement missing a string.\n", 0x0707);
                exit(EXIT_FAILURE);
            }
            const char *start = curr->next->start;
            const char *end = curr->next->end;

            curr = curr->next;

            size_t size = (end-start);
            char *str = arenaAlloc(sizeof(char)*size);
            memcpy(str, start, size);
            str[size] = '\0';

            temp.str = str;
            temp.type = ACT_PRINT;

            bodyAppend(state->compBody, temp);
        }
        else if((word = dictionaryLookup(state->dict, curr->start, len)) != NULL){  // try to do a dictionary lookup
            // we have an entry in the dictionary
            temp.word = word;
            temp.type = ACT_WORD;
            // make an action with a pointer to the entry (&word) then append
            bodyAppend(state->compBody, temp);
        }

        state->curr = state->curr->next;
    }
    // add a ACT_EOF to end the word
    Action end = {0};
    end.type = ACT_EOF;
    bodyAppend(state->compBody, end);

    dictionaryAdd(state->dict, entryInit(name, nameLen, WORD_USERDEF, runUserWord, 0, body));
    state->compileMode = 0;
}

int main(void){
    arenaInit();

    // central state used by funcitons
    State *state = stateInit(dictionaryInit(), NULL, dstack, &dsp, rstack, &rsp);

    // add builtins to the dictionary
    {   // this is in a block just to mark a special area to do it
        Entry *temp = NULL;

        // (.)
        temp = entryInit(".", 1, WORD_BUILTIN, dot, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // (.s)
        temp = entryInit(".s", 2, WORD_BUILTIN, dotS, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // (.")
        temp = entryInit(".\"", 2, WORD_BUILTIN, dotQuote, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // (dup)
        temp = entryInit("dup", 3, WORD_BUILTIN, dup, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // (drop)
        temp = entryInit("drop", 4, WORD_BUILTIN, drop, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // (swap)
        temp = entryInit("swap", 4, WORD_BUILTIN, swap, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // {over}
        temp = entryInit("over", 4, WORD_BUILTIN, over, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // (rot)
        temp = entryInit("rot", 3, WORD_BUILTIN, rot, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // {nip}
        temp = entryInit("nip", 3, WORD_BUILTIN, nip, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // (tuck)
        temp = entryInit("tuck", 4, WORD_BUILTIN, tuck, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // (2dup)
        temp = entryInit("2dup", 4, WORD_BUILTIN, twodup, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // (2drop)
        temp = entryInit("2drop", 5, WORD_BUILTIN, twodrop, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // (depth)
        temp = entryInit("depth", 5, WORD_BUILTIN, depth, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // (clear)
        temp = entryInit("clear", 5, WORD_BUILTIN, clear, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // (mod)
        temp = entryInit("mod", 3, WORD_BUILTIN, mod, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // /mod
        temp = entryInit("/mod", 4, WORD_BUILTIN, slashmod, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // negate
        temp = entryInit("negate", 6, WORD_BUILTIN, negate, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // abs
        temp = entryInit("abs", 3, WORD_BUILTIN, fsabs, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // 1+
        temp = entryInit("1+", 2, WORD_BUILTIN, oneplus, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // 1-
        temp = entryInit("1-", 2, WORD_BUILTIN, onemin, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // 2*
        temp = entryInit("2*", 2, WORD_BUILTIN, twotimes, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // 2/
        temp = entryInit("2/", 2, WORD_BUILTIN, twodiv, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // (+)
        temp = entryInit("+", 1, WORD_BUILTIN, fadd, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // (-)
        temp = entryInit("-", 1, WORD_BUILTIN, fsub, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // (*)
        temp = entryInit("*", 1, WORD_BUILTIN, fmul, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // (/)
        temp = entryInit("/", 1, WORD_BUILTIN, fdiv, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // (=)
        temp = entryInit("=", 1, WORD_BUILTIN, eq, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // (<>)
        temp = entryInit("<>", 2, WORD_BUILTIN, neq, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // (<)
        temp = entryInit("<", 1, WORD_BUILTIN, lt, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // (>)
        temp = entryInit(">", 1, WORD_BUILTIN, gt, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // (<=)
        temp = entryInit("<=", 2, WORD_BUILTIN, lte, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // (>=)
        temp = entryInit(">=", 2, WORD_BUILTIN, gte, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // (0=)
        temp = entryInit("0=", 2, WORD_BUILTIN, zeq, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // (0<)
        temp = entryInit("0<", 2, WORD_BUILTIN, zlt, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // (0>)
        temp = entryInit("0>", 2, WORD_BUILTIN, zgt, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // (and)
        temp = entryInit("and", 3, WORD_BUILTIN, and, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // (or)
        temp = entryInit("or", 2, WORD_BUILTIN, or, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // (xor)
        temp = entryInit("xor", 3, WORD_BUILTIN, xor, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // (invert)
        temp = entryInit("invert", 6, WORD_BUILTIN, invert, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // (lshift)
        temp = entryInit("lshift", 6, WORD_BUILTIN, lshift, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // (rshift)
        temp = entryInit("rshift", 6, WORD_BUILTIN, rshift, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // (cr)
        temp = entryInit("cr", 2, WORD_BUILTIN, cr, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // (space)
        temp = entryInit("space", 5, WORD_BUILTIN, space, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // (spaces)
        temp = entryInit("spaces", 6, WORD_BUILTIN, spaces, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // (emit)
        temp = entryInit("emit", 4, WORD_BUILTIN, emit, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // (u.)
        temp = entryInit("u.", 2, WORD_BUILTIN, unsigndot, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // (words)
        temp = entryInit("words", 5, WORD_BUILTIN, words, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // (bye)
        temp = entryInit("bye", 3, WORD_BUILTIN, bye, 0, NULL);
        dictionaryAdd(state->dict, temp);

        // (:)
        temp = entryInit(":", 1, WORD_BUILTIN, userword, 0, NULL);
        dictionaryAdd(state->dict, temp);
    }

    printf("\x1b[1;31mFORTHISH\x1b[0m\nA FORTH-like language made to be as dead simple as I can think of making a language.\n");
    // main loop
    for(; printf("\n\x1b[34m>\x1b[0m "), fgets(line, sizeof(line), stdin);){
        // exit case
        line[strcspn(line, "\n")] = '\0';
        if(strcmp(line, "bye") == 0){
            break;
        }

        TokenList *list = tokenizeSrc(line);

        // update the central state's list
        state->list = state->curr = list;

        // tokenListPrint(list);

        for(; state->curr != NULL; state->curr = state->curr->next){
            char temp[MAX_TOKEN_LEN] = {0};
            size_t len = (state->curr->end-state->curr->start);

            if(len >= MAX_TOKEN_LEN){
                fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tToken is longer than buffer length of (%d).\nToken contents: \" %.*s \"", 0x0670, MAX_TOKEN_LEN, (int)len, state->curr->start);
                exit(EXIT_FAILURE);
            }

            memcpy(temp, state->curr->start, len >= MAX_TOKEN_LEN ? MAX_TOKEN_LEN-1 : len);

            char *p;
            scell num = SIGNED strtoimax(temp, &p, 0);

            // spoilers
            Entry *word;
            
            if(((p-temp) == (len)) && (p != temp)){   // seee if it's a number
                // we have a number, all we have to do is put it on the stack as unsigned
                dPush(UNSIGNED num);
                continue;
            }
            else if((word = dictionaryLookup(state->dict, state->curr->start, len)) != NULL){  // try to do a dictionary lookup
                // we have an entry in the dictionary
                word->behavior(state);
            }
            else{   // error for undefined word
                fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tUnidentified word.\n\t\tWord: \" %.*s \"\n", 0x1018, (int)len, state->curr->start);
            }
        }
    }

    arenaDestroy();
    return EXIT_SUCCESS;
}
