; ============================================================
;                   ERROR DEMONSTRATION FILE
;  This file intentionally triggers every possible error code
;       Format: code   ; error code -N — what went wrong
; ============================================================


; ---- error -1 : Label name clashes with a real instruction name ----
; The label "add" is also an instruction mnemonic — not allowed!
add: ldc 5                      ; error -1 — "add" is a reserved instruction name, cant use it as a label


; ---- error -2 : Duplicate label ----
; Two different lines both claim the label "myLabel" — the assembler gets confused!
myLabel: ldc 10                 ; this first one is fine
myLabel: ldc 20                 ; error -2 — "myLabel" already exists up above, duplicate!


; ---- error -3 : Invalid label (non-alpha numeric characters or starts with digit) ----
; Labels must start with a letter and contain only letters/digits
1badLabel: ldc 5                ; error -3 — labels cannot start with a digit, that is illegal
my-Label: ldc 5                 ; error -3 — hyphen "-" is not alphanumeric, invalid label character


; ---- error -4 : Empty label (label with colon but nothing after it) ----
; A label on its own line with nothing after it — just floating there!
lonelyLabel:                    ; error -4 (WARNING) — label exists but has no instruction or data after it


; ---- error -5 : Invalid mnemonic (instruction name we don't recognise) ----
; Typed an instruction name that doesn't exist in our instruction table
foo 5                           ; error -5 — "foo" is not a real instruction, typo?
subz                            ; error -5 — "subz" doesn't exist either, not a valid mnemonic
mov A B                         ; error -5 — "mov" is not in this ISA at all


; ---- error -6 : Unexpected operand (gave extra stuff to an instruction that wants nothing) ----
; Instructions like "add", "sub", "HALT" take NO operands — extra stuff is wrong
add 5                           ; error -6 — "add" takes no operand, it works on registers A and B directly
sub 10                          ; error -6 — "sub" also takes no operand, remove the 5
HALT 99                         ; error -6 — HALT just stops, it doesn't need a number!
return 3                        ; error -6 — "return" takes no operand either
a2sp 1                          ; error -6 — "a2sp" takes no operand
sp2a 7                          ; error -6 — "sp2a" takes no operand


; ---- error -6 also : Too many operands on an instruction that wants exactly one ----
ldc 5 10                        ; error -6 — "ldc" wants ONE number, not two numbers
adc 3 7                         ; error -6 — "adc" also only takes one operand
br myLabel extraStuff           ; error -6 — "br" only takes one label/offset, not two things


; ---- error -8 : Label not found / invalid operand ----
; Used a label that was never defined anywhere in the file
ldc ghostLabel                  ; error -8 — "ghostLabel" was never declared anywhere
br neverDefined                 ; error -8 — trying to branch to a label that doesnt exist
ldl @@@                         ; error -8 — "@@@" is neither a number nor a known label, totally invalid


; ---- error -9 : Missing operand (forgot to write the number/label) ----
; Instructions that NEED an operand but you forgot to give them one!
ldc                             ; error -9 — "ldc" needs a number to load! like "ldc 42"
adc                             ; error -9 — "adc" also needs a number
ldl                             ; error -9 — "ldl" needs an offset number
stl                             ; error -9 — "stl" needs an offset number
ldnl                            ; error -9 — "ldnl" needs an offset
stnl                            ; error -9 — "stnl" needs an offset
adj                             ; error -9 — "adj" needs a number to adjust the stack by
br                              ; error -9 — "br" needs to know WHERE to branch to!
brz                             ; error -9 — "brz" needs a branch target label or offset
brlz                            ; error -9 — "brlz" needs a branch target too


; ---- error -10 : SET used without a label ----
; SET is a pseudo-instruction that sets a label's value — but without a label, who gets set?
SET 42                          ; error -10 — no label in front of SET, pointless! needs "someName: SET 42"


; ---- error -11 (WARNING) : Unlabeled "data" allocation ----
; "data" puts raw bytes in memory, but without a label you can never find it again!
data 0xDEAD                     ; error -11 (WARNING) — data with no label is unreachable, its lost in memory


; ---- error -12 : Positive overflow (operand too big for 24-bit field) ----
; The operand field is only 24 bits wide — max unsigned value is 16777215 (0xFFFFFF)
ldc 16777216                    ; error -12 — 16777216 is 0x1000000, one too many, doesn't fit in 24 bits!
ldc 99999999                    ; error -12 — way too big, massively overflows 24 bits
ldc 0x1000000                   ; error -12 — hex version of 16777216, same overflow


; ---- error -13 : Negative overflow (operand too negative for 24-bit signed field) ----
; The minimum signed 24-bit value is -8388608 (-2^23) — going below that is an overflow
ldc -8388609                    ; error -13 — one below the minimum, just barely too negative
ldc -9999999                    ; error -13 — way too negative, underflows the 24-bit signed range
adj -8388609                    ; error -13 — negative overflow applies to adj too


; ============================================================
;    VALID lines for contrast — these should assemble fine
; ============================================================

goodLabel:                      ; WARNING -4 only — but still assembles (empty label)
           ldc 0                ; OK — instruction with valid operand
           adc 1                ; OK — instruction with valid operand
           add                  ; OK — no operand needed for add
           HALT                 ; OK — clean program end
anotherLabel: SET 0xFF          ; OK — label: SET value, totally fine pseudo-instruction
           data 0xCAFE          ; WARNING -11 only — but still assembles (unlabeled data)
namedData: data 0x123456        ; OK — labeled data, no warning