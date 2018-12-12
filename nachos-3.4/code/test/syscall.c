#include "syscall.h"

int fd1,fd2;
char buffer[20];

int main(){
	Create("file1.txt");
	Create("file2.txt");
	fd1 = Open("file1.txt");
	fd2 = Open("file2.txt");
	Write("teststring", 20, fd1);
	Read(buffer, 20 , fd1);
	Write(buffer, 20 , fd2);
	Close(fd1);
	Close(fd2);
	Halt();
}	
