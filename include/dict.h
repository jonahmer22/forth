#ifndef DICT_H
#define DICT_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <setjmp.h>
#include "types.h"
#include "token.h"

// compiling stuff

typedef struct Entry Entry;

typedef enum ActionType{
    ACT_NUM = 0,   // push num
    ACT_PRINT,     // print str
    ACT_WORD,      // call word->behavior
    ACT_BRANCH,    // unconditional jump; num = target index
    ACT_BRANCH0,   // jump if top-of-stack == 0 (pops); num = target index
    ACT_DO,        // ( limit start -- ) R:( -- limit i ); num = exit index (after LOOP)
    ACT_LOOP,      // R:( limit i -- ) if ++i<limit branch; num = body-start index
    ACT_PLUS_LOOP, // R:( limit i -- ) if (i+=n)<limit branch; num = body-start index
    ACT_I,         // ( -- i ) copy top of loop frame
    ACT_J,         // ( -- j ) copy outer loop index
    ACT_EXIT,         // return from word early
    ACT_COMPILE_WORD, // compile-time: append ACT_WORD(word) to current compBody
    ACT_LEAVE,     // discard loop frame and branch past LOOP; num = target index
    ACT_DOES,        // does> runtime: patch last-created word; num = does-part start index
    ACT_ABORT_QUOTE, // runtime ABORT": str = message; pops flag, aborts if nonzero
    ACT_EOF
} ActionType;

typedef struct Action{
    ActionType type;

    cell   num; // also used for storing encoded branch offsets
    char  *str;
    Entry *word;
} Action;

typedef struct Body{
    Action *actions;

    size_t used;
    size_t cap;
} Body;

Body *bodyInit(void);
void  bodyEnsureSize(Body *body, size_t size);
void  bodyAppend(Body *body, Action action);

// dictionary bs

typedef enum WordType{
    WORD_BUILTIN = 0,
    WORD_USERDEF
} WordType;

typedef struct Entry{
    char   name[MAX_TOKEN_LEN];
    size_t nLen;

    // marks whether or not it's a builtin or user defined
    WordType type;

    // for builtins they should just point to a C function I guess
    void (*behavior)(void *);

    // for user defined words, this should be compiled
    // idk what the fuck to do here, I need to implement a basic VM for this first?
    // apparently the answer is this
    Body *body;

    // for CREATE'd / DOES> words
    cell  *data_field;  // points into data_space
    Body  *does_body;   // runtime body set by DOES>

    // immediate, hidden, compileonly, etc.
    uint8_t flags;

    struct Entry *prev;
} Entry;

Entry *entryInit(const char *name, size_t nLen, WordType type, void (*behavior)(void *), uint8_t flags, Body *body);

typedef struct Dictionary{
    Entry *head;

    size_t len;
} Dictionary;

Dictionary *dictionaryInit(void);
void        dictionaryAdd(Dictionary *dict, Entry *entry);
Entry      *dictionaryLookup(Dictionary *dict, const char *start, size_t nLen);

// universal state struct

typedef struct State{
    Dictionary *dict;

    Entry *compiling;    // the word that is currently being compiled
    Body  *compBody;     // the body of the word that is currently being compiled

    TokenList *list;
    TokenList *curr;

    cell   *dstack;
    size_t *dsp;

    uint64_t *rstack;
    size_t   *rsp;

    int     compileMode;

    uint8_t status;

    // compile-time backpatch stack
    size_t cpstack[64];
    size_t csp;

    // LEAVE forward-branch patch list (indexed by loop nesting)
    size_t leave_patches[256];
    size_t leave_sp;

    // input / source state
    FILE   *input;           // current input file (NULL = stdin)
    char    ibuf[LINE_SIZE]; // current input line (SOURCE)
    size_t  ibuf_len;        // length of ibuf
    cell    inp;             // >IN - parse offset into ibuf

    // addressable STATE variable (0 = interpret, 1 = compile)
    cell    state_var;

    // last defined and last CREATEd entries
    Entry  *last_created;

    // number-output hold area for <# # #S #> HOLD SIGN
    char    hbuf[68];
    size_t  hbuf_len;

    jmp_buf quit_jmp;        // longjmp target for QUIT/ABORT
    int     quit_jmp_valid;  // 1 once quit_jmp is set
    int     in_evaluate;     // 1 while inside EVALUATE (SOURCE-ID = -1)
} State;

State *stateInit(Dictionary *dict, TokenList *list, cell *dstack, size_t *dsp, uint64_t *rstack, size_t *rsp);

// read next line from state->input (or stdin) into state->ibuf
// returns 1 on success, 0 on EOF
int stateRefill(State *state);

#endif
