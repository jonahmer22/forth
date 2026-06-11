#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/types.h"
#include "include/stack.h"
#include "include/token.h"
#include "include/dict.h"
#include "include/builtins.h"
#include "arena/arena.h"
#include "cliargs/cliargs.h"

// version / description

#define VERSION "0.1.0"
#define DESCRIPTION "A simplistic FORTH implementation made to be as dead simple as I can think of making a it."

int main(int argc, char **argv){
    // set up and parse CLI arguments
    cliargsSetDescription(DESCRIPTION);
    cliargsSetVersion(VERSION);
    cliargsParse(argc, argv);

    arenaInit();

    // central state
    State *state = stateInit(dictionaryInit(), NULL, dstack, &dsp, rstack, &rsp);

    registerBuiltins(state);

    // run a file if one was passed on the command line
    char *filename = cliargsSubcommand();
    if(filename){
        evalFile(state, filename);
        arenaDestroy();
        return EXIT_SUCCESS;
    }

    printf("\x1b[1;31mFORTHISH\x1b[0m\n%s\n", DESCRIPTION);

    // main loop
    for(; printf("\n\x1b[34m>\x1b[0m "), fgets(state->ibuf, LINE_SIZE, stdin);){
        // exit case
        state->ibuf[strcspn(state->ibuf, "\n")] = '\0';
        if(strcmp(state->ibuf, "bye") == 0){
            break;
        }

        state->ibuf_len = strlen(state->ibuf);
        state->inp = 0;

        // tokenListPrint(list);

        state->list = state->curr = tokenizeSrc(state->ibuf);
        interpLine(state);
    }

    arenaDestroy();
    return EXIT_SUCCESS;
}
