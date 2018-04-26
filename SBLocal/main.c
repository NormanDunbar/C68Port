#include <stdio.h>
#include <stdlib.h>
#include "SBLocal.h"

void test_1();
void test_2();

/* Start here. */
int main()
{
    printf("Testing, testing, 1, 2, 3!\n\n");
    test_1();
    printf("\nTesting Complete.\n\n");
}


/* Function to declare a couple of locals, but then
 * call test_2() to mess about with them. */
 void test_1() {
    SBLOCAL betty;      /* SBLocalVariableNode *betty in other words */
    SBLOCAL testString;
    SBLOCAL testArrayInteger;
    SBLOCAL testArrayInteger3;
    SBLOCAL testArrayInteger5;
    SBLOCAL testArrayFloat2;
    SBLOCAL testArrayFloat4;
    SB_INTEGER *i_ptr;
    SB_FLOAT *f_ptr;
    short x;
    short y;
    unsigned int offset;

    printf("test_1(): Entry\n");

    /* Start a new LOCal scope. */
    beginScope();

    /* Create a new LOCal Integer and assign a value to it. */
    betty = LOCAL_INTEGER("Betty"); /* newLocal(SBLOCAL_INTEGER, "Betty"); */
    SET_LOCAL_INTEGER("Betty", 616); /* setSBLocalVariable_i(betty, 616); */
    printf("test_1(): Betty set to 616\n");

    /* Create a 50 character string. */
    testString = LOCAL_STRING("test_string", 10);
    setSBLocalVariable_s(testString, "This is a test string.");
    //SET_LOCAL_STRING("test_string", "This is a test string.");
    printf("test_1(): Test_String is currently '%s'\n", GET_LOCAL_STRING("test_string"));


    /* Call test_2, which has its own LOCal scope, but will see
     * LOCal(s) declared here, and can change them too. Betty will
     * be changed within test_2(). */
    printf("test_1(): About to call test_2()...\n");
    test_2();
    printf("test_1(): Returned from test_2()\n");

    /* Prove that Betty and TestString were changed. */
    printf("test_1(): Betty is now %d.\n", GET_LOCAL_INTEGER("Betty")); /* getSBLocalVariable_i(betty)); */
    printf("test_1(): Test_String is now  '%s'\n", GET_LOCAL_STRING("test_string"));

    /* Arrays - Integer */
    printf("test_1(): Creating Integer arrays...\n");
    testArrayInteger = LOCAL_ARRAY_INTEGER("testArrayInteger", 10);
    //testArrayInteger3 = LOCAL_ARRAY_INTEGER3("testArrayInteger3", 5, 6, 5);
    //testArrayInteger5 = LOCAL_ARRAY_INTEGER5("testArrayInteger5", 5, 6, 1, 2, 8);

    /* Arrays - Float */

    printf("test_1(): Creating Float arrays...\n");
    testArrayFloat2 = LOCAL_ARRAY_FLOAT2("testArrayFloat", 2, 4);
    //testArrayFloat4 = LOCAL_ARRAY_FLOAT4("testArrayFloat3", 5, 6, 5, 6);

    /* Can we see values from the first and last elements? */
    /* All of testArrayInteger so, index [0] to index [10] ... */
    i_ptr = (SB_INTEGER *)testArrayInteger->variable.variableValue.arrayValue;
    for (x = 0; x <= 10; x++) {
        printf("test_1(): testArrayInteger[%d] = %d\n", x, i_ptr[x]);
    }

    /* All of testArrayFloat2 so, index [0][0] to index [2][4] ... */
    f_ptr = (SB_FLOAT *)testArrayFloat2->variable.variableValue.arrayValue;
    for (x = 0; x < 2; x++) {
        for (y = 0; y <= 4; y++) {
            offset = (x*5*SB_ARRAY_FLOAT_SIZE) + (y*SB_ARRAY_FLOAT_SIZE);
            printf("test_1(): testArrayFloat2[%d][%d] (aka %d) = %03.3f\n", x, y, offset, f_ptr[offset]);
        }
    }

endScope:       /* Single exit point from test_1(). */

    /* End the current LOCal scope. */
    endCurrentScope();

    printf("test_1(): Exit\n");
 }

/* Function to declare a couple of locals, but then
 * use an existing one to change the value of. */
 void test_2() {
    SBLOCAL temp;

    printf("\n\ttest_2(): Entry\n");

    /* Begin a new LOCal Scope for test_2(). */
    beginScope();

    /* Create an integer and a floating point LOCal variable. */
    LOCAL_INTEGER("Fred"); /* newLocal(SBLOCAL_INTEGER, "Fred"); */
    SET_LOCAL_INTEGER("Fred", 666); /* setSBLocalVariable_i(temp, 666); */

    temp = LOCAL_FLOAT("Wilma"); /* newLocal(SBLOCAL_FLOAT, "Wilma"); */
    SET_LOCAL_FLOAT("Wilma", 666.616); /* setSBLocalVariable(temp, 666.616); */

    /* Look for a LOCal named Betty - in ALL LOCal scopes. */
    printf("\n\tLooking for Betty ...\n");
    temp = findSBLocalVariableByName("Betty");
    if (!temp) {
        printf("\ttest_2(): Cannot find Betty.\n");
        goto endScope;      /* In case of multiple exit points. */
    }

    /* Betty has been found. Use the "direct" access methods to
     * display Betty's details. */
    printf("\ttest_2(): Found Betty at %p.\n", temp);

    printf("\ttest_2(): (Direct) Type is %d, aka %s.\n",
           temp->variable.variableType, getSBLocalVariableTypeName(temp));

    printf("\ttest_2(): (Direct) Value is %d\n", temp->variable.variableValue.integerValue);

    printf("\ttest_2(): (Indirect) Type is %d, aka %s.\n",
           LOCAL_TYPE("Betty"), LOCAL_TYPE_NAME("Betty"));

    printf("\ttest_2(): (Indirect) Value is %d\n", GET_LOCAL_INTEGER("Betty"));

    /* Now, change Betty from 616 to 12345. */
    /* The hard way, without the above pointer. */
    /* Not quite as efficient I admit. */
    SET_LOCAL_INTEGER("Betty", 12345); /* setSBLocalVariable_i(temp, 12345); */

    /* This will fail! */
    /* But how badly? */
    printf("\n\tLooking for BamBam (won't be found!)\n\n");
    SET_LOCAL_INTEGER("BamBam", 54321);

    /* Strings, they can be trouble! */
    printf("\n\tLooking for 'test_string' ...\n\n");
    printf("\tCurrent Value is '%s'\n", GET_LOCAL_STRING("test_string"));
    printf("\n\tSetting 'test_string' to 'A new short value.'\n");
    temp = findSBLocalVariableByName("test_string");
    if (!temp) {
        printf("\ttest_2(): Cannot find 'test_string'\n");
    } else {
        printf("\ttest_2(): Found 'test_string' at %p\n", temp);
        printf("\ttest_2(): Max Size for 'test_string' is %d\n", temp->variable.maxLength);
        printf("\ttest_2(): 'test_string' pointer at %p\n", temp->variable.variableValue.arrayValue);
        //SET_LOCAL_STRING("test_string", "A new short value.");
        setSBLocalVariable_s(temp, "A new short value.");
        printf("\tCurrent Value is now '%s'\n", GET_LOCAL_STRING("test_string"));
    }


  endScope:
    endCurrentScope();
    printf("\n\ttest_2(): Exit\n\n");
 }


