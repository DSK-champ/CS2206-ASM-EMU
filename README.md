# CS2206 — Assembler & Emulator

**Author     :** Darla Sravan Kumar  
**Roll No :** 2401CS45  

---

## Overview
A complete implementation of a **two-pass assembler (`asm.cpp`)** and a **machine emulator (`emu.cpp`)** for the SIMPLEX architecture.

- Converts `.asm` → `.obj`
- Executes machine code with tracing, memory dump, and register tracking
- Fully handles labels, pseudo-instructions, and error detection

---

## Build & Run

```bash
g++ asm.cpp -o asm
g++ emu.cpp -o emu

./asm file.asm
./emu file.obj -trace -bfrafr -memdump
```

 Outputs:
- `.obj` → binary
- `.lst` → listing
- `.log` → errors (if any)
- `.trace`, `.bfrafr`, `.memdump` → emulator outputs

---

## Features

### Assembler
- Two-pass design (label resolution + encoding)
- Supports decimal, hex, octal
- Handles `SET` and `data`
- Full error reporting (no early exit)

### Emulator
- Executes all SIMPLEX instructions
- Stack + memory simulation
- Runtime error detection (bounds, opcode, etc.)
- Debug flags:
  - `-trace`
  - `-bfrafr`
  - `-memdump`

---

## Test Programs

| File | Description |
|------|------------|
| `01_bubble.asm` | Bubble sort |
| `02_factorial.asm` | Factorial (no MUL) |
| `03_fibonacci.asm` | Fibonacci |
| `04_palindrome.asm` | Palindrome check |
| `05_errors.asm` | All error cases |
| `06_SET.asm` | SET demo |

---

## Limitations
- 24-bit data constraint
- No infinite loop detection
- Logical shift only (`shr`)
- Stack pointer fixed at start
  
---
## 🖥️ Logisim Implementation (SIMPLEX CPU)

The SIMPLEX architecture was also fully implemented in **Logisim**, replicating the datapath and control flow of the assembler + emulator design.

### 🔧 Components Implemented
- Program Counter (PC) with increment and branch logic  
- Stack Pointer (SP)  
- Register file (A, B)  
- ALU with arithmetic and logical operations  
- Instruction memory and data memory  
- Multiplexers controlling data flow  
- Sign-extension unit (24 → 32 bits)

---

### 🔀 MUX Control Design

#### MUX A
| Sel | Input |
|-----|------|
| 00 | ALU output |
| 01 | B |
| 10 | Data |
| 11 | Immediate |

#### MUX B
| Sel | Input |
|-----|------|
| 00 | A |
| 01 | ALU output |
| 10 | Data |
| 11 | Immediate |

#### MUX PC
| Sel | Input |
|-----|------|
| 00 | PC + 1 |
| 01 | ALU output (PC + offset) |
| 10 | A |
| 11 | PC |

---

### ⚙️ Key Highlights
- Accurate datapath simulation of instruction execution  
- Correct branching using ALU-computed offsets  
- Supports immediate, register, and memory-based operations  
- Instruction flow visible through wiring and signal propagation  

---

### 📈 Outcome
This Logisim model validates the correctness of the assembler and emulator by providing a **hardware-level visualization** of execution.


---

## Demo
https://youtu.be/VXjq0EIDQ50

---

## 🧾 Declaration
All code was written independently without copying or collaboration.
