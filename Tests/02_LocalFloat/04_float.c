#include <stdio.h>
#include <stdlib.h>
#include "SBLocal.h"

void test_1();
void test_2();
void dumpScopeStack();
void dumpVariable(SBLOCAL variable);

int main()
{
    printf("\nTEST FILE: %s\n\n", __FILE__);
    printf("This test creates a pair of LOCal FLOATs, in different scopes, with values, and displays their details.\n\n");
    printf("EXPECTED RESULTS:\n");
    printf("The address of the each scope to be printed, followed by\n");
    printf("the details of a single float, named 'testLocal', the value\n");
    printf("of 'testLocal' should be 12345.567 and 8765.4321 in differing scopes.\n\n");
    test_1();
    printf("\nTest complete.\n\n");
}


/* Function to create a second new scope. */
void test_1() {
    SBLOCAL variable;

    /* Create a new scope and save the address. */
    beginScope();
    variable = LOCAL_FLOAT("testLocal");
    SET_LOCAL_FLOAT("testLocal", 12345.567);

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
    variable = LOCAL_FLOAT("testLocal");
    SET_LOCAL_FLOAT("testLocal", 8765.4321);

    /* Show details. */
    dumpScopeStack();
    
endScope:
    endCurrentScope();
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
    printf("Variable->Value : %f\n", getSBLocalVariable(variable));
    
    printf("\n");
 }