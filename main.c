#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/types.h"
#include "include/stack.h"
#include "include/token.h"
#include "include/dict.h"
#include "include/builtins.h"
#include "arena/arena.h"

// version / description

#define VERSION     "0.1.0"
#define DESCRIPTION "a FORTH-like language made to be as dead simple as I can think of making a language."

static void printVersion(){
    printf("forth %s\n", VERSION);
}

static void printHelp(){
    printf("forth %s\n", VERSION);
    printf("%s\n\n", DESCRIPTION);
    printf("usage:\n");
    printf("  forth              start the interactive REPL\n");
    printf("  forth <file>       run a .fs file\n");
    printf("\n");
    printf("options:\n");
    printf("  -h, --help         show this message\n");
    printf("  -v, --version      show version\n");
}

int main(int argc, char **argv){
    // handle flags before doing anything else
    if(argc > 1){
        if(strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0){
            printHelp();
            return EXIT_SUCCESS;
        }
        if(strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0){
            printVersion();
            return EXIT_SUCCESS;
        }
    }

    arenaInit();

    // central state
    State *state = stateInit(dictionaryInit(), NULL, dstack, &dsp, rstack, &rsp);

    registerBuiltins(state);

    // run a file if one was passed on the command line
    if(argc > 1){
        evalFile(state, argv[1]);
        arenaDestroy();
        return EXIT_SUCCESS;
    }

    printf("\x1b[1;31mFORTHISH\x1b[0m\nA FORTH-like language made to be as dead simple as I can think of making a language.\n");

    // main loop
    for(; printf("\n\x1b[34m>\x1b[0m "), fgets(state->ibuf, LINE_SIZE, stdin);){
        // exit case
        state->ibuf[strcspn(state->ibuf, "\n")] = '\0';
        if(strcmp(state->ibuf, "bye") == 0){
            break;
        }

        state->ibuf_len = strlen(state->ibuf);
        state->inp      = 0;

        // tokenListPrint(list);

        state->list = state->curr = tokenizeSrc(state->ibuf);
        interpLine(state);
    }

    arenaDestroy();
    return EXIT_SUCCESS;
}
