; Darla Sravan Kumar - 2401CS45
;
; Iterative factorial in SIMPLEX assembly.
;
; Since SIMPLEX has no MUL instruction, multiplication is done
; by using a nested addition loop (multiply A by B = add A to itself B times)
;
; Stack layout (adj -5):
;   SP+0 = result     (running factorial, starts at 1)
;   SP+1 = i          (outer loop counter, 2 .. n)
;   SP+2 = n          (input, read-only)
;   SP+3 = mul_accum  (accumulates result * i)
;   SP+4 = mul_cnt    (counts i down to 0)
;
; ---------------------------------------------------------------

        ldc     0x1000
        a2sp
        adj     -5
        ldc     n

        ldnl    0               ; A = n = 6
        stl     2               ; SP+2 = n

        ldc     1
        stl     0               ; SP+0 = 1

        ldc     2
        stl     1               ; SP+1 = 2

outerloop:                      ; while i <= n: check n - i >= 0
        ldl     2               ; A = n
        ldl     1               ; B = n, A = i
        sub                     ; A = n - i
        brlz    done            ; if n - i < 0, i > n, exit
                                ; multiply result * i by repeated addition
        ldc     0
        stl     3               ; SP+3 = mul_accum = 0
        ldl     1               ; A = i
        stl     4               ; SP+4 = mul_cnt = i

mulloop:
        ldl     4               ; A = mul_cnt
        brz     muldone
        ldl     0               ; B = mul_cnt,   A = result
        ldl     3               ; B = result,    A = mul_accum
        add                     ; A = mul_accum + result
        stl     3               ; SP+3 = new mul_accum
        ldl     4               ; A = mul_cnt
        adc     -1
        stl     4               ; mul_cnt--
        br      mulloop

muldone:                        ; result = mul_accum
        ldl     3
        stl     0               ; SP+0 = new result

        ldl     1
        adc     1
        stl     1               ; i++

        br      outerloop

done:
        ldl     0               ; A = result = n!
        ldc     answer
        stnl    0               ; mem[answer] = n!
        HALT

n:      data    6               ; compute 6!
answer: data    0               ; result : 720 = 0x000002D0
