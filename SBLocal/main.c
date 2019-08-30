#include <stdio.h>
#include <stdlib.h>
#include "SBLocal.h"

/*
 * This is a dummy "main.c" file to ensure that  the code in SBLocal.c compiles ok.
 * and can be called. The real tests are to be found in the TESTS directory, elsewhere.
 */
int main()
{
    SBLOCAL mainScope = beginScope();
    printf("Testing, testing, 1, 2, 3!\n\n");
    printf("mainScope is at %p.\n", mainScope);
    printf("\nTesting Complete.\n\n");
    endCurrentScope();
}

