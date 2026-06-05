#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/types.h"
#include "../include/stack.h"
#include "../include/token.h"
#include "../include/dict.h"
#include "../include/builtins.h"
#include "../arena/arena.h"

// input line

char line[LINE_SIZE];

int main(void){
    arenaInit();

    // central state
    State *state = stateInit(dictionaryInit(), NULL, dstack, &dsp, rstack, &rsp);

    registerBuiltins(state);

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
            char   temp[MAX_TOKEN_LEN] = {0};
            size_t len = (state->curr->end-state->curr->start);

            if(len >= MAX_TOKEN_LEN){
                fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tToken is longer than buffer length of (%d).\nToken contents: \" %.*s \"", 0x0670, MAX_TOKEN_LEN, (int)len, state->curr->start);
                exit(EXIT_FAILURE);
            }

            memcpy(temp, state->curr->start, len >= MAX_TOKEN_LEN ? MAX_TOKEN_LEN-1 : len);

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
            else{   // error for undefined word
                fprintf(stderr, "\x1b[1;93m[ERROR 0x%04X]:\x1b[0m\tUnidentified word.\n\t\tWord: \" %.*s \"\n", 0x1018, (int)len, state->curr->start);
            }
        }
    }

    arenaDestroy();
    return EXIT_SUCCESS;
}
