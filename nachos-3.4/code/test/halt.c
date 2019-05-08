/* halt.c
 *	Simple program to test whether running a user program works.
 *	
 *	Just do a "syscall" that shuts down the OS.
 *
 * 	NOTE: for some reason, user programs with global data structures 
 *	sometimes haven't worked in the Nachos environment.  So be careful
 *	out there!  One option is to allocate data structures as 
 * 	automatics within a procedure, but if you do this, you have to
 *	be careful to allocate a big enough stack to hold the automatics!
 */

#include "syscall.h"

int id1;
int id2;
char* in;
char* out;

int spaceid;
void simple()
{
    spaceid = Exec("../test/fileop");
}
int
main()
{
    /*
    //File syscall test
    Create("filetest");
    in = "hello world!adfasflasfjlzxpadf";
    id1 = Open("filetest");
    Write(in, 12, id1);
    id2 = Open("filetest");
    Read(out, 5, id2);
    Read(out, 7, id2);
    Close(id1);
    Close(id2);
    Halt();
    */
    Fork(simple);
    Yield();
    Join(spaceid);
    Exit(0);
}
