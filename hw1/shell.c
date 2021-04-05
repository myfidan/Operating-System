
#include <stdio.h>

void ShowPrimes(){
	printf("ShowPrimes.asm\n");
	//load $a0 showprimes.asm in shell.asm
	//and $v0 syscall 18
	//then call syscall in assembly
}

void Factorize(){
	printf("factorize\n");
	//load $a0 factorize.asm in shell.asm
	//and $v0 syscall 18
	//then call syscall in assembly
}

void BubbleSort(){
	printf("BubbleSort\n");
	//load $a0 BubbleSort.asm in shell.asm
	//and $v0 syscall 18
	//then call syscall in assembly
}

void exitCall(){
	printf("exit\n");
	//li $v0,10
    //syscall 
}

int main(){
	int x;
	do{
		printf("***Choose program you want to run***\n");
		printf("1-)ShowPrimes.asm\n2-)Factorize.asm\n3-)BubbleSort.asm\n4-)Exit\n");
		printf("Enter number(1-3) for running a program or exit(4):");
		scanf("%d", &x);
		//li $v0,5
		//syscall
		
		if(x == 1){
			//show prime
			ShowPrimes();
		}
		else if(x == 2){
			//factorize
			Factorize();
		}
		else if(x == 3){
			//bubble sort
			BubbleSort();
		}
		else if(x == 4){
			//exit
			exitCall();
		}
		else{
			//error
			printf("Please enter 1,2,3 or 4..\n" );
		}


	}while(x != 4);
}