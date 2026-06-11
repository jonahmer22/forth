5 10 +
. cr cr

: square dup * ;

create arr 10 cells allot

: elem  cells arr + ;

: fill-arr
  10 0 do
    i 10 *  i elem !
  loop ;

: print-arr
  10 0 do
    i elem @ . cr
  loop ;

fill-arr
print-arr

cr

arr 9 cells + @ square . cr
