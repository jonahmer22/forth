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
void forth_qdo(void *na);       // ?DO

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

// new memory / data space words

void forth_cfetch(void *na);     // C@
void forth_cstore(void *na);     // C!
void forth_ccomma(void *na);     // C,
void forth_comma(void *na);      // ,
void forth_cell_plus(void *na);  // CELL+
void forth_char_plus(void *na);  // CHAR+
void forth_chars(void *na);      // CHARS
void forth_aligned(void *na);    // ALIGNED
void forth_align(void *na);      // ALIGN
void forth_fill(void *na);       // FILL
void forth_move(void *na);       // MOVE
void forth_erase(void *na);      // ERASE
void forth_pad(void *na);        // PAD
void forth_unused(void *na);     // UNUSED
void forth_create(void *na);     // CREATE
void forth_does(void *na);       // DOES>

// execution tokens

void forth_tick(void *na);    // '
void forth_execute(void *na); // EXECUTE

// number base

void forth_hex(void *na);     // HEX
void forth_decimal(void *na); // DECIMAL
void forth_base(void *na);    // BASE

// misc i/o

void forth_key(void *na);    // KEY
void forth_type(void *na);   // TYPE
void forth_accept(void *na); // ACCEPT
void forth_dot_r(void *na);  // .R
void forth_u_dot_r(void *na);// U.R

// number formatting

void forth_hold(void *na);         // HOLD
void forth_holds(void *na);        // HOLDS
void forth_sign(void *na);         // SIGN
void forth_less_hash(void *na);    // <#
void forth_hash(void *na);         // #
void forth_hash_s(void *na);       // #S
void forth_hash_greater(void *na); // #>

// stack

void forth_qdup(void *na);    // ?DUP
void forth_max(void *na);     // MAX
void forth_min(void *na);     // MIN
void forth_2over(void *na);   // 2OVER
void forth_2swap(void *na);   // 2SWAP
void forth_2store(void *na);  // 2!
void forth_2fetch(void *na);  // 2@
void forth_pick(void *na);    // PICK
void forth_roll(void *na);    // ROLL
void forth_2tor(void *na);    // 2>R
void forth_2rfrom(void *na);  // 2R>
void forth_2rfetch(void *na); // 2R@

// arithmetic

void forth_star_slash(void *na);      // */
void forth_star_slash_mod(void *na);  // */MOD
void forth_mstar(void *na);           // M*
void forth_umstar(void *na);          // UM*
void forth_fm_slash_mod(void *na);    // FM/MOD
void forth_sm_slash_rem(void *na);    // SM/REM
void forth_um_slash_mod(void *na);    // UM/MOD
void forth_s_to_d(void *na);          // S>D

// comparisons / booleans

void forth_zero_ne(void *na);   // 0<>
void forth_u_less(void *na);    // U<
void forth_u_greater(void *na); // U>
void forth_within(void *na);    // WITHIN
void forth_false(void *na);     // FALSE
void forth_true(void *na);      // TRUE
void forth_bl(void *na);        // BL

// characters / strings

void forth_char(void *na);       // CHAR
void forth_count(void *na);      // COUNT
void forth_s_quote(void *na);    // S"  (interpret)
void forth_s_quote_compile(void *na); // S"  (compile/smart)

// comments

void forth_backslash(void *na);  // '\'
void forth_paren(void *na);      // (
void forth_dot_paren(void *na);  // .(

// interpreter state

void forth_state(void *na);        // STATE
void forth_source(void *na);       // SOURCE
void forth_to_in(void *na);        // >IN
void forth_refill(void *na);       // REFILL
void forth_source_id(void *na);    // SOURCE-ID
void forth_evaluate(void *na);     // EVALUATE

// compile-mode words

void forth_lbracket(void *na);     // [
void forth_rbracket(void *na);     // ]
void forth_literal(void *na);      // LITERAL
void forth_tick_bracket(void *na); // [']
void forth_char_bracket(void *na); // [CHAR]
void forth_postpone(void *na);     // POSTPONE
void forth_immediate(void *na);    // IMMEDIATE

// dictionary / parsing

void forth_find(void *na);         // FIND
void forth_to_body(void *na);      // >BODY
void forth_word(void *na);         // WORD
void forth_to_number(void *na);    // >NUMBER

// error handling

void forth_abort(void *na);        // ABORT
void forth_abort_quote(void *na);  // ABORT"
void forth_quit(void *na);         // QUIT

// defining words

void forth_noname(void *na);       // :NONAME
void forth_value(void *na);        // VALUE
void forth_to(void *na);           // TO

// case/of

void forth_case(void *na);         // CASE
void forth_of(void *na);           // OF
void forth_endof(void *na);        // ENDOF
void forth_endcase(void *na);      // ENDCASE

// word definition

void runUserWord(void *na);
void userword(void *na);    // :

// shared interpreter loop

void interpLine(State *state);

// file evaluation

void evalFile(State *state, const char *path);
void forth_include(void *na);    // INCLUDE
void forth_included(void *na);   // INCLUDED

// environment

void forth_environment_q(void *na); // ENVIRONMENT?

// registers all builtins into the dictionary
void registerBuiltins(State *state);

#endif
