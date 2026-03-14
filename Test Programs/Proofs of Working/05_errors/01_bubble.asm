; Bubble Sort in SIMPLEX assembly
; Sorts arr[] of n integers ascending, in-place.
;
; SIMPLEX stnl semantics:  mem[A + offset] = B   (A = address, B = value)
; SIMPLEX ldl  semantics:  B = A;  A = mem[SP+n]
;
; Stack frame inside bsort (adj -3):
;   SP+0 = return address
;   SP+1 = i   (outer counter, n-1 down to 1)
;   SP+2 = j   (inner counter, 0 up to i-1)
;
; During inner-loop body an extra slot is pushed (adj -1):
;   SP+0 = temp  (= arr[j], saved for the swap)
;   SP+1 = return address
;   SP+2 = i
;   SP+3 = j

        ldc     0x1000          ; init stack pointer
        a2sp
        ldc     n               ; address of n
        ldnl    0               ; A = n
        call    bsort
        HALT

bsort:
        adj     -3
        stl     0               ; save return address  (A still = n from call)
        adc     -1              ; A = n-1
        stl     1               ; i = n-1

outer:
        ldl     1               ; A = i
        brz     done            ; i == 0 → finished
        ldc     0
        stl     2               ; j = 0

inner:
        ldl     1               ; A = i
        ldl     2               ; A = j, B = i  → sub gives i-j
        sub                     ; A = i - j
        brz     nextOuter       ; j == i → advance outer
        brlz    nextOuter       ; j > i  → advance outer (guard)

        ; ---- load arr[j], push as temp ----
        ldc     arr
        ldl     2               ; A = j
        add                     ; A = arr+j
        ldnl    0               ; A = arr[j]
        adj     -1              ; push temp slot
        stl     0               ; temp = arr[j]
        ; frame now: SP+0=temp, SP+1=retaddr, SP+2=i, SP+3=j

        ; ---- load arr[j+1] ----
        ldc     arr
        ldl     3               ; A = j  (j is at SP+3 now)
        adc     1               ; A = j+1
        add                     ; A = arr+j+1
        ldnl    0               ; A = arr[j+1]

        ; ---- compare: arr[j+1] - arr[j] ----
        ldl     0               ; A = temp=arr[j],  B = arr[j+1]
        sub                     ; A = arr[j+1] - arr[j]  (= B - A)
        brlz    doSwap
        br      noSwap

doSwap:
        ; --- write arr[j+1] into arr[j] ---
        ; Need: A = arr+j (address),  B = arr[j+1] (value)
        ; Technique: compute address, store in a 2nd temp, load value, then ldl temp2
        ;            ldl temp2 does: B=old_A(=value), A=temp2(=address) → perfect for stnl
        ;
        adj     -1              ; push addr-temp slot  (SP+0)
        ; frame: SP+0=addr_temp, SP+1=arr[j](temp), SP+2=retaddr, SP+3=i, SP+4=j

        ldc     arr
        ldl     4               ; A = j
        add                     ; A = arr+j  (destination address for first stnl)
        stl     0               ; addr_temp = arr+j

        ldc     arr             ; A = arr_base
        ldl     4               ; A = j
        adc     1               ; A = j+1
        add                     ; A = arr+j+1
        ldnl    0               ; A = arr[j+1]  (value to write)
        ldl     0               ; B = arr[j+1], A = arr+j  ← B=value, A=address ✓
        stnl    0               ; mem[arr+j] = arr[j+1]  ✓

        ; --- write old arr[j] (= SP+1 after adj) into arr[j+1] ---
        ldc     arr
        ldl     4               ; A = j
        adc     1               ; A = j+1
        add                     ; A = arr+j+1  (destination address)
        stl     0               ; addr_temp = arr+j+1

        ldl     1               ; A = arr[j] (old, from temp slot at SP+1),  B = arr[j+1]
        ldl     0               ; B = arr[j],  A = arr+j+1  ← B=value, A=address ✓
        stnl    0               ; mem[arr+j+1] = arr[j]  ✓

        adj     1               ; pop addr_temp slot

noSwap:
        adj     1               ; pop arr[j] temp slot
        ldl     2               ; A = j
        adc     1
        stl     2               ; j++
        br      inner

nextOuter:
        ldl     1               ; A = i
        adc     -1
        stl     1               ; i--
        br      outer

done:
        ldl     0               ; restore return address into A
        adj     3
        return

n:      data    8
arr:    data    42
        data    7
        data    19
        data    3
        data    55
        data    1
        data    28
        data    14
