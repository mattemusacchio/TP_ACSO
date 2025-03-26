.text
adds X2, X0, 10
subs X3, X2, 15
adds X4, X3, X2
adds X1, X0, 16777220
cmp X4, X2
ble foo
subs X5, X4, X2

bar:
HLT 0

foo:
ands X6, X4, X2
orr X7, X3, X5
stur X7, [X1, 0x0]
cmp X7, X6
beq bar
adds X3, X3, X2
HLT 0
