	.arch msp430g2553
	.p2align 1,0
	.text


	.global update_siren_frequency
	.comm siren_frequency, 2 ; Reserve 2 bytes for siren_frequency
	.comm siren_direction, 2 ; Reserve 2 bytes for siren_direction


update_siren_frequency:
	mov &siren_frequency, r12 ;  Load siren_frequency into r12
	mov &siren_direction, r13 ;  Load siren_direction into r13


	cmp #1, r13 		;  Check if direction is increasing (1)
	jne decrease_tone


increase_tone:
	add #50, r12 		;  Add 50 to siren_frequency
	cmp #2000, r12 		;  Check if siren_frequency >= 2000
	jl update_and_return


	mov #-1, &siren_direction ;  Reverse direction: siren_direction = -1
	mov #2000, r12 		;  Limit frequency to 2000
	jmp update_and_return


decrease_tone:
	sub #50, r12 		;  Subtract 50 from siren_frequency
	cmp #500, r12 		;  Check if siren_frequency <= 500
	jge update_and_return


	mov #1, &siren_direction ;  Reverse direction: siren_direction = 1
	mov #500, r12 		;  Limit frequency to 500


update_and_return:
	mov r12, &siren_frequency ;  Store updated siren_frequency back to memory

	    ret 		;  Return the updated siren_frequency in r12
