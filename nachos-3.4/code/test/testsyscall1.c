#include "syscall.h"

int main(){
	int tid = Exec("../test/testsyscall");
	//Join(tid);
	Yield();
	Exit(0);
}	
