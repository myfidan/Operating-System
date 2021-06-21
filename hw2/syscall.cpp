/* SPIM S20 MIPS simulator.
   Execute SPIM syscalls, both in simulator and bare mode.
   Execute MIPS syscalls in bare mode, when running on MIPS systems.
   Copyright (c) 1990-2010, James R. Larus.
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification,
   are permitted provided that the following conditions are met:

   Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

   Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation and/or
   other materials provided with the distribution.

   Neither the name of the James R. Larus nor the names of its contributors may be
   used to endorse or promote products derived from this software without specific
   prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
   GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
   OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#ifndef _WIN32
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>

#ifdef _WIN32
#include <io.h>
#endif

#include "spim.h"
#include "string-stream.h"
#include "inst.h"
#include "reg.h"
#include "mem.h"
#include "sym-tbl.h"
#include "syscall.h"

#include <iostream>
#include <vector> // For thread table

using namespace std;

#ifdef _WIN32
/* Windows has an handler that is invoked when an invalid argument is passed to a system
   call. https://msdn.microsoft.com/en-us/library/a9yf33zb(v=vs.110).aspx

   All good, except that the handler tries to invoke Watson and then kill spim with an exception.

   Override the handler to just report an error.
*/

#include <stdio.h>
#include <stdlib.h>
#include <crtdbg.h>

void myInvalidParameterHandler(const wchar_t* expression,
   const wchar_t* function, 
   const wchar_t* file, 
   unsigned int line, 
   uintptr_t pReserved)
{
  if (function != NULL)
    {
      run_error ("Bad parameter to system call: %s\n", function);
    }
  else
    {
      run_error ("Bad parameter to system call\n");
    }
}

static _invalid_parameter_handler oldHandler;

void windowsParameterHandlingControl(int flag )
{
  static _invalid_parameter_handler oldHandler;
  static _invalid_parameter_handler newHandler = myInvalidParameterHandler;

  if (flag == 0)
    {
      oldHandler = _set_invalid_parameter_handler(newHandler);
      _CrtSetReportMode(_CRT_ASSERT, 0); // Disable the message box for assertions.
    }
  else
    {
      newHandler = _set_invalid_parameter_handler(oldHandler);
      _CrtSetReportMode(_CRT_ASSERT, 1);  // Enable the message box for assertions.
    }
}
#endif

int threadIdCount = 0;
int currentThread = 0;
int waitCount = 0;
struct Thread{
  /*
    All threads have separete Id,name,Program counter,
    Stack,State and registers. 
  */
  int ThreadID;
  char ThreadName[50];
  char ThreadState[10];
  char processName[10]; // Process name alwaays "init" because there is only 1 process
  mem_addr Thread_PC; // Program counter for Thread
  mem_addr ThreadStack; // New Stack for Thread
  //St
  mem_word *thread_stack_seg;
  short *thread_stack_seg_h;
  BYTE_TYPE *thread_stack_seg_b;
  mem_addr thread_stack_bot;
  int thread_stack_size;
  // Registers for Separete threads

  reg_word ThreadReg[R_LENGTH];
  reg_word ThreadHI, ThreadLO;
  reg_word ThreadCCR[4][32], ThreadCPR[4][32];
};

struct mutexStruct{
  int ThreadID;
  char mutexName[50];
};

//Hold mutexes here
vector<mutexStruct> mutexes;

//My Thread table as a vector
vector<Thread> ThreadTable;
//Round-robin scheduling
vector<Thread> roundRobin;
bool threadExit = false;


/*You implement your handler here*/
void SPIM_timerHandler()
{
   // Implement your handler..

  //If there is only 1 thread then it means no scheduling
  
    //Save current thread info
    if(roundRobin.size() > 1){
      cout<<endl;
      cout<<"***Interrupt Happend***"<<endl;

      ThreadTable[currentThread].Thread_PC=PC;
      //ThreadTable[currentThread].ThreadStack=R[29];
      strcpy( ThreadTable[currentThread].ThreadState,"Ready");
      ThreadTable[currentThread].ThreadHI = HI;
      ThreadTable[currentThread].ThreadLO = LO;
      ThreadTable[currentThread].thread_stack_seg =  stack_seg;
      ThreadTable[currentThread].thread_stack_seg_h = stack_seg_h;
      ThreadTable[currentThread].thread_stack_seg_b = stack_seg_b;
      ThreadTable[currentThread].thread_stack_bot = stack_bot;
      for(int i=0; i<R_LENGTH; i++){
        ThreadTable[currentThread].ThreadReg[i] = R[i];
      }
      for(int i=0; i<4; i++){
        for(int j=0; j<32; j++){
          ThreadTable[currentThread].ThreadCCR[i][j] = CCR[i][j];
          ThreadTable[currentThread].ThreadCPR[i][j] = CPR[i][j];
        }
      }
      roundRobin[0].Thread_PC = ThreadTable[currentThread].Thread_PC;
      strcpy(roundRobin[0].ThreadState,ThreadTable[currentThread].ThreadState );
      roundRobin[0].thread_stack_seg = ThreadTable[currentThread].thread_stack_seg;
      //move this thread to end of the list
      roundRobin.erase(roundRobin.begin());
      if(!threadExit){
        roundRobin.push_back(ThreadTable[currentThread]);
      }
    
      
      cout<<"Switching Thread "<<currentThread<<" to ";
    //Switch to next thread now next thread is in 0 position
    currentThread = roundRobin[0].ThreadID;
     
    
    PC = ThreadTable[currentThread].Thread_PC;
    //R[29] = ThreadTable[currentThread].ThreadStack;
    strcpy( ThreadTable[currentThread].ThreadState,"Running");
    HI = ThreadTable[currentThread].ThreadHI;
    LO = ThreadTable[currentThread].ThreadLO;
    stack_seg = ThreadTable[currentThread].thread_stack_seg;
    stack_seg_h = ThreadTable[currentThread].thread_stack_seg_h;
    stack_seg_b = ThreadTable[currentThread].thread_stack_seg_b;
    stack_bot = ThreadTable[currentThread].thread_stack_bot;
    for(int i=0; i<R_LENGTH; i++){
      R[i] = ThreadTable[currentThread].ThreadReg[i];
    }
    for(int i=0; i<4; i++){
      for(int j=0; j<32; j++){
        CCR[i][j] = ThreadTable[currentThread].ThreadCCR[i][j];
        CPR[i][j] = ThreadTable[currentThread].ThreadCPR[i][j];
      }
    }
    roundRobin[0].Thread_PC = ThreadTable[currentThread].Thread_PC;
     strcpy(roundRobin[0].ThreadState,ThreadTable[currentThread].ThreadState );
    
      roundRobin[0].thread_stack_seg = ThreadTable[currentThread].thread_stack_seg;

    cout<<currentThread<<endl;
    cout<<"Printing all infos in Thread Table"<<endl;
    for(size_t i=0; i != roundRobin.size(); i++){
        cout<<"Thread ID:\t"<<roundRobin[i].ThreadID<<endl;
        cout<<"Thread Name:\t"<<roundRobin[i].ThreadName<<endl;
        cout<<"Thread PC:\t"<<roundRobin[i].Thread_PC<<endl;
        cout<<"Thread state:\t"<<roundRobin[i].ThreadState<<endl;
        cout<<"Thread stackPointer:\t"<<roundRobin[i].thread_stack_seg<<endl;
        cout<<"Process Name:\t"<<roundRobin[i].processName<<endl;
    }
  }
  else{
    currentThread = roundRobin[0].ThreadID;
  }
  threadExit = false;
  /*
   try
   {
	throw logic_error( "NotImplementedException\n" );
   }
   catch ( exception &e )
   {
      cerr <<  endl << "Caught: " << e.what( ) << endl;

   };
   */
   
}
/* Decides which syscall to execute or simulate.  Returns zero upon
   exit syscall and non-zero to continue execution. */
int
do_syscall ()
{
#ifdef _WIN32
    windowsParameterHandlingControl(0);
#endif

  /* Syscalls for the source-language version of SPIM.  These are easier to
     use than the real syscall and are portable to non-MIPS operating
     systems. */

  switch (R[REG_V0])
    {
    case PRINT_INT_SYSCALL:
      write_output (console_out, "%d", R[REG_A0]);
      break;

    case PRINT_FLOAT_SYSCALL:
      {
	float val = FPR_S (REG_FA0);

	write_output (console_out, "%.8f", val);
	break;
      }

    case PRINT_DOUBLE_SYSCALL:
      write_output (console_out, "%.18g", FPR[REG_FA0 / 2]);
      break;

    case PRINT_STRING_SYSCALL:
      write_output (console_out, "%s", mem_reference (R[REG_A0]));
      break;

    case READ_INT_SYSCALL:
      {
	static char str [256];

	read_input (str, 256);
	R[REG_RES] = atol (str);
	break;
      }

    case READ_FLOAT_SYSCALL:
      {
	static char str [256];

	read_input (str, 256);
	FPR_S (REG_FRES) = (float) atof (str);
	break;
      }

    case READ_DOUBLE_SYSCALL:
      {
	static char str [256];

	read_input (str, 256);
	FPR [REG_FRES] = atof (str);
	break;
      }

    case READ_STRING_SYSCALL:
      {
	read_input ( (char *) mem_reference (R[REG_A0]), R[REG_A1]);
	data_modified = true;
	break;
      }

    case SBRK_SYSCALL:
      {
	mem_addr x = data_top;
	expand_data (R[REG_A0]);
	R[REG_RES] = x;
	data_modified = true;
	break;
      }

    case PRINT_CHARACTER_SYSCALL:
      write_output (console_out, "%c", R[REG_A0]);
      break;

    case READ_CHARACTER_SYSCALL:
      {
	static char str [2];

	read_input (str, 2);
	if (*str == '\0') *str = '\n';      /* makes xspim = spim */
	R[REG_RES] = (long) str[0];
	break;
      }

    case EXIT_SYSCALL:
      spim_return_value = 0;
      return (0);

    case EXIT2_SYSCALL:
      spim_return_value = R[REG_A0];	/* value passed to spim's exit() call */
      return (0);

    case OPEN_SYSCALL:
      {
#ifdef _WIN32
        R[REG_RES] = _open((char*)mem_reference (R[REG_A0]), R[REG_A1], R[REG_A2]);
#else
	R[REG_RES] = open((char*)mem_reference (R[REG_A0]), R[REG_A1], R[REG_A2]);
#endif
	break;
      }

    case READ_SYSCALL:
      {
	/* Test if address is valid */
	(void)mem_reference (R[REG_A1] + R[REG_A2] - 1);
#ifdef _WIN32
	R[REG_RES] = _read(R[REG_A0], mem_reference (R[REG_A1]), R[REG_A2]);
#else
	R[REG_RES] = read(R[REG_A0], mem_reference (R[REG_A1]), R[REG_A2]);
#endif
	data_modified = true;
	break;
      }

    case WRITE_SYSCALL:
      {
	/* Test if address is valid */
	(void)mem_reference (R[REG_A1] + R[REG_A2] - 1);
#ifdef _WIN32
	R[REG_RES] = _write(R[REG_A0], mem_reference (R[REG_A1]), R[REG_A2]);
#else
	R[REG_RES] = write(R[REG_A0], mem_reference (R[REG_A1]), R[REG_A2]);
#endif
	break;
      }

    case CLOSE_SYSCALL:
      {
#ifdef _WIN32
	R[REG_RES] = _close(R[REG_A0]);
#else
	R[REG_RES] = close(R[REG_A0]);
#endif
	break;
      }
      case THREAD_CREATE:
      {
        printf("*****Thread Create*****\n");
        //Create thread
        struct Thread newThr; 
        //Fill thread     
        newThr.ThreadID = threadIdCount++;
        char temp[50];
        snprintf(temp,50,"Thread_%d",newThr.ThreadID);
        strcpy(newThr.ThreadName,temp);
        newThr.Thread_PC = PC+4;
        strcpy(newThr.ThreadState,"Ready");
        strcpy(newThr.processName,"init");

        newThr.thread_stack_size = ROUND_UP(initial_stack_size, BYTES_PER_WORD);
        newThr.thread_stack_seg =  (mem_word *)malloc(newThr.thread_stack_size);
        memcpy(newThr.thread_stack_seg,stack_seg,newThr.thread_stack_size);
        newThr.thread_stack_seg_h = stack_seg_h;
        newThr.thread_stack_seg_b = stack_seg_b;
        newThr.thread_stack_bot = stack_bot;

        //newThr.ThreadStack = R[29];
        cout<<"Thread ID:\t"<<newThr.ThreadID<<endl;
        cout<<"Thread Name:\t"<<newThr.ThreadName<<endl;
        cout<<"Thread PC:\t"<<newThr.Thread_PC<<endl;
        cout<<"Thread state:\t"<<newThr.ThreadState<<endl;
        cout<<"Thread stackPointer:\t"<<newThr.thread_stack_seg<<endl;
        cout<<"Process Name:\t"<<newThr.processName<<endl;
        //PC += 4;
        for(int i=0; i<R_LENGTH; i++){
          newThr.ThreadReg[i] = R[i];
        }
        for(int i=0; i<4; i++){
          for(int j=0; j<32; j++){
            newThr.ThreadCCR[i][j] = CCR[i][j];
            newThr.ThreadCPR[i][j] = CPR[i][j];
          }
        }
        newThr.ThreadHI = HI;
        newThr.ThreadLO = LO;
        newThr.ThreadReg[2] = 0; // Make this thread v0 = 0
        
        ThreadTable.push_back(newThr);
        roundRobin.push_back(newThr);
        break;
      }

      case THREAD_JOIN:
       {
        if(waitCount > 0){

          R[10] -= waitCount;
          waitCount = 0;
        }
        //cout<<"ThreadJoin"<<endl;

      break;
      }
      case THREAD_EXIT:
      {
        // suan thread tabledan silmiyorum ordanda sil
        cout<<"ThreadExit"<<endl;
        int tempId = roundRobin[0].ThreadID;
        roundRobin.erase(roundRobin.begin());
        
        //Switch to next thread now next thread is in 0 position
        currentThread = roundRobin[0].ThreadID;
        
        PC = ThreadTable[currentThread].Thread_PC;
        PC -= 4;
        
        //R[29] = ThreadTable[currentThread].ThreadStack;
        strcpy( ThreadTable[currentThread].ThreadState,"Running");
        HI = ThreadTable[currentThread].ThreadHI;
        LO = ThreadTable[currentThread].ThreadLO;
        stack_seg = ThreadTable[currentThread].thread_stack_seg;
        stack_seg_h = ThreadTable[currentThread].thread_stack_seg_h;
        stack_seg_b = ThreadTable[currentThread].thread_stack_seg_b;
        stack_bot = ThreadTable[currentThread].thread_stack_bot;
        for(int i=0; i<R_LENGTH; i++){
          R[i] = ThreadTable[currentThread].ThreadReg[i];
        }
        
        for(int i=0; i<4; i++){
          for(int j=0; j<32; j++){
            CCR[i][j] = ThreadTable[currentThread].ThreadCCR[i][j];
            CPR[i][j] = ThreadTable[currentThread].ThreadCPR[i][j];
          }
        }

        roundRobin[0].Thread_PC = ThreadTable[currentThread].Thread_PC;
        strcpy(roundRobin[0].ThreadState,ThreadTable[currentThread].ThreadState );
        roundRobin[0].thread_stack_seg = ThreadTable[currentThread].thread_stack_seg;

        waitCount++;
        cout<<"Switching from thread "<<tempId<<" to "<<roundRobin[0].ThreadID<<endl;
        cout<<"Printing all infos in Thread Table"<<endl;
        for(size_t i=0; i != roundRobin.size(); i++){
          cout<<"Thread ID:\t"<<roundRobin[i].ThreadID<<endl;
          cout<<"Thread Name:\t"<<roundRobin[i].ThreadName<<endl;
          cout<<"Thread PC:\t"<<roundRobin[i].Thread_PC<<endl;
          cout<<"Thread state:\t"<<roundRobin[i].ThreadState<<endl;
          cout<<"Thread stackPointer:\t"<<roundRobin[i].thread_stack_seg<<endl;
          cout<<"Process Name:\t"<<roundRobin[i].processName<<endl;
        }


        break;
      }

        

      case INIT_THREAD:
      {
        cout<<"INITTHREAD"<<endl;
        //Create thread
        struct Thread newThr;
        //Fill thread
       
        newThr.ThreadID = threadIdCount++;
        char temp[50];
        snprintf(temp,50,"MainThread");
        strcpy(newThr.ThreadName,temp);
        newThr.Thread_PC = PC;
        strcpy(newThr.ThreadState,"Running");
        strcpy(newThr.processName,"init");
        newThr.ThreadStack = R[29];
        cout<<"Thread ID:\t"<<newThr.ThreadID<<endl;
        cout<<"Thread Name:\t"<<newThr.ThreadName<<endl;
        cout<<"Thread PC:\t"<<newThr.Thread_PC<<endl;
        cout<<"Thread state:\t"<<newThr.ThreadState<<endl;
        cout<<"Thread stackPointer:\t"<<newThr.ThreadStack<<endl;
        cout<<"Process Name:\t"<<newThr.processName<<endl;
        //PC += 4;
        for(int i=0; i<R_LENGTH; i++){
          newThr.ThreadReg[i] = R[i];
        }
        for(int i=0; i<4; i++){
          for(int j=0; j<32; j++){
            newThr.ThreadCCR[i][j] = CCR[i][j];
            newThr.ThreadCPR[i][j] = CPR[i][j];
          }
        }
        newThr.ThreadHI = HI;
        newThr.ThreadLO = LO;
        
        newThr.thread_stack_seg = stack_seg;
        newThr.thread_stack_seg_h = stack_seg_h;
        newThr.thread_stack_seg_b = stack_seg_b;
        newThr.thread_stack_bot = stack_bot;

        ThreadTable.push_back(newThr);
        roundRobin.push_back(newThr);
        
        break;
      }
      case MUTEX_LOCK:
      {
        int currentId = roundRobin[0].ThreadID;
        char temp[50];
        //strcpy(temp,R[REG_A0]);
        strcpy(temp,(char *) mem_reference (R[REG_A0]));
        int check = 0;
        for(size_t i = 0; i < mutexes.size(); i++){
          if(strcmp(temp,mutexes[i].mutexName) == 0){
            check = 1;
            break;
          }
        }

        if(check == 1){ // Someone already has mutex lock
          PC = ThreadTable[currentThread].Thread_PC;
          PC -= 4;
          
          //R[29] = ThreadTable[currentThread].ThreadStack;
          strcpy( ThreadTable[currentThread].ThreadState,"Running");
          HI = ThreadTable[currentThread].ThreadHI;
          LO = ThreadTable[currentThread].ThreadLO;
          stack_seg = ThreadTable[currentThread].thread_stack_seg;
          stack_seg_h = ThreadTable[currentThread].thread_stack_seg_h;
          stack_seg_b = ThreadTable[currentThread].thread_stack_seg_b;
          stack_bot = ThreadTable[currentThread].thread_stack_bot;
          for(int i=0; i<R_LENGTH; i++){
            R[i] = ThreadTable[currentThread].ThreadReg[i];
          }
          
          for(int i=0; i<4; i++){
            for(int j=0; j<32; j++){
              CCR[i][j] = ThreadTable[currentThread].ThreadCCR[i][j];
              CPR[i][j] = ThreadTable[currentThread].ThreadCPR[i][j];
            }
          }
          
          
        }
        else{
          struct mutexStruct newMut;
          newMut.ThreadID = roundRobin[0].ThreadID;
          strcpy(newMut.mutexName,(char *) mem_reference (R[REG_A0]));
          mutexes.push_back(newMut);
        }

        break;
      }
        
      case MUTEX_UNLOCK:
      {
        int mutID = roundRobin[0].ThreadID;
        int count = 0;
        for(size_t i = 0; i < mutexes.size(); i++){
          if(mutexes[i].ThreadID == mutID){
            break;
          }
          count++;
        }
        mutexes.erase(mutexes.begin()+count);
        
        break;
      }  
        
    default:
      run_error ("Unknown system call: %d\n", R[REG_V0]);
      break;
    }

#ifdef _WIN32
    windowsParameterHandlingControl(1);
#endif
  return (1);
}


void
handle_exception ()
{
  if (!quiet && CP0_ExCode != ExcCode_Int)
    error ("Exception occurred at PC=0x%08x\n", CP0_EPC);

  exception_occurred = 0;
  PC = EXCEPTION_ADDR;

  switch (CP0_ExCode)
    {
    case ExcCode_Int:
      break;

    case ExcCode_AdEL:
      if (!quiet)
	error ("  Unaligned address in inst/data fetch: 0x%08x\n", CP0_BadVAddr);
      break;

    case ExcCode_AdES:
      if (!quiet)
	error ("  Unaligned address in store: 0x%08x\n", CP0_BadVAddr);
      break;

    case ExcCode_IBE:
      if (!quiet)
	error ("  Bad address in text read: 0x%08x\n", CP0_BadVAddr);
      break;

    case ExcCode_DBE:
      if (!quiet)
	error ("  Bad address in data/stack read: 0x%08x\n", CP0_BadVAddr);
      break;

    case ExcCode_Sys:
      if (!quiet)
	error ("  Error in syscall\n");
      break;

    case ExcCode_Bp:
      exception_occurred = 0;
      return;

    case ExcCode_RI:
      if (!quiet)
	error ("  Reserved instruction execution\n");
      break;

    case ExcCode_CpU:
      if (!quiet)
	error ("  Coprocessor unuable\n");
      break;

    case ExcCode_Ov:
      if (!quiet)
	error ("  Arithmetic overflow\n");
      break;

    case ExcCode_Tr:
      if (!quiet)
	error ("  Trap\n");
      break;

    case ExcCode_FPE:
      if (!quiet)
	error ("  Floating point\n");
      break;

    default:
      if (!quiet)
	error ("Unknown exception: %d\n", CP0_ExCode);
      break;
    }
}
