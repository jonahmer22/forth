#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/dict.h"
#include "../arena/arena.h"

// body bs

Body *bodyInit(){
    Body *body = malloc(sizeof(Body));

    body->actions = malloc(sizeof(Action)*BASE_SIZE);
    body->cap  = BASE_SIZE;
    body->used = 0;

    return body;
}

void bodyEnsureSize(Body *body, size_t size){
    if(body->cap < size){
        for(; body->cap < size; body->cap <<= 1);

        body->actions = realloc(body->actions, sizeof(Action)*body->cap);
    }
}

void bodyAppend(Body *body, Action action){
    bodyEnsureSize(body, body->used + 1);

    body->actions[body->used++] = action;
}

// dictionary bs

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

Dictionary *dictionaryInit(){
    Dictionary *temp = arenaAlloc(sizeof(Dictionary));

    temp->head = NULL;
    temp->len  = 0;

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

State *stateInit(Dictionary *dict, TokenList *list, cell *dstack, size_t *dsp, uint64_t *rstack, size_t *rsp){
    State *state = arenaAlloc(sizeof(State));

    {
        state->dict = dict;

        state->compiling = NULL;
        state->compBody  = NULL;

        state->list = state->curr = list;

        state->dstack = dstack;
        state->dsp    = dsp;

        state->rstack = rstack;
        state->rsp    = rsp;

        state->compileMode = 0;

        state->status = 0;

        state->csp = 0;
    }

    return state;
}
