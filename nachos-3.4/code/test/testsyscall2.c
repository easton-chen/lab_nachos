#include "syscall.h"

void func()
{
	Create("testfile.txt");
}

int main(){
	Fork(func);
	Exit(0);
}	
