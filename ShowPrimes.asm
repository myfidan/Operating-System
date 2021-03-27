

.data
	prime:   .asciiz "prime\n"
	nonPrime: .asciiz "notprime\n"
	blankSpace: .asciiz " "

	.extern foobar 4

.text
        .globl main


checkPrime:
	addi $t0,$0,2
	addi $a2,$0,0 #return value if 0 prime ,1 not prime
	blt $a1,$t0,exitNotPrimeLoop
primeloop:
	beq $t0,$a1,exitPrimeLoop
	div $a1,$t0
	mfhi $t3 # get mod result to t3

	beq $t3,$0,exitNotPrimeLoop

	addi $t0,$t0,1
	j primeloop

exitPrimeLoop: j printPrime

exitNotPrimeLoop:
	addi $a2,$0,1
	j printNotPrime

printNotPrime:

	li $v0, 1
	add $a0,$a1,$0
	syscall

	li $v0, 4       # syscall 4 (print_str)
    la $a0, blankSpace     # argument: string
    syscall

	li $v0, 4       # syscall 4 (print_str)
    la $a0, nonPrime     # argument: string
    syscall 
    jr $ra

printPrime:

	li $v0, 1
	add $a0,$a1,$0
	syscall

	li $v0, 4       # syscall 4 (print_str)
    la $a0, blankSpace     # argument: string
    syscall

	li $v0, 4       # syscall 4 (print_str)
    la $a0, prime     # argument: string
    syscall 
    jr $ra



mainLoop:
	add $a1,$0,$t4
	jal checkPrime
	beq $t4,$t5,exit
	addi $t4,$t4,1
	j mainLoop

exit:
	li $v0, 10
	syscall

main:
	
	addi $t4,$0,0
	addi $t5,$0,1000
	j mainLoop

	

    li $v0, 10
	syscall