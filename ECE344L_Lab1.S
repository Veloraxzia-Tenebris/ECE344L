#include <p32xxxx.h>
.globl main
.data
.text
.set noreorder
.ent main

main:
	li t0, 1				# Sets t0 to 1 for odd looping

sum:
	beq t0, 559, square		# Since needs to include 557 (Change to 399 + 2 for testing 399)
	nop
	add v0, v0, t0			# v0 = v0 + t0, so v0_n+1 = v0_n + (v0_n + 2)
	addi t0, t0, 2			# t0 = t0 + 2, so t0_n+1 is the next odd number from t0_n
	b sum
	nop

square:
	mult v0, v0			# Hi/Lo = v0 * v0
	mflo t0				# t0 = Lo
	mfhi v1				# v1 = Hi
	bnez v1, square2		# If Lo contains all of the multiplication, continue, else, jump to square2
	nop
	move v1, t0			# v1 = t0
	b end_m
	nop

square2:
	li v1, -1				# v1 = -1 if anything is in Hi
	b end_m
	nop

end_m:
	nop
	b end_m
	nop

.end main