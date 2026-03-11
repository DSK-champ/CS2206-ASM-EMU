; ============================================================
; bubblesort.asm  -  Bubble Sort in SIMPLEX Assembly Language
; ============================================================
;
; Sorts the array 'arr' (n elements) into ascending order.
;
; SIMPLEX register summary:
;   A   = accumulator (top of 2-reg internal stack)
;   B   = second register (B is implicitly updated by ldc/ldl etc.)
;   SP  = stack pointer
;   PC  = program counter
;
; Stack frame for bsort (3 locals, adj -3):
;   SP+0  = saved return address
;   SP+1  = i  (outer-loop counter, n-1 down to 1)
;   SP+2  = j  (inner-loop counter, 0 up to i-1)
; ============================================================

; -- Entry point -----------------------------------------------
        ldc     0x1000          ; A := 4096 -- chosen stack base address
        a2sp                    ; SP := A; A := B  -- initialise stack pointer

        ldc     n               ; A := address of variable n
        ldnl    0               ; A := mem[n]  -- load the count value (8)

        call    bsort           ; call bsort; 'call' does:
                                ;   B := A  (old A = count n)
                                ;   A := PC (return address saved in A)
                                ;   PC := PC + offset  (jump to bsort)
        HALT                    ; stop emulator when sort returns

; -- bsort: Bubble Sort subroutine ----------------------------
; On entry (as set by 'call' instruction):
;   A = return address
;   B = n  (the count that was in A before the call)
bsort:
        adj     -3              ; allocate 3 local slots on the stack
        stl     0               ; mem[SP+0] := A  -- save return address
                                ;   stl shifts B->A, so now A = old B = n
        ; Initialise i = n - 1  (total outer passes needed)
        adc     -1              ; A := n - 1
        stl     1               ; mem[SP+1] := i = n-1

; -- Outer loop: while i > 0 ----------------------------------
outer:
        ldl     1               ; A := i
        brz     done            ; if i == 0, array is sorted -- exit

        ; Reset inner counter j = 0 for each outer pass
        ldc     0               ; A := 0
        stl     2               ; mem[SP+2] := j = 0

; -- Inner loop: while j < i ----------------------------------
inner:
        ; Compute i - j; if <= 0 the inner pass is complete
        ldl     1               ; A := i
        ldl     2               ; A := j,  B := i
        sub                     ; A := B - A = i - j  (sub does A := B - A)
        brz     nextOuter      ; i - j == 0  (j == i)  -- end inner loop
        brlz    nextOuter      ; i - j <  0  (safety)  -- end inner loop

        ; -- Load arr[j] into temporary stack slot ------------
        ldc     arr             ; A := base address of array
        ldl     2               ; A := j,  B := arrBase
        add                     ; A := arrBase + j  (address of arr[j])
        ldnl    0               ; A := mem[arr+j]  =  arr[j]

        adj     -1              ; push 1 extra temp slot  (becomes new SP+0)
                                ;   existing locals shift: now SP+1,SP+2,SP+3
        stl     0               ; mem[SP+0] := arr[j]  -- save for compare/swap

        ; -- Load arr[j+1] ------------------------------------
        ldc     arr             ; A := arrBase
        ldl     2               ; A := j  (now at SP+2 due to extra slot)
        adc     1               ; A := j + 1
        add                     ; A := arrBase + j + 1
        ldnl    0               ; A := arr[j+1]

        ; -- Compare: arr[j+1] - arr[j] -----------------------
        ; sub computes A := B - A
        ;   after ldnl: A = arr[j+1], B = arrBase+j+1 (trash)
        ;   we reload arr[j] from temp to set up correctly
        ldl     0               ; A := arr[j],  B := arr[j+1]
        sub                     ; A := arr[j+1] - arr[j]
                                ;   < 0  means arr[j+1] < arr[j]  -> swap
                                ;   >= 0 means already in order   -> no swap
        brlz    doSwap         ; branch if out of order (need to swap)
        br      noSwap         ; otherwise skip swap

; -- Swap arr[j] and arr[j+1] ---------------------------------
doSwap:
        ; Step 1: write old arr[j+1] into position arr[j]
        ldc     arr             ; A := arrBase
        ldl     2               ; A := j  (SP+2)
        adc     1               ; A := j + 1
        add                     ; A := address of arr[j+1]
        ldnl    0               ; A := arr[j+1] value
                                ;   B is now stale from ldc

        ldc     arr             ; A := arrBase,      B := arr[j+1]
        ldl     2               ; A := j,             B := arr[j+1]
        add                     ; A := arrBase + j,  B := arr[j+1]
        stnl    0               ; mem[arr+j] := B  =  old arr[j+1]  (DONE step 1)

        ; Step 2: write old arr[j] (saved at SP+0) into arr[j+1]
        ldl     0               ; A := saved arr[j],  B = stale
        ldc     arr             ; A := arrBase,      B := old arr[j]
        ldl     2               ; A := j,             B := old arr[j]
        adc     1               ; A := j + 1,         B := old arr[j]
        add                     ; A := arrBase+j+1,  B := old arr[j]
        stnl    0               ; mem[arr+j+1] := B  =  old arr[j]  (DONE step 2)

noSwap:
        adj     1               ; pop temp slot; SP restored to bsort frame

        ; j := j + 1
        ldl     2               ; A := j
        adc     1               ; A := j + 1
        stl     2               ; mem[SP+2] := j + 1

        br      inner           ; repeat inner loop

; -- Decrement i and repeat outer loop ------------------------
nextOuter:
        ldl     1               ; A := i
        adc     -1              ; A := i - 1
        stl     1               ; mem[SP+1] := i - 1

        br      outer           ; repeat outer loop

; -- Return ---------------------------------------------------
done:
        ldl     0               ; A := saved return address
        adj     3               ; free the 3 local slots
        return                  ; PC := A  -- return to caller

; -- Data section ---------------------------------------------
n:      data    8               ; length of the array to sort

; Unsorted input array (8 integers, sorted in-place)
arr:    data    42              ; arr[0]  initial value 42
        data    7               ; arr[1]  initial value  7
        data    19              ; arr[2]  initial value 19
        data    3               ; arr[3]  initial value  3
        data    55              ; arr[4]  initial value 55
        data    1               ; arr[5]  initial value  1
        data    28              ; arr[6]  initial value 28
        data    14              ; arr[7]  initial value 14

result: data    0               ; placeholder (unused)