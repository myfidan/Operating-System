.data
	array: .word 7 20 12 60 11 5 75 200 21 33 44 68 99 105 54 190 17 345 0 1
	MAX: .word 20
	ThreadCount: .word 4

	mutex: .asciiz "mutex"
	after: .asciiz "After Wait Main Thread Continue\n"
	sorted: .asciiz "New array after merge sort:\n"
	space: .asciiz " "
	part: .word 0
	
	
.text


main:
	lw $t2,ThreadCount
	
	li $v0,21
	syscall		#init process

	addi $t4,$0,0

threadCreateLoop:
	beq $t4,$t2,wait

	li $v0,18	#create threaed
	syscall
	beqz $v0,thread2
	addi $t4,$t4,1
	j threadCreateLoop	

	


wait:
	li $v0,19 # Thread join
	syscall
	beqz $t2,AfterWait
	j wait

AfterWait:
	#First merge after threads finish
	addi $a1,$0,0 # low = 0
	
	lw $t0,MAX  #  (MAX / 2 - 1) / 2
	addi $t1,$0,2
	div $t0,$t1
	mflo $t0
	sub $a2,$t0,1
	div $a2,$t1
	mflo $a2

	lw $t0,MAX		# 	MAX / 2 - 1
	div $t0,$t1
	mflo $t0
	sub $a3,$t0,1

	jal merge

	# second merge

	lw $t0,MAX
	addi $t1,$0,2
	div $t0,$t1
	mflo $a1

	sub $t2,$t0,1
	sub $t2,$t2,$a1
	div $t2,$t1
	mflo $t2
	add $a2,$a1,$t2

	lw $t0,MAX
	addi $a3,$t0,-1

	jal merge

	addi $a1,$0,0

	lw $t0,MAX
	addi $t0,$t0,-1 # MAX-1
	addi $t1,$0,2
	div $t0,$t1 # (MAX-1)/2
	mflo $a2

	move $a3,$t0

	jal merge

	li $v0,4
	la $a0,after
	syscall

	
	li $v0,4
	la $a0,sorted
	syscall

	addi $t0,$0,0
	
	addi $t1,$0,4
	lw $t5,MAX
	mult $t5,$t1
	mflo $t5

print_loop:
	li $v0,1
	lw $a0,array($t0)
	syscall

	li $v0,4
    la $a0,space
    syscall

	addi $t0,$t0,4
	beq $t0,$t5,exit_print_loop
	j print_loop

exit_print_loop:

	li $v0,10
	syscall


thread2:

	#load array
	la $s0,array
	
	li $v0,22	#Mutex lock
    la $a0,mutex
    syscall
    
    la $a1,part
 	lw $t0,part # -> thread_part = part
 	addi $a2,$t0,1	
    sw $a2,0($a1)

    ## call mutex unlock here
    li $v0,23	#Mutex unlock
	syscall
    


	
 	lw $t5,MAX	# -> t5 = max
 	lw $t6,ThreadCount	# -> t6 = ThreadCount
 	div $t5,$t6
 	mflo $t4
 	mul $t1,$t0,$t4 # -> $ t1 = low

 	addi $t2,$t0,1
 	mul $t2,$t2,$t4
 	addi $t2,$t2,-1 # -> t2 = high 

 	### Now t1 has low
 	### t2 has high

 	sub $t3,$t2,$t1
 	addi $t4,$0,2
 	div $t3,$t4
 	mflo $t4
 	add $t3,$t1,$t4

 	### now t3 has mid 



    blt $t1,$t2,trueCond
 

	li $v0,20	#Thread Exit
	syscall

trueCond:
	# if low < high
	# take low and mid to a1 and a2 to pass argument
	# between functions
	
	addi $a1,$t1,0
	addi $a2,$t3,0
	jal merge_sort

	addi $a1,$t3,1
	addi $a2,$t2,0
	jal merge_sort


	addi $a1,$t1,0
	addi $a2,$t3,0
	addi $a3,$t2,0
	jal merge

	
	li $v0,20	#Thread Exit
	syscall


merge_sort:
	
 	addi	$sp, $sp, -16
	sw	$ra, 0($sp)	
	sw	$a1, 4($sp)	#a1 first argument
	sw	$a2, 8($sp)	#a2 second argument
	

	#calculate mid point and store it in a3
	sub $t5,$a2,$a1
	addi $t6,$0,2
	div $t5,$t6
	mflo $t7
	add $a3,$a1,$t7

	sw $a3,12($sp) # a3 mid point

   	bge $a1,$a2,mergesortFinish

   	lw $a2,12($sp)

   	jal merge_sort

   	lw $a1,12($sp)
   	addi $a1,$a1,1
   	lw $a2,8($sp)

   	jal merge_sort

   	lw $a1,4($sp)
	lw $a2,12($sp)
	lw $a3,8($sp)

   	jal merge

mergesortFinish:
	lw	$ra, 0($sp)		
	addi	$sp, $sp, 16		
	jr	$ra

merge:
	addi $s6,$0,-16

	# n1 -> s0
	sub $s0,$a2,$a1
	addi $s0,$s0,1

	# n2 -> s1
	sub $s1,$a3,$a2

	# i -> s2
	addi $s2,$0,16 # left array start index 16

	# j -> s3
	addi $s3,$0,16
	addi $s5,$0,4
	mul $s4,$s0,$s5
	add $s3,$s3,$s4 # r,ght array start index 16+n1*4 size

	mult $s5,$s0
	mflo $s7
	sub $s7,$0,$s7
	add $s6,$s7,$s6

	mult $s5,$s1
	mflo $s7
	sub $s7,$0,$s7
	add $s6,$s7,$s6

	
	add	$sp, $sp, $s6
	sw	$ra, 0($sp)	
	sw	$a1, 4($sp)	#a1 first argument low
	sw	$a2, 8($sp)	#a2 second argument mid
	sw $a3,12($sp) # a3 third point high

## DUMMY LOOP FOR SEE INTERRUPT
	#addi $t8,$t0,1000
#sil:
#	beqz $t8,silEnd
#	addi $t8,$t8,-1
#	j sil
#silEnd:

	#fill stack arrays left and right
	addi $t8,$s2,0



	addi $a0,$0,0
	# find a[i+low]
	add $t9,$a0,$a1
	mult $t9,$s5
	mflo $t9

leftLoop:
	beq $s2,$s3,leftLoopExit




	lw $s4,array($t9)
	add $s5,$sp,$s2
	sw $s4,($s5)
	
	

	addi $t9,$t9,4
	addi $a0,$a0,4
	addi $s2,$s2,4
	j leftLoop


leftLoopExit:
	addi $s2,$t8,0
					#Make t8 containe last position of right array
	addi $t8,$s3,0
	addi $s4,$0,4
	mult $s4,$s1
	mflo $s4
	add $t8,$t8,$s4

	addi $t9,$s3,0
	addi $a0,$a2,1
	addi $s4,$0,4
	mult $s4,$a0
	mflo $a0

rightLoop:
	beq $s3,$t8,rightLoopExit


	lw $s4,array($a0)
	add $s5,$sp,$s3
	sw $s4,($s5)

	addi $a0,$a0,4
	addi $s3,$s3,4
	j rightLoop

rightLoopExit:
	addi $s3,$t9,0

	addi $s4,$0,16
	add $s5,$sp,$s4


 	# k = low
	addi $s4,$a1,0
	# i = 0
	addi $t8,$0,0
	# j = 0
	addi $t9,$0,0
	#left
	add $a1,$sp,$s2
	#right
	add $a2,$sp,$s3
while1:
	bge $t8,$s0,while1End
	bge $t9,$s1,while1End

	lw $a3,0($a1) # a3 left value
	lw $s5,0($a2) # s5 right value
	#k*4
	addi $s7,$0,4
	mult $s4,$s7
	mflo $s7
	ble $a3,$s5,if
	j else
if:	
	sw $a3,array($s7)
	addi $a1,$a1,4	
	addi $t8,$t8,1
	j AfterIfElse
else:
	sw $s5,array($s7)
	addi $a2,$a2,4
	addi $t9,$t9,1
AfterIfElse:

	addi $s4,$s4,1
	j while1

while1End:

while2:
	bge $t8,$s0,while2End
	lw $a3,0($a1) # a3 left value
	#k*4
	addi $s7,$0,4
	mult $s4,$s7
	mflo $s7

	sw $a3,array($s7)
	addi $a1,$a1,4	
	addi $t8,$t8,1	
	addi $s4,$s4,1
	j while2

while2End:

while3:
	bge $t9,$s1,while3End
	lw $s5,0($a2) # s5 right value
	#k*4
	addi $s7,$0,4
	mult $s4,$s7
	mflo $s7

	sw $s5,array($s7)
	addi $a2,$a2,4
	addi $t9,$t9,1
	addi $s4,$s4,1
	j while3

while3End:


	sub $s7,$0,$s6  ## bunu sonra mergeEnd başına al
mergeEnd:
	

	lw $ra, 0($sp)
	lw $a1,4($sp)
	lw $a2,8($sp)
	lw $a3,12($sp)
	add $sp, $sp, $s7

	jr $ra


