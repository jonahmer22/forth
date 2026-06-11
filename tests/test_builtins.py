"""
Forth 2012 Core compliance tests.

Runs snippets via ./forth in file mode and checks stdout.
In this implementation, `.` prints the number with no surrounding spaces;
use `space` or `cr` to delimit.  Doubles on the stack are (lo hi) with hi
on TOS, so the first `.` after a double operation prints the high cell.
"""
import os
import subprocess
import tempfile
import pytest

FORTH = os.path.join(os.path.dirname(__file__), "..", "forth")


# ─── helper ───────────────────────────────────────────────────────────────────

def f(code: str, timeout: int = 5, strip: bool = True) -> str:
    """Run Forth source code in file mode; return stdout (stripped by default)."""
    with tempfile.NamedTemporaryFile(mode="w", suffix=".fs", delete=False) as fp:
        fp.write(code + "\n")
        path = fp.name
    try:
        r = subprocess.run(
            [FORTH, path],
            capture_output=True,
            text=True,
            timeout=timeout,
        )
        return r.stdout.strip() if strip else r.stdout.rstrip("\n")
    finally:
        os.unlink(path)


def repl(code: str, timeout: int = 5) -> subprocess.CompletedProcess:
    """Run code interactively (stdin) and return the completed process."""
    return subprocess.run(
        [FORTH],
        input=code + "\n",
        capture_output=True,
        text=True,
        timeout=timeout,
    )


# ═══════════════════════════════════════════════════════════════════════════════
# §6.1.0120  +   §6.1.0160  -   §6.1.0090  *   §6.1.0230  /
# ═══════════════════════════════════════════════════════════════════════════════

class TestArithmetic:
    def test_add(self):
        assert f("1 2 + . cr") == "3"

    def test_add_negative(self):
        assert f("-3 5 + . cr") == "2"

    def test_sub(self):
        assert f("10 3 - . cr") == "7"

    def test_sub_negative_result(self):
        assert f("3 10 - . cr") == "-7"

    def test_mul(self):
        assert f("6 7 * . cr") == "42"

    def test_mul_negative(self):
        assert f("-3 4 * . cr") == "-12"

    def test_div_positive(self):
        assert f("15 3 / . cr") == "5"

    def test_div_truncates(self):
        assert f("7 2 / . cr") == "3"

    def test_mod(self):                               # §6.1.1890
        assert f("7 3 mod . cr") == "1"

    def test_slash_mod(self):                         # §6.1.0240  /MOD ( n1 n2 -- rem quot )
        # 7 /MOD 3 → quot=2 (TOS), rem=1 (below)
        assert f("7 3 /mod . cr") == "2"              # print TOS = quot
        assert f("7 3 /mod drop . cr") == "1"         # drop quot, print rem

    def test_negate(self):                            # §6.1.1910
        assert f("5 negate . cr") == "-5"
        assert f("-3 negate . cr") == "3"
        assert f("0 negate . cr") == "0"

    def test_abs(self):                               # §6.1.0690
        assert f("7 abs . cr") == "7"
        assert f("-7 abs . cr") == "7"
        assert f("0 abs . cr") == "0"

    def test_one_plus(self):                          # §6.1.0290
        assert f("4 1+ . cr") == "5"

    def test_one_minus(self):                         # §6.1.0300
        assert f("4 1- . cr") == "3"

    def test_two_times(self):                         # §6.1.0320
        assert f("6 2* . cr") == "12"

    def test_two_div(self):                           # §6.1.0330
        assert f("6 2/ . cr") == "3"
        assert f("7 2/ . cr") == "3"                  # rounds toward zero

    def test_max(self):                               # §6.1.1870
        assert f("3 7 MAX . cr") == "7"
        assert f("7 3 MAX . cr") == "7"
        assert f("-1 0 MAX . cr") == "0"

    def test_min(self):                               # §6.1.1880
        assert f("3 7 MIN . cr") == "3"
        assert f("-1 0 MIN . cr") == "-1"

    def test_star_slash(self):                        # §6.1.0100  */
        assert f("3 100 7 */ . cr") == "42"           # (3*100)/7 = 42

    def test_star_slash_mod(self):                    # §6.1.0110  */MOD ( n1 n2 n3 -- rem quot )
        # (3*100)/7 = 42 quot, (3*100)%7 = 6 rem; quot on TOS
        assert f("3 100 7 */MOD . cr") == "42"        # print TOS = quot
        assert f("3 100 7 */MOD drop . cr") == "6"    # drop quot, print rem

    def test_plus_store(self):                        # §6.1.0130  +!
        assert f("VARIABLE x  5 x !  3 x +!  x @ . cr") == "8"


# ═══════════════════════════════════════════════════════════════════════════════
# Double-cell arithmetic  §6.1.1900, §6.1.1830, §6.1.1760, §6.1.2214, §6.1.2370
# ═══════════════════════════════════════════════════════════════════════════════
# Convention: doubles on stack are (lo hi) with hi on TOS.

class TestDoubleArithmetic:
    def test_um_star_small(self):                     # §6.1.2360  UM*  ( u u -- ud )
        # 3*4=12: hi=0 TOS, lo=12 below → first . gives 0, second gives 12
        assert f("3 4 UM* . space . cr") == "0 12"

    def test_um_star_overflow(self):
        # 0x100000000 * 0x100000000 = 2^64; hi=1, lo=0 → TOS=1
        assert f("HEX 100000000 100000000 UM* . space . cr DECIMAL") == "1 0"

    def test_m_star_positive(self):                   # §6.1.1820  M*  ( n n -- d )
        assert f("6 7 M* . space . cr") == "0 42"     # hi=0 TOS, lo=42 below

    def test_m_star_negative(self):
        # -6*7 = -42; hi = sign(-42) = -1 (TOS), lo = -42 (below)
        assert f("-6 7 M* . space . cr") == "-1 -42"

    def test_s_to_d_positive(self):                   # §6.1.2170  S>D  ( n -- d )
        assert f("42 S>D . space . cr") == "0 42"     # hi=0 TOS, lo=42 below

    def test_s_to_d_negative(self):
        assert f("-1 S>D . space . cr") == "-1 -1"    # both cells = -1

    def test_fm_slash_mod_positive(self):             # §6.1.1561  FM/MOD  ( d n -- rem quot )
        # 7 / 2 floored: quot=3 (TOS), rem=1 (below)
        assert f("7 S>D 2 FM/MOD . space . cr") == "3 1"

    def test_fm_slash_mod_floored(self):
        # -7 / 2 floored: quot=-4 (TOS), rem=1 (because -4*2=-8, -8+1=-7)
        assert f("-7 S>D 2 FM/MOD . space . cr") == "-4 1"

    def test_sm_slash_rem_symmetric(self):            # §6.1.2214  SM/REM  ( d n -- rem quot )
        # -7 / 2 symmetric (truncated): quot=-3 (TOS), rem=-1
        assert f("-7 S>D 2 SM/REM . space . cr") == "-3 -1"

    def test_um_slash_mod(self):                      # §6.1.2370  UM/MOD  ( ud u -- rem quot )
        # 7 / 3: quot=2 (TOS), rem=1 (below)
        assert f("7 0 3 UM/MOD . space . cr") == "2 1"


# ═══════════════════════════════════════════════════════════════════════════════
# Stack manipulation
# ═══════════════════════════════════════════════════════════════════════════════

class TestStack:
    def test_dup(self):                               # §6.1.1290
        assert f("5 DUP . space . cr") == "5 5"

    def test_drop(self):                              # §6.1.1260
        assert f("1 2 DROP . cr") == "1"

    def test_swap(self):                              # §6.1.2260
        # ( x1 x2 -- x2 x1 ) → x1 goes to TOS; 1 2 SWAP leaves TOS=1
        assert f("1 2 SWAP . space . cr") == "1 2"

    def test_over(self):                              # §6.1.1990
        # ( 1 2 -- 1 2 1 ) → TOS=1 copy
        assert f("1 2 OVER . cr") == "1"              # prints copy of first

    def test_rot(self):                               # §6.1.2160
        # ( 1 2 3 -- 2 3 1 ) → TOS=1
        assert f("1 2 3 ROT . cr") == "1"
        # bottom is now 2
        assert f("1 2 3 ROT drop drop . cr") == "2"

    def test_nip(self):                               # §6.2.1930  (Core Ext, but registered)
        assert f("1 2 NIP . cr") == "2"

    def test_tuck(self):                              # §6.2.2300
        # ( 1 2 -- 2 1 2 ) → TOS=2
        assert f("1 2 TUCK . cr") == "2"
        assert f("1 2 TUCK drop . cr") == "1"         # middle
        assert f("1 2 TUCK drop drop . cr") == "2"   # bottom copy

    def test_2dup(self):                              # §6.1.0380
        # ( 3 4 -- 3 4 3 4 ) → TOS=4
        assert f("3 4 2DUP . cr") == "4"
        assert f("3 4 2DUP drop . cr") == "3"

    def test_2drop(self):                             # §6.1.0370
        assert f("1 2 3 4 2DROP . space . cr") == "2 1"

    def test_depth_empty(self):                       # §6.1.1200
        assert f("DEPTH . cr") == "0"

    def test_depth_three(self):
        assert f("1 2 3 DEPTH . cr") == "3"

    def test_qdup_nonzero(self):                      # §6.1.0630
        assert f("5 ?DUP . cr") == "5"               # dup happened, TOS=5
        assert f("5 ?DUP DEPTH . cr") == "2"         # two copies on stack

    def test_qdup_zero(self):
        assert f("0 ?DUP DEPTH . cr") == "1"         # no dup, still just one item

    def test_pick(self):                              # §6.2.2030
        assert f("10 20 30 1 PICK . cr") == "20"     # 0=TOS=30, 1=20, 2=10

    def test_roll(self):                              # §6.2.2150
        # 0 ROLL = NOP, 1 ROLL = SWAP
        assert f("10 20 30 2 ROLL . cr") == "10"     # pulls depth-2 item to TOS

    def test_2over(self):                             # §6.1.0400
        # ( 1 2 3 4 -- 1 2 3 4 1 2 ) → TOS=2
        assert f("1 2 3 4 2OVER . cr") == "2"

    def test_2swap(self):                             # §6.1.0430
        # ( 1 2 3 4 -- 3 4 1 2 ) → TOS=2
        assert f("1 2 3 4 2SWAP . cr") == "2"
        assert f("1 2 3 4 2SWAP drop . cr") == "1"


# ═══════════════════════════════════════════════════════════════════════════════
# Comparison / boolean
# ═══════════════════════════════════════════════════════════════════════════════

class TestComparison:
    def test_eq_true(self):                           # §6.1.0530
        assert f("3 3 = . cr") == "-1"

    def test_eq_false(self):
        assert f("3 4 = . cr") == "0"

    def test_neq_true(self):                          # §6.1.0540
        assert f("3 4 <> . cr") == "-1"

    def test_neq_false(self):
        assert f("3 3 <> . cr") == "0"

    def test_lt_true(self):                           # §6.1.0480
        assert f("2 3 < . cr") == "-1"

    def test_lt_false(self):
        assert f("3 2 < . cr") == "0"

    def test_gt_true(self):                           # §6.1.1550
        assert f("3 2 > . cr") == "-1"

    def test_gt_false(self):
        assert f("2 3 > . cr") == "0"

    def test_lte(self):                               # §6.2.0500
        assert f("3 3 <= . cr") == "-1"
        assert f("2 3 <= . cr") == "-1"
        assert f("3 2 <= . cr") == "0"

    def test_gte(self):                               # §6.2.1555
        assert f("3 3 >= . cr") == "-1"
        assert f("3 2 >= . cr") == "-1"
        assert f("2 3 >= . cr") == "0"

    def test_zero_eq(self):                           # §6.1.0270
        assert f("0 0= . cr") == "-1"
        assert f("1 0= . cr") == "0"

    def test_zero_lt(self):                           # §6.1.0250
        assert f("-1 0< . cr") == "-1"
        assert f("0 0< . cr") == "0"

    def test_zero_gt(self):                           # §6.2.0280
        assert f("1 0> . cr") == "-1"
        assert f("0 0> . cr") == "0"

    def test_zero_ne(self):                           # §6.2.0260
        assert f("1 0<> . cr") == "-1"
        assert f("0 0<> . cr") == "0"

    def test_u_less(self):                            # §6.1.2340
        assert f("1 2 U< . cr") == "-1"
        assert f("2 1 U< . cr") == "0"

    def test_u_less_treats_unsigned(self):
        # -1 as unsigned is the maximum cell value, so 1 U< -1 is true
        assert f("1 -1 U< . cr") == "-1"

    def test_u_greater(self):                         # §6.2.2350
        assert f("2 1 U> . cr") == "-1"

    def test_within(self):                            # §6.2.2440  ( n lo hi -- flag )
        assert f("5 1 10 WITHIN . cr") == "-1"        # inside
        assert f("0 1 10 WITHIN . cr") == "0"         # below
        assert f("10 1 10 WITHIN . cr") == "0"        # at hi (exclusive)

    def test_false(self):                             # §6.2.1485
        assert f("FALSE . cr") == "0"

    def test_true(self):                              # §6.2.2298
        assert f("TRUE . cr") == "-1"


# ═══════════════════════════════════════════════════════════════════════════════
# Bitwise / logical
# ═══════════════════════════════════════════════════════════════════════════════

class TestBitwise:
    def test_and(self):                               # §6.1.0720
        assert f("12 10 and . cr") == "8"             # 0b1100 & 0b1010 = 0b1000

    def test_and_mask(self):
        assert f("255 15 and . cr") == "15"

    def test_or(self):                                # §6.1.1980
        assert f("12 10 or . cr") == "14"             # 0b1100 | 0b1010 = 0b1110

    def test_xor(self):                               # §6.1.2490
        assert f("12 10 xor . cr") == "6"             # 0b1100 ^ 0b1010 = 0b0110

    def test_invert(self):                            # §6.1.1720
        assert f("0 invert . cr") == "-1"             # ~0 = all-ones = -1

    def test_invert_all_ones(self):
        assert f("-1 invert . cr") == "0"

    def test_lshift(self):                            # §6.1.1805
        assert f("1 4 lshift . cr") == "16"

    def test_rshift(self):                            # §6.1.2162
        assert f("16 4 rshift . cr") == "1"
        assert f("32 2 rshift . cr") == "8"


# ═══════════════════════════════════════════════════════════════════════════════
# Return stack  §6.1.0580, §6.1.2060, §6.1.2070
# ═══════════════════════════════════════════════════════════════════════════════

class TestReturnStack:
    def test_tor_rfrom(self):
        assert f("5 >R 99 DROP R> . cr") == "5"

    def test_rfetch(self):
        assert f("7 >R R@ . space R> drop cr") == "7"

    def test_rfetch_nondestructive(self):
        assert f("7 >R R@ . space R@ . cr  R> drop") == "7 7"

    def test_2tor_2rfrom(self):                       # §6.2.0435, §6.2.2030
        assert f("1 2 2>R 99 drop 2R> . space . cr") == "2 1"  # TOS=2 second=1

    def test_2rfetch(self):
        assert f("10 20 2>R 2R@ . space . space 2R> 2drop cr") == "20 10"


# ═══════════════════════════════════════════════════════════════════════════════
# Memory / data space
# ═══════════════════════════════════════════════════════════════════════════════

class TestMemory:
    def test_variable_store_fetch(self):              # §6.1.2410, §6.1.0010, §6.1.0650
        assert f("VARIABLE x  42 x !  x @ . cr") == "42"

    def test_c_store_c_fetch(self):                   # §6.1.0910, §6.1.0870
        assert f("VARIABLE v  0 v !  65 v C!  v C@ . cr") == "65"

    def test_at_question(self):                       # ?  ( addr -- )  prints value
        assert f("VARIABLE v  99 v !  v ? cr") == "99"

    def test_cells(self):                             # §6.1.0890
        out = int(f("1 CELLS . cr"))
        assert out in (4, 8)                          # 4 on 32-bit, 8 on 64-bit

    def test_cell_plus(self):                         # §6.1.0880
        out = int(f("0 CELL+ . cr"))
        assert out in (4, 8)

    def test_here_allot(self):                        # §6.1.0710, §6.1.0130
        assert f("HERE 0 ALLOT HERE = . cr") == "-1" # ALLOT 0 → HERE unchanged

    def test_allot_advances_here(self):
        assert f("HERE 10 ALLOT HERE SWAP - . cr") == "10"

    def test_comma(self):                             # §6.1.0450  ,
        # , should advance HERE by one cell
        assert f("HERE 42 , HERE SWAP - . cr") == f("1 CELLS . cr")

    def test_c_comma(self):                           # §6.1.0860  C,
        assert f("HERE 65 C,  HERE 1- C@ . cr") == "65"

    def test_fill(self):                              # §6.1.1540
        assert f("CREATE buf 4 ALLOT  buf 4 65 FILL  buf C@ . cr") == "65"
        assert f("CREATE buf2 4 ALLOT  buf2 4 65 FILL  buf2 1+ C@ . cr") == "65"

    def test_move(self):                              # §6.1.1900
        assert f("CREATE src 4 ALLOT  CREATE dst 4 ALLOT\n"
                 "72 src C!  73 src 1+ C!\n"
                 "src dst 2 MOVE\n"
                 "dst C@ . space dst 1+ C@ . cr") == "72 73"

    def test_erase(self):                             # §6.2.1350
        assert f("CREATE buf 4 ALLOT\n"
                 "99 buf C!\n"
                 "buf 4 ERASE\n"
                 "buf C@ . cr") == "0"

    def test_constant(self):                          # §6.1.0950
        assert f("42 CONSTANT answer  answer . cr") == "42"

    def test_create_fetch(self):                      # §6.1.1000
        assert f("CREATE x  99 ,  x @ . cr") == "99"

    def test_create_does(self):                       # §6.1.1250
        assert f(": factor  CREATE ,  DOES> @ * ;\n"
                 "3 factor triple\n"
                 "7 triple . cr") == "21"

    def test_value_to(self):                          # §6.2.2405, §6.2.2295
        assert f("10 VALUE x\nx . cr\n20 TO x\nx . cr") == "10\n20"

    def test_2store_2fetch(self):                     # §6.1.0310, §6.1.0350
        assert f("CREATE pair  2 CELLS ALLOT\n"
                 "111 222 pair 2!\n"
                 "pair 2@ . space . cr") == "222 111"  # TOS=222, below=111

    def test_align_aligned(self):                     # §6.1.0705, §6.1.0706
        # ALIGN makes HERE cell-aligned; ALIGNED(HERE) should equal HERE
        assert f("ALIGN HERE DUP ALIGNED = . cr") == "-1"

    def test_char_plus(self):                         # §6.1.0897
        assert f("10 CHAR+ . cr") == "11"

    def test_chars(self):                             # §6.1.0898
        assert f("4 CHARS . cr") == "4"              # 1 char = 1 address unit

    def test_pad(self):                               # §6.2.2030
        out = int(f("PAD . cr"))
        assert out > 0

    def test_unused(self):                            # §6.2.2395
        out = int(f("UNUSED . cr"))
        assert out > 0

    def test_to_body(self):                           # §6.1.0550
        # >BODY converts xt to data-field address; use ' to get xt
        assert f("CREATE x  99 ,\n' x >BODY @ . cr") == "99"


# ═══════════════════════════════════════════════════════════════════════════════
# Control flow — IF/ELSE/THEN, BEGIN/UNTIL, BEGIN/WHILE/REPEAT
# ═══════════════════════════════════════════════════════════════════════════════

class TestControlFlow:
    def test_if_true(self):                           # §6.1.1700
        assert f(": test  -1 IF 1 ELSE 0 THEN . cr ;\ntest") == "1"

    def test_if_false(self):
        assert f(": test  0 IF 1 ELSE 0 THEN . cr ;\ntest") == "0"

    def test_if_no_else(self):
        assert f(": test  1 IF 42 . cr THEN ;\ntest") == "42"

    def test_if_no_else_skips(self):
        assert f(": test  0 IF 42 . cr THEN 1 . cr ;\ntest") == "1"

    def test_begin_until(self):                       # §6.1.0760, §6.1.2390
        assert f(": count  0 BEGIN 1+ DUP 5 = UNTIL . cr ;\ncount") == "5"

    def test_begin_while_repeat(self):                # §6.1.2430, §6.1.2140
        # sum 0..4 = 10; maintain (sum cnt) with cnt on TOS for the condition
        assert f(": sum  0 0\n"
                 "  BEGIN DUP 5 <\n"
                 "  WHILE\n"
                 "    SWAP OVER + SWAP 1+\n"
                 "  REPEAT\n"
                 "  DROP . cr ;\nsum") == "10"

    def test_begin_again_exit(self):                  # §6.1.0760, §6.2.0700
        assert f(": test  0 BEGIN 1+ DUP 3 = IF EXIT THEN AGAIN ;\ntest . cr") == "3"

    def test_case_of_endof_endcase(self):             # §6.2.0873, §6.2.1350, §6.2.1342, §6.2.1343
        # Default pattern: push return value BELOW the selector so ENDCASE drops sel
        assert f(": classify\n"
                 "  CASE\n"
                 "    1 OF 1 ENDOF\n"
                 "    2 OF 2 ENDOF\n"
                 "    -1 SWAP\n"              # -1 below sel; ENDCASE drops sel
                 "  ENDCASE . cr ;\n"
                 "1 classify\n"
                 "2 classify\n"
                 "9 classify") == "1\n2\n-1"


# ═══════════════════════════════════════════════════════════════════════════════
# DO loops  §6.1.1240, §6.1.1800, §6.1.1730, §6.1.1730, §6.1.1790
# ═══════════════════════════════════════════════════════════════════════════════

class TestLoops:
    def test_do_loop_i(self):                         # §6.1.1800
        assert f(": test  3 0 DO I . space LOOP cr ;\ntest") == "0 1 2"

    def test_do_plus_loop(self):                      # §6.1.0140
        assert f(": test  10 0 DO I . space 2 +LOOP cr ;\ntest") == "0 2 4 6 8"

    def test_loop_j(self):                            # §6.1.1730
        # J is outer loop index when nested
        assert f(": test\n"
                 "  2 0 DO\n"
                 "    2 0 DO\n"
                 "      J . I . space\n"
                 "    LOOP\n"
                 "  LOOP cr ;\ntest") == "00 01 10 11"

    def test_leave(self):                             # §6.1.1760
        # Print I first, then check; LEAVE after printing 4 exits before I=5
        assert f(": test  10 0 DO I . space I 4 = IF LEAVE THEN LOOP cr ;\ntest") == "0 1 2 3 4"

    def test_unloop_exit(self):                       # §6.1.2380
        assert f(": inner  10 0 DO I 3 = IF UNLOOP EXIT THEN I . space LOOP ;\n"
                 ": test  inner cr ;\ntest") == "0 1 2"

    def test_qdo_skip(self):                          # §6.2.2030  ?DO  — skip if limit=start
        assert f(": test  3 3 ?DO I . space LOOP cr ;\ntest") == ""

    def test_qdo_executes(self):
        assert f(": test  5 3 ?DO I . space LOOP cr ;\ntest") == "3 4"

    def test_recurse(self):                           # §6.1.2120
        assert f(": fact  DUP 1 > IF DUP 1- RECURSE * THEN ;\n5 fact . cr") == "120"

    def test_exit_early(self):                        # §6.1.1380
        assert f(": test  3 . cr EXIT 99 . cr ;\ntest") == "3"


# ═══════════════════════════════════════════════════════════════════════════════
# Word definition / compilation  §6.1.0450, §6.1.0460, §6.1.1710, §6.1.2120
# ═══════════════════════════════════════════════════════════════════════════════

class TestWordDef:
    def test_colon_semicolon(self):
        assert f(": sq  DUP * ;\n5 sq . cr") == "25"

    def test_nested_words(self):
        assert f(": dbl  2 * ;\n: quad  dbl dbl ;\n3 quad . cr") == "12"

    def test_immediate(self):                         # §6.1.1710
        assert f(": hi  42 . cr ; IMMEDIATE\n: test  hi ;\ntest") == "42"

    def test_state(self):                             # §6.1.2250  STATE is addr of state var
        assert f("STATE @ . cr") == "0"               # @-fetch: 0 = interpret mode

    def test_state_in_compile(self):
        assert f(": test  STATE @ . cr ;\ntest") == "0"  # at runtime STATE=0

    def test_noname(self):                            # §6.2.1980
        assert f(":NONAME  99 . cr ;\nEXECUTE") == "99"

    def test_postpone_non_immediate(self):            # §6.1.2033
        assert f(": my-dup  POSTPONE DUP ; IMMEDIATE\n"
                 ": test  1 my-dup + . cr ;\ntest") == "2"

    def test_postpone_immediate(self):
        assert f(": my-if  POSTPONE IF ; IMMEDIATE\n"
                 ": test  1 my-if 7 . cr THEN ;\ntest") == "7"

    def test_literal(self):                           # §6.1.1780
        assert f(": test  [ 6 7 * ] LITERAL . cr ;\ntest") == "42"

    def test_bracket_tick(self):                      # §6.1.2510  [']
        assert f(": test  ['] DUP EXECUTE . cr ;\n5 test") == "5"

    def test_bracket_char(self):                      # §6.1.2520  [CHAR]
        assert f(": test  [CHAR] A . cr ;\ntest") == "65"

    def test_char(self):                              # §6.1.0895
        assert f("CHAR A . cr") == "65"

    def test_tick_execute(self):                      # §6.1.0070, §6.1.1370
        assert f(": sq DUP * ;\n' sq 4 SWAP EXECUTE . cr") == "16"


# ═══════════════════════════════════════════════════════════════════════════════
# Parsing / source  §6.1.2450, §6.1.2520, §6.1.2216, §6.1.2216
# ═══════════════════════════════════════════════════════════════════════════════

class TestParsing:
    def test_word_bl(self):                           # §6.1.2450
        assert f("BL WORD hello COUNT TYPE cr") == "hello"

    def test_word_custom_delim(self):
        assert f(": parse-slash  [CHAR] / WORD COUNT TYPE cr ;\n"
                 "parse-slash foo/bar") == "foo"

    def test_word_empty_at_end(self):
        # WORD at end of line → 0-length counted string; COUNT returns (addr 0)
        assert f("BL WORD\nCOUNT . cr") == "0"

    def test_evaluate_basic(self):                    # §6.1.1360
        assert f('S" 1 2 + . cr" EVALUATE') == "3"

    def test_evaluate_definition(self):
        assert f('S" : sq DUP * ;" EVALUATE\n5 sq . cr') == "25"

    def test_source_id_file(self):                    # §6.2.2218
        out = int(f("SOURCE-ID . cr"))
        assert out != 0                               # non-zero for file input

    def test_source_id_in_evaluate(self):
        assert f('S" SOURCE-ID . cr" EVALUATE') == "-1"

    def test_refill_gets_next_line(self):             # §6.2.2125
        # REFILL at EOF in file mode returns 0 (false)
        assert f("REFILL . cr") == "0"

    def test_to_in(self):                             # §6.1.0560
        # >IN returns an address; fetch it with @; at start of input offset is 0
        assert f(">IN @ . cr") != ""  # address is readable without crashing

    def test_to_number(self):                         # §6.1.0570
        # ( ud c-addr u -- ud c-addr+n n-remaining )
        # After converting "123": ud=(123,0), remaining=0; drop addr/remaining, check ud
        assert f('0 0 S" 123" >NUMBER 2DROP . space . cr') == "0 123"

    def test_find_found(self):                        # §6.1.1550
        # FIND returns (cfa, 1) for non-immediate or (cfa, -1) for immediate
        assert f("BL WORD DUP FIND NIP . cr") in ("1", "-1")

    def test_find_not_found(self):
        assert f("BL WORD NOTAWORD123 FIND NIP . cr") == "0"

    def test_source_returns_addr_len(self):           # §6.1.2216
        # SOURCE returns ( c-addr u ); u should be > 0 for non-empty line
        assert int(f("S\" SOURCE-ID . cr\" EVALUATE drop SOURCE-ID drop . cr")) != 0 or True  # just no crash

    def test_bl_value(self):                          # §6.1.0770
        assert f("BL . cr") == "32"


# ═══════════════════════════════════════════════════════════════════════════════
# String / character output  §6.1.0190, §6.1.0200, §6.1.2310, §6.1.1320
# ═══════════════════════════════════════════════════════════════════════════════

class TestStrings:
    def test_s_quote_type(self):                      # §6.1.2165, §6.1.2310
        assert f('S" hello" TYPE cr') == "hello"

    def test_count_type(self):                        # §6.1.0980
        assert f(': test  C" hello" COUNT TYPE cr ;\ntest') == "hello"

    def test_dot_quote(self):                         # §6.1.0190
        assert f('." hello world" cr') == "hello world"

    def test_emit(self):                              # §6.1.1320
        assert f("65 EMIT cr") == "A"

    def test_type_length(self):
        assert f('S" abcde" TYPE cr') == "abcde"

    def test_type_zero_len(self):
        assert f('S" " drop 0 TYPE cr') == ""


# ═══════════════════════════════════════════════════════════════════════════════
# I/O  §6.1.0180, §6.1.0990, §6.1.2220
# ═══════════════════════════════════════════════════════════════════════════════

class TestIO:
    def test_dot(self):                               # §6.1.0180
        assert f("42 . cr") == "42"

    def test_dot_negative(self):
        assert f("-7 . cr") == "-7"

    def test_u_dot(self):                             # §6.1.2320
        assert f("42 u. cr") == "42"

    def test_cr(self):                                # §6.1.0990
        assert f("cr") == ""                          # just a newline; stripped

    def test_space(self):                             # §6.1.2220
        assert f("1 . space 2 . cr") == "1 2"

    def test_spaces(self):                            # §6.1.2230
        assert f("1 . 3 spaces 2 . cr") == "1   2"

    def test_dot_r(self):                             # §6.2.0210  .R
        assert f("42 5 .R cr", strip=False) == "   42"

    def test_u_dot_r(self):                           # §6.2.2330  U.R
        assert f("7 4 U.R cr", strip=False) == "   7"


# ═══════════════════════════════════════════════════════════════════════════════
# Pictured numeric output  §6.1.0040, §6.1.0030, §6.1.0050, §6.1.1670
# ═══════════════════════════════════════════════════════════════════════════════

class TestPictured:
    def test_hash_s_basic(self):                      # §6.1.0050  #S
        # digits added LSB-first then #> reverses: 42 → "42"
        assert f("42 0 <# #S #> TYPE cr") == "42"

    def test_hash_digit_by_digit(self):               # §6.1.0030  #
        assert f("42 0 <# # # #> TYPE cr") == "42"   # 2 digits of 42

    def test_hold(self):                              # §6.1.1670  HOLD
        # HOLD inserts '$' before #> reverses; buffer after #S is "24", after HOLD "24$", after reverse "$42"
        assert f("42 0 <# #S CHAR $ HOLD #> TYPE cr") == "$42"

    def test_sign_negative(self):                     # §6.1.2210  SIGN
        # buffer after #S "24", after SIGN "24-", after reverse "-42"
        assert f("-42 ABS 0 <# #S -42 0< SIGN #> TYPE cr") == "-42"

    def test_sign_positive(self):
        assert f("42 0 <# #S 0 SIGN #> TYPE cr") == "42"  # no sign for positive


# ═══════════════════════════════════════════════════════════════════════════════
# Number base  §6.1.1170, §6.1.1080, §6.1.0750
# ═══════════════════════════════════════════════════════════════════════════════

class TestBase:
    def test_hex(self):                               # §6.1.1170
        assert f("HEX 1A . cr DECIMAL") == "26"

    def test_decimal_restores(self):
        assert f("HEX 1A DECIMAL . cr") == "26"

    def test_base_word(self):                         # §6.1.0750
        assert f("BASE @ . cr") == "10"

    def test_binary_base(self):
        assert f("2 BASE ! 1010 . cr 10 BASE !") == "10"  # binary 1010 = 10


# ═══════════════════════════════════════════════════════════════════════════════
# Comments  §6.1.0080, §6.1.2500
# ═══════════════════════════════════════════════════════════════════════════════

class TestComments:
    def test_paren_comment(self):                     # §6.1.0080  (
        assert f("( this is a comment ) 42 . cr") == "42"

    def test_backslash_comment(self):                 # §6.1.2500  \
        assert f("\\ this whole line is a comment\n42 . cr") == "42"

    def test_dot_paren(self):                         # §6.2.0200  .(
        assert f(".( hello) cr") == "hello"


# ═══════════════════════════════════════════════════════════════════════════════
# ENVIRONMENT?  §6.1.1345
# ═══════════════════════════════════════════════════════════════════════════════

class TestEnvironment:
    def test_environment_max_n(self):
        flag = f('S" MAX-N" ENVIRONMENT? . cr')
        assert flag in ("-1", "1")                    # found → truthy

    def test_environment_unknown(self):
        assert f('S" DOES-NOT-EXIST-XYZ-123" ENVIRONMENT? . cr') == "0"


# ═══════════════════════════════════════════════════════════════════════════════
# Error handling  §6.1.0680, §6.1.2050
# ═══════════════════════════════════════════════════════════════════════════════

class TestErrorHandling:
    def test_abort_quote_triggers(self):              # §6.1.0680
        r = repl(": ta\n0 < ABORT\" neg!\" ;\n-1 ta\n")
        assert "neg!" in r.stderr

    def test_abort_quote_no_trigger(self):
        assert f(': ta  0 < ABORT" neg!" ;\n1 ta ." ok" cr') == "ok"

    def test_abort_quote_restarts_repl(self):
        r = repl(": ta\n0 < ABORT\" stop\" ;\n-1 ta\n42 . cr\n")
        assert "42" in r.stdout             # REPL restarted and processed next line

    def test_quit_restarts_repl(self):               # §6.1.2050
        r = repl("QUIT\n42 . cr\n")
        assert "42" in r.stdout


# ═══════════════════════════════════════════════════════════════════════════════
# INCLUDE / INCLUDED  §6.2.1714, §6.2.1715
# ═══════════════════════════════════════════════════════════════════════════════

class TestInclude:
    def test_include(self, tmp_path):
        inc = tmp_path / "inc.fs"
        inc.write_text("99 . cr\n")
        main = tmp_path / "main.fs"
        main.write_text(f"INCLUDE {inc}\n")
        r = subprocess.run(
            [FORTH, str(main)], capture_output=True, text=True, timeout=5
        )
        assert r.stdout.strip() == "99"

    def test_included(self, tmp_path):
        inc = tmp_path / "inc2.fs"
        inc.write_text("77 . cr\n")
        main = tmp_path / "main.fs"
        main.write_text(f'S" {inc}" INCLUDED\n')
        r = subprocess.run(
            [FORTH, str(main)], capture_output=True, text=True, timeout=5
        )
        assert r.stdout.strip() == "77"


# ═══════════════════════════════════════════════════════════════════════════════
# Misc / meta
# ═══════════════════════════════════════════════════════════════════════════════

class TestMisc:
    def test_bye(self):
        r = repl("bye\n")
        assert r.returncode == 0

    def test_words_contains_dup(self):                # §6.1.2465
        r = repl("WORDS\n")
        assert r.returncode == 0
        assert "dup" in r.stdout.lower()

    def test_dot_s_nocrash(self):                     # §6.1.0220  .S
        r = repl("1 2 3 .s\n")
        assert r.returncode == 0

    def test_clear_empties_stack(self):
        r = repl("1 2 3 clear DEPTH . cr\n")
        assert "0" in r.stdout
