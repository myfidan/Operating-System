.data
	menuHeader: .asciiz "***Choose program you want to run***\n"
	menu: .asciiz "1-)ShowPrimes.asm\n2-)Factorize.asm\n3-)BubbleSort.asm\n4-)Exit\n"
	menuFooter: .asciiz "Enter number(1-3) for running a program or exit(4):"
	err: .asciiz "Please enter 1,2,3 or 4..\n"

	prime: .asciiz "ShowPrimes.asm"
	factorize: .asciiz "Factorize.asm"
	bubblesort: .asciiz "BubbleSort.asm"

.text

main:
	li $v0,4
	la $a0,menuHeader
	syscall

	li $v0,4
	la $a0,menu
	syscall

	li $v0,4
	la $a0,menuFooter
	syscall

	li $v0,5
	syscall
	move $t4,$v0 ## Take input 1-4

	addi $t0,$0,1 #t0 = ShowPrimes
	addi $t1,$0,2 #t1 = Factorize
	addi $t2,$0,3 #t2 = BubbleSort
	addi $t3,$0,4

	beq $t4,$t0,ShowPrimes
	beq $t4,$t1,Factorize
	beq $t4,$t2,BubbleSort
	beq $t4,$t3,Exit

	li $v0,4
	la $a0,err
	syscall

	j main


ShowPrimes:
	li $v0,18
	la $a0,prime
	syscall

	j main

Factorize:
	li $v0,18
	la $a0,factorize
	syscall
	
	j main

BubbleSort:
	li $v0,18
	la $a0,bubblesort
	syscall
	
	j main	

Exit:
	li $v0,10
	syscall		