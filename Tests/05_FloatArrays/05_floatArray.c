#include <stdio.h>
#include <stdlib.h>
#include "SBLocal.h"

#define CALC(x,y,z,a,b) ((((x)*6.5) + ((y)*4.3) + ((z)*2.1) + ((a)/10.0)) * ((b)+1.15))

void test_1();
void dumpScopeStack();
void dumpVariable(SBLOCAL variable);

const int maxXDim = 2;
const int maxYDim = 3;
const int maxZDim = 4;
const int maxADim = 5;
const int maxBDim = 6;

const int maxDec = 3;

int main()
{
    printf("\nTEST FILE: %s\n\n", __FILE__);
    printf("This test creates a 5 Dimension LOCal FP ARRAY and displays its details.\n\n");
    printf("EXPECTED RESULTS:\n");
    printf("The address of the new scope to be printed, followed by\n");
    printf("the details of a 5D fp array, named 'testArray', the values\n");
    printf("of the array[0][0][0][0][0] to array[%d][%d][%d][%d][%d]) will then be displayed.\n\n", maxXDim, maxYDim, maxZDim, maxADim, maxBDim);
    test_1();
    printf("\nTest complete.\n\n");
}

/* Function to create a single new scope. */
 void test_1() {
    SBLOCAL variable;
    int x;
    int y;
    int z;
    int a;
    int b;

    /* Create a new scope and save the address. */
    beginScope();
    variable = LOCAL_ARRAY_FLOAT5("testArray", maxXDim, maxYDim, maxZDim, maxADim, maxBDim);

    for (x = 0; x <= maxXDim; x++) {
        for (y = 0; y <= maxYDim; y++) {
            for (z = 0; z <= maxZDim; z++) {
                for (a = 0; a <= maxADim; a++) {
                    for (b = 0; b <= maxBDim; b++) {
                        /* printf("testArray[%d][%d][%d][%d][%d] = %f\n", x, y, z, a,b, CALC(x,y,z,a,b)); */
                        SET_FLOAT_ELEMENT5("testArray", x, y, z, a, b, CALC(x,y,z,a,b));
                    }
                }
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
    int a;
    int b;

    printf("\nVariable address: %p\n", variable);
    printf("Variable->Next  : %p\n", variable->next);
    printf("Variable->Type  : %s\n", getSBLocalVariableTypeName(variable));
    printf("Variable->Name  : '%s'\n", variable->variable.variableName);
    printf("Variable->maxLength: %d (Size in bytes)\n", variable->variable.maxLength);

    for (x = 0; x < SB_ARRAY_MAX_DIMENSIONS; x++) {
        printf("Variable->arrayDimensions[%d]: %d\n", x, variable->variable.arrayDimensions[x]);
    }

    printf("\nMaxXDim = %d, maxYDim = %d, maxYDim = %d, maxADim = %d, maxBDim = %d\n\n", maxXDim, maxYDim, maxZDim, maxADim, maxBDim);
    for (x = 0; x <= maxXDim; x++) {
        for (y = 0; y <= maxYDim; y++) {
            for (z = 0; z <= maxZDim; z++) {
                for (a = 0; a <= maxADim; a++) {
                    for (b = 0; b <= maxBDim; b++) {
                        printf("Variable->Value[%d][%d][%d][%d][%d] : %.*f (expected %.*f)\n", x, y, z, a, b, maxDec, GET_FLOAT_ELEMENT5("testArray", x, y, z, a, b), maxDec, CALC(x,y,z,a,b));
                    }
                    printf("\n");
                }
                printf("\n");
            }
            printf("\n");
        }
        printf("\n");
    }
    printf("\n");
    

 }
