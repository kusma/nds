.altmacro

.macro datafile symbol, filename
.global \symbol
.align
\symbol:
.incbin "\filename"
.endm
