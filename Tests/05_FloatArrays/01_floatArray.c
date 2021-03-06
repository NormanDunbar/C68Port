#include <stdio.h>
#include <stdlib.h>
#include "SBLocal.h"

void test_1();
void dumpScopeStack();
void dumpVariable(SBLOCAL variable);

const int maxDim = 10;
const int maxDecPlaces = 2;


int main()
{
    printf("\nTEST FILE: %s\n\n", __FILE__);
    printf("This test creates a LOCal FLOAT ARRAY and displays its details.\n\n");
    printf("EXPECTED RESULTS:\n");
    printf("The address of the new scope to be printed, followed by\n");
    printf("the details of a single float array, named 'testArray', the values\n");
    printf("of the array (0.00 - 20.20 step 2.02) will then be displayed.\n\n");
    test_1();
    printf("\nTest complete.\n\n");
}

/* Function to create a single new scope. */
 void test_1() {
    SBLOCAL variable;
    int x;

    /* Create a new scope and save the address. */
    beginScope();
    variable = LOCAL_ARRAY_FLOAT("testArray", maxDim);

    /* Set testArray[x] = x*2. */
    for (x = 0; x <= maxDim; x++) {
        SET_FLOAT_ELEMENT("testArray", x, x*2.02);
        /* setArrayElement_i(variable, x*2, x, -1); */
    }
    
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
    int x;

    printf("\nVariable address: %p\n", variable);
    printf("Variable->Next  : %p\n", variable->next);
    printf("Variable->Type  : %s\n", getSBLocalVariableTypeName(variable));
    printf("Variable->Name  : '%s'\n", variable->variable.variableName);
    printf("Variable->maxLength: %d (Size in bytes)\n", variable->variable.maxLength);

    for (x = 0; x < SB_ARRAY_MAX_DIMENSIONS; x++) {
        printf("Variable->arrayDimensions[%d]: %d\n", x, variable->variable.arrayDimensions[x]);
    }

    for (x = 0; x <= maxDim; x++) {
        printf("Variable->Value[%2d] : %7.*f\n", x, maxDecPlaces, GET_FLOAT_ELEMENT("testArray", x));
    }


    printf("\n");
 }
