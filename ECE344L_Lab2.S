#include <p32xxxx.h>
.globl main
.data
	first_vector: .word 1, 2, 3, 4, 5, 6		# Elements of the first array
	second_vector: .word 2, 8, -2, 2, 2, 3		# Elements of the second array
.text
.set noreorder
.set reg_store, 0xA0000250		# Set memory location for storing ra
.ent main

main:
	li v0, 0					# Initialize v0 to 0
	li v1, 0					# Initialize v1 to 0
	jal dot_product_0			# Store the current stack pointer in ra and jump to the dot product function
	nop
	b end_main				# Jump to the end of the program
	nop

dot_product_0:
	li t0, 6					# t0 is the counter variable, set to the length of the arrays, 6
	li t1, 0					# t1 is the total sum variable sum, initialized to 0
	la s0, first_vector			# s0 is set to the address of the first array
	la s1, second_vector		# s1 is set to the address of the second array
	sw ra, reg_store			# ra is saved to memory

dot_product_1:
	blez t0, dot_product_2		# If the counter variable has looped through all elements of the array (t0 = 0), then end dot_product
	nop
	bltz v1, dot_product_2		# If there is an overflow in multiplication, then end dot_product
	nop
	lw a0, 0(s0)				# Load the nth element of the first array into a0
	lw a1, 0(s1)				# Load the nth element of the second array into a1
	addi s0, s0, 4				# Increment the pointer to the first array by 4 (a word)
	addi s1, s1, 4				# Increment the pointer to the second array by 4 (a word)
	jal multiply_elements_0		# Store the current stack pointer in ra and multiply the two elements
	nop
	add t1, t1, v0				# Add the product of the multiplication to the total sum variable (t1 += v0)
	add t0, t0, -1				# Decrement the counter variable by 1 (t0--)
	b dot_product_1			# Loop the dot product calculaiton
	nop

dot_product_2:
	lw ra, reg_store			# Load the original return address stored in reg_store to ra
	jr ra					# Jump to the original return address
	nop

multiply_elements_0:
	mult a0, a1				# Multiply the nth elements of the arrays
	mflo v0					# Load the lower portion of the multiplication to v0
	mfhi v1					# Load the upper portion of the multiplication to v1
	bnez v1, multiply_elements_1	# If there is anything in the upper portion of the multiplication, set v1 to -1
	li v1, 1					# Else, v1 = 1
	jr ra					# Jump to the return address
	nop

multiply_elements_1:
	li v1, -1					# Set v1 to -1
	jr ra					# Jump to the return address
	nop

end_main:
	nop
	b end_main				# An empty infinite loop to end the program
	nop

.end main