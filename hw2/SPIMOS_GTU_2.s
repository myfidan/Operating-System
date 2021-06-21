.data
	ThreadCount: .word 2
	msg: .asciiz "Enter (1) for mutex producer-consumer or (2) for no mutex producer-consumer\n"
	mutex: .asciiz "mutex" #mutex
	producerLock: .asciiz "*****Producer START*****\n"
	producing: .asciiz "PRODUCING...\n"
	producerUnlock: .asciiz "*****Producer END*****\n"
	consumerLock: .asciiz "*****Consumer START*****\n"
	consuming: .asciiz "CONSUMING...\n"
	consumerUnlock: .asciiz "*****Consumer END*****\n"

.text


main:

	lw $t2,ThreadCount

	li $v0,21
	syscall		#init process
	
	li $v0,4
	la $a0,msg
	syscall

	li $v0,5
	syscall

	move $t6,$v0

	beq $t6,2,nomutex
	
	

	li $v0,18	#create threaed
	syscall
	beqz $v0,producer

	li $v0,18	#create threaed
	syscall
	beqz $v0,consumer

wait:
	li $v0,19 # Thread join
	syscall
	beqz $t2,AfterWait
	j wait

AfterWait:
		
	li $v0,10
	syscall

nomutex:

	li $v0,18	#create threaed
	syscall
	beqz $v0,nomutexproducer

	li $v0,18	#create threaed
	syscall
	beqz $v0,nomutexconsumer

nomutexwait:
	li $v0,19 # Thread join
	syscall
	beqz $t2,nomutexAfterWait
	j nomutexwait

nomutexAfterWait:
		
	li $v0,10
	syscall




producer:
	addi $t1,$0,0
	addi $t2,$0,10
producerLoop:
	beq $t1,$t2,endProducer

	addi $t3,$0,0
	addi $t4,$0,1000

	li $v0,22	#Mutex lock
    la $a0,mutex
    syscall

    li $v0,4
	la $a0,producerLock
	syscall

	li $v0,4
	la $a0,producing
	syscall

Producersleep:
	beq $t3,$t4,endProducerSleep
	addi $t3,$t3,1
	j Producersleep
endProducerSleep:

	addi $t1,$t1,1

	li $v0,4
	la $a0,producerUnlock
	syscall

	 ## call mutex unlock here
    li $v0,23	#Mutex unlock
	syscall

	j producerLoop

endProducer:

	li $v0,20	#Thread Exit
	syscall



consumer:
	addi $t1,$0,0
	addi $t2,$0,10
consumerLoop:
	beq $t1,$t2,endConsumer

	addi $t3,$0,0
	addi $t4,$0,1000

	li $v0,22	#Mutex lock
    la $a0,mutex
    syscall

    li $v0,4
	la $a0,consumerLock
	syscall


	li $v0,4
	la $a0,consuming
	syscall


consumerSleep:
	beq $t3,$t4,endConsumerSleep
	addi $t3,$t3,1
	j consumerSleep
endConsumerSleep:

	addi $t1,$t1,1

	li $v0,4
	la $a0,consumerUnlock
	syscall

	 ## call mutex unlock here
    li $v0,23	#Mutex unlock
	syscall

	j consumerLoop

endConsumer:

	li $v0,20	#Thread Exit
	syscall



nomutexproducer:	
	addi $t1,$0,0
	addi $t2,$0,10
nomutexproducerLoop:
	beq $t1,$t2,nomutexendProducer

	addi $t3,$0,0
	addi $t4,$0,1000


    li $v0,4
	la $a0,producerLock
	syscall


	li $v0,4
	la $a0,producing
	syscall

nomutexProducersleep:
	beq $t3,$t4,nomutexendProducerSleep
	addi $t3,$t3,1
	j nomutexProducersleep
nomutexendProducerSleep:

	addi $t1,$t1,1

	li $v0,4
	la $a0,producerUnlock
	syscall


	j nomutexproducerLoop

nomutexendProducer:

	li $v0,20	#Thread Exit
	syscall



nomutexconsumer:
	addi $t1,$0,0
	addi $t2,$0,10
nomutexconsumerLoop:
	beq $t1,$t2,nomutexendConsumer

	addi $t3,$0,0
	addi $t4,$0,1000

	

    li $v0,4
	la $a0,consumerLock
	syscall

	li $v0,4
	la $a0,consuming
	syscall

nomutexconsumerSleep:
	beq $t3,$t4,nomutexendConsumerSleep
	addi $t3,$t3,1
	j nomutexconsumerSleep
nomutexendConsumerSleep:

	addi $t1,$t1,1

	li $v0,4
	la $a0,consumerUnlock
	syscall

	 

	j nomutexconsumerLoop

nomutexendConsumer:

	li $v0,20	#Thread Exit
	syscall