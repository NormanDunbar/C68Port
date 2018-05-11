#include <stdio.h>
#include <stdlib.h>
#include "SBLocal.h"

void test_1();
void dumpScopeStack();

const int maxXDim = 2;
const int maxYDim = 3;
const int maxZDim = 4;

const int xOR = 2+1;
const int yOR = 3+1;
const int zOR = 4+1;

const int x = 0;
const int y = 0;
const int z = 0;




int main()
{
    printf("\nTEST FILE: %s\n\n", __FILE__);
    printf("This test creates a 3 Dimension LOCal INTEGER ARRAY and displays its details.\n\n");
    printf("EXPECTED RESULTS:\n");
    printf("The address of the new scope to be printed, followed by\n");
    printf("the details of a 3D integer array, named 'testArray'.\n");
    printf("Plus, some error messages are expected as I attempt to set an\n");
    printf("out of range indexes for the array.\n");
    test_1();
    printf("\nTest complete.\n\n");
}

/* Function to create a single new scope. */
 void test_1() {
    SBLOCAL variable;

    /* Create a new scope and save the address. */
    beginScope();
    variable = LOCAL_ARRAY_INTEGER3("testArray", maxXDim, maxYDim, maxZDim);

    printf("About to go out of range ...\n");
    
    /* X out of range. */
    SET_INTEGER_ELEMENT3("testArray", xOR, y, x, 12345);
    
    /* We won't get to the others as we exit on errors that would replicate
     * what would happen in SuperBASIC. Out of Range would stop the program.
     */
    
    /* Y out of range. */
    SET_INTEGER_ELEMENT3("testArray", x, yOR, z, 12345);
    
    /* Z out of range. */
    SET_INTEGER_ELEMENT3("testArray", x, y, zOR, 12345);
    
    /* X,Y out of range. */
    SET_INTEGER_ELEMENT3("testArray", xOR, yOR, z, 12345);
    
    /* X,Z out of range. */
    SET_INTEGER_ELEMENT3("testArray", xOR, y, zOR, 12345);
    
    /* Y,Z out of range. */
    SET_INTEGER_ELEMENT3("testArray", xOR, y, zOR, 12345);
    
    /* X,Y,Z out of range. */
    SET_INTEGER_ELEMENT3("testArray", xOR, yOR, zOR, 12345);
    
    
endScope:
    endCurrentScope();
 }


