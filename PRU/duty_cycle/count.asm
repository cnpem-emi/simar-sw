; PRU1 - Duty Cycle
	.global asm_count

asm_count:
	ZERO		&r16,4
	ZERO		&r17,4

	LOOP end_count, 200

count_up:
	ADD			r16, r16, 1
	QBBS		count_up, r31.b0, 5

count_down:
	ADD			r17, r17, 1
	QBBC		count_down, r31.b0, 5

end_count:
	SBBO		&r16, r14, 0, 4
	SBBO		&r17, r14, 4, 4
	JMP 		r3.w2
