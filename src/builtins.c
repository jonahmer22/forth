#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../include/builtins.h"
#include "../include/stack.h"
#include "../arena/arena.h"

// number base (changed by HEX / DECIMAL)
int num_base = 10;

// data space for VARIABLE / ALLOT / HERE
uint8_t data_space[DATA_SIZE];
size_t  data_here = 0;

// builtin functions

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
    const char *end   = state->curr->next->end;

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
    cell temp    = dstack[dsp-2];
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
    cell temp    = dstack[dsp-3];
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
void fadd(void *na){    // +
    dPush(dPop() + dPop());
}
void fsub(void *na){    // -
    cell rhs = dPop();
    cell lhs = dPop();
    dPush(lhs - rhs);
}
void fmul(void *na){    // *
    dPush(UNSIGNED (SIGNED dPop() * SIGNED dPop()));
}
void fdiv(void *na){    // /
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
void lt(void *na){      // <
    scell rhs = SIGNED dPop();
    scell lhs = SIGNED dPop();
    dPush(UNSIGNED (lhs < rhs ? -1 : 0));
}
void gt(void *na){      // >
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

// return stack words

void tor(void *na){     // >R  ( n -- )  R:( -- n )
    rPush((uint64_t)dPop());
}
void rfrom(void *na){   // R>  ( -- n )  R:( n -- )
    dPush((cell)rPop());
}
void rfetch(void *na){  // R@  ( -- n )  R:( n -- n )
    if(rsp == 0){
        fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tReturn stack underflow on R@.\n", 0x8010);
        exit(EXIT_FAILURE);
    }
    dPush((cell)rstack[rsp-1]);
}

// user word execution / compilation

// execBody runs a compiled body directly without a dict lookup.
// this is what gets called when one user word calls another — bypassing
// runUserWord's state->curr lookup which would point at the wrong token.
static void execBody(Body *body, void *na){
    Action *action = body->actions;
    for(size_t i = 0; action[i].type != ACT_EOF; ){
        switch(action[i].type){
            case ACT_NUM:{
                dPush(action[i].num);
                i++;
                break;
            }
            case ACT_PRINT:{
                printf("%s", action[i].str);
                i++;
                break;
            }
            case ACT_WORD:{
                if(action[i].word->type == WORD_USERDEF){
                    execBody(action[i].word->body, na);
                } else {
                    action[i].word->behavior(na);
                }
                i++;
                break;
            }
            case ACT_BRANCH:{
                i = (size_t)action[i].num;
                break;
            }
            case ACT_BRANCH0:{
                cell flag = dPop();
                i = (flag == 0) ? (size_t)action[i].num : i + 1;
                break;
            }
            case ACT_DO:{
                cell start = dPop();
                cell limit = dPop();
                rPush((uint64_t)limit);
                rPush((uint64_t)start);
                i++;
                break;
            }
            case ACT_LOOP:{
                rstack[rsp-1]++;
                if(rstack[rsp-1] >= rstack[rsp-2]){
                    rsp -= 2;   // pop loop frame
                    i++;
                } else {
                    i = (size_t)action[i].num;  // branch back to body start
                }
                break;
            }
            case ACT_PLUS_LOOP:{
                scell n = SIGNED dPop();
                rstack[rsp-1] = (uint64_t)((int64_t)rstack[rsp-1] + n);
                if((int64_t)rstack[rsp-1] >= (int64_t)rstack[rsp-2]){
                    rsp -= 2;
                    i++;
                } else {
                    i = (size_t)action[i].num;
                }
                break;
            }
            case ACT_I:{
                dPush((cell)rstack[rsp-1]);
                i++;
                break;
            }
            case ACT_J:{
                dPush((cell)rstack[rsp-3]);
                i++;
                break;
            }
            case ACT_EXIT:{
                return;
            }
            default:{
                i++;
                break;
            }
        }
    }
}

// runUserWord is called by the interpreter loop; it looks up the word by
// state->curr and hands off to execBody
void runUserWord(void *na){
    State *state = (State *)na;
    Entry *word = dictionaryLookup(state->dict, state->curr->start, (size_t)(state->curr->end-state->curr->start));
    execBody(word->body, na);
}

void userword(void *na){// : and ;
    State *state = (State *)na;

    // move past ':'
    state->curr = state->curr->next;

    char   name[MAX_TOKEN_LEN] = {0};
    size_t nameLen = (size_t)(state->curr->end - state->curr->start);
    Body  *body = bodyInit();

    if(memcmp(";", state->curr->start, nameLen) == 0){
        fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tEmpty word definition.\n", 0x5331);
        return;
    }

    memcpy(name, state->curr->start, nameLen);
    state->curr = state->curr->next;    // move past name

    // set up compile state
    state->compileMode = 1;
    state->compBody    = body;
    // create a stub entry for RECURSE
    Entry *stub = entryInit(name, nameLen, WORD_USERDEF, runUserWord, 0, body);
    state->compiling = stub;

    while(state->curr != NULL && *(state->curr->start) != ';'){
        size_t len = (size_t)(state->curr->end - state->curr->start);

        char  *p;
        scell  num = SIGNED strtoimax(state->curr->start, &p, num_base);

        Action act = {0};
        Entry *word;

        if(((size_t)(p - state->curr->start) == len) && (p != state->curr->start)){
            // number literal
            act.num  = UNSIGNED num;
            act.type = ACT_NUM;
            bodyAppend(state->compBody, act);
        }
        else if((len == 2) && !memcmp(".\"", state->curr->start, len)){
            // ." string "
            if(state->curr->next == NULL){
                fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tPrint statement missing a string.\n", 0x0707);
                exit(EXIT_FAILURE);
            }
            state->curr = state->curr->next;
            const char *start = state->curr->start;
            const char *end   = state->curr->end;
            size_t size = (size_t)(end - start);
            char *str = arenaAlloc(sizeof(char) * (size + 1));
            memcpy(str, start, size);
            str[size] = '\0';
            act.str  = str;
            act.type = ACT_PRINT;
            bodyAppend(state->compBody, act);
        }
        else if((word = dictionaryLookup(state->dict, state->curr->start, len)) != NULL){
            if(word->flags & FLAG_IMMEDIATE){
                // immediate: execute now so it can compile things
                // immediate words do NOT advance state->curr themselves
                word->behavior(state);
            } else {
                act.word = word;
                act.type = ACT_WORD;
                bodyAppend(state->compBody, act);
            }
        }
        else {
            fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tUnidentified word in word definition.\n\t\tWord: \" %.*s \"\n", 0x1019, (int)len, state->curr->start);
        }

        state->curr = state->curr->next;
    }

    // terminate body
    Action eof_act = {0};
    eof_act.type = ACT_EOF;
    bodyAppend(state->compBody, eof_act);

    dictionaryAdd(state->dict, stub);

    state->compileMode = 0;
    state->compiling   = NULL;
    state->compBody    = NULL;
}

// helper used by immediate words

static void compile_error_not_in_def(const char *name){
    fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\t'%s' only allowed inside a word definition.\n", 0x5500, name);
}

// conditionals (immediate, compile-only)

void forth_if(void *na){    // IF  ( flag -- )
    State *state = (State *)na;
    if(!state->compileMode){ compile_error_not_in_def("IF"); return; }
    // emit BRANCH0 with placeholder target; push its index for THEN/ELSE to patch
    Action a = {0};
    a.type = ACT_BRANCH0;
    a.num  = 0;
    bodyAppend(state->compBody, a);
    state->cpstack[state->csp++] = state->compBody->used - 1;
}

void forth_else(void *na){  // ELSE
    State *state = (State *)na;
    if(!state->compileMode){ compile_error_not_in_def("ELSE"); return; }
    // emit unconditional jump (placeholder); patch the IF branch to here+1
    Action a = {0};
    a.type = ACT_BRANCH;
    a.num  = 0;
    bodyAppend(state->compBody, a);
    size_t else_branch = state->compBody->used - 1;
    size_t if_branch   = state->cpstack[--state->csp];
    state->compBody->actions[if_branch].num = (cell)state->compBody->used; // IF jumps past ELSE's branch
    state->cpstack[state->csp++] = else_branch; // THEN will patch this
}

void forth_then(void *na){  // THEN
    State *state = (State *)na;
    if(!state->compileMode){ compile_error_not_in_def("THEN"); return; }
    size_t idx = state->cpstack[--state->csp];
    state->compBody->actions[idx].num = (cell)state->compBody->used;
}

// loops (immediate, compile-only)

void forth_begin(void *na){ // BEGIN
    State *state = (State *)na;
    if(!state->compileMode){ compile_error_not_in_def("BEGIN"); return; }
    state->cpstack[state->csp++] = state->compBody->used; // save branch-back target
}

void forth_until(void *na){ // UNTIL  ( flag -- )  branches back to BEGIN if false
    State *state = (State *)na;
    if(!state->compileMode){ compile_error_not_in_def("UNTIL"); return; }
    size_t begin = state->cpstack[--state->csp];
    Action a = {0};
    a.type = ACT_BRANCH0;
    a.num  = (cell)begin;
    bodyAppend(state->compBody, a);
}

void forth_again(void *na){ // AGAIN  ( -- )  unconditional loop back to BEGIN
    State *state = (State *)na;
    if(!state->compileMode){ compile_error_not_in_def("AGAIN"); return; }
    size_t begin = state->cpstack[--state->csp];
    Action a = {0};
    a.type = ACT_BRANCH;
    a.num  = (cell)begin;
    bodyAppend(state->compBody, a);
}

void forth_while(void *na){ // WHILE  ( flag -- )
    // cpstack before: begin_idx
    // cpstack after:  while_branch_idx  begin_idx
    State *state = (State *)na;
    if(!state->compileMode){ compile_error_not_in_def("WHILE"); return; }
    Action a = {0};
    a.type = ACT_BRANCH0;
    a.num  = 0; // patched by REPEAT
    bodyAppend(state->compBody, a);
    size_t while_idx = state->compBody->used - 1;
    size_t begin_idx = state->cpstack[state->csp - 1];
    state->cpstack[state->csp - 1] = while_idx;
    state->cpstack[state->csp++]   = begin_idx;
}

void forth_repeat(void *na){ // REPEAT
    State *state = (State *)na;
    if(!state->compileMode){ compile_error_not_in_def("REPEAT"); return; }
    size_t begin_idx = state->cpstack[--state->csp];
    size_t while_idx = state->cpstack[--state->csp];
    Action a = {0};
    a.type = ACT_BRANCH;
    a.num  = (cell)begin_idx;
    bodyAppend(state->compBody, a);
    state->compBody->actions[while_idx].num = (cell)state->compBody->used;
}

void forth_do(void *na){    // DO  ( limit start -- )
    State *state = (State *)na;
    if(!state->compileMode){ compile_error_not_in_def("DO"); return; }
    Action a = {0};
    a.type = ACT_DO;
    a.num  = 0; // exit address, patched by LOOP
    bodyAppend(state->compBody, a);
    state->cpstack[state->csp++] = state->compBody->used - 1; // index of DO action
}

void forth_loop(void *na){  // LOOP
    State *state = (State *)na;
    if(!state->compileMode){ compile_error_not_in_def("LOOP"); return; }
    size_t do_idx = state->cpstack[--state->csp];
    Action a = {0};
    a.type = ACT_LOOP;
    a.num  = (cell)(do_idx + 1); // branch back to instruction after DO
    bodyAppend(state->compBody, a);
    state->compBody->actions[do_idx].num = (cell)state->compBody->used; // DO exit = after LOOP
}

void forth_plus_loop(void *na){ // +LOOP  ( n -- )
    State *state = (State *)na;
    if(!state->compileMode){ compile_error_not_in_def("+LOOP"); return; }
    size_t do_idx = state->cpstack[--state->csp];
    Action a = {0};
    a.type = ACT_PLUS_LOOP;
    a.num  = (cell)(do_idx + 1);
    bodyAppend(state->compBody, a);
    state->compBody->actions[do_idx].num = (cell)state->compBody->used;
}

void forth_i(void *na){     // I  ( -- index )
    dPush((cell)rstack[rsp-1]);
}
void forth_j(void *na){     // J  ( -- outer-index )
    dPush((cell)rstack[rsp-3]);
}
void forth_leave(void *na){ // LEAVE  ( -- )  force loop exit on next LOOP check
    if(rsp < 2){ return; }
    rstack[rsp-1] = rstack[rsp-2]; // set index = limit so LOOP exits
}

// exit / recurse (compile-only)

void forth_exit(void *na){  // EXIT
    State *state = (State *)na;
    if(!state->compileMode){ compile_error_not_in_def("EXIT"); return; }
    Action a = {0};
    a.type = ACT_EXIT;
    bodyAppend(state->compBody, a);
}

void forth_recurse(void *na){ // RECURSE  compile a call to the word being defined
    State *state = (State *)na;
    if(!state->compileMode){ compile_error_not_in_def("RECURSE"); return; }
    if(state->compiling == NULL){ return; }
    Action a = {0};
    a.type = ACT_WORD;
    a.word = state->compiling;
    bodyAppend(state->compBody, a);
}

// variables and memory

void forth_fetch(void *na){ // @  ( addr -- val )
    cell addr = dPop();
    dPush(*(cell *)addr);
}
void forth_store(void *na){ // !  ( val addr -- )
    cell addr = dPop();
    cell val  = dPop();
    *(cell *)addr = val;
}
void forth_plus_store(void *na){ // +!  ( n addr -- )
    cell addr = dPop();
    *(cell *)addr += dPop();
}
void forth_question(void *na){ // ?  ( addr -- )
    cell addr = dPop();
    printf(P_SGN_ESC " ", *(scell *)addr);
}
void forth_cells(void *na){ // CELLS  ( n -- n*size )
    dPush(dPop() * sizeof(cell));
}
void forth_allot(void *na){ // ALLOT  ( n -- )
    size_t n = (size_t)dPop();
    if(data_here + n >= DATA_SIZE){
        fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tData space exhausted.\n", 0x7100);
        exit(EXIT_FAILURE);
    }
    data_here += n;
}
void forth_here(void *na){  // HERE  ( -- addr )
    dPush((cell)&data_space[data_here]);
}

// VARIABLE name  ( -- )  creates a word that does ( -- addr )
void forth_variable(void *na){
    State *state = (State *)na;
    if(state->curr->next == NULL){
        fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tVARIABLE missing name.\n", 0x5400);
        return;
    }
    state->curr = state->curr->next;
    size_t len = (size_t)(state->curr->end - state->curr->start);
    char name[MAX_TOKEN_LEN] = {0};
    memcpy(name, state->curr->start, len);

    if(data_here + sizeof(cell) >= DATA_SIZE){
        fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tData space exhausted.\n", 0x7100);
        exit(EXIT_FAILURE);
    }
    cell *slot = (cell *)&data_space[data_here];
    *slot = 0;
    data_here += sizeof(cell);

    Body *body = bodyInit();
    Action a = {0};
    a.type = ACT_NUM;
    a.num  = (cell)slot;
    bodyAppend(body, a);
    Action eof = {0};
    eof.type = ACT_EOF;
    bodyAppend(body, eof);

    dictionaryAdd(state->dict, entryInit(name, len, WORD_USERDEF, runUserWord, 0, body));
}

// CONSTANT name  ( n -- )  creates a word that does ( -- n )
void forth_constant(void *na){
    State *state = (State *)na;
    cell val = dPop();
    if(state->curr->next == NULL){
        fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tCONSTANT missing name.\n", 0x5401);
        return;
    }
    state->curr = state->curr->next;
    size_t len = (size_t)(state->curr->end - state->curr->start);
    char name[MAX_TOKEN_LEN] = {0};
    memcpy(name, state->curr->start, len);

    Body *body = bodyInit();
    Action a = {0};
    a.type = ACT_NUM;
    a.num  = val;
    bodyAppend(body, a);
    Action eof = {0};
    eof.type = ACT_EOF;
    bodyAppend(body, eof);

    dictionaryAdd(state->dict, entryInit(name, len, WORD_USERDEF, runUserWord, 0, body));
}

// execution tokens

void forth_tick(void *na){  // '  ( -- xt )
    State *state = (State *)na;
    if(state->curr->next == NULL){
        fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\t\\' missing word name.\n", 0x5402);
        return;
    }
    state->curr = state->curr->next;
    size_t len  = (size_t)(state->curr->end - state->curr->start);
    Entry *word = dictionaryLookup(state->dict, state->curr->start, len);
    if(word == NULL){
        fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tUnknown word for \\': \" %.*s \".\n", 0x1020, (int)len, state->curr->start);
        return;
    }
    dPush((cell)word);
}

void forth_execute(void *na){ // EXECUTE  ( xt -- )
    Entry *word = (Entry *)dPop();
    word->behavior(na);
}

// number base

void forth_hex(void *na){     num_base = 16; }
void forth_decimal(void *na){ num_base = 10; }
void forth_base(void *na){    dPush((cell)num_base); }  // BASE  ( -- base )

// misc i/o

void forth_key(void *na){   // KEY  ( -- char )
    dPush((cell)getchar());
}
void forth_type(void *na){  // TYPE  ( addr len -- )
    cell len  = dPop();
    cell addr = dPop();
    fwrite((char *)addr, 1, (size_t)len, stdout);
}

// file evaluation

void evalFile(State *state, const char *path){
    FILE *f = fopen(path, "r");
    if(f == NULL){
        fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tCould not open file: %s\n", 0x6100, path);
        return;
    }

    // save token state so nested INCLUDE / returning to the REPL works correctly
    TokenList *saved_list = state->list;
    TokenList *saved_curr = state->curr;

    char buf[LINE_SIZE];
    while(fgets(buf, sizeof(buf), f) != NULL){
        buf[strcspn(buf, "\n")] = '\0';

        // skip blank lines and line comments
        if(buf[0] == '\0' || (buf[0] == '\\' && buf[1] == ' '))
            continue;

        TokenList *list = tokenizeSrc(buf);
        state->list = state->curr = list;

        for(; state->curr != NULL; state->curr = state->curr->next){
            char   temp[MAX_TOKEN_LEN] = {0};
            size_t len = (state->curr->end - state->curr->start);

            if(len >= MAX_TOKEN_LEN){
                fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tToken is longer than buffer length of (%d).\n", 0x0670, MAX_TOKEN_LEN);
                fclose(f);
                return;
            }

            memcpy(temp, state->curr->start, len);

            char  *p;
            scell  num = SIGNED strtoimax(temp, &p, num_base);

            Entry *word;

            if(((p-temp) == (ptrdiff_t)len) && (p != temp)){
                dPush(UNSIGNED num);
                continue;
            }
            else if((word = dictionaryLookup(state->dict, state->curr->start, len)) != NULL){
                if(word->flags & FLAG_COMPILE_ONLY){
                    fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\t'%s' is compile-only.\n", 0x5500, word->name);
                } else {
                    word->behavior(state);
                }
            }
            else{
                fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tUnidentified word.\n\t\tWord: \" %.*s \"\n", 0x1018, (int)len, state->curr->start);
            }
        }
    }

    fclose(f);

    // restore token state
    state->list = saved_list;
    state->curr = saved_curr;
}

void forth_include(void *na){   // INCLUDE <path>
    State *state = (State *)na;
    if(state->curr->next == NULL){
        fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tINCLUDE missing filename.\n", 0x6101);
        return;
    }
    state->curr = state->curr->next;
    size_t len = (size_t)(state->curr->end - state->curr->start);
    char path[MAX_TOKEN_LEN] = {0};
    memcpy(path, state->curr->start, len);
    evalFile(state, path);
}

// registers all builtins into the dictionary

void registerBuiltins(State *state){
    Entry *temp = NULL;

    // i/o

    temp = entryInit(".", 1, WORD_BUILTIN, dot, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit(".s", 2, WORD_BUILTIN, dotS, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit(".\"", 2, WORD_BUILTIN, dotQuote, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("cr", 2, WORD_BUILTIN, cr, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("space", 5, WORD_BUILTIN, space, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("spaces", 6, WORD_BUILTIN, spaces, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("emit", 4, WORD_BUILTIN, emit, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("u.", 2, WORD_BUILTIN, unsigndot, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("words", 5, WORD_BUILTIN, words, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("bye", 3, WORD_BUILTIN, bye, 0, NULL);
    dictionaryAdd(state->dict, temp);

    // stack manipulation

    temp = entryInit("dup", 3, WORD_BUILTIN, dup, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("drop", 4, WORD_BUILTIN, drop, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("swap", 4, WORD_BUILTIN, swap, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("over", 4, WORD_BUILTIN, over, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("rot", 3, WORD_BUILTIN, rot, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("nip", 3, WORD_BUILTIN, nip, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("tuck", 4, WORD_BUILTIN, tuck, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("2dup", 4, WORD_BUILTIN, twodup, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("2drop", 5, WORD_BUILTIN, twodrop, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("depth", 5, WORD_BUILTIN, depth, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("clear", 5, WORD_BUILTIN, clear, 0, NULL);
    dictionaryAdd(state->dict, temp);

    // arithmetic

    temp = entryInit("mod", 3, WORD_BUILTIN, mod, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("/mod", 4, WORD_BUILTIN, slashmod, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("negate", 6, WORD_BUILTIN, negate, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("abs", 3, WORD_BUILTIN, fsabs, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("1+", 2, WORD_BUILTIN, oneplus, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("1-", 2, WORD_BUILTIN, onemin, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("2*", 2, WORD_BUILTIN, twotimes, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("2/", 2, WORD_BUILTIN, twodiv, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("+", 1, WORD_BUILTIN, fadd, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("-", 1, WORD_BUILTIN, fsub, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("*", 1, WORD_BUILTIN, fmul, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("/", 1, WORD_BUILTIN, fdiv, 0, NULL);
    dictionaryAdd(state->dict, temp);

    // comparison

    temp = entryInit("=", 1, WORD_BUILTIN, eq, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("<>", 2, WORD_BUILTIN, neq, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("<", 1, WORD_BUILTIN, lt, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit(">", 1, WORD_BUILTIN, gt, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("<=", 2, WORD_BUILTIN, lte, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit(">=", 2, WORD_BUILTIN, gte, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("0=", 2, WORD_BUILTIN, zeq, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("0<", 2, WORD_BUILTIN, zlt, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("0>", 2, WORD_BUILTIN, zgt, 0, NULL);
    dictionaryAdd(state->dict, temp);

    // bitwise

    temp = entryInit("and", 3, WORD_BUILTIN, and, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("or", 2, WORD_BUILTIN, or, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("xor", 3, WORD_BUILTIN, xor, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("invert", 6, WORD_BUILTIN, invert, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("lshift", 6, WORD_BUILTIN, lshift, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("rshift", 6, WORD_BUILTIN, rshift, 0, NULL);
    dictionaryAdd(state->dict, temp);

    // word definition

    temp = entryInit(":", 1, WORD_BUILTIN, userword, 0, NULL);
    dictionaryAdd(state->dict, temp);

    // return stack

    temp = entryInit(">R", 2, WORD_BUILTIN, tor, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("R>", 2, WORD_BUILTIN, rfrom, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("R@", 2, WORD_BUILTIN, rfetch, 0, NULL);
    dictionaryAdd(state->dict, temp);

    // conditionals (immediate + compile-only)

    temp = entryInit("IF",   2, WORD_BUILTIN, forth_if,   FLAG_IMMEDIATE|FLAG_COMPILE_ONLY, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("ELSE", 4, WORD_BUILTIN, forth_else, FLAG_IMMEDIATE|FLAG_COMPILE_ONLY, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("THEN", 4, WORD_BUILTIN, forth_then, FLAG_IMMEDIATE|FLAG_COMPILE_ONLY, NULL);
    dictionaryAdd(state->dict, temp);

    // loops (immediate + compile-only)

    temp = entryInit("BEGIN",  5, WORD_BUILTIN, forth_begin,     FLAG_IMMEDIATE|FLAG_COMPILE_ONLY, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("UNTIL",  5, WORD_BUILTIN, forth_until,     FLAG_IMMEDIATE|FLAG_COMPILE_ONLY, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("AGAIN",  5, WORD_BUILTIN, forth_again,     FLAG_IMMEDIATE|FLAG_COMPILE_ONLY, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("WHILE",  5, WORD_BUILTIN, forth_while,     FLAG_IMMEDIATE|FLAG_COMPILE_ONLY, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("REPEAT", 6, WORD_BUILTIN, forth_repeat,    FLAG_IMMEDIATE|FLAG_COMPILE_ONLY, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("DO",     2, WORD_BUILTIN, forth_do,        FLAG_IMMEDIATE|FLAG_COMPILE_ONLY, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("LOOP",   4, WORD_BUILTIN, forth_loop,      FLAG_IMMEDIATE|FLAG_COMPILE_ONLY, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("+LOOP",  5, WORD_BUILTIN, forth_plus_loop, FLAG_IMMEDIATE|FLAG_COMPILE_ONLY, NULL);
    dictionaryAdd(state->dict, temp);

    // loop helpers (regular builtins, usable anywhere)

    temp = entryInit("I",     1, WORD_BUILTIN, forth_i,     0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("J",     1, WORD_BUILTIN, forth_j,     0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("LEAVE", 5, WORD_BUILTIN, forth_leave, 0, NULL);
    dictionaryAdd(state->dict, temp);

    // exit / recurse (immediate + compile-only)

    temp = entryInit("EXIT",    4, WORD_BUILTIN, forth_exit,    FLAG_IMMEDIATE|FLAG_COMPILE_ONLY, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("RECURSE", 7, WORD_BUILTIN, forth_recurse, FLAG_IMMEDIATE|FLAG_COMPILE_ONLY, NULL);
    dictionaryAdd(state->dict, temp);

    // memory

    temp = entryInit("@",        1, WORD_BUILTIN, forth_fetch,      0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("!",        1, WORD_BUILTIN, forth_store,      0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("+!",       2, WORD_BUILTIN, forth_plus_store, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("?",        1, WORD_BUILTIN, forth_question,   0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("CELLS",    5, WORD_BUILTIN, forth_cells,      0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("ALLOT",    5, WORD_BUILTIN, forth_allot,      0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("HERE",     4, WORD_BUILTIN, forth_here,       0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("VARIABLE", 8, WORD_BUILTIN, forth_variable,   0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("CONSTANT", 8, WORD_BUILTIN, forth_constant,   0, NULL);
    dictionaryAdd(state->dict, temp);

    // execution tokens

    temp = entryInit("'",       1, WORD_BUILTIN, forth_tick,    0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("EXECUTE", 7, WORD_BUILTIN, forth_execute, 0, NULL);
    dictionaryAdd(state->dict, temp);

    // number base

    temp = entryInit("HEX",     3, WORD_BUILTIN, forth_hex,     0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("DECIMAL", 7, WORD_BUILTIN, forth_decimal, 0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("BASE",    4, WORD_BUILTIN, forth_base,    0, NULL);
    dictionaryAdd(state->dict, temp);

    // misc i/o

    temp = entryInit("KEY",  3, WORD_BUILTIN, forth_key,  0, NULL);
    dictionaryAdd(state->dict, temp);

    temp = entryInit("TYPE", 4, WORD_BUILTIN, forth_type, 0, NULL);
    dictionaryAdd(state->dict, temp);

    // file evaluation

    temp = entryInit("INCLUDE", 7, WORD_BUILTIN, forth_include, 0, NULL);
    dictionaryAdd(state->dict, temp);
}
