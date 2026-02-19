# mipsu

`mipsu` is a small, self-contained UNIX-style toolkit for working with
MIPS32 instructions built in c89.

## Usage

```
MIPS32 utilities

usage:
  mipsu decode <hex | bin>
  mipsu disasm <hex | bin>
  mipsu disasm -f <file>
  mipsu encode -R <rs> <rt> <rd> <sh> <fn>
  mipsu encode -I <op> <rs> <rt> <imm>
  mipsu encode -J <op> <addr>
  mipsu asm    <mips>
  mipsu asm    -f <file>
  mipsu --version
  mipsu --help | -h

commands:
  decode  32bit instruction -> bitfield
  disasm  32bit instruction -> assembly
  encode  bitfield -> 32bit instruction
  asm     assembly -> 32bit instruction

flags:
  -q, --quiet     minimal output
  -v, --verbose   maximal output
      --no-color  plain output
  -n, --nreg      use numbers when formatting registers
  -s, --strict    enable strict parsing
      --raw       handle raw binary files

options:
  -o <file>, --output <file>  Specify an output file
  -f <file>, --file   <file>  Specify an input file
```

### Decoding

Decode instruction fields from machine code.

```sh
mipsu decode 0x00b81020
```

Output

```
hex:  0x00B81020
type: R
-------
rs:  0x05  ($a1)
rt:  0x18  ($t8)
rd:  0x02  ($v0)
sh:  0x00  (0)
fn:  0x20  (add)
```

### Encoding

Encode instruction fields into machine code.

```sh
mipsu encode R 0x05 0x18 0x02 0x00 0x20
```

Output

```
hex:  0x00B81020
type: R
-------
rs:  0x05  ($a1)
rt:  0x18  ($t8)
rd:  0x02  ($v0)
sh:  0x00  (0)
fn:  0x20  (add)
```

### Disassembly

Disassemble machine code into human-readable assembly.

```sh
mipsu disasm 0x00b81020
```

Output

```
0x00B81020  add      $v0  , $a1  , $t8
```

### Assembly

Assemble human-readable assembly into machine code.

```sh
mipsu asm 'add $v0, $a1, $t8'
```

Output

```
0x00B81020  add      $v0  , $a1  , $t8
```

### Raw binary support

Both `disasm` and `asm` can operate directly on raw binary files.

Example assembly (`mips.asm`):

```asm
add  $v0, $a1, $t8
addu $v0, $a1, $t8
sub  $v0, $a1, $t8
subu $v0, $a1, $t8
```

Assemble to raw binary and then disassemble it

```sh
mipsu asm -f mips.asm --raw -o mips.bin
mipsu disasm --raw -f mips.bin
```

Output

```
0x00B81020  add      $v0  , $a1  , $t8
0x00B81021  addu     $v0  , $a1  , $t8
0x00B81022  sub      $v0  , $a1  , $t8
0x00B81023  subu     $v0  , $a1  , $t8
```

## Build

```sh
cc -std=c89 -Wall -Wextra -O2 mipsu.c -o mipsu
```

## License

MIT license. See `LICENSE` for more details.
