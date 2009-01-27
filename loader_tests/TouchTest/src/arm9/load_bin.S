.section .rodata

.align
.global load_bin
load_bin:
.incbin "data/load.bin"

.global load_bin_end
load_bin_end:

.global load_bin_size
load_bin_size:
.word load_bin_end - load_bin
