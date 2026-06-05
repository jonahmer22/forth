#ifndef BUILTINS_H
#define BUILTINS_H

#include <stddef.h>
#include <stdint.h>
#include "dict.h"

// number base (changed by HEX / DECIMAL)
extern int num_base;

// data space for VARIABLE / ALLOT / HERE
extern uint8_t data_space[DATA_SIZE];
extern size_t  data_here;

// i/o

void dot(void *na);         // .
void dotS(void *na);        // .s
void dotQuote(void *na);    // ."
void cr(void *na);          // cr
void space(void *na);       // space
void spaces(void *na);      // spaces
void emit(void *na);        // emit
void unsigndot(void *na);   // u.
void words(void *na);       // words
void bye(void *na);         // bye

// stack manipulation

void dup(void *na);         // dup
void drop(void *na);        // drop
void swap(void *na);        // swap
void over(void *na);        // over
void rot(void *na);         // rot
void nip(void *na);         // nip
void tuck(void *na);        // tuck
void twodup(void *na);      // 2dup
void twodrop(void *na);     // 2drop
void depth(void *na);       // depth
void clear(void *na);       // clear

// arithmetic

void mod(void *na);         // mod
void slashmod(void *na);    // /mod
void negate(void *na);      // negate
void fsabs(void *na);       // abs
void oneplus(void *na);     // 1+
void onemin(void *na);      // 1-
void twotimes(void *na);    // 2*
void twodiv(void *na);      // 2/
void fadd(void *na);        // +
void fsub(void *na);        // -
void fmul(void *na);        // *
void fdiv(void *na);        // /

// comparison

void eq(void *na);          // =
void neq(void *na);         // <>
void lt(void *na);          // <
void gt(void *na);          // >
void lte(void *na);         // <=
void gte(void *na);         // >=
void zeq(void *na);         // 0=
void zlt(void *na);         // 0<
void zgt(void *na);         // 0>

// bitwise

void and(void *na);         // and
void or(void *na);          // or
void xor(void *na);         // xor
void invert(void *na);      // invert
void lshift(void *na);      // lshift
void rshift(void *na);      // rshift

// return stack

void tor(void *na);         // >R
void rfrom(void *na);       // R>
void rfetch(void *na);      // R@

// conditionals (immediate, compile-only)

void forth_if(void *na);    // IF
void forth_else(void *na);  // ELSE
void forth_then(void *na);  // THEN

// loops (immediate, compile-only)

void forth_begin(void *na);     // BEGIN
void forth_until(void *na);     // UNTIL
void forth_again(void *na);     // AGAIN
void forth_while(void *na);     // WHILE
void forth_repeat(void *na);    // REPEAT
void forth_do(void *na);        // DO
void forth_loop(void *na);      // LOOP
void forth_plus_loop(void *na); // +LOOP

// loop helpers

void forth_i(void *na);     // I
void forth_j(void *na);     // J
void forth_leave(void *na); // LEAVE

// exit / recurse (compile-only)

void forth_exit(void *na);    // EXIT
void forth_recurse(void *na); // RECURSE

// memory

void forth_fetch(void *na);      // @
void forth_store(void *na);      // !
void forth_plus_store(void *na); // +!
void forth_question(void *na);   // ?
void forth_cells(void *na);      // CELLS
void forth_allot(void *na);      // ALLOT
void forth_here(void *na);       // HERE
void forth_variable(void *na);   // VARIABLE
void forth_constant(void *na);   // CONSTANT

// execution tokens

void forth_tick(void *na);    // '
void forth_execute(void *na); // EXECUTE

// number base

void forth_hex(void *na);     // HEX
void forth_decimal(void *na); // DECIMAL
void forth_base(void *na);    // BASE

// misc i/o

void forth_key(void *na);  // KEY
void forth_type(void *na); // TYPE

// word definition

void runUserWord(void *na);
void userword(void *na);    // :

// registers all builtins into the dictionary
void registerBuiltins(State *state);

#endif
