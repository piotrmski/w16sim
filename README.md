# W16

W16 is an imaginary, minimal 8-bit microarchitecture and ISA (instruction set architecture) with only **eight instructions, one addressing mode, and 13 bits of addressable memory**.

See full details regarding the hypothetical [microarchitecture](/docs/microarchitecture.md). 

See also [W16 assembler](https://github.com/piotrmski/w16asm).

## Instruction set architecture

The word size is 8 bits. 2<sup>13</sup> memory addresses are addressable. Byte order is little-endian.

Two registers exist in this architecture, both are initialized to 0:

- program counter "PC" containing the address of the currently executed instruction (13 bits),
- general purpose register "A" (8 bits).

The instruction set includes two data transfer instructions (`LD` and `ST`), one arithmetic instruction (`ADD`), two bitwise logic instructions (`AND` and `NOT`) and three control flow instructions (`JMP`, `JMN` and `JMZ`).

All instructions are two words wide and follow the same format:

- the 3 most significant bits are the opcode,
- the other 13 bits are the instruction argument, which is an absolute memory address.

| Instruction | Opcode | Description | Operation |
| ----------- | ------ | ----------- | ----------|
| `LD X`      | `000`  | Load from memory | - Loads \[X] into A<br/>- Increments PC by 2 |
| `NOT X`     | `001`  | Bitwise NOT | - Loads bitwise complement of \[X] into A<br/>- Increments PC by 2 |
| `ADD X`     | `010`  | Arithmetic ADD | - Performs arithmetic addition between \[X] and \[A]<br/>- Stores the result in A<br/>- Increments PC by 2 |
| `AND X`     | `011`  | Bitwise AND | - Performs bitwise operation "AND" between \[X] and \[A]<br/>- Stores the result in A<br/>- Increments PC by 2 |
| `ST X`      | `100`  | Store to memory | - Stores \[A] in memory at the address X<br/>- Increments PC by 2 |
| `JMP X`     | `101`  | Unconditional jump | - Loads X into PC |
| `JMN X`     | `110`  | Jump if negative | If the most significant bit of \[A] is set:<br/>&nbsp;&nbsp;&nbsp;&nbsp;- Loads X into PC<br/>Otherwise:<br/>&nbsp;&nbsp;&nbsp;&nbsp;- Increments PC by 2 |
| `JMZ X`     | `111`  | Jump if zero | If all bits of \[A] are unset:<br/>&nbsp;&nbsp;&nbsp;&nbsp;- Loads X into PC<br/>Otherwise:<br/>&nbsp;&nbsp;&nbsp;&nbsp;- Increments PC by 2 |

\[X] refers to the value at memory address X; \[A] refers to the value in the register A.

## Memory map

Terminal I/O and monotonic clock functionalities are facilitated via the following memory map:

- 0x0000-0x1FFA - program memory,
- 0x1FFB-0x1FFE - 32-bit relative time - reading from these addresses yields a number of milliseconds since the simulator has started until the last (or current) time 0x1FFB was (or is) read,
- 0x1FFF - terminal I/O:
    - Loading from 0x1FFF removes a character from the standard input buffer and yields it, or 0 if the buffer is empty,
    - Storing to 0x1FFF pushes a character to the standard output buffer.

# W16 simulator

This repository contains a simulator that features terminal input/output and a monotonic clock.

TODO

# License

Copyright (C) 2025 Piotr Marczyński. This program is licensed under GNU GPL v3. See the file COPYING.