; test1.asm
label:	; an unused label
	ldc 0
	ldc -5
	ldc +5
	ldc 0xFFFFFF
	ldc 16777216
	ldc 0xFFFFFFFFFF
loop: br loop ; an infinite loop
br next	;offset should be zero
next:
    	ldc loop ; load code address
	ldc var1 ; forward ref
var1: data 0 ; a variable