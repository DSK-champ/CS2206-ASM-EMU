; test_palindrome.asm
; Darla Sravan Kumar - 2401CS45
;
; Palindrome number checker in SIMPLEX assembly.
;
; Reverses the digits of a positive integer and compares with original.
; result = 1 if palindrome, result = 0 if not.
;
; Algorithm:
;   original = n
;   rev  = 0,  temp = n
;   while temp > 0:
;       digit = temp mod 10      (via repeated subtraction of 10)
;       temp  = temp / 10        (quotient from same division)
;       rev   = rev * 10 + digit
;   result = (rev == original) ? 1 : 0
;
; Stack layout (SP base = 0x1000, adj -7):
;   SP+0 = original   (n, read-only copy)
;   SP+1 = rev        (reversed number, built digit by digit)
;   SP+2 = temp       (working copy of n, consumed digit by digit)
;   SP+3 = digit      (current extracted digit = temp mod 10)
;   SP+4 = quotient   (temp / 10)
;   SP+5 = mul_accum  (accumulator for rev * 10)
;   SP+6 = mul_cnt    (loop counter, counts down from 10)
;
; To test a different number, change the value after  n: data
; ---------------------------------------------------------------

        ldc     0x1000          ; load stack base address
        a2sp                    ; SP = 0x1000
        adj     -7              ; allocate 7 stack slots

        ; --- initialise variables ---
        ldc     n               ; A = address of n
        ldnl    0               ; A = value at n
        stl     0               ; SP+0 = original = n

        ldc     0
        stl     1               ; SP+1 = rev = 0

        ldl     0
        stl     2               ; SP+2 = temp = n

; ---------------------------------------------------------------
; revloop: extract one digit per iteration until temp == 0
; ---------------------------------------------------------------
revloop:
        ldl     2               ; A = temp
        brz     compare         ; if temp == 0, reversal done

        ; --- divide temp by 10 (repeated subtraction) ---
        ldc     0
        stl     4               ; SP+4 = quotient = 0

divloop:
        ldl     2               ; A = temp
        adc     -10             ; A = temp - 10
        brlz    divdone         ; if result < 0, temp < 10 → remainder found

        ldl     2
        adc     -10
        stl     2               ; temp -= 10

        ldl     4
        adc     1
        stl     4               ; quotient++

        br      divloop

divdone:
        ldl     2
        stl     3               ; SP+3 = digit = remainder (temp mod 10)

        ldl     4
        stl     2               ; SP+2 = temp = quotient (temp / 10)

        ; --- multiply rev by 10 via repeated addition ---
        ldc     0
        stl     5               ; SP+5 = mul_accum = 0

        ldc     10
        stl     6               ; SP+6 = mul_cnt = 10

mul10loop:
        ldl     6               ; A = counter
        brz     mul10done       ; if counter == 0, done

        ldl     1               ; A = rev
        ldl     5               ; B = rev,  A = mul_accum
        add                     ; A = mul_accum + rev
        stl     5               ; mul_accum += rev

        ldl     6
        adc     -1
        stl     6               ; counter--

        br      mul10loop

mul10done:
        ; rev = (rev * 10) + digit
        ldl     3               ; A = digit
        ldl     5               ; B = digit,  A = mul_accum  (= rev * 10)
        add                     ; A = rev * 10 + digit
        stl     1               ; SP+1 = new rev

        br      revloop

; ---------------------------------------------------------------
; compare: check if rev == original
; ---------------------------------------------------------------
compare:
        ldl     0               ; A = original
        ldl     1               ; B = original,  A = rev
        sub                     ; A = rev - original
        brz     ispalindrome    ; if zero, they match

notpalindrome:
        ldc     0
        ldc     answer
        stnl    0               ; answer = 0
        HALT

ispalindrome:
        ldc     1
        ldc     answer
        stnl    0               ; answer = 1
        HALT

; ---------------------------------------------------------------
; Data section
; ---------------------------------------------------------------
n:      data    121           ; ← change this value to test different numbers
answer: data    0               ; result written here: 1 = palindrome, 0 = not