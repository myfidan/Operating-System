
	.data
array: .space 40
headerMsg: .asciiz "Enter 10 number for sorting: "
space: .asciiz " "

	.text

read_numbers:
	addi $t0,$0,0
read_loop:
	li $v0,5
	syscall
	sw $v0,array($t0)
	addi $t0,$t0,4
	beq $t0,40,exit_read_loop
	j read_loop

exit_read_loop:
	jr $ra	



sort:
	sub $a2,$a2,4
	addi $t0,$0,0
	addi $t1,$0,4
	beq $a2,$0,exit_sort
inner_loop:
	beq $t0,$a2,sort
	lw $t5,array($t0)
	lw $t6,array($t1)
	bgt $t5,$t6,swap
swap_back:	
	addi $t0,$t0,4
	addi $t1,$t1,4
	j inner_loop

swap: # swap t0 and t1
	lw $t2,array($t0)
	lw $t3,array($t1)
	sw $t2,array($t1)
	sw $t3,array($t0)
	j swap_back
	

exit_sort:
	jr $ra

print_array:
	addi $t0,$0,0
print_loop:
	li $v0,1
	lw $a0,array($t0)
	syscall

	li $v0,4
    la $a0,space
    syscall

	addi $t0,$t0,4
	beq $t0,40,exit_print_loop
	j print_loop

exit_print_loop:
	jr $ra	

main:
	li $v0,4
    la $a0,headerMsg
    syscall
	
	jal read_numbers
	addi $a2,$0,40
	jal sort
	jal print_array
	li $v0,10
	syscall
