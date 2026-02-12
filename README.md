# mipsu

`mipsu` is a small, self-contained UNIX-style toolkit for working with
MIPS32 instructions built in c89.

## Usage

### Decoding

```sh
mipsu decode 0x00b81020
```

Output:

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

Output:

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

Output:

```
0x00B81020  add      $v0  , $a1  , $t8
```

## Build

```sh
cc -std=c89 -Wall -Wextra -O2 mipsu.c -o mipsu
```

## License

MIT license. See `LICENSE.txt` for more details.
