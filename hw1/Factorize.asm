.data
	msg: .asciiz "Enter a number for finding its factors: "
	comma: .asciiz ","
	header: .asciiz "factors of "
	is: .asciiz " is "
	zero: .asciiz "Every number is a factor of 0"
	negative: .asciiz "-"
	newline: .asciiz "\n"
.text
	

findFactors:

	addi $t6,$0,1
	addi $t5,$0,0
	addi $t1,$0,1
	blt $t0,$0,negativeCase
	
loop:
	div $t0,$t1
	mfhi $t3 # get mod result to t3
	beq $t3,$0,FactorFounded
cont:
	addi $t1,$t1,1
	bgt $t1,$t0,exit
	j loop

FactorFounded:
	# print #t1
	li $v0, 1
	add $a0,$t1,$0
	syscall

	beq $t5,$t6,writeNegative

	beq $t1,$t0,exit # dont write last comma

	li $v0,4
	la $a0,comma
	syscall
	j cont

exit:
	li $v0,4
	la $a0,newline
	syscall

	li $v0,10
	syscall


zeroCase:
	li $v0,4
	la $a0,zero
	syscall
	j exit

negativeCase:
	addi $t5,$0,1
	sub $t0,$0,$t0
	j loop

writeNegative:
	li $v0,4
	la $a0,comma
	syscall
	
	li $v0,4
	la $a0,negative
	syscall

	li $v0, 1
	add $a0,$t1,$0
	syscall

	beq $t1,$t0,exit

	li $v0,4
	la $a0,comma
	syscall

	j cont


main:
	li $v0,4
	la $a0,msg
	syscall

	li $v0,5
	syscall
	move $t0,$v0

	beq $t0,$0,zeroCase # check given num is zero

	li $v0,4
	la $a0,header
	syscall

	li $v0, 1
	add $a0,$t0,$0
	syscall

	li $v0,4
	la $a0,is
	syscall

	
	jal findFactors
