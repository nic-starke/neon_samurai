# Binary Analysis

To determine the memory usage of symbols in the binary there are a few tools available:

Note that you will need to build with debug symbols

## Puncover

- Checkout puncover
- Setup a pytho venv inside the checkout, and active it.
- `pip install requirements.txt`
- `pip install setuptools`
- `python runner.py --elf_file neosam.elf --gcc-tools-base /usr/bin/avr-`

## Elf_Diff

`./bin/elf_diff --bin_prefix "avr-" neosam.elf neosam.elf`
