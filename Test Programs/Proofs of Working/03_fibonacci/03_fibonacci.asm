; Darla Sravan Kumar - 2401CS45
;
; Iterative Fibonacci - stores full series fib(0)..fib(n) into array 'arr'.
;
; Stack layout (adj -5):
;   SP+0 = a        (fib(i-2), starts 0)
;   SP+1 = b        (fib(i-1), starts 1)
;   SP+2 = i        (loop counter, starts 2)
;   SP+3 = fibtemp  (fib_i for current iteration)
;   SP+4 = addrtemp (arr+i address for stnl)
;
; ---------------------------------------------------------------

        ldc     0x1000
        a2sp
        adj     -5

        ldc     0
        ldc     arr
        stnl    0               ; mem[arr+0] = 0

        ldc     1
        ldc     arr
        stnl    1               ; mem[arr+1] = 1

        ldc     0
        stl     0               ; SP+0 = a = 0
        
        ldc     1
        stl     1               ; SP+1 = b = 1

        ldc     2
        stl     2               ; SP+2 = i = 2

fibloop:                        ; while i <= n
        ldc     n
        ldnl    0               ; A = n
        ldl     2               ; B = n, A = i
        sub                     ; A = n - i
        brlz    done            ; if n - i < 0 (i > n), div

        ldl     1               ; B = old, A = b
        ldl     0               ; B = b,   A = a
        add                     ; A = fib_i = a+b
                                
        stl     3               ; SP+3 = fib_i,  A = B = b

        stl     0               ; SP+0 = b,  A = B = b

                                ; b = fib_i
        ldl     3               ; B = b, A = fib_i
        stl     1               ; SP+1 = fib_i (new b),  A = B = b

        ldc     arr             ; B = b, A = arr
        ldl     2               ; B = arr, A = i
        add                     ; A = arr + i,  B = arr
        stl     4               ; SP+4 = arr+i,  A = B = arr

        ldl     3               ; B = arr, A = fib_i   (SP+3 = fib_i)
        ldl     4               ; B = fib_i, A = arr+i (SP+4 = arr+i)
        stnl    0               ; mem[arr+i] = B = fib_i  ✓

        ldl     2               ; B = arr+i, A = i
        adc     1
        stl     2               ; SP+2 = i + 1

        br      fibloop

done:
        HALT

n:      data    10
arr:    data    0               