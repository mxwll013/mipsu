# mipsu

`mipsu` is a small, self-contained UNIX-style toolkit for working with
MIPS32 instructions built in c89.

## Usage

```
MIPS32 utilities

usage:
  mipsu decode <hex | bin>
  mipsu disasm [hex | bin]
  mipsu encode -R <rs> <rt> <rd> <sh> <fn>
  mipsu encode -I <op> <rs> <rt> <imm>
  mipsu encode -J <op> <addr>
  mipsu asm    [assembly]
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
  -n, --nreg      use register numbers
```

### Decoding

```sh
mipsu decode 0x00b81020
```

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

```sh
mipsu encode R 0x05 0x18 0x02 0x00 0x20
```

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

```sh
mipsu disasm 0x00b81020
```

```
0x00B81020  add      $v0  , $a1  , $t8
```

### Assembly

```sh
mipsu asm 'add $v0 $a1 $t8'
```

```
0x00B81020  add      $v0  , $a1  , $t8
```

## Build

```sh
cc -std=c89 -Wall -Wextra -O2 mipsu.c -o mipsu
```

## License

MIT license. See `LICENSE` for more details.
