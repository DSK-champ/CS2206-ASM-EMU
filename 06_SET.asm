; ============================================================
; SET PSEUDO-INSTRUCTION DEMO
; ============================================================
; SET lets you give a name to a constant value.
; Instead of the label pointing to a memory address,
; it holds a fixed number you choose.
; Think of it like: #define MAX 100  in C
; ============================================================

; --- Define some named constants using SET ---
MAX:    SET 100         ; MAX  = 100  (decimal)
LIMIT:  SET 0x0A        ; LIMIT = 10  (hex — same as writing 10)
NEG:    SET -5          ; NEG  = -5   (negative values work too!)
MASK:   SET 0xFF        ; MASK = 255  (useful bitmask constant)

; --- Now use those constants in real instructions ---

        ldc MAX         ; load 100 into register A  (same as: ldc 100)
        adc LIMIT       ; add 10 to A  → A is now 110  (same as: adc 10)
        adc NEG         ; add -5 to A  → A is now 105  (same as: adc -5)

; --- Practical example: loop N times using a SET constant ---
COUNT:  SET 7           ; we want to loop 7 times

        ldc COUNT       ; A = 7  (our loop counter)
loop:   adc -1          ; A = A - 1
        brlz done       ; if A < 0, we're done
        br   loop       ; else go back to loop

done:   HALT