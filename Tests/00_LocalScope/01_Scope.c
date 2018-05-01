#include <stdio.h>
#include <stdlib.h>
#include "SBLocal.h"

void test_1();
void test_2();
void dumpScopeStack();



int main()
{
    printf("\nTEST FILE: %s\n\n", __FILE__);
    printf("This test creates two scopes and displays their details.\n\n");
    printf("EXPECTED RESULTS:\n");
    printf("The address of the new scopes to be printed, followed by\n");
    printf("three addresses from the scope stack, the first two should match\n");
    printf("the scope addresses listed, and the last should be all zeros.\n\n");
    test_1();
    printf("\nTest complete.\n\n");
}


/* Function to create the first new scope. */
 void test_1() {
    SBLOCAL scope;

    /* Create a new scope and save the address. */
    scope = beginScope();
    printf("1st scope created at: 0x%p\n", scope);

    /* Create nested scope. */
    test_2();
    
endScope:
    endCurrentScope();
 }

 

 /* Function to create a second new scope. */
 void test_2() {
    SBLOCAL scope;

    /* Create a new scope and save the address. */
    scope = beginScope();
    printf("2nd scope created at: 0x%p\n", scope);

    /* Show details. */
    dumpScopeStack();
    
endScope:
    endCurrentScope();
 }

 
void dumpScopeStack() {
    /* List the addresses of all the known scopes. */
    unsigned short level = 0;   /* Oldest scope level. */
    SBLOCAL scope;

    while (1) {
        scope = peekSBLocalScopeLevel(level);
        printf("SBLocalStack[%d]   =   0x%p\n", level, scope);
        
        /* Are we done? */
        if (!scope) {
            break;
        }
        
        level++;
    }
}
 
