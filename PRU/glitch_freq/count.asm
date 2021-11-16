; PRU1 - Glitch and frequency

	.define r16, 		COUNT1
	.define r30.t3,		OUT1		; P8_44

	.define r17, 		COUNT2
	.define r30.t1,		OUT2		; P8_46
	.global asm_count

asm_count:
	ZERO		&COUNT1,4
	ZERO		&COUNT2,4

count1:
	QBBC		count2, r31.b0, 2	; P8_43
	ADD			COUNT1, COUNT1, 1
	CLR			OUT1

count2:
	SET			OUT1
	QBBC		ret_loop, r31.b0, 0	; P8_45
	ADD			COUNT2, COUNT2, 1
	CLR			OUT2
ret_loop:
	SET			OUT2
	QBBC   		count1, r31.b3, 7	; If kick bit is set (message received), return

end_count:
	SBBO		&COUNT1, r14, 0, 4
	SBBO		&COUNT2, r14, 4, 4	; Copy pulse count to PRU memory
	JMP         r3.w2
