# W13

W13 is an imaginary, minimal 8-bit microarchitecture and ISA (instruction set architecture) with only **eight instructions, one addressing mode, and 13 bits of addressable memory**.

See full details regarding the hypothetical [microarchitecture](/docs/microarchitecture.md). 

See also [W13 assembler](https://github.com/piotrmski/w13asm).

## Instruction set architecture

The word size is 8 bits. 2<sup>13</sup> memory addresses are addressable. Byte order is little-endian.

Two registers exist in this architecture, both are initialized to 0:

- program counter "PC" containing the address of the currently executed instruction (13 bits),
- general purpose register "A" (8 bits).

The instruction set includes two data transfer instructions (`LD` and `ST`), one arithmetic instruction (`ADD`), two bitwise logic instructions (`AND` and `NOT`) and three control flow instructions (`JMP`, `JMN` and `JMZ`).

All instructions are two words wide and follow the same format:

- the 3 most significant bits are the opcode,
- the other 13 bits are the instruction argument, which is an absolute memory address.

| Instruction | Opcode | Description | Operation | Clock cycles |
| ----------- | ------ | ----------- | ----------| ------------ |
| `LD X`      | `000`  | Load from memory | - Loads \[X] into A<br/>- Increments PC by 2 | 4 |
| `NOT X`     | `001`  | Bitwise NOT | - Loads bitwise complement of \[X] into A<br/>- Increments PC by 2 | 4 |
| `ADD X`     | `010`  | Arithmetic ADD | - Performs arithmetic addition between \[X] and \[A]<br/>- Stores the result in A<br/>- Increments PC by 2 | 4 |
| `AND X`     | `011`  | Bitwise AND | - Performs bitwise operation "AND" between \[X] and \[A]<br/>- Stores the result in A<br/>- Increments PC by 2 | 4 |
| `ST X`      | `100`  | Store to memory | - Stores \[A] in memory at the address X<br/>- Increments PC by 2 | 4 |
| `JMP X`     | `101`  | Unconditional jump | - Loads X into PC | 3 |
| `JMN X`     | `110`  | Jump if negative | If the most significant bit of \[A] is set:<br/>&nbsp;&nbsp;&nbsp;&nbsp;- Loads X into PC<br/>Otherwise:<br/>&nbsp;&nbsp;&nbsp;&nbsp;- Increments PC by 2 | 3 |
| `JMZ X`     | `111`  | Jump if zero | If all bits of \[A] are unset:<br/>&nbsp;&nbsp;&nbsp;&nbsp;- Loads X into PC<br/>Otherwise:<br/>&nbsp;&nbsp;&nbsp;&nbsp;- Increments PC by 2 | 3 |

\[X] refers to the value at memory address X; \[A] refers to the value in the register A.

## Memory map

Program memory is at 0x0000-0x1FFA.

Two additional memory-mapped registers exist to facilitate terminal I/O and monotonic clock functionalities.

- Terminal I/O register at 0x1FFF:
    - Putting a character in the standard input updates the register to the character value,
    - Loading from 0x1FFF yields the register value and clears it,
    - Storing to 0x1FFF pushes a character to the standard output buffer.
- Monotonic clock value register at 0x1FFB-0x1FFE:
    - Loading from 0x1FFB updates the register to the current number of milliseconds since the simulator has started and yields the least significant byte of the register value,
    - Loading from 0x1FFC-0x1FFE yields more significant bytes of the register value,
    - Storing to 0x1FFB-0x1FFE does nothing.

# W13 simulator

This repository contains a reference W13 simulator that features terminal input/output and a monotonic clock.

## Usage

Run `w13asm path/to/program.bin` to run the simulator until ^C is pressed, or until a JMP instruction to the current address (unconditional infinite loop) is detected.

Options: 

- `-c` or `--clock` followed by a number between 1 and 1000 - maximum clock frequency in kHz. Default is 1.
- `-d` or `--debug` - launches the simulator in paused state and enables the debugger.
- `-s` or `--symbols` followed by a path to a CSV file - supplies the debugger with names and contents of memory addresses.

The symbols file is optionally produced by [the assembler](https://github.com/piotrmski/w13asm). It has the following columns:

- the memory address,
- data type (one of following: "char", "int", or "instruction"),
- label name (unique; 0-31 characters: digits, upper- or lowercase letters, and underscores; the first character can't be a digit).

Main features of the debugger:

- listing the contents of program memory,
- listing the values of registers,
- disassembling instructions,
- stepping through instructions,
- setting breakpoints,
- updating program memory and registers in runtime.

## Building

A C compiler supporting the C23 standard, aliased as `CC` (such as `GCC` or `Clang`) and `make` in a POSIX-compliant environment (such as Linux or MacOS) are required to build this simulator from source.

Run `make` to build the simulator. The `w13sim` executable will be produced in the `dist` directory.

# License

Copyright (C) 2025 Piotr Marczyński. This program is licensed under GNU GPL v3. See the file COPYING.