.text
movz X1, 0x1000
lsl X1, X1, 16
movz X10, 0x1234
stur X10, [X1, 0x0]
sturb W10, [X1, 0x6]
ldur X13, [X1, 0x0]
ldur X14, [X1, 0x4]
ldurb W15, [X1, 0x6]
mul X16, X13, X14
HLT 0


12340000
340000