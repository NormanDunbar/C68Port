#include <stdio.h>
#include <stdlib.h>
#include "SBLocal.h"

void test_1();
void dumpScopeStack();
void dumpVariable(SBLOCAL variable);

int main()
{
    printf("\nTEST FILE: %s\n\n", __FILE__);
    printf("This test creates a pair of LOCal INTEGERs, with values, and displays their details.\n\n");
    printf("EXPECTED RESULTS:\n");
    printf("The address of the new scope to be printed, followed by\n");
    printf("the details of a pair of integers, named 'testLocal2/1', the value\n");
    printf("of 'testLocal1' should be 12345 and 'testLocal2' will be 8765.\n\n");
    test_1();
    printf("\nTest complete.\n\n");
}


/* Function to create a single new scope. */
 void test_1() {
    SBLOCAL variable1;
    SBLOCAL variable2;

    /* Create a new scope and save the address. */
    beginScope();
    variable1 = LOCAL_INTEGER("testLocal1");
    SET_LOCAL_INTEGER("testLocal1", 12345);

    variable2 = LOCAL_INTEGER("testLocal2");
    SET_LOCAL_INTEGER("testLocal2", 8765);
    
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
    printf("Variable->Value : %d\n", getSBLocalVariable_i(variable));
    
    printf("\n");
 }