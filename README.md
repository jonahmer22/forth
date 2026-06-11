# forth

![Version](https://img.shields.io/badge/version-1.0.0-blue)
![Standard](https://img.shields.io/badge/standard-Forth%202012%20Core-brightgreen)
![Tests](https://img.shields.io/badge/tests-186%20passing-brightgreen)
![Language](https://img.shields.io/badge/language-C-lightgrey)
![License](https://img.shields.io/badge/license-MIT-blue)

A lightweight, Forth 2012 Core-compliant Forth interpreter written in C. Designed to be as simple and readable as possible while fully implementing the standard.

---

## Features

- **Forth 2012 Core compliant** — implements the full Core word set including all required control flow, string handling, and numeric output words
- **Bytecode VM** — words compile to an internal action list; execution is fast and stack-safe
- **Number literal prefixes** — `#` (decimal), `$` (hex), `%` (binary), `'c'` (character)
- **Full pictured numeric output** — `<#`, `#`, `#S`, `#>`, `HOLD`, `SIGN`
- **Exception handling** — `ABORT`, `ABORT"`, `QUIT` with `longjmp`-based REPL recovery
- **File I/O** — `INCLUDE`, `EVALUATE`, `SOURCE-ID`
- **Defining words** — `CREATE`, `DOES>`, `:NONAME`, `VARIABLE`, `CONSTANT`, `VALUE`
- **String words** — `S"`, `."`, `C"`, `.(`, `S\"` (with escape sequences)
- **Loop constructs** — `DO`, `?DO`, `LOOP`, `+LOOP`, `LEAVE`, `UNLOOP`, `I`, `J`
- **Case statement** — `CASE`, `OF`, `ENDOF`, `ENDCASE`
- **Arena allocator** — all dictionary and compilation memory managed in a single arena; zero fragmentation
- **ANSI color output** — error messages and REPL prompt use terminal colors

---

## Building

Requires `gcc` and `make`.

```sh
make
```

The binary is output as `./forth`.

### Install system-wide

```sh
./install.sh
```

Copies the binary to `/usr/local/bin/forth`. To uninstall:

```sh
./uninstall.sh
```

---

## Usage

### Interactive REPL

```sh
./forth
```

```
FORTHISH
A simplistic FORTH implementation made to be as dead simple as I can think of making it.

> 2 3 + . cr
5
 ok
> : square  dup * ;
 ok
> 7 square . cr
49
 ok
```

### Run a file

```sh
./forth myprogram.fs
```

---

## Quick Language Reference

### Arithmetic

| Word | Stack effect | Description |
|------|-------------|-------------|
| `+` | `( n1 n2 -- n3 )` | Add |
| `-` | `( n1 n2 -- n3 )` | Subtract |
| `*` | `( n1 n2 -- n3 )` | Multiply |
| `/` | `( n1 n2 -- n3 )` | Divide |
| `MOD` | `( n1 n2 -- n3 )` | Remainder |
| `/MOD` | `( n1 n2 -- rem quot )` | Remainder and quotient |

### Stack manipulation

| Word | Stack effect |
|------|-------------|
| `DUP` | `( n -- n n )` |
| `DROP` | `( n -- )` |
| `SWAP` | `( n1 n2 -- n2 n1 )` |
| `OVER` | `( n1 n2 -- n1 n2 n1 )` |
| `ROT` | `( n1 n2 n3 -- n2 n3 n1 )` |
| `2DUP` | `( n1 n2 -- n1 n2 n1 n2 )` |

### Defining words

```forth
: square  dup * ;
: greet  ." Hello, world!" cr ;

variable x
42 x !
x @ .          \ prints 42

: make-adder  create , does> @ + ;
5 make-adder add5
3 add5 .       \ prints 8
```

### Control flow

```forth
\ IF / ELSE / THEN
x @ 0 > if ." positive" else ." non-positive" then cr

\ DO / LOOP
5 0 do i . loop cr      \ prints 0 1 2 3 4

\ ?DO (skips if limit = start)
n 0 ?do i . loop

\ BEGIN / UNTIL
0 begin 1+ dup . dup 5 = until drop cr

\ CASE
x @ case
  1 of ." one"   endof
  2 of ." two"   endof
  ." other"
endcase
```

### Number bases

```forth
hex         \ switch to base 16
decimal     \ switch to base 10

$FF .       \ hex literal  -> 255
#255 .      \ decimal literal -> 255
%11111111 . \ binary literal -> 255
'A' .       \ character literal -> 65
```

---

## Architecture

The interpreter is a **Switch-threaded interpreter**. Compilation (`:`/`:NONAME`) translates Forth source into an `Action` array. Each `Action` is a tagged union holding one of:

| Action type | Description |
|-------------|-------------|
| `ACT_NUM` | Push a literal cell value |
| `ACT_PRINT` | Print a compile-time string |
| `ACT_WORD` | Call a word's runtime behavior |
| `ACT_COMPILE_WORD` | Compile a word into the current definition (used by `POSTPONE`) |
| `ACT_BRANCH` / `ACT_BRANCH0` | Unconditional / conditional jump |
| `ACT_DO` / `ACT_LOOP` / `ACT_PLUS_LOOP` | Loop primitives |
| `ACT_LEAVE` | Exit loop early, discard loop frame |
| `ACT_I` / `ACT_J` | Copy loop indices |
| `ACT_EXIT` | Early return from word |
| `ACT_DOES` | Patch a `CREATE`d word with a `DOES>` body |
| `ACT_ABORT_QUOTE` | Runtime `ABORT"` check |
| `ACT_EOF` | End of word body |

The interpreter walks the action array in `execBody()`. Immediate words execute at compile time by calling their C function directly; all other words append an `ACT_WORD` action.

---

## Testing

Tests require Python 3 and pytest.

```sh
python3 -m venv env
source env/bin/activate
pip install pytest
pytest tests/test_builtins.py -v
```

186 tests covering arithmetic, stack ops, control flow, string handling, number bases, defining words, exceptions, and Forth 2012 compliance.

---

## Forth 2012 Core Compliance

This implementation targets the [Forth 2012 Standard](https://forth-standard.org/standard/core) Core word set. All required Core words are implemented. Known intentional deviations:

- `POSTPONE` for immediate words compiles runtime semantics (sufficient for all tested cases)
- `SOURCE-ID` returns the raw `FILE*` cast to cell for file input (non-zero, as required)

---

## License

[GPLv3](LICENSE)
