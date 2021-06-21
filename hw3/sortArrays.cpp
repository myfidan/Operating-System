#include <iostream>
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <cmath>
#include <pthread.h>
#include <fstream>
#include <vector>
#include <ctime> // For time info
#include <limits>

using namespace std;

int frameSize;
int numPhysical;
int numVirtual;
char pageReplacement[10];
char tableType[30];
int pageTablePrintInt;
char diskFileName[50];

int currentPageTablePrint = 0;

struct Page{
	int* memory;
};

struct PageEntry{
	int showingPageNum;
	bool referanced = false;
	bool modified = false;
	//This time info used for LRU algorithm
	double time;
};

struct Info{
	int numberOfRead;
	int numberOfWrite;
	int numberOfPageMiss;
	int numberOfPageRep;
	int numberOfDiskPageWrite;
	int numberOfDiskPageRead;
};

// Info for all threads
Info informations[5];
// 0 -> Bubble
// 1 -> Quick
// 2 -> Merge
// 3 -> Linear
// 4 -> Binary

//File for disk
fstream mydisk;

Page* virtualMemory;
Page* physicalMemory;


PageEntry* VirtualMemoryPT;
PageEntry* PhysicalMemoryPT;

//Used for second change
vector<PageEntry*>SecondChange;
//Used for WSClock page replacement
double ThreshHold = 0; //ThreshHold for WSClock
int WSIndex = 0; //WSclock data structure index
PageEntry** Clock; //WSClock circular array

char second[10];
char leastRecent[10];
char wsclock[10];

char bubbleS[10];
char quickS[10];
char mergeS[10];
char linearS[10];
char binaryS[10];

int scCount = 0;
int lruCount = 0;
int wsclockCount = 0;

//Used for linear searc 3 of them in virtual memory 2 of them not
int linear_search_arr[5];
int binary_search_arr[5];

pthread_mutex_t lock;

void Init_infos();
void create_memory(int argc, char** argv);
void fill_virtual_memory_init();
int get(unsigned int index,char* tName);
void set(unsigned int index, int value, char * tName);
bool checkHit(int pageNum);
int* getPageFromDisk(int VpageNum);
void update_disk(int deletedPage);
int SecondChange_get(int pageNum,int pageIndex,char* threadType);
void SecondChange_set(int pageNum,int pageIndex,int value,char* threadType);
int LRU_get(int pageNum,int pageIndex,char* threadType);
void LRU_set(int pageNum,int pageIndex,int value,char* threadType);
int WsClock_get(int pageNum,int pageIndex,char* threadType);
void WsClock_set(int pageNum,int pageIndex,int value,char* threadType);

void* thread_bubble_sort(void* arg);
void* thread_quick_sort(void* arg);
void* thread_merge(void* arg);
void* thread_linear(void* arg);
void* thread_binary(void* arg);

void bubbleSort(int startIndex,int endIndex);
void quickSort(int startIndex,int endIndex);
int partition(int startIndex, int endIndex);
void mergeAfterSort();
void linear_search(int value);
int binary_search(int value,int left,int right);

void inc_DiskWrite(char* threadType);
void inc_DiskRead(char* threadType);
void inc_PageMiss(char* threadType);
void inc_PageRep(char* threadType);
void printPageTable();

int main(int argc, char** argv){

	if(argc != 8){
		cout<<"Wrong argument.."<<endl;
		exit(1);
	}
	strcpy(second,"SC");
	strcpy(leastRecent,"LRU");
	strcpy(wsclock,"WSClock");
	strcpy(bubbleS,"bubble");
	strcpy(quickS,"quick");
	strcpy(mergeS,"merge");
	strcpy(linearS,"linear");
	strcpy(binaryS,"binary");

	create_memory(argc,argv);

	pthread_t threads[5];

	pthread_create(&threads[0],NULL,&thread_bubble_sort,(void *)0);
	pthread_create(&threads[1],NULL,&thread_quick_sort,(void *)1);

	pthread_join(threads[0],NULL);
	pthread_join(threads[1],NULL);

	pthread_create(&threads[2],NULL,&thread_merge,NULL);
	pthread_join(threads[2],NULL);
	
	pthread_create(&threads[3],NULL,&thread_linear,NULL);
	pthread_create(&threads[4],NULL,&thread_binary,NULL);
	
	pthread_join(threads[3],NULL);
	pthread_join(threads[4],NULL);

	for(int i=0; i<numVirtual; i++){
		for(int j=0; j<frameSize; j++){
			cout<<virtualMemory[i].memory[j]<<endl;
		}
		cout<<endl;
	}

	cout<<endl;

   
	for(int i=0; i<5; i++){
		cout<<"Thread "<<i<<endl;
		cout<<"Number of Read: "<<informations[i].numberOfRead<<endl;
		cout<<"Number of Write: "<<informations[i].numberOfWrite<<endl;
		cout<<"Number of Page Miss: "<<informations[i].numberOfPageMiss<<endl;
		cout<<"Number of Page Replacements: "<<informations[i].numberOfDiskPageRead<<endl;
		cout<<"Number of Disk Page Writes: "<<informations[i].numberOfDiskPageWrite<<endl;
		cout<<"Number of Disk page Reads: "<<informations[i].numberOfDiskPageRead<<endl;
		cout<<endl;
	}
	return 0;
}

//init informations
void Init_infos(){

	for(int i=0; i<5; i++){
		informations[i].numberOfRead = 0;
		informations[i].numberOfWrite = 0;
		informations[i].numberOfPageMiss = 0;
		informations[i].numberOfPageRep = 0;
		informations[i].numberOfDiskPageWrite = 0;
		informations[i].numberOfDiskPageRead = 0;
	}

}

void create_memory(int argc, char** argv){
	frameSize = pow(2,atoi(argv[1]));
	numPhysical = pow(2,atoi(argv[2]));
	numVirtual = pow(2,atoi(argv[3]));
	strcpy(pageReplacement,argv[4]);
	strcpy(tableType,argv[5]);
	pageTablePrintInt = atoi(argv[6]);
	strcpy(diskFileName,argv[7]);

	virtualMemory = (Page*)malloc(sizeof(struct Page)*numVirtual);
	physicalMemory = (Page*)malloc(sizeof(struct Page)*numPhysical);

	VirtualMemoryPT = (PageEntry*)malloc(sizeof(struct PageEntry) * numVirtual);
	PhysicalMemoryPT = (PageEntry*)malloc(sizeof(struct PageEntry) * numVirtual);
	Clock = (PageEntry**)malloc(sizeof(struct PageEntry*) * numPhysical);
	for(int i=0; i<numVirtual; i++){
		VirtualMemoryPT[i].referanced = false;
		VirtualMemoryPT[i].modified = false;

		if(i < numPhysical){
			VirtualMemoryPT[i].showingPageNum = i;
			clock_t end = clock();
  			double elapsed_secs = double(end) / CLOCKS_PER_SEC;
			VirtualMemoryPT[i].time = elapsed_secs;
			Clock[i] = &VirtualMemoryPT[i];
		}
		else{
			VirtualMemoryPT[i].showingPageNum = -1;
		}
		virtualMemory[i].memory = (int*)malloc(sizeof(int)*frameSize);
	}
	for(int i=0; i<numPhysical; i++){
		informations[0].numberOfDiskPageRead++;
		informations[0].numberOfPageRep++;
		informations[0].numberOfPageMiss++;
		SecondChange.push_back(&VirtualMemoryPT[i]);
		PhysicalMemoryPT[i].showingPageNum = i;
		physicalMemory[i].memory = (int*)malloc(sizeof(int)*frameSize);
	}
	for(int i=numPhysical; i<numVirtual; i++){
		PhysicalMemoryPT[i].showingPageNum = -1;
	}
	fill_virtual_memory_init();
}



void fill_virtual_memory_init(){
	
 	mydisk.open ("disk.dat",std::fstream::out | std::fstream::in | ios::trunc);
  	
  	srand(1000);
	for(int i=0; i<numVirtual; i++){
		for (int j = 0; j < frameSize; ++j){
			virtualMemory[i].memory[j] = rand();
			mydisk<<virtualMemory[i].memory[j]<<endl;
		}
	}
	for(int i=0; i<3; i++){
		linear_search_arr[i] = virtualMemory[0].memory[i];
		binary_search_arr[i] = virtualMemory[0].memory[i];
	}
	for(int k = 0; k<numPhysical; k++){
		for (int j = 0; j < frameSize; ++j){
			physicalMemory[k].memory[j] = virtualMemory[k].memory[j];
		}
	}
	
}


int get(unsigned int index,char* tName){
	currentPageTablePrint++;
	if(pageTablePrintInt == currentPageTablePrint){
		printPageTable();
		currentPageTablePrint = 0;
	}

	int pageNum = (index / frameSize);
	int pageIndex = (index % frameSize);
	int returnValue = 0;
	// if bubble sort call
	if(strcmp(tName,bubbleS) == 0){
		informations[0].numberOfRead++;
		if(strcmp(pageReplacement,second) == 0 ){
		scCount++;
		returnValue = SecondChange_get(pageNum,pageIndex,bubbleS);
		}
		else if(strcmp(pageReplacement,leastRecent) == 0){
			lruCount++;
			returnValue = LRU_get(pageNum,pageIndex,bubbleS);
		}
		else if(strcmp(pageReplacement,wsclock) == 0){
			wsclockCount++;
			returnValue = WsClock_get(pageNum,pageIndex,bubbleS);
		}
	}
	// if quick sort call
	else if(strcmp(tName,quickS) == 0){
		informations[1].numberOfRead++;
		if(strcmp(pageReplacement,second) == 0 ){
		scCount++;
		returnValue = SecondChange_get(pageNum,pageIndex,quickS);
		}
		else if(strcmp(pageReplacement,leastRecent) == 0){
			lruCount++;
			returnValue = LRU_get(pageNum,pageIndex,quickS);
		}
		else if(strcmp(pageReplacement,wsclock) == 0){
			wsclockCount++;
			returnValue = WsClock_get(pageNum,pageIndex,quickS);
		}
	}
	// if merge thread call
	else if(strcmp(tName,mergeS) == 0){
		informations[2].numberOfRead++;
		if(strcmp(pageReplacement,second) == 0 ){
		scCount++;
		returnValue = SecondChange_get(pageNum,pageIndex,mergeS);
		}
		else if(strcmp(pageReplacement,leastRecent) == 0){
			lruCount++;
			returnValue = LRU_get(pageNum,pageIndex,mergeS);
		}
		else if(strcmp(pageReplacement,wsclock) == 0){
			wsclockCount++;
			returnValue = WsClock_get(pageNum,pageIndex,mergeS);
		}
	}
	//if linear search
	else if(strcmp(tName,linearS) == 0){
		informations[3].numberOfRead++;
		if(strcmp(pageReplacement,second) == 0 ){
		scCount++;
		returnValue = SecondChange_get(pageNum,pageIndex,linearS);
		}
		else if(strcmp(pageReplacement,leastRecent) == 0){
			lruCount++;
			returnValue = LRU_get(pageNum,pageIndex,linearS);
		}
		else if(strcmp(pageReplacement,wsclock) == 0){
			wsclockCount++;
			returnValue = WsClock_get(pageNum,pageIndex,linearS);
		}
	}
	else if(strcmp(tName,binaryS) == 0){
		informations[4].numberOfRead++;
		if(strcmp(pageReplacement,second) == 0 ){
		scCount++;
		returnValue = SecondChange_get(pageNum,pageIndex,binaryS);
		}
		else if(strcmp(pageReplacement,leastRecent) == 0){
			lruCount++;
			returnValue = LRU_get(pageNum,pageIndex,binaryS);
		}
		else if(strcmp(pageReplacement,wsclock) == 0){
			wsclockCount++;
			returnValue = WsClock_get(pageNum,pageIndex,binaryS);
		}
	}
	
	

	return returnValue;
}

void set(unsigned int index, int value, char * tName){
	currentPageTablePrint++;
	if(pageTablePrintInt == currentPageTablePrint){
		printPageTable();
		currentPageTablePrint = 0;
	}

	int pageNum = (index / frameSize);
	int pageIndex = (index % frameSize);
	//if bubble sort thread call
	if(strcmp(tName,bubbleS) == 0){
		informations[0].numberOfWrite++;
		if(strcmp(pageReplacement,second) == 0 ){
			scCount++;
			SecondChange_set(pageNum,pageIndex,value,bubbleS);
		}
		else if(strcmp(pageReplacement,leastRecent) ==0){
			lruCount++;
			LRU_set(pageNum,pageIndex,value,bubbleS);
		}
		else if(strcmp(pageReplacement,wsclock) == 0){
			wsclockCount++;
			WsClock_set(pageNum,pageIndex,value,bubbleS);
		}
	}
	//if quick sort thread call
	else if(strcmp(tName,quickS) == 0){
		informations[1].numberOfWrite++;
		if(strcmp(pageReplacement,second) == 0 ){
			scCount++;
			SecondChange_set(pageNum,pageIndex,value,quickS);
		}
		else if(strcmp(pageReplacement,leastRecent) ==0){
			lruCount++;
			LRU_set(pageNum,pageIndex,value,quickS);
		}
		else if(strcmp(pageReplacement,wsclock) == 0){
			wsclockCount++;
			WsClock_set(pageNum,pageIndex,value,quickS);
		}
	}
	//if merge thread call
	else if(strcmp(tName,mergeS) == 0){
		informations[2].numberOfWrite++;
		if(strcmp(pageReplacement,second) == 0 ){
			scCount++;
			SecondChange_set(pageNum,pageIndex,value,mergeS);
		}
		else if(strcmp(pageReplacement,leastRecent) ==0){
			lruCount++;
			LRU_set(pageNum,pageIndex,value,mergeS);
		}
		else if(strcmp(pageReplacement,wsclock) == 0){
			wsclockCount++;
			WsClock_set(pageNum,pageIndex,value,mergeS);
		}
	}
	// if linear search thread
	else if(strcmp(tName,linearS) == 0){
		informations[3].numberOfWrite++;
		if(strcmp(pageReplacement,second) == 0 ){
			scCount++;
			SecondChange_set(pageNum,pageIndex,value,linearS);
		}
		else if(strcmp(pageReplacement,leastRecent) ==0){
			lruCount++;
			LRU_set(pageNum,pageIndex,value,linearS);
		}
		else if(strcmp(pageReplacement,wsclock) == 0){
			wsclockCount++;
			WsClock_set(pageNum,pageIndex,value,linearS);
		}
	}
	else if(strcmp(tName,binaryS) == 0){
		informations[4].numberOfWrite++;
		if(strcmp(pageReplacement,second) == 0 ){
			scCount++;
			SecondChange_set(pageNum,pageIndex,value,binaryS);
		}
		else if(strcmp(pageReplacement,leastRecent) ==0){
			lruCount++;
			LRU_set(pageNum,pageIndex,value,binaryS);
		}
		else if(strcmp(pageReplacement,wsclock) == 0){
			wsclockCount++;
			WsClock_set(pageNum,pageIndex,value,binaryS);
		}
	}
	
	
}

bool checkHit(int pageNum){
	if(VirtualMemoryPT[pageNum].showingPageNum == -1){
		return false;
	}
	return true;
}

int* getPageFromDisk(int VpageNum){
	
	int* newPage = (int*)malloc(sizeof(int) * frameSize);
	int check = 0;
	int check2 = 0;
	char takeLine[15];
	mydisk.seekg (0, mydisk.beg);
	for(int i=0; i<numVirtual; i++){
		if(VpageNum == i) check = 1;
		for(int j=0; j<frameSize; j++){
			if(check == 1){
				check2 = 1;
				mydisk.getline(takeLine,15);
				newPage[j] = atoi(takeLine);
			}
			else{
				mydisk.getline(takeLine,15);
			}
		}
		if(check2 == 1) break;

	} 

	mydisk.seekg (0, mydisk.beg);
	return newPage;
}

void update_disk(int deletedPage) {
	int flag;
	for(int j =0; j< numVirtual; j++){
		if(VirtualMemoryPT[j].showingPageNum == deletedPage){
			flag = j;
			break;
		}
	}
	int* arr = (int*)malloc(sizeof(int)*frameSize*numVirtual);
	char takeLine[15];
	mydisk.seekg (0, mydisk.beg);
	for(int i=0; i<frameSize* numVirtual; i++){
		mydisk.getline(takeLine,15);
		arr[i] = atoi(takeLine);
	}
	int k =0;
	for(int i=flag*frameSize; i<(flag*frameSize)+frameSize; i++){
		arr[i] = virtualMemory[flag].memory[k++];
	}
	mydisk.close();
	mydisk.open("disk.dat",std::fstream::out | std::fstream::in | ios::trunc);
	for(int i=0; i<frameSize* numVirtual; i++){
		mydisk<<arr[i]<<endl;
		//cout<<arr[i]<<endl;
	}
	mydisk.seekg (0, mydisk.beg);
	
}


int SecondChange_get(int pageNum,int pageIndex,char* threadType){
	int returnValue = 0;
	if(checkHit(pageNum)){ // if page in physical memory
		int physicalPageNum = VirtualMemoryPT[pageNum].showingPageNum;
		VirtualMemoryPT[pageNum].referanced = true;
		returnValue = physicalMemory[physicalPageNum].memory[pageIndex];
		return returnValue;
	}
	else{ // if page in disk
	//If page in disk a page replacement must be made..
		inc_PageMiss(threadType);
		inc_PageRep(threadType);
		for(int i=0; i<numPhysical; i++){
			if(SecondChange[i]->referanced == true){
				SecondChange[i]->referanced  = false;
				PageEntry* temp = SecondChange[i];
				SecondChange.erase(SecondChange.begin());
				SecondChange.push_back(temp);
				i--;
			}
			else{
				int deletedPage = SecondChange[i]->showingPageNum;
				if(SecondChange[i]->modified == true){ // This page modified rewrite file
					
					update_disk(deletedPage);
					inc_DiskWrite(threadType);
				}
				int flag;
				for(int j =0; j< numVirtual; j++){
					if(VirtualMemoryPT[j].showingPageNum == deletedPage){
						flag = j;
						break;
					}
				}
				VirtualMemoryPT[flag].showingPageNum = -1;
				VirtualMemoryPT[flag].referanced = false; //this 2 new added
				VirtualMemoryPT[flag].modified = false;

				VirtualMemoryPT[pageNum].showingPageNum = deletedPage;
				VirtualMemoryPT[pageNum].referanced = false;
				VirtualMemoryPT[pageNum].modified = false;
				int* arr = getPageFromDisk(pageNum);
				inc_DiskRead(threadType);
				int startIndex = frameSize * deletedPage;
				int k = 0;
				for(int i =0; i<frameSize; i++){
					physicalMemory[deletedPage].memory[i] = arr[i];

				}

				SecondChange.erase(SecondChange.begin());
				SecondChange.push_back(&VirtualMemoryPT[pageNum]);
				int physicalPageNum = VirtualMemoryPT[pageNum].showingPageNum;
				VirtualMemoryPT[pageNum].referanced = true;
				returnValue = physicalMemory[physicalPageNum].memory[pageIndex];
				return returnValue;
				
			}
		}
	}
}

void SecondChange_set(int pageNum,int pageIndex,int value,char* threadType){

	if(checkHit(pageNum)){ // if page in physical memory
		int physicalPageNum = VirtualMemoryPT[pageNum].showingPageNum;
		VirtualMemoryPT[pageNum].modified = true;
		virtualMemory[pageNum].memory[pageIndex] = value;
		physicalMemory[physicalPageNum].memory[pageIndex] = value;
		
	}
	else{
		inc_PageMiss(threadType);
		inc_PageRep(threadType);
		for(int i=0; i<numPhysical; i++){
			if(SecondChange[i]->referanced == true){
				SecondChange[i]->referanced  = false;
				PageEntry* temp = SecondChange[i];
				SecondChange.erase(SecondChange.begin());
				SecondChange.push_back(temp);
				i--;
			}
			else{
				int deletedPage = SecondChange[i]->showingPageNum;
				if(SecondChange[i]->modified == true){ // This page modified rewrite file
					
					update_disk(deletedPage);
					inc_DiskWrite(threadType);
				}
				int flag;
				for(int j =0; j< numVirtual; j++){
					if(VirtualMemoryPT[j].showingPageNum == deletedPage){
						flag = j;
						break;
					}
				}
				VirtualMemoryPT[flag].showingPageNum = -1;
				VirtualMemoryPT[flag].referanced = false; //this 2 new added
				VirtualMemoryPT[flag].modified = false;

				VirtualMemoryPT[pageNum].showingPageNum = deletedPage;
				VirtualMemoryPT[pageNum].referanced = false;
				VirtualMemoryPT[pageNum].modified = true;
				int* arr = getPageFromDisk(pageNum);
				inc_DiskRead(threadType);
				int startIndex = frameSize * deletedPage;
				int k = 0;
				for(int i =0; i<frameSize; i++){
					physicalMemory[deletedPage].memory[i] = arr[i];

				}

				SecondChange.erase(SecondChange.begin());
				SecondChange.push_back(&VirtualMemoryPT[pageNum]);
				int physicalPageNum = VirtualMemoryPT[pageNum].showingPageNum;
				//SET physical memory and virtual

				virtualMemory[pageNum].memory[pageIndex] = value;
				physicalMemory[physicalPageNum].memory[pageIndex] = value;
				break;
			}
		}
	}
}

int LRU_get(int pageNum,int pageIndex,char* threadType){
	int returnValue = 0;
	if(checkHit(pageNum)){ // if page in physical memory
		int physicalPageNum = VirtualMemoryPT[pageNum].showingPageNum;
		returnValue = physicalMemory[physicalPageNum].memory[pageIndex];
		//update LRU time
		clock_t end = clock();
  		double elapsed_secs = double(end) / CLOCKS_PER_SEC;
		VirtualMemoryPT[pageNum].time = elapsed_secs;
		return returnValue;
	}
	else{ // if page in disk
	//If page in disk a page replacement must be made..
		inc_PageMiss(threadType);	
		inc_PageRep(threadType);
		//Find LRU
		double minTime = std::numeric_limits<double>::max();
		int index = 0;
		for(int i=0; i<numVirtual; i++){
			if(VirtualMemoryPT[i].showingPageNum != -1){
				if(VirtualMemoryPT[i].time < minTime){
					minTime = VirtualMemoryPT[i].time;
					index = i;
				}
			}
		}

		int deletedPage =  VirtualMemoryPT[index].showingPageNum;
		if(VirtualMemoryPT[index].modified == true){ // This page modified rewrite file
			update_disk(deletedPage);
			inc_DiskWrite(threadType);
		}
		VirtualMemoryPT[pageNum].showingPageNum = deletedPage;
		clock_t end = clock();
  		double elapsed_secs = double(end) / CLOCKS_PER_SEC;
		VirtualMemoryPT[pageNum].time = elapsed_secs;
		VirtualMemoryPT[index].showingPageNum = -1;
		VirtualMemoryPT[index].referanced = false; //this 2 new added
		VirtualMemoryPT[index].modified = false;
		int* arr = getPageFromDisk(pageNum);
		inc_DiskRead(threadType);
		int k = 0;
		for(int i =0; i<frameSize; i++){
			physicalMemory[deletedPage].memory[i] = arr[i];

		}
		int physicalPageNum = VirtualMemoryPT[pageNum].showingPageNum;
		returnValue = physicalMemory[physicalPageNum].memory[pageIndex];
		return returnValue;

	}
	return returnValue;
}

void LRU_set(int pageNum,int pageIndex,int value,char* threadType){

	if(checkHit(pageNum)){ // if page in physical memory
		int physicalPageNum = VirtualMemoryPT[pageNum].showingPageNum;
		VirtualMemoryPT[pageNum].modified = true;
		virtualMemory[pageNum].memory[pageIndex] = value;
		physicalMemory[physicalPageNum].memory[pageIndex] = value;
		clock_t end = clock();
  		double elapsed_secs = double(end) / CLOCKS_PER_SEC;
		VirtualMemoryPT[pageNum].time = elapsed_secs;
		
	}
	else{
		inc_PageMiss(threadType);
		inc_PageRep(threadType);
		double minTime = std::numeric_limits<double>::max();
		int index = 0;
		for(int i=0; i<numVirtual; i++){
			if(VirtualMemoryPT[i].showingPageNum != -1){
				if(VirtualMemoryPT[i].time < minTime){
					minTime = VirtualMemoryPT[i].time;
					index = i;
				}
			}
		}

		int deletedPage =  VirtualMemoryPT[index].showingPageNum;
		if(VirtualMemoryPT[index].modified == true){ // This page modified rewrite file
			update_disk(deletedPage);
			inc_DiskWrite(threadType);
		}
		VirtualMemoryPT[pageNum].showingPageNum = deletedPage;
		clock_t end = clock();
  		double elapsed_secs = double(end) / CLOCKS_PER_SEC;
		VirtualMemoryPT[pageNum].time = elapsed_secs;
		VirtualMemoryPT[index].showingPageNum = -1;
		VirtualMemoryPT[index].referanced = false; //this 2 new added
		VirtualMemoryPT[index].modified = false;
		int* arr = getPageFromDisk(pageNum);
		inc_DiskRead(threadType);
		int k = 0;
		for(int i =0; i<frameSize; i++){
			physicalMemory[deletedPage].memory[i] = arr[i];

		}
		int physicalPageNum = VirtualMemoryPT[pageNum].showingPageNum;
				//SET physical memory and virtual

		virtualMemory[pageNum].memory[pageIndex] = value;
		physicalMemory[physicalPageNum].memory[pageIndex] = value;
		VirtualMemoryPT[pageNum].modified = true;
	}
}


int WsClock_get(int pageNum,int pageIndex,char* threadType){
	int returnValue = 0;
	if(checkHit(pageNum)){ // if page in physical memory
		int physicalPageNum = VirtualMemoryPT[pageNum].showingPageNum;
		returnValue = physicalMemory[physicalPageNum].memory[pageIndex];
		//update LRU time
		clock_t end = clock();
  		double elapsed_secs = double(end) / CLOCKS_PER_SEC;
		VirtualMemoryPT[pageNum].time = elapsed_secs;
		VirtualMemoryPT[pageNum].referanced = true;
		return returnValue;
	}
	else{ //Page fault
		inc_PageMiss(threadType);
		inc_PageRep(threadType);
		if(WSIndex>=numPhysical) WSIndex = 0;
		while(WSIndex<numPhysical){

			if(Clock[WSIndex]->referanced == true){
				Clock[WSIndex]->referanced = false;
			}
			else{//If its bigger than thresh hold time
				if(Clock[WSIndex]->time >= ThreshHold){
					ThreshHold = Clock[WSIndex]->time; //SET new Thresh hold value
					int deletedPage = Clock[WSIndex]->showingPageNum;
					if(Clock[WSIndex]->modified == true){
						update_disk(deletedPage);
						inc_DiskWrite(threadType);
					}
					Clock[WSIndex]->showingPageNum = -1;
					Clock[WSIndex]->referanced = false;
					Clock[WSIndex]->modified = false;
					Clock[WSIndex] = &VirtualMemoryPT[pageNum];
					Clock[WSIndex]->referanced = true;
					clock_t end = clock();
  					double elapsed_secs = double(end) / CLOCKS_PER_SEC;

  					int* arr = getPageFromDisk(pageNum);
  					inc_DiskRead(threadType);
					int k = 0;
					for(int i =0; i<frameSize; i++){
						physicalMemory[deletedPage].memory[i] = arr[i];

					}
					VirtualMemoryPT[pageNum].time = elapsed_secs;
					VirtualMemoryPT[pageNum].showingPageNum = deletedPage;
					int physicalPageNum = VirtualMemoryPT[pageNum].showingPageNum;
					returnValue = physicalMemory[physicalPageNum].memory[pageIndex];
					return returnValue;
					
				}
			}
			//Circler so return beginning of array again
			WSIndex++;
			if (WSIndex == numPhysical) WSIndex = 0;
		}

	}
}


void WsClock_set(int pageNum,int pageIndex,int value,char* threadType){
	if(checkHit(pageNum)){ // if page in physical memory
		int physicalPageNum = VirtualMemoryPT[pageNum].showingPageNum;
		physicalMemory[physicalPageNum].memory[pageIndex] = value;
		virtualMemory[pageNum].memory[pageIndex] = value;
		//update LRU time
		clock_t end = clock();
  		double elapsed_secs = double(end) / CLOCKS_PER_SEC;
		VirtualMemoryPT[pageNum].time = elapsed_secs;
		VirtualMemoryPT[pageNum].referanced = true;
		VirtualMemoryPT[pageNum].modified = true;
	}
	else{
		inc_PageMiss(threadType);
		inc_PageRep(threadType);
		if(WSIndex>=numPhysical) WSIndex = 0;
		while(WSIndex<numPhysical){

			if(Clock[WSIndex]->referanced == true){
				Clock[WSIndex]->referanced = false;
			}
			else{//If its bigger than thresh hold time
				if(Clock[WSIndex]->time >= ThreshHold){
					ThreshHold = Clock[WSIndex]->time; //SET new Thresh hold value
					int deletedPage = Clock[WSIndex]->showingPageNum;
					if(Clock[WSIndex]->modified == true){
						update_disk(deletedPage);
						inc_DiskWrite(threadType);
					}
					Clock[WSIndex]->showingPageNum = -1;
					Clock[WSIndex]->referanced = false;
					Clock[WSIndex]->modified = false;
					Clock[WSIndex] = &VirtualMemoryPT[pageNum];
					Clock[WSIndex]->referanced = false;
					Clock[WSIndex]->modified = true;
					clock_t end = clock();
  					double elapsed_secs = double(end) / CLOCKS_PER_SEC;

  					int* arr = getPageFromDisk(pageNum);
  					inc_DiskRead(threadType);
					int k = 0;
					for(int i =0; i<frameSize; i++){
						physicalMemory[deletedPage].memory[i] = arr[i];

					}
					VirtualMemoryPT[pageNum].time = elapsed_secs;
					VirtualMemoryPT[pageNum].showingPageNum = deletedPage;
					int physicalPageNum = VirtualMemoryPT[pageNum].showingPageNum;
					physicalMemory[physicalPageNum].memory[pageIndex] = value;
					virtualMemory[pageNum].memory[pageIndex] = value;
					break;
				}
			}
			//Circler so return beginning of array again
			WSIndex++;
			if (WSIndex == numPhysical) WSIndex = 0;
		}
	}
}

void* thread_bubble_sort(void* arg){
	int part = (int) arg;
	if(part == 0){
		int end = (frameSize * numVirtual) / 2;
		bubbleSort(0,end);
	}
	else{
		int start= (frameSize * numVirtual) / 2 ;
		bubbleSort(start,frameSize * numVirtual);
	}
	return NULL;
}

void bubbleSort(int startIndex,int endIndex){
	int k = 0;
	pthread_mutex_lock(&lock);
	for(int i=0; i<(frameSize*numVirtual)/2-1; i++){
		for(int j=startIndex; j<endIndex-i-1; j++){
			
			// x = virtual_memory[j]
			// y = virtual_memory[j+1]
			int x = get(j,bubbleS);
			int y = get(j+1,bubbleS);
			if(x > y){
				set(j,y,bubbleS);
				set(j+1,x,bubbleS);
			}
			
		}
	}	
	pthread_mutex_unlock(&lock);
}



void* thread_quick_sort(void* arg){
	int part = (int) arg;
	if(part == 0){
		int end = (frameSize * numVirtual) / 2;
		quickSort(0,end-1);
	}
	else if(part == 2){
		quickSort(0,(frameSize*numVirtual)-1);
	}
	else{
		int start= (frameSize * numVirtual) / 2 ;
		quickSort(start,(frameSize * numVirtual)-1);
	}
	return NULL;
}


void quickSort(int startIndex,int endIndex){

	if(startIndex < endIndex){
		int pivot = partition(startIndex,endIndex);

		quickSort(startIndex,pivot-1);
		quickSort(pivot+1,endIndex);
	}
}

int partition(int startIndex, int endIndex){
	
	//Take right most element pivot
	pthread_mutex_lock(&lock);
	int pivot = get(endIndex,quickS);//virtual_memory[endIndex];
  	int i = startIndex - 1;
  	int temp;
  	for (int j = startIndex; j < endIndex; j++) {
  		
  		// x = virtualMemory[j]
  		int x = get(j,quickS);
	    if (x <= pivot) { 
	      i++;
	      // y = VirtualMemory[i]
	      int y = get(i,quickS);
	      set(i,x,quickS);
	      set(j,y,quickS);
	    }

    }
    int x = get(i+1,quickS);
    int y = get(endIndex,quickS);
    set(i+1,y,quickS);
    set(endIndex,x,quickS);
    pthread_mutex_unlock(&lock);
    return (i + 1);
}

void* thread_merge(void* arg){
	mergeAfterSort();
	return NULL;
}

void mergeAfterSort(){
	
	int size = numVirtual*frameSize;
	int* arr1 = (int*)malloc(sizeof(int)*size/2);
	int* arr2 = (int*)malloc(sizeof(int)*size/2);
	pthread_mutex_lock(&lock);
	for(int i=0; i<size/2; i++){
		arr1[i] = get(i,mergeS);
	}
	int k=0;
	for(int i=size/2; i<size; i++){
		arr2[k++] = get(i,mergeS);
	}
	pthread_mutex_unlock(&lock);

	int left = 0;
	int right = 0;
	k = 0;
	pthread_mutex_lock(&lock);
	while(left < size/2 && right < size/2){
		int x = arr1[left];
		int y = arr2[right];
		if(x <= y){
			set(k++,x,mergeS);
			left++;
		}
		else{
			set(k++,y,mergeS);
			right++;
		}
	}
	pthread_mutex_unlock(&lock);
	pthread_mutex_lock(&lock);
	while(left<size/2){
		int x = arr1[left];
		set(k++,x,mergeS);
		left++;
	}
	while(right<size/2){
		int y = arr2[right];
		set(k++,y,mergeS);
		right++;
	}
	pthread_mutex_unlock(&lock);
	free(arr1);
	free(arr2);
}


void* thread_linear(void* arg){

	linear_search_arr[3] = -1;
	linear_search_arr[4] = -1;
	for(int i=0; i<5; i++){
		linear_search(linear_search_arr[i]);
	}
}

void linear_search(int value){

	for(int i=0; i<numVirtual*frameSize; i++){
		pthread_mutex_lock(&lock);
		int x = get(i,linearS);
		pthread_mutex_unlock(&lock);
		if(x == value) break;
	}
}

void* thread_binary(void* arg){
	binary_search_arr[3] = -1;
	binary_search_arr[4] = -1;
	for(int i=0; i<5; i++){
		binary_search(binary_search_arr[i],0,(frameSize*numVirtual)-1);
		
	}
}

int binary_search(int value,int left,int right){
	if(right >= left){
		int mid = left+(right-left)/2;
		pthread_mutex_lock(&lock);
		int midV = get(mid,binaryS);
		pthread_mutex_unlock(&lock);
		if(midV == value) return mid;

		if(midV > value){
			return binary_search(value,left,mid-1);
		}
		return binary_search(value,mid+1,right);
	}
	return -1;
}


void inc_DiskWrite(char* threadType){
	if(strcmp(threadType,bubbleS) == 0){
		informations[0].numberOfDiskPageWrite++;
	}
	else if(strcmp(threadType,quickS) == 0){
		informations[1].numberOfDiskPageWrite++;
	}
	else if(strcmp(threadType,mergeS) == 0){
		informations[2].numberOfDiskPageWrite++;
	}
	else if(strcmp(threadType,linearS) == 0){
		informations[3].numberOfDiskPageWrite++;
	}
	else if(strcmp(threadType,binaryS) == 0){
		informations[4].numberOfDiskPageWrite++;
	}
}

void inc_DiskRead(char* threadType){
	if(strcmp(threadType,bubbleS) == 0){
		informations[0].numberOfDiskPageRead++;
	}
	else if(strcmp(threadType,quickS) == 0){
		informations[1].numberOfDiskPageRead++;
	}
	else if(strcmp(threadType,mergeS) == 0){
		informations[2].numberOfDiskPageRead++;
	}
	else if(strcmp(threadType,linearS) == 0){
		informations[3].numberOfDiskPageRead++;
	}
	else if(strcmp(threadType,binaryS) == 0){
		informations[4].numberOfDiskPageRead++;
	}
}

void inc_PageMiss(char* threadType){
	if(strcmp(threadType,bubbleS) == 0){
		informations[0].numberOfPageMiss++;
	}
	else if(strcmp(threadType,quickS) == 0){
		informations[1].numberOfPageMiss++;
	}
	else if(strcmp(threadType,mergeS) == 0){
		informations[2].numberOfPageMiss++;
	}
	else if(strcmp(threadType,linearS) == 0){
		informations[3].numberOfPageMiss++;
	}
	else if(strcmp(threadType,binaryS) == 0){
		informations[4].numberOfPageMiss++;
	}
}

void inc_PageRep(char* threadType){
	if(strcmp(threadType,bubbleS) == 0){
		informations[0].numberOfPageRep++;
	}
	else if(strcmp(threadType,quickS) == 0){
		informations[1].numberOfPageRep++;
	}
	else if(strcmp(threadType,mergeS) == 0){
		informations[2].numberOfPageRep++;
	}
	else if(strcmp(threadType,linearS) == 0){
		informations[3].numberOfPageRep++;
	}
	else if(strcmp(threadType,binaryS) == 0){
		informations[4].numberOfPageRep++;
	}
}


void printPageTable(){
	
	for(int i=0; i<numVirtual; i++){
		cout<<"Printing Page Table:"<<endl;
		cout<<"VirtualMemoryPT["<<i<<"]:"<<endl;
		cout<<"showingPageNum: "<<VirtualMemoryPT[i].showingPageNum<<endl;
		cout<<"Referanced: "<<VirtualMemoryPT[i].referanced<<endl;
		cout<<"modified: "<<VirtualMemoryPT[i].modified<<endl;
		cout<<"Time: "<<VirtualMemoryPT[i].time<<endl;
		cout<<endl;
	}
	cout<<"Printing page table end"<<endl;
	cout<<endl;
}
