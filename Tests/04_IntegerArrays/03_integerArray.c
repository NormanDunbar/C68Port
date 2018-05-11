#include <stdio.h>
#include <stdlib.h>
#include "SBLocal.h"

#define CALC(x,y,z) (((x)*4) + ((y)*2) + ((z)))

void test_1();
void dumpScopeStack();
void dumpVariable(SBLOCAL variable);

const int maxXDim = 3;
const int maxYDim = 5;
const int maxZDim = 2;

int main()
{
    printf("\nTEST FILE: %s\n\n", __FILE__);
    printf("This test creates a 3 Dimension LOCal INTEGER ARRAY and displays its details.\n\n");
    printf("EXPECTED RESULTS:\n");
    printf("The address of the new scope to be printed, followed by\n");
    printf("the details of a 3D integer array, named 'testArray', the values\n");
    printf("of the array[0][0][0] to array[%d][%d][%d]) will then be displayed.\n\n", maxXDim, maxYDim, maxZDim);
    test_1();
    printf("\nTest complete.\n\n");
}

/* Function to create a single new scope. */
 void test_1() {
    SBLOCAL variable;
    int x;
    int y;
    int z;

    /* Create a new scope and save the address. */
    beginScope();
    variable = LOCAL_ARRAY_INTEGER3("testArray", maxXDim, maxYDim, maxZDim);

    /* Set testArray[x] = x*2. */
    for (x = 0; x <= maxXDim; x++) {
        for (y = 0; y <= maxYDim; y++) {
            for (z = 0; z <= maxZDim; z++) {
                /* printf("testArray[%d][%d][%d] = %d\n", x, y, z, CALC(x,y,z)); */
                SET_INTEGER_ELEMENT3("testArray", x, y, z, CALC(x,y,z));
            }
        }
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
    int y;
    int z;

    printf("\nVariable address: %p\n", variable);
    printf("Variable->Next  : %p\n", variable->next);
    printf("Variable->Type  : %s\n", getSBLocalVariableTypeName(variable));
    printf("Variable->Name  : '%s'\n", variable->variable.variableName);
    printf("Variable->maxLength: %d (Size in bytes)\n", variable->variable.maxLength);

    for (x = 0; x < SB_ARRAY_MAX_DIMENSIONS; x++) {
        printf("Variable->arrayDimensions[%d]: %d\n", x, variable->variable.arrayDimensions[x]);
    }

    printf("\nMaxXDim = %d, maxYDim = %d, maxYDim = %d\n\n", maxXDim, maxYDim, maxZDim);
    for (x = 0; x <= maxXDim; x++) {
        for (y = 0; y <= maxYDim; y++) {
            for (z = 0; z <= maxZDim; z++) {
                printf("Variable->Value[%d][%d][%d] : %d (expected %d)\n", x, y, z,  GET_INTEGER_ELEMENT3("testArray", x, y, z), CALC(x,y,z));
            }
            printf("\n");
        }
        printf("\n");
    }

    printf("\n");
 }
