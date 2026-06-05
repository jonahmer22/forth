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
// forward declarations needed by execBody / doesRuntime
static void execBody(Body *body, void *na);
static void doesRuntime(void *na);

static void execBody(Body *body, void *na){
    State  *state  = (State *)na;
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
                Entry *w = action[i].word;
                if(w->type == WORD_USERDEF){
                    if(w->does_body){
                        dPush((cell)w->data_field);
                        execBody(w->does_body, na);
                    } else {
                        execBody(w->body, na);
                    }
                } else {
                    w->behavior(na);
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
            case ACT_DOES:{
                // patch last_created's body and behavior at runtime
                size_t does_start = (size_t)action[i].num;
                Body  *db = bodyInit();
                for(size_t j = does_start; action[j].type != ACT_EOF; j++){
                    bodyAppend(db, action[j]);
                }
                Action eof = {0}; eof.type = ACT_EOF;
                bodyAppend(db, eof);
                state->last_created->does_body = db;
                state->last_created->behavior  = doesRuntime;
                return; // stop executing the build part
            }
            default:{
                i++;
                break;
            }
        }
    }
}

// runtime for DOES> words: look up entry via state->curr, push data_field, run does_body
static void doesRuntime(void *na){
    State *state = (State *)na;
    Entry *word  = dictionaryLookup(state->dict, state->curr->start,
                                    (size_t)(state->curr->end - state->curr->start));
    dPush((cell)word->data_field);
    execBody(word->does_body, na);
}

// runUserWord is called by the interpreter loop; it looks up the word by
// state->curr and hands off to execBody
void runUserWord(void *na){
    State *state = (State *)na;
    Entry *word = dictionaryLookup(state->dict, state->curr->start, (size_t)(state->curr->end-state->curr->start));
    execBody(word->body, na);
}

// compileToken: process one token in compile mode (or interpret mode if state_var==0)
// used by both userword and ] to process tokens
static void compileToken(State *state){
    size_t len = (size_t)(state->curr->end - state->curr->start);

    // in interpret mode (after [), execute everything
    if(state->state_var == 0){
        char  *p;
        char   tmp[MAX_TOKEN_LEN] = {0};
        memcpy(tmp, state->curr->start, len);
        scell  num = SIGNED strtoimax(tmp, &p, num_base);
        if(((size_t)(p - tmp) == len) && (p != tmp)){
            dPush(UNSIGNED num);
        } else {
            Entry *word = dictionaryLookup(state->dict, state->curr->start, len);
            if(word) word->behavior(state);
            else fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tUnidentified word: \" %.*s \"\n", 0x1019, (int)len, state->curr->start);
        }
        return;
    }

    // compile mode
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
            // try to refill if reading from a file
            if(!stateRefill(state)){ fprintf(stderr, "[ERROR]: unterminated .\" \n"); return; }
            state->curr = tokenizeSrc(state->ibuf);
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
            word->behavior(state);
        } else {
            act.word = word;
            act.type = ACT_WORD;
            bodyAppend(state->compBody, act);
        }
    }
    else {
        fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tUnidentified word in definition.\n\t\tWord: \" %.*s \"\n", 0x1019, (int)len, state->curr->start);
    }
}

void userword(void *na){// : and ;
    State *state = (State *)na;

    // move past ':'
    state->curr = state->curr->next;

    // may need to refill if ':' was the last token on the line
    while(state->curr == NULL){
        if(!stateRefill(state)) return;
        state->curr = tokenizeSrc(state->ibuf);
    }

    char   name[MAX_TOKEN_LEN] = {0};
    size_t nameLen = (size_t)(state->curr->end - state->curr->start);
    Body  *body = bodyInit();

    if(nameLen == 1 && *state->curr->start == ';'){
        fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tEmpty word definition.\n", 0x5331);
        return;
    }

    memcpy(name, state->curr->start, nameLen);
    state->curr = state->curr->next;

    // set up compile state
    state->compileMode  = 1;
    state->state_var    = 1;
    state->compBody     = body;
    Entry *stub = entryInit(name, nameLen, WORD_USERDEF, runUserWord, 0, body);
    state->compiling = stub;

    for(;;){
        // refill if we've run out of tokens on this line
        while(state->curr == NULL){
            if(!stateRefill(state)){
                fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tUnexpected EOF in word definition.\n", 0x5332);
                goto done;
            }
            state->curr = tokenizeSrc(state->ibuf);
        }

        // check for end of definition (only when in compile mode)
        if(state->state_var != 0 && state->curr->end - state->curr->start == 1 && *state->curr->start == ';'){
            state->curr = state->curr->next; // consume ;
            break;
        }

        compileToken(state);
        state->curr = state->curr->next;
    }

done:
    {   // terminate body
    Action eof_act = {0};
    eof_act.type = ACT_EOF;
    bodyAppend(state->compBody, eof_act);
    }

    dictionaryAdd(state->dict, stub);

    state->compileMode  = 0;
    state->state_var    = 0;
    state->compiling    = NULL;
    state->compBody     = NULL;
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

// shared interpreter loop used by evalFile and main
void interpLine(State *state){
    for(; state->curr != NULL; state->curr = state->curr ? state->curr->next : NULL){
        char   temp[MAX_TOKEN_LEN] = {0};
        size_t len = (size_t)(state->curr->end - state->curr->start);

        if(len >= MAX_TOKEN_LEN){
            fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tToken too long (%d max).\n", 0x0670, MAX_TOKEN_LEN);
            exit(EXIT_FAILURE);
        }

        memcpy(temp, state->curr->start, len);
        // update >IN to end of current token
        if(state->curr->end >= state->ibuf && state->curr->end <= state->ibuf + state->ibuf_len)
            state->inp = (cell)(state->curr->end - state->ibuf);

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

void evalFile(State *state, const char *path){
    FILE *f = fopen(path, "r");
    if(f == NULL){
        fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tCould not open file: %s\n", 0x6100, path);
        return;
    }

    // save outer input state
    FILE      *saved_input    = state->input;
    TokenList *saved_list     = state->list;
    TokenList *saved_curr     = state->curr;
    char       saved_ibuf[LINE_SIZE];
    size_t     saved_ibuf_len = state->ibuf_len;
    cell       saved_inp      = state->inp;
    memcpy(saved_ibuf, state->ibuf, LINE_SIZE);

    state->input = f;

    while(stateRefill(state)){
        state->list = state->curr = tokenizeSrc(state->ibuf);
        interpLine(state);
    }

    fclose(f);

    // restore outer input state
    state->input    = saved_input;
    state->list     = saved_list;
    state->curr     = saved_curr;
    state->ibuf_len = saved_ibuf_len;
    state->inp      = saved_inp;
    memcpy(state->ibuf, saved_ibuf, LINE_SIZE);
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

// ── comments ────────────────────────────────────────────────────────────────

void forth_backslash(void *na){     // \  line comment — consume rest of line
    State *state = (State *)na;
    // consume all remaining tokens on this line; interpLine will stop when curr==NULL
    while(state->curr != NULL) state->curr = state->curr->next;
}

void forth_paren(void *na){         // (  inline comment — skip until )
    // convention: leave state->curr at the ) token; interpLine's for-loop advances past it
    State *state = (State *)na;
    while(state->curr != NULL){
        size_t len = (size_t)(state->curr->end - state->curr->start);
        if(len > 0 && state->curr->start[len-1] == ')') return;
        state->curr = state->curr->next;
    }
    // not found on this line; consume further lines (multiline comments)
    while(stateRefill(state)){
        state->curr = tokenizeSrc(state->ibuf);
        while(state->curr != NULL){
            size_t len = (size_t)(state->curr->end - state->curr->start);
            if(len > 0 && state->curr->start[len-1] == ')') return;
            state->curr = state->curr->next;
        }
    }
}

void forth_dot_paren(void *na){     // .(  print until )
    State *state = (State *)na;
    while(state->curr != NULL){
        size_t len = (size_t)(state->curr->end - state->curr->start);
        int ends = (len > 0 && state->curr->start[len-1] == ')');
        size_t print_len = ends ? len - 1 : len;
        printf("%.*s", (int)print_len, state->curr->start);
        if(ends) return; // leave curr here; for-loop advances past
        state->curr = state->curr->next;
        putc(' ', stdout);
    }
}

// ── stack ops ────────────────────────────────────────────────────────────────

void forth_qdup(void *na){      // ?DUP  ( n -- n n | 0 )
    cell t = dPop();
    dPush(t);
    if(t) dPush(t);
}
void forth_max(void *na){       // MAX  ( n1 n2 -- max )
    scell b = SIGNED dPop();
    scell a = SIGNED dPop();
    dPush(UNSIGNED (a > b ? a : b));
}
void forth_min(void *na){       // MIN  ( n1 n2 -- min )
    scell b = SIGNED dPop();
    scell a = SIGNED dPop();
    dPush(UNSIGNED (a < b ? a : b));
}
void forth_2over(void *na){     // 2OVER  ( d1 d2 -- d1 d2 d1 )
    cell lo = dstack[dsp-4];
    cell hi = dstack[dsp-3];
    dPush(lo); dPush(hi);
}
void forth_2swap(void *na){     // 2SWAP  ( d1 d2 -- d2 d1 )
    cell d2hi = dPop(); cell d2lo = dPop();
    cell d1hi = dPop(); cell d1lo = dPop();
    dPush(d2lo); dPush(d2hi);
    dPush(d1lo); dPush(d1hi);
}
void forth_2store(void *na){    // 2!  ( d addr -- )
    cell addr = dPop();
    cell hi   = dPop();
    cell lo   = dPop();
    *(cell *)addr       = lo;
    *((cell *)addr + 1) = hi;
}
void forth_2fetch(void *na){    // 2@  ( addr -- d )
    cell addr = dPop();
    dPush(*(cell *)addr);
    dPush(*((cell *)addr + 1));
}
void forth_pick(void *na){      // PICK  ( xu...x1 x0 u -- xu...x1 x0 xu )
    cell u = dPop();
    dPush(dstack[dsp - 1 - u]);
}
void forth_roll(void *na){      // ROLL  ( xu xu-1...x0 u -- xu-1...x0 xu )
    cell u = dPop();
    if(u == 0) return;
    cell t = dstack[dsp - 1 - u];
    for(size_t i = dsp - 1 - u; i < dsp - 1; i++)
        dstack[i] = dstack[i+1];
    dstack[dsp-1] = t;
}
void forth_2tor(void *na){      // 2>R  ( d -- ) R:( -- d )
    cell hi = dPop(); cell lo = dPop();
    rPush((uint64_t)lo); rPush((uint64_t)hi);
}
void forth_2rfrom(void *na){    // 2R>  ( -- d ) R:( d -- )
    cell hi = (cell)rPop(); cell lo = (cell)rPop();
    dPush(lo); dPush(hi);
}
void forth_2rfetch(void *na){   // 2R@  ( -- d ) R:( d -- d )
    dPush((cell)rstack[rsp-2]);
    dPush((cell)rstack[rsp-1]);
}

// ── arithmetic ───────────────────────────────────────────────────────────────

void forth_star_slash(void *na){    // */  ( n1 n2 n3 -- n4 )  n4 = n1*n2/n3
    scell n3 = SIGNED dPop();
    scell n2 = SIGNED dPop();
    scell n1 = SIGNED dPop();
    if(n3 == 0){ fprintf(stderr, "[ERROR]: */ divide by zero\n"); return; }
    dPush(UNSIGNED ((n1 * n2) / n3));
}
void forth_star_slash_mod(void *na){ // */MOD  ( n1 n2 n3 -- n4 n5 )
    scell n3 = SIGNED dPop();
    scell n2 = SIGNED dPop();
    scell n1 = SIGNED dPop();
    if(n3 == 0){ fprintf(stderr, "[ERROR]: */MOD divide by zero\n"); return; }
    dPush(UNSIGNED ((n1 * n2) % n3));
    dPush(UNSIGNED ((n1 * n2) / n3));
}
void forth_mstar(void *na){     // M*  ( n1 n2 -- d )  signed 32x32->64
    scell n2 = SIGNED dPop();
    scell n1 = SIGNED dPop();
    int64_t d = (int64_t)n1 * (int64_t)n2;
    dPush((cell)(uint64_t)d & (cell)0xFFFFFFFFFFFFFFFFULL);
    dPush((cell)((uint64_t)d >> 32));
}
void forth_umstar(void *na){    // UM*  ( u1 u2 -- ud )  unsigned 32x32->64
    uint64_t u2 = (uint64_t)dPop();
    uint64_t u1 = (uint64_t)dPop();
    uint64_t ud = u1 * u2;
    dPush((cell)(ud & 0xFFFFFFFF));
    dPush((cell)(ud >> 32));
}
void forth_fm_slash_mod(void *na){ // FM/MOD  ( d n -- rem quot )  floored
    scell  n  = SIGNED dPop();
    cell   hi = dPop();
    cell   lo = dPop();
    int64_t d = (int64_t)(((uint64_t)hi << 32) | (uint64_t)lo);
    if(n == 0){ fprintf(stderr, "[ERROR]: FM/MOD divide by zero\n"); return; }
    int64_t q = d / (int64_t)n;
    int64_t r = d % (int64_t)n;
    // floored: adjust if remainder has wrong sign
    if(r != 0 && (r < 0) != ((int64_t)n < 0)){ q--; r += n; }
    dPush(UNSIGNED (scell)r);
    dPush(UNSIGNED (scell)q);
}
void forth_sm_slash_rem(void *na){ // SM/REM  ( d n -- rem quot )  symmetric
    scell  n  = SIGNED dPop();
    cell   hi = dPop();
    cell   lo = dPop();
    int64_t d = (int64_t)(((uint64_t)hi << 32) | (uint64_t)lo);
    if(n == 0){ fprintf(stderr, "[ERROR]: SM/REM divide by zero\n"); return; }
    dPush(UNSIGNED (scell)(d % (int64_t)n));
    dPush(UNSIGNED (scell)(d / (int64_t)n));
}
void forth_um_slash_mod(void *na){ // UM/MOD  ( ud u -- rem quot )  unsigned
    cell    u  = dPop();
    cell    hi = dPop();
    cell    lo = dPop();
    uint64_t ud = ((uint64_t)hi << 32) | (uint64_t)lo;
    if(u == 0){ fprintf(stderr, "[ERROR]: UM/MOD divide by zero\n"); return; }
    dPush((cell)(ud % u));
    dPush((cell)(ud / u));
}
void forth_s_to_d(void *na){    // S>D  ( n -- d )
    scell n = SIGNED dPop();
    dPush(UNSIGNED n);
    dPush(n < 0 ? (cell)-1 : 0);
}

// ── comparisons ──────────────────────────────────────────────────────────────

void forth_zero_ne(void *na){   // 0<>  ( n -- flag )
    dPush(dPop() != 0 ? (cell)-1 : 0);
}
void forth_u_less(void *na){    // U<  ( u1 u2 -- flag )
    cell u2 = dPop(); cell u1 = dPop();
    dPush(u1 < u2 ? (cell)-1 : 0);
}
void forth_u_greater(void *na){ // U>  ( u1 u2 -- flag )
    cell u2 = dPop(); cell u1 = dPop();
    dPush(u1 > u2 ? (cell)-1 : 0);
}
void forth_within(void *na){    // WITHIN  ( u ul uh -- flag )  ul <= u < uh
    cell uh = dPop(); cell ul = dPop(); cell u = dPop();
    dPush((u - ul) < (uh - ul) ? (cell)-1 : 0);
}
void forth_false(void *na){ dPush(0); }         // FALSE
void forth_true(void *na){  dPush((cell)-1); }  // TRUE
void forth_bl(void *na){    dPush(32); }         // BL  ( -- 32 )

// ── memory / data space ──────────────────────────────────────────────────────

void forth_cfetch(void *na){    // C@  ( addr -- char )
    cell addr = dPop();
    dPush((cell)*(uint8_t *)addr);
}
void forth_cstore(void *na){    // C!  ( char addr -- )
    cell addr = dPop();
    *(uint8_t *)addr = (uint8_t)dPop();
}
void forth_ccomma(void *na){    // C,  ( char -- )
    if(data_here + 1 >= DATA_SIZE){ fprintf(stderr, "[ERROR]: data space full\n"); exit(EXIT_FAILURE); }
    data_space[data_here++] = (uint8_t)dPop();
}
void forth_comma(void *na){     // ,  ( n -- )
    if(data_here + sizeof(cell) > DATA_SIZE){ fprintf(stderr, "[ERROR]: data space full\n"); exit(EXIT_FAILURE); }
    *(cell *)&data_space[data_here] = dPop();
    data_here += sizeof(cell);
}
void forth_cell_plus(void *na){ // CELL+  ( addr -- addr+cell )
    dPush(dPop() + sizeof(cell));
}
void forth_char_plus(void *na){ // CHAR+  ( addr -- addr+1 )
    dPush(dPop() + 1);
}
void forth_chars(void *na){     // CHARS  ( n -- n )  chars are 1 byte; no-op
    // already in bytes
}
void forth_aligned(void *na){   // ALIGNED  ( addr -- a-addr )
    cell addr = dPop();
    cell mask = sizeof(cell) - 1;
    dPush((addr + mask) & ~mask);
}
void forth_align(void *na){     // ALIGN  ( -- )  align data_here
    size_t mask = sizeof(cell) - 1;
    data_here = (data_here + mask) & ~mask;
}
void forth_fill(void *na){      // FILL  ( addr u char -- )
    cell ch   = dPop();
    cell u    = dPop();
    cell addr = dPop();
    memset((void *)addr, (int)ch, (size_t)u);
}
void forth_move(void *na){      // MOVE  ( addr1 addr2 u -- )
    cell u     = dPop();
    cell addr2 = dPop();
    cell addr1 = dPop();
    memmove((void *)addr2, (void *)addr1, (size_t)u);
}
void forth_erase(void *na){     // ERASE  ( addr u -- )
    cell u    = dPop();
    cell addr = dPop();
    memset((void *)addr, 0, (size_t)u);
}
void forth_pad(void *na){       // PAD  ( -- addr )  scratch area 64 bytes past HERE
    dPush((cell)&data_space[data_here + 64]);
}
void forth_unused(void *na){    // UNUSED  ( -- u )
    dPush((cell)(DATA_SIZE - data_here));
}

// ── characters / strings ─────────────────────────────────────────────────────

void forth_char(void *na){      // CHAR  ( "<spaces>name" -- char )
    State *state = (State *)na;
    if(state->curr->next == NULL){ dPush(0); return; }
    state->curr = state->curr->next;
    dPush((cell)(unsigned char)*state->curr->start);
}
void forth_count(void *na){     // COUNT  ( c-addr -- c-addr+1 len )
    cell addr = dPop();
    cell len  = (cell)*(uint8_t *)addr;
    dPush(addr + 1);
    dPush(len);
}
void forth_dot_r(void *na){     // .R  ( n width -- )
    int  w = (int)SIGNED dPop();
    scell n = SIGNED dPop();
    char buf[32];
    int  len = snprintf(buf, sizeof(buf), P_SGN_ESC, (scell)n);
    for(int i = len; i < w; i++) putc(' ', stdout);
    fputs(buf, stdout);
}
void forth_u_dot_r(void *na){   // U.R  ( u width -- )
    int  w = (int)SIGNED dPop();
    cell u = dPop();
    char buf[32];
    int  len = snprintf(buf, sizeof(buf), P_UNS_ESC, u);
    for(int i = len; i < w; i++) putc(' ', stdout);
    fputs(buf, stdout);
}

// ── number formatting (<# # #S #> HOLD SIGN) ────────────────────────────────

void forth_hold(void *na){      // HOLD  ( char -- )
    State *state = (State *)na;
    if(state->hbuf_len < sizeof(state->hbuf))
        state->hbuf[state->hbuf_len++] = (char)dPop();
}
void forth_holds(void *na){     // HOLDS  ( addr len -- )
    State *state = (State *)na;
    cell len  = dPop();
    cell addr = dPop();
    for(cell i = len; i > 0; i--)
        if(state->hbuf_len < sizeof(state->hbuf))
            state->hbuf[state->hbuf_len++] = ((char *)addr)[i-1];
}
void forth_sign(void *na){      // SIGN  ( n -- )
    if(SIGNED dPop() < 0){
        State *state = (State *)na;
        if(state->hbuf_len < sizeof(state->hbuf))
            state->hbuf[state->hbuf_len++] = '-';
    }
}
void forth_less_hash(void *na){ // <#  ( -- )  start number conversion
    State *state = (State *)na;
    state->hbuf_len = 0;
}
void forth_hash(void *na){      // #  ( ud -- ud )  convert one digit
    State *state = (State *)na;
    cell hi   = dPop();
    cell lo   = dPop();
    uint64_t ud = ((uint64_t)hi << 32) | (uint64_t)lo;
    uint64_t digit = ud % (uint64_t)num_base;
    ud /= (uint64_t)num_base;
    char ch = digit < 10 ? '0' + (char)digit : 'a' + (char)(digit - 10);
    if(state->hbuf_len < sizeof(state->hbuf))
        state->hbuf[state->hbuf_len++] = ch;
    dPush((cell)(ud & 0xFFFFFFFF));
    dPush((cell)(ud >> 32));
}
void forth_hash_s(void *na){    // #S  ( ud -- 0 0 )  convert all digits
    State *state = (State *)na;
    do { forth_hash(na); }
    while(dstack[dsp-1] != 0 || dstack[dsp-2] != 0);
    (void)state;
}
void forth_hash_greater(void *na){ // #>  ( ud -- addr len )
    State *state = (State *)na;
    dPop(); dPop(); // drop ud
    // reverse the hold buffer in place
    for(size_t i = 0, j = state->hbuf_len - 1; i < j; i++, j--){
        char t = state->hbuf[i];
        state->hbuf[i] = state->hbuf[j];
        state->hbuf[j] = t;
    }
    dPush((cell)state->hbuf);
    dPush((cell)state->hbuf_len);
}

// ── string literals ──────────────────────────────────────────────────────────

// helper: parse S" string from the current token stream into data_space
// returns pointer to null-terminated copy; pushes addr+len if push_it
static void parseStringLiteral(State *state, int push_it){
    // state->curr currently points to the S" token
    // the string follows up to the closing "
    // our tokenizer already split the string content as the next token
    if(state->curr->next == NULL){
        if(push_it){ dPush((cell)""); dPush(0); }
        return;
    }
    state->curr = state->curr->next;
    const char *start = state->curr->start;
    const char *end   = state->curr->end;
    size_t len = (size_t)(end - start);

    // copy into data_space so it survives
    if(data_here + len + 1 > DATA_SIZE){ fprintf(stderr, "[ERROR]: data space full\n"); exit(EXIT_FAILURE); }
    char *dst = (char *)&data_space[data_here];
    memcpy(dst, start, len);
    dst[len] = '\0';
    data_here += len + 1;

    if(push_it){ dPush((cell)dst); dPush((cell)len); }
}

void forth_s_quote(void *na){   // S"  ( -- addr len )  at runtime
    State *state = (State *)na;
    parseStringLiteral(state, 1);
}

// compile-time S": emit two ACT_NUM actions for addr and len
void forth_s_quote_compile(void *na){
    State *state = (State *)na;
    if(!state->compileMode){ forth_s_quote(na); return; }

    if(state->curr->next == NULL){
        Action a = {0}; a.type = ACT_NUM; a.num = (cell)"";
        bodyAppend(state->compBody, a);
        a.num = 0; bodyAppend(state->compBody, a);
        return;
    }
    state->curr = state->curr->next;
    const char *start = state->curr->start;
    const char *end   = state->curr->end;
    size_t len = (size_t)(end - start);

    if(data_here + len + 1 > DATA_SIZE){ fprintf(stderr, "[ERROR]: data space full\n"); exit(EXIT_FAILURE); }
    char *dst = (char *)&data_space[data_here];
    memcpy(dst, start, len);
    dst[len] = '\0';
    data_here += len + 1;

    Action a = {0};
    a.type = ACT_NUM; a.num = (cell)dst;
    bodyAppend(state->compBody, a);
    a.num = (cell)len;
    bodyAppend(state->compBody, a);
}

// ── I/O ──────────────────────────────────────────────────────────────────────

void forth_accept(void *na){    // ACCEPT  ( addr len -- actual )
    cell maxlen = dPop();
    cell addr   = dPop();
    char *buf   = (char *)addr;
    if(fgets(buf, (int)maxlen + 1, stdin) == NULL){ dPush(0); return; }
    size_t n = strcspn(buf, "\n");
    buf[n] = '\0';
    dPush((cell)n);
}

// ── interpreter state ────────────────────────────────────────────────────────

void forth_state(void *na){     // STATE  ( -- addr )
    State *state = (State *)na;
    dPush((cell)&state->state_var);
}
void forth_source(void *na){    // SOURCE  ( -- addr len )
    State *state = (State *)na;
    dPush((cell)state->ibuf);
    dPush((cell)state->ibuf_len);
}
void forth_to_in(void *na){     // >IN  ( -- addr )
    State *state = (State *)na;
    dPush((cell)&state->inp);
}
void forth_refill(void *na){    // REFILL  ( -- flag )
    State *state = (State *)na;
    int ok = stateRefill(state);
    if(ok) state->curr = tokenizeSrc(state->ibuf);
    dPush(ok ? (cell)-1 : 0);
}
void forth_source_id(void *na){ // SOURCE-ID  ( -- 0|-1|fileid )
    State *state = (State *)na;
    dPush(state->input ? (cell)(intptr_t)state->input : 0);
}
void forth_evaluate(void *na){  // EVALUATE  ( addr len -- )
    State *state = (State *)na;
    cell len  = dPop();
    cell addr = dPop();

    // save current source state
    char       saved_ibuf[LINE_SIZE];
    size_t     saved_len = state->ibuf_len;
    cell       saved_inp = state->inp;
    TokenList *saved_list = state->list;
    TokenList *saved_curr = state->curr;
    FILE      *saved_input = state->input;
    memcpy(saved_ibuf, state->ibuf, LINE_SIZE);

    // copy the string into ibuf and evaluate
    size_t n = len < LINE_SIZE - 1 ? (size_t)len : LINE_SIZE - 1;
    memcpy(state->ibuf, (char *)addr, n);
    state->ibuf[n] = '\0';
    state->ibuf_len = n;
    state->inp = 0;
    state->input = NULL; // mark as string input

    state->list = state->curr = tokenizeSrc(state->ibuf);
    interpLine(state);

    memcpy(state->ibuf, saved_ibuf, LINE_SIZE);
    state->ibuf_len = saved_len;
    state->inp      = saved_inp;
    state->list     = saved_list;
    state->curr     = saved_curr;
    state->input    = saved_input;
}

// ── compile-mode words ───────────────────────────────────────────────────────

void forth_lbracket(void *na){  // [  switch to interpret mode
    State *state = (State *)na;
    state->compileMode = 0;
    state->state_var   = 0;
}

void forth_rbracket(void *na){  // ]  switch to compile mode
    State *state = (State *)na;
    state->compileMode = 1;
    state->state_var   = 1;
}

void forth_literal(void *na){   // LITERAL  ( n -- )  compile n as literal
    State *state = (State *)na;
    if(!state->compileMode){ return; } // in interpret mode it's a no-op
    Action a = {0};
    a.type = ACT_NUM;
    a.num  = dPop();
    bodyAppend(state->compBody, a);
}

void forth_tick_bracket(void *na){ // [']  compile xt of next word
    State *state = (State *)na;
    if(!state->compileMode){ compile_error_not_in_def("[']"); return; }
    state->curr = state->curr->next;
    if(!state->curr) return;
    size_t len  = (size_t)(state->curr->end - state->curr->start);
    Entry *word = dictionaryLookup(state->dict, state->curr->start, len);
    if(!word){ fprintf(stderr, "[ERROR]: ['] unknown word\n"); return; }
    Action a = {0};
    a.type = ACT_NUM;
    a.num  = (cell)word;
    bodyAppend(state->compBody, a);
}

void forth_char_bracket(void *na){ // [CHAR]  compile char of next token
    State *state = (State *)na;
    if(!state->compileMode){ compile_error_not_in_def("[CHAR]"); return; }
    state->curr = state->curr->next;
    if(!state->curr) return;
    Action a = {0};
    a.type = ACT_NUM;
    a.num  = (cell)(unsigned char)*state->curr->start;
    bodyAppend(state->compBody, a);
}

void forth_postpone(void *na){  // POSTPONE  compile compilation semantics of next word
    State *state = (State *)na;
    if(!state->compileMode){ compile_error_not_in_def("POSTPONE"); return; }
    state->curr = state->curr->next;
    if(!state->curr) return;
    size_t len  = (size_t)(state->curr->end - state->curr->start);
    Entry *word = dictionaryLookup(state->dict, state->curr->start, len);
    if(!word){ fprintf(stderr, "[ERROR]: POSTPONE unknown word: %.*s\n", (int)len, state->curr->start); return; }
    Action a = {0};
    a.type = ACT_WORD;
    a.word = word;
    bodyAppend(state->compBody, a);
}

void forth_immediate(void *na){ // IMMEDIATE  mark last defined word as immediate
    State *state = (State *)na;
    if(state->dict->head)
        state->dict->head->flags |= FLAG_IMMEDIATE;
}

// ── defining words ───────────────────────────────────────────────────────────

void forth_create(void *na){    // CREATE name  ( -- )
    State *state = (State *)na;
    if(state->curr->next == NULL){ fprintf(stderr, "[ERROR]: CREATE missing name\n"); return; }
    state->curr = state->curr->next;
    size_t len = (size_t)(state->curr->end - state->curr->start);
    char name[MAX_TOKEN_LEN] = {0};
    memcpy(name, state->curr->start, len);

    // align data_here
    size_t mask = sizeof(cell) - 1;
    data_here = (data_here + mask) & ~mask;

    // the data field is the current HERE
    cell *data = (cell *)&data_space[data_here];

    // body: push data address
    Body *body = bodyInit();
    Action a = {0};
    a.type = ACT_NUM;
    a.num  = (cell)data;
    bodyAppend(body, a);
    Action eof = {0}; eof.type = ACT_EOF;
    bodyAppend(body, eof);

    Entry *e = entryInit(name, len, WORD_USERDEF, runUserWord, 0, body);
    e->data_field = data;
    dictionaryAdd(state->dict, e);
    state->last_created = e;
}

void forth_does(void *na){      // DOES>  (immediate, compile-only)
    State *state = (State *)na;
    if(!state->compileMode){ compile_error_not_in_def("DOES>"); return; }
    // emit ACT_DOES with the index of the first does-part action
    Action a = {0};
    a.type = ACT_DOES;
    a.num  = (cell)(state->compBody->used + 1); // does part starts after this action
    bodyAppend(state->compBody, a);
}

// ── dictionary / parsing ─────────────────────────────────────────────────────

void forth_find(void *na){      // FIND  ( c-addr -- c-addr 0 | xt 1 | xt -1 )
    State *state = (State *)na;
    cell addr  = dPop();
    uint8_t len = *(uint8_t *)addr;
    const char *name = (const char *)addr + 1;
    Entry *e = dictionaryLookup(state->dict, name, len);
    if(!e){ dPush(addr); dPush(0); return; }
    dPush((cell)e);
    dPush(e->flags & FLAG_IMMEDIATE ? (cell)1 : (cell)-1);
}

void forth_to_body(void *na){   // >BODY  ( xt -- a-addr )
    Entry *e = (Entry *)dPop();
    dPush((cell)e->data_field);
}

void forth_word(void *na){      // WORD  ( char -- c-addr )  parse delimited word
    State *state = (State *)na;
    char delim = (char)dPop();
    // skip to next token; for simplicity use state->curr->next
    if(state->curr->next) state->curr = state->curr->next;
    if(!state->curr){ dPush((cell)""); return; }
    size_t len = (size_t)(state->curr->end - state->curr->start);
    if(len > 255) len = 255;
    // build a counted string in PAD area
    char *buf = (char *)&data_space[data_here + 64];
    buf[0] = (char)len;
    memcpy(buf+1, state->curr->start, len);
    buf[len+1] = '\0';
    dPush((cell)buf);
    (void)delim;
}

void forth_to_number(void *na){ // >NUMBER  ( ud c-addr u1 -- ud c-addr+u2 u2 )
    cell   u1   = dPop();
    cell   addr = dPop();
    cell   ud_hi = dPop();
    cell   ud_lo = dPop();
    uint64_t ud = ((uint64_t)ud_hi << 32) | ud_lo;
    const char *p = (const char *)addr;
    size_t i = 0;
    while(i < u1){
        char c = p[i];
        int  digit;
        if(c >= '0' && c <= '9')      digit = c - '0';
        else if(c >= 'a' && c <= 'z') digit = c - 'a' + 10;
        else if(c >= 'A' && c <= 'Z') digit = c - 'A' + 10;
        else break;
        if(digit >= num_base) break;
        ud = ud * (uint64_t)num_base + (uint64_t)digit;
        i++;
    }
    dPush((cell)(ud & 0xFFFFFFFF));
    dPush((cell)(ud >> 32));
    dPush(addr + (cell)i);
    dPush((cell)(u1 - i));
}

// ── error handling ───────────────────────────────────────────────────────────

void forth_abort(void *na){     // ABORT
    dClear(); rClear();
    fprintf(stderr, "\x1b[1;93m[ABORT]\x1b[0m\n");
    // consume remaining tokens
    State *state = (State *)na;
    state->curr = NULL;
}

void forth_abort_quote(void *na){ // ABORT"  ( flag -- )  if flag, print and abort
    State *state = (State *)na;
    if(state->compileMode){ compile_error_not_in_def("ABORT\""); return; }
    // get the string (next token in token stream)
    if(state->curr->next) state->curr = state->curr->next;
    else return;
    cell flag = dPop();
    if(flag){
        const char *s   = state->curr->start;
        size_t      len = (size_t)(state->curr->end - state->curr->start);
        fprintf(stderr, "\x1b[1;93m[ABORT]: %.*s\x1b[0m\n", (int)len, s);
        dClear(); rClear();
        state->curr = NULL;
    }
}

void forth_quit(void *na){      // QUIT  — abandon current operation, back to prompt
    State *state = (State *)na;
    state->curr = NULL;
    dClear(); rClear();
    state->compileMode = 0;
    state->state_var   = 0;
    state->compBody    = NULL;
    state->compiling   = NULL;
}

// ── :NONAME ──────────────────────────────────────────────────────────────────

void forth_noname(void *na){    // :NONAME  ( -- xt )
    State *state = (State *)na;
    Body  *body  = bodyInit();

    state->compileMode  = 1;
    state->state_var    = 1;
    state->compBody     = body;

    // create anonymous entry with empty name
    Entry *stub = entryInit("", 0, WORD_USERDEF, runUserWord, 0, body);
    state->compiling = stub;

    dPush((cell)stub);  // push xt before compiling

    // compilation continues — the user writes code then ;
    // but we need to process it now (similar to userword without consuming a name)
    // just return; ] is now active and the main loop will compile tokens
}

// ── CASE/OF/ENDOF/ENDCASE ────────────────────────────────────────────────────

void forth_case(void *na){      // CASE  push 0 marker onto cpstack
    State *state = (State *)na;
    if(!state->compileMode){ compile_error_not_in_def("CASE"); return; }
    state->cpstack[state->csp++] = 0; // case-sys: count of OF branches
}

void forth_of(void *na){        // OF  ( x1 x2 -- | x1 )
    State *state = (State *)na;
    if(!state->compileMode){ compile_error_not_in_def("OF"); return; }
    // compile: OVER = IF DROP ... (THEN patched by ENDOF)
    // emit: OVER
    Entry *over_e = dictionaryLookup(state->dict, "over", 4);
    Entry *eq_e   = dictionaryLookup(state->dict, "=",    1);
    if(over_e){ Action a={0}; a.type=ACT_WORD; a.word=over_e; bodyAppend(state->compBody, a); }
    if(eq_e)  { Action a={0}; a.type=ACT_WORD; a.word=eq_e;   bodyAppend(state->compBody, a); }
    // emit BRANCH0 (placeholder)
    Action br = {0}; br.type = ACT_BRANCH0; br.num = 0;
    bodyAppend(state->compBody, br);
    size_t br_idx = state->compBody->used - 1;
    // emit DROP (the matched value)
    Entry *drop_e = dictionaryLookup(state->dict, "drop", 4);
    if(drop_e){ Action a={0}; a.type=ACT_WORD; a.word=drop_e; bodyAppend(state->compBody, a); }
    state->cpstack[state->csp++] = br_idx;
}

void forth_endof(void *na){     // ENDOF
    State *state = (State *)na;
    if(!state->compileMode){ compile_error_not_in_def("ENDOF"); return; }
    // emit unconditional branch past the ENDCASE (placeholder)
    Action a = {0}; a.type = ACT_BRANCH; a.num = 0;
    bodyAppend(state->compBody, a);
    size_t endof_br = state->compBody->used - 1;
    // patch the OF's BRANCH0 to jump here (past ENDOF's branch)
    size_t of_br = state->cpstack[--state->csp];
    state->compBody->actions[of_br].num = (cell)state->compBody->used;
    // save ENDOF branch for ENDCASE to patch
    state->cpstack[state->csp++] = endof_br;
    // re-push the case-sys counter slot
    // rotate: move endof_br below the case count
    // simple: push endof_br on top, ENDCASE will collect them
}

void forth_endcase(void *na){   // ENDCASE  ( x -- )
    State *state = (State *)na;
    if(!state->compileMode){ compile_error_not_in_def("ENDCASE"); return; }
    // emit DROP (discard selector)
    Entry *drop_e = dictionaryLookup(state->dict, "drop", 4);
    if(drop_e){ Action a={0}; a.type=ACT_WORD; a.word=drop_e; bodyAppend(state->compBody, a); }
    // patch all ENDOF branches to here
    // we need to find them - they're everything on cpstack until the 0 marker
    while(state->csp > 0 && state->cpstack[state->csp-1] != 0){
        size_t br = state->cpstack[--state->csp];
        state->compBody->actions[br].num = (cell)state->compBody->used;
    }
    if(state->csp > 0) state->csp--; // pop the 0 marker
}

// ── VALUE / TO ───────────────────────────────────────────────────────────────

void forth_value(void *na){     // VALUE name  ( n -- )
    State *state = (State *)na;
    cell val = dPop();
    if(state->curr->next == NULL){ fprintf(stderr, "[ERROR]: VALUE missing name\n"); return; }
    state->curr = state->curr->next;
    size_t len = (size_t)(state->curr->end - state->curr->start);
    char name[MAX_TOKEN_LEN] = {0};
    memcpy(name, state->curr->start, len);

    // allocate a cell in data_space to hold the value
    size_t mask = sizeof(cell) - 1;
    data_here = (data_here + mask) & ~mask;
    cell *slot = (cell *)&data_space[data_here];
    *slot = val;
    data_here += sizeof(cell);

    // the word pushes *slot (the value), not the address
    // achieved by: push slot address, then call @
    Body *body = bodyInit();
    Action a = {0}; a.type = ACT_NUM; a.num = (cell)slot;
    bodyAppend(body, a);
    Entry *fetch_e = dictionaryLookup(state->dict, "@", 1);
    if(fetch_e){ Action b = {0}; b.type = ACT_WORD; b.word = fetch_e; bodyAppend(body, b); }
    Action eof = {0}; eof.type = ACT_EOF; bodyAppend(body, eof);

    Entry *e = entryInit(name, len, WORD_USERDEF, runUserWord, 0, body);
    e->data_field = slot;
    // mark as a VALUE so TO knows to store
    e->flags |= 0x10; // repurpose a flag bit
    dictionaryAdd(state->dict, e);
}

void forth_to(void *na){        // TO name  ( n -- )
    State *state = (State *)na;
    if(state->curr->next == NULL){ fprintf(stderr, "[ERROR]: TO missing name\n"); return; }
    state->curr = state->curr->next;
    size_t len  = (size_t)(state->curr->end - state->curr->start);
    Entry *e = dictionaryLookup(state->dict, state->curr->start, len);
    if(!e || !e->data_field){ fprintf(stderr, "[ERROR]: TO: not a VALUE\n"); return; }
    *e->data_field = dPop();
}

// ── INCLUDED ─────────────────────────────────────────────────────────────────

void forth_included(void *na){  // INCLUDED  ( addr len -- )
    State *state = (State *)na;
    cell len  = dPop();
    cell addr = dPop();
    char path[MAX_TOKEN_LEN] = {0};
    size_t n = len < MAX_TOKEN_LEN - 1 ? (size_t)len : MAX_TOKEN_LEN - 1;
    memcpy(path, (char *)addr, n);
    evalFile(state, path);
}

// ── ENVIRONMENT? ─────────────────────────────────────────────────────────────

void forth_environment_q(void *na){ // ENVIRONMENT?  ( addr len -- false | ... true )
    cell len  = dPop();
    cell addr = dPop();
    // report the things we know about; everything else is false
    char name[64] = {0};
    size_t n = len < 63 ? (size_t)len : 63;
    memcpy(name, (char *)addr, n);
    if(strcmp(name, "/COUNTED-STRING") == 0){ dPush(255); dPush((cell)-1); return; }
    if(strcmp(name, "/HOLD") == 0){           dPush(64);  dPush((cell)-1); return; }
    if(strcmp(name, "/PAD") == 0){            dPush(64);  dPush((cell)-1); return; }
    if(strcmp(name, "ADDRESS-UNIT-BITS") == 0){ dPush(8); dPush((cell)-1); return; }
    if(strcmp(name, "FLOORED") == 0){         dPush(0);   dPush((cell)-1); return; }
    if(strcmp(name, "MAX-CHAR") == 0){        dPush(255); dPush((cell)-1); return; }
    if(strcmp(name, "MAX-N") == 0){           dPush((cell)((cell)1<<(sizeof(cell)*8-1))-1); dPush((cell)-1); return; }
    if(strcmp(name, "MAX-U") == 0){           dPush((cell)-1); dPush((cell)-1); return; }
    if(strcmp(name, "STACK-CELLS") == 0){     dPush(STACK_SIZE); dPush((cell)-1); return; }
    dPush(0); // false — not found
}

// ── ?DO ──────────────────────────────────────────────────────────────────────

void forth_qdo(void *na){       // ?DO  ( limit start -- )  skip if equal
    State *state = (State *)na;
    if(!state->compileMode){ compile_error_not_in_def("?DO"); return; }
    // emit: 2DUP = BRANCH0(exit) DO
    // at runtime: if limit==start skip the loop entirely
    // We emit a runtime check: push limit==start flag, branch0 to after LOOP if equal
    // Then emit ACT_DO
    Action br = {0}; br.type = ACT_BRANCH0; br.num = 0; // patched by LOOP
    bodyAppend(state->compBody, br);
    size_t br_idx = state->compBody->used - 1;
    // emit ACT_DO
    Action do_act = {0}; do_act.type = ACT_DO; do_act.num = 0;
    bodyAppend(state->compBody, do_act);
    size_t do_idx = state->compBody->used - 1;
    // push both on cpstack: br_idx first (for ?DO skip), then do_idx (for LOOP)
    state->cpstack[state->csp++] = br_idx;
    state->cpstack[state->csp++] = do_idx;
}

// ── registers all builtins into the dictionary

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

    temp = entryInit("INCLUDE",  7, WORD_BUILTIN, forth_include,  0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("INCLUDED", 8, WORD_BUILTIN, forth_included, 0, NULL);
    dictionaryAdd(state->dict, temp);

    // comments

    temp = entryInit("\\", 1, WORD_BUILTIN, forth_backslash, FLAG_IMMEDIATE, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("(", 1, WORD_BUILTIN, forth_paren, FLAG_IMMEDIATE, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit(".(", 2, WORD_BUILTIN, forth_dot_paren, FLAG_IMMEDIATE, NULL);
    dictionaryAdd(state->dict, temp);

    // stack extras

    temp = entryInit("?DUP",  4, WORD_BUILTIN, forth_qdup,    0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("MAX",   3, WORD_BUILTIN, forth_max,     0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("MIN",   3, WORD_BUILTIN, forth_min,     0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("2OVER", 5, WORD_BUILTIN, forth_2over,   0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("2SWAP", 5, WORD_BUILTIN, forth_2swap,   0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("2!",    2, WORD_BUILTIN, forth_2store,  0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("2@",    2, WORD_BUILTIN, forth_2fetch,  0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("PICK",  4, WORD_BUILTIN, forth_pick,    0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("ROLL",  4, WORD_BUILTIN, forth_roll,    0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("2>R",   3, WORD_BUILTIN, forth_2tor,    0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("2R>",   3, WORD_BUILTIN, forth_2rfrom,  0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("2R@",   3, WORD_BUILTIN, forth_2rfetch, 0, NULL);
    dictionaryAdd(state->dict, temp);

    // arithmetic extras

    temp = entryInit("*/",      2, WORD_BUILTIN, forth_star_slash,     0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("*/MOD",   5, WORD_BUILTIN, forth_star_slash_mod, 0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("M*",      2, WORD_BUILTIN, forth_mstar,          0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("UM*",     3, WORD_BUILTIN, forth_umstar,         0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("FM/MOD",  6, WORD_BUILTIN, forth_fm_slash_mod,   0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("SM/REM",  6, WORD_BUILTIN, forth_sm_slash_rem,   0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("UM/MOD",  6, WORD_BUILTIN, forth_um_slash_mod,   0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("S>D",     3, WORD_BUILTIN, forth_s_to_d,         0, NULL);
    dictionaryAdd(state->dict, temp);

    // comparisons / booleans

    temp = entryInit("0<>",   3, WORD_BUILTIN, forth_zero_ne,   0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("U<",    2, WORD_BUILTIN, forth_u_less,    0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("U>",    2, WORD_BUILTIN, forth_u_greater, 0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("WITHIN",6, WORD_BUILTIN, forth_within,    0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("FALSE", 5, WORD_BUILTIN, forth_false,     0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("TRUE",  4, WORD_BUILTIN, forth_true,      0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("BL",    2, WORD_BUILTIN, forth_bl,        0, NULL);
    dictionaryAdd(state->dict, temp);

    // memory extras

    temp = entryInit("C@",      2, WORD_BUILTIN, forth_cfetch,    0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("C!",      2, WORD_BUILTIN, forth_cstore,    0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("C,",      2, WORD_BUILTIN, forth_ccomma,    0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit(",",       1, WORD_BUILTIN, forth_comma,     0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("CELL+",   5, WORD_BUILTIN, forth_cell_plus, 0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("CHAR+",   5, WORD_BUILTIN, forth_char_plus, 0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("CHARS",   5, WORD_BUILTIN, forth_chars,     0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("ALIGNED", 7, WORD_BUILTIN, forth_aligned,   0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("ALIGN",   5, WORD_BUILTIN, forth_align,     0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("FILL",    4, WORD_BUILTIN, forth_fill,      0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("MOVE",    4, WORD_BUILTIN, forth_move,      0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("ERASE",   5, WORD_BUILTIN, forth_erase,     0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("PAD",     3, WORD_BUILTIN, forth_pad,       0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("UNUSED",  6, WORD_BUILTIN, forth_unused,    0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("CREATE",  6, WORD_BUILTIN, forth_create,    0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("DOES>",   5, WORD_BUILTIN, forth_does,      FLAG_IMMEDIATE|FLAG_COMPILE_ONLY, NULL);
    dictionaryAdd(state->dict, temp);

    // character / string

    temp = entryInit("CHAR",  4, WORD_BUILTIN, forth_char,           0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("COUNT", 5, WORD_BUILTIN, forth_count,          0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("S\"",   2, WORD_BUILTIN, forth_s_quote_compile, FLAG_IMMEDIATE, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit(".R",    2, WORD_BUILTIN, forth_dot_r,           0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("U.R",   3, WORD_BUILTIN, forth_u_dot_r,         0, NULL);
    dictionaryAdd(state->dict, temp);

    // number formatting

    temp = entryInit("HOLD", 4, WORD_BUILTIN, forth_hold,         0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("HOLDS",5, WORD_BUILTIN, forth_holds,        0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("SIGN", 4, WORD_BUILTIN, forth_sign,         0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("<#",   2, WORD_BUILTIN, forth_less_hash,    0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("#",    1, WORD_BUILTIN, forth_hash,         0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("#S",   2, WORD_BUILTIN, forth_hash_s,       0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("#>",   2, WORD_BUILTIN, forth_hash_greater, 0, NULL);
    dictionaryAdd(state->dict, temp);

    // i/o extras

    temp = entryInit("ACCEPT", 6, WORD_BUILTIN, forth_accept, 0, NULL);
    dictionaryAdd(state->dict, temp);

    // interpreter state

    temp = entryInit("STATE",     5, WORD_BUILTIN, forth_state,     0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("SOURCE",    6, WORD_BUILTIN, forth_source,    0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit(">IN",       3, WORD_BUILTIN, forth_to_in,     0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("REFILL",    6, WORD_BUILTIN, forth_refill,    0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("SOURCE-ID", 9, WORD_BUILTIN, forth_source_id, 0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("EVALUATE",  8, WORD_BUILTIN, forth_evaluate,  0, NULL);
    dictionaryAdd(state->dict, temp);

    // compile-mode words

    temp = entryInit("[",         1, WORD_BUILTIN, forth_lbracket,    FLAG_IMMEDIATE, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("]",         1, WORD_BUILTIN, forth_rbracket,    0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("LITERAL",   7, WORD_BUILTIN, forth_literal,     FLAG_IMMEDIATE, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("[']",       3, WORD_BUILTIN, forth_tick_bracket, FLAG_IMMEDIATE|FLAG_COMPILE_ONLY, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("[CHAR]",    6, WORD_BUILTIN, forth_char_bracket, FLAG_IMMEDIATE|FLAG_COMPILE_ONLY, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("POSTPONE",  8, WORD_BUILTIN, forth_postpone,    FLAG_IMMEDIATE|FLAG_COMPILE_ONLY, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("IMMEDIATE", 9, WORD_BUILTIN, forth_immediate,   0, NULL);
    dictionaryAdd(state->dict, temp);

    // dictionary / parsing

    temp = entryInit("FIND",     4, WORD_BUILTIN, forth_find,      0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit(">BODY",    5, WORD_BUILTIN, forth_to_body,   0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("WORD",     4, WORD_BUILTIN, forth_word,      0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit(">NUMBER",  7, WORD_BUILTIN, forth_to_number, 0, NULL);
    dictionaryAdd(state->dict, temp);

    // error handling

    temp = entryInit("ABORT",    5, WORD_BUILTIN, forth_abort,        0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("ABORT\"",  6, WORD_BUILTIN, forth_abort_quote,  0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("QUIT",     4, WORD_BUILTIN, forth_quit,         0, NULL);
    dictionaryAdd(state->dict, temp);

    // defining words

    temp = entryInit(":NONAME",  7, WORD_BUILTIN, forth_noname,       0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("VALUE",    5, WORD_BUILTIN, forth_value,        0, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("TO",       2, WORD_BUILTIN, forth_to,           0, NULL);
    dictionaryAdd(state->dict, temp);

    // case / of

    temp = entryInit("CASE",    4, WORD_BUILTIN, forth_case,    FLAG_IMMEDIATE|FLAG_COMPILE_ONLY, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("OF",      2, WORD_BUILTIN, forth_of,      FLAG_IMMEDIATE|FLAG_COMPILE_ONLY, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("ENDOF",   5, WORD_BUILTIN, forth_endof,   FLAG_IMMEDIATE|FLAG_COMPILE_ONLY, NULL);
    dictionaryAdd(state->dict, temp);
    temp = entryInit("ENDCASE",  7, WORD_BUILTIN, forth_endcase, FLAG_IMMEDIATE|FLAG_COMPILE_ONLY, NULL);
    dictionaryAdd(state->dict, temp);

    // ?DO

    temp = entryInit("?DO",     3, WORD_BUILTIN, forth_qdo, FLAG_IMMEDIATE|FLAG_COMPILE_ONLY, NULL);
    dictionaryAdd(state->dict, temp);

    // environment

    temp = entryInit("ENVIRONMENT?", 12, WORD_BUILTIN, forth_environment_q, 0, NULL);
    dictionaryAdd(state->dict, temp);
}
