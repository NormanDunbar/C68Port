#include <stdio.h>
#include <stdlib.h>
#include "SBLocal.h"

void test_1();
void test_2();
void test_3();
void dumpScopeStack();
void dumpVariable(SBLOCAL variable);

int main()
{
    printf("\nTEST FILE: %s\n\n", __FILE__);
    printf("This test creates a pair of LOCal INTEGERs, in different scopes, with values, and displays their details.\n\n");
    printf("Then, a third nested scope level is entered and the variable's details\n");
    printf("are displayed as seen from that scope level - it should match the \n");
    printf("variable with the value 8765.\n");
    printf("EXPECTED RESULTS:\n");
    printf("The address of the each scope to be printed, followed by\n");
    printf("the details of a single integer, named 'testLocal', the value\n");
    printf("of 'testLocal' should be 12345 and 8765 in differing scopes.\n");
    printf("The 8765 variable's details should be visible from the third scope.\n\n");
    
    test_1();
    printf("\nTest complete.\n\n");
}


/* Function to create a first scope. */
void test_1() {
    SBLOCAL variable;

    /* Create a new scope and save the address. */
    beginScope();
    variable = LOCAL_INTEGER("testLocal");
    SET_LOCAL_INTEGER("testLocal", 12345);

    /* Nest a second scope. */
    test_2();
    
endScope:
    endCurrentScope();
}

/* Function to create a second new scope. */
void test_2() {
    SBLOCAL variable;

    /* Create a new scope and save the address. */
    beginScope();
    variable = LOCAL_INTEGER("testLocal");
    SET_LOCAL_INTEGER("testLocal", 8765);

    /* Show details. */
    dumpScopeStack();
    
    /* Go nested! */
    test_3();
    
endScope:
    endCurrentScope();
}


void test_3() {
    /* Extract the most recent LOCal integer named "testLocal". */
    SBLOCAL variable = findSBLocalVariableByName("testLocal");
    
    printf("\nLooking for variable 'testLocal'...\n");
    
    /* Dump variable details. */
    dumpVariable(variable);
    
}    


void dumpScopeStack() {
    /* List the addresses of all the known scopes. */
    unsigned short level = 0;   /* Oldest scope level. */
    SBLOCAL scope;
    SBLOCAL variable;

    while (1) {
        scope = peekSBLocalScopeLevel(level);
        printf("SBLocalStack[%d] = 0x%p\n", level, scope);
        
        /* Are we done? */
        if (!scope) {
            break;
        }
        
        /* Any variables? */
        variable = scope;
        while (1) {
            variable = variable->next;
            if(!variable) {
                break;
            }
            
            /* Dump variable details. */
            dumpVariable(variable);
        }
        
        level++;
    }
}
 

 void dumpVariable(SBLOCAL variable) {
    /* Display details of a LOCal variable. */
    printf("\nVariable address: %p\n", variable);
    printf("Variable->Next  : %p\n", variable->next);
    printf("Variable->Type  : %s\n", getSBLocalVariableTypeName(variable));
    printf("Variable->Name  : '%s'\n", variable->variable.variableName);
    printf("Variable->Value : %d\n", getSBLocalVariable_i(variable));
    
    printf("\n");
 }