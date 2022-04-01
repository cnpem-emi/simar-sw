; PRU1 - Glitch and frequency

	.define r16, 		COUNT1
	.define r30.t3,		OUT1			; P8_44 - Glitch

	.define r17, 		COUNT2
	.define r30.t1,		OUT2			; P8_46 - Frequency

	.define r18,		CYCLE_SET
	.define r19,		CYCLE_CLEAR

	.global asm_count

asm_count:
	ZERO		&COUNT1,4
	ZERO		&COUNT2,4
	ZERO		&CYCLE_SET,4
	ZERO		&CYCLE_CLEAR,4
	SET			OUT1
	SET			OUT2

count1:
	SET			OUT1
	QBBC		count2, r31.b0, 2			; P8_43 - Glitch
	ADD			COUNT1, COUNT1, 1
	SET			OUT1
	NOP										; Compensates for janky signals
	NOP										; I'm not using JAL because we'd end up at 3 instructions

count2:
	CLR			OUT2
    NOP
    NOP
	QBBC		cycle_up, r31.b0, 0			; P8_45 - Frequency
    NOP
	ADD			COUNT2, COUNT2, 1
	SET         OUT2
	NOP										; This is just about as fast as we can go
	NOP

cycle_up:
	NOP
	NOP
	NOP
	QBBC		cycle_down, r31.b0, 4		; P8_41 - Power factor
	ADD			CYCLE_SET, CYCLE_SET, 1
	JMP			ret_loop

cycle_down:
	ADD			CYCLE_CLEAR, CYCLE_CLEAR, 1

ret_loop:
	NOP
	NOP
	NOP
	NOP
	QBBC   		count1, r31, 31				; If kick bit is set (message received), return

end_count:
	SBBO		&COUNT1, r14, 0, 4			; Copy pulse count to PRU memory
	SBBO		&COUNT2, r14, 4, 4
	SBBO		&CYCLE_SET, r14, 8, 4		; Copy duty cycle data to PRU memory
	SBBO		&CYCLE_CLEAR, r14, 12, 4
	JMP         r3.w2

