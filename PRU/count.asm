; PRU0 - CountingPRU

	.define r16, 		COUNT1
	.define r30.t1,		OUT1		; P9_29
	.define r31.t3, 	IN1			; P9_28

	.define r17, 		COUNT2
	.define r30.t0,		OUT2		; P9_31
	.define r31.t2, 	IN2			; P9_30

	.define r18, 		COUNT3
	.define r30.t5,		OUT3		; P9_27
	.define r31.t16, 	IN3			; P9_24

	.define r19, 		COUNT4
	.define r30.t14,	OUT4		; P8_12
	.define r31.t15, 	IN4			; P8_15
	.global asm_count

asm_count:
	ZERO		&COUNT1,4
	ZERO		&COUNT2,4
	ZERO		&COUNT3,4
	ZERO		&COUNT4,4

count1:
	QBBC		count2, r31.b0, 3
	ADD			COUNT1, COUNT1, 1
	CLR			OUT1

count2:
	QBBC		count3, r31.b0, 2
	ADD			COUNT2, COUNT2, 1
	CLR			OUT2

count3:
	LDI			r30, 0x03
	QBBC		count4, r31.b2, 0
	ADD			COUNT3, COUNT3, 1
	CLR			OUT3

count4:
	SET			OUT3
	QBBC		ret_loop, r31.b1, 6
	ADD			COUNT4, COUNT4, 1
	CLR			OUT4
ret_loop:
	SET			OUT4
	QBBC   		count1, r31.b3, 6	; If kick bit is set (message received), return

end_count:
	SBBO		&COUNT1, r14, 0, 4
	SBBO		&COUNT2, r14, 4, 4
	SBBO		&COUNT3, r14, 8, 4
	SBBO		&COUNT4, r14, 12, 4	; Copy pulse count to PRU memory
