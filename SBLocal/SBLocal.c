#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "SBLocal.h"

/* This file handles all the code necessary to create, destroy, find,
 * retrieve and set values for SuperBASIC LOCal variables.
 *
 * All this crud is required to emulate the way that SuperBASIC handles LOCals
 * in that they are visible and accessible to *any* called FuNction or PROCedure
 * called from the PROCedure or FuNction  that defined the LOCals.
 *
 * NOTE: because just about everything in this file is declared "static" to
 * prevent its use from outside this file, we MUST have the declarations of
 * the functions AND the functions in the same file.
 *
 * If they were declared static in a header file, any module that included the
 * header would result in lots of warnings about functions being declared static
 * but not defined. Ask me how I know!
 */


/*=================================================================== PRIVATE */

/* Push a root pointer to a linked list of local variables onto the
 * scope stack. */
static void pushSBLocalScope(SBLOCAL root);

/* Pop a root pointer to a linked list of local variables off of the
 * scope stack. Returns the address of the root node for the list
 * just popped. */
static SBLOCAL popSBLocalScope();

/* Fetch the current scope from the stack. */
static SBLOCAL peekSBLocalScope();

/* Internal helper routines to create nodes. */
static SBLOCAL createNode();

/* Internal helper routines to create arrays of any kind.
 * These are deleted within deleteNode() below. */
static void *createArray(unsigned short bytesRequired);

/* And to delete them too. */
static void deleteNode(SBLOCAL node);

static short max_sblocal_type = sizeof(sblocal_types)/sizeof(sblocal_types[0]) -1;

/* Calculate the offset into an array of 'n' dimensions. */
static unsigned getArrayOffset(SBLOCAL variable, va_list args);

/*===================================================================PRIVATE */

/* The stack used for LOCal scopes. As a new PROCedure or FuNction is
 * entered, which creates LOCal variable, a new scope needs to be created and
 * the variables added to that scope.
 *
 * The stackPointer is the *next* slot to be used in the stack. */
static unsigned short stackPointer = 0;
static SBLOCAL SBLocalStack[MAX_STACK_DEPTH];

/* Push a root pointer to a linked list of local variables onto the
 * scope stack. */
static void pushSBLocalScope(SBLOCAL root) {
    /* Prevent over-run. */
    if (stackPointer < MAX_STACK_DEPTH) {
        SBLocalStack[stackPointer++] = root;
        /* printf("pushSBLocalScope(): SBLocalStack[%d] = %p\n", stackPointer-1, root); */
    } else {
        printf("pushSBLocalScope(): Stack overflow.\n");
        exit(1);
    }
}

/* Pop a root pointer to a linked list of local variables off of the
 * scope stack. Returns the address of the root node for the list
 * just popped. Remember that stackPointer is the *next* free slot,
 * not the current TOS. */
static SBLOCAL popSBLocalScope() {
    /* Prevent under-run. */
    if (stackPointer > 0) {
        SBLOCAL temp = SBLocalStack[--stackPointer];
        SBLocalStack[stackPointer] = NULL;
        /* printf("popSBLocalScope(): SBLocalStack[%d] = %p\n", stackPointer, temp); */
        return temp;
    } else {
        printf("popSBLocalScope(): Stack underflow.\n");
        exit(1);
    }
}

/* Fetch the current scope from the stack. If there is no item on the
 * stack, return NULL. */
static SBLOCAL peekSBLocalScope() {
    if (stackPointer > 0) {
        unsigned short tempSP = stackPointer;
        return SBLocalStack[--tempSP];
    } else {
        return NULL;
    }
}


/* Fetch scope from the stack at the given level. 0 = oldest scope. */
SBLOCAL peekSBLocalScopeLevel(unsigned short level) {
    if (level > MAX_STACK_DEPTH) {
        printf("peekSBLocalScopeLevel(): Level (%d) too deep.\n", level);
    }

    /* This returns NULL if we go deeper than first scope level. */
    return SBLocalStack[level];
}


/* Return a pointer to the most recent scope for a particular
 * local variable. Scans backwards through the nested scopes
 * until we find it, or not. */
SBLOCAL findSBLocalVariableByName(char *variableName) {
    SBLOCAL thisScope;
    SBLOCAL thisRoot;
    short tempSP = stackPointer -1;    /* Current Scope pointer; */

    /* Loop through all the scopes in reverse order. */
    while (tempSP >= 0) {
        thisRoot = SBLocalStack[tempSP];
        /* Does this scope have any LOCals defined yet> */
        if (!thisRoot->next) {
            continue;
        }

        /* We must have some LOCals then. */
        thisScope = thisRoot->next;
        while (thisScope) {
            /* Walk the variable list. */
            if (strncmp(variableName, thisScope->variable.variableName, MAX_LOCAL_NAME_SIZE) == 0) {
                /* We have out variable. */
                return thisScope;
            }

            /* Point at the next variable. */
            thisScope = thisScope->next;
        }

        /* Not found in this scope? Try the previous one. */
        tempSP--;
    }

    /* We did not find it. */
    printf("findSBLocalVariable(): Variable '%s' not found.\n", variableName);
    return NULL;
}


/* Internal helper routines to create nodes. Returns the node's address
 * or NULL if we are out of memory. */
static SBLOCAL createNode() {
    SBLOCAL temp;

    temp = (SBLOCAL )malloc(sizeof(SBLocalVariableNode));
    if (temp) {
        /* Make sure we have no LOCals, yet. Used */
        /* by deleteNode() later. */
        temp->next = NULL;
        temp->variable.variableName[0] = '\0';
        temp->variable.variableType = SBLOCAL_UNDEFINED;
        temp->variable.variableValue.arrayValue = NULL;
        temp->variable.maxLength = 0;
        /* printf("createNode(): New node at %p\n", temp); */
    } else {
        printf("createNode(): Out of memory\n");
    }

    return temp;
}

/* Internal helper routine to create arrays. Returns the array's address
 * or NULL if we are out of memory. */
static void *createArray(unsigned short bytesRequired) {

    void *temp;

    /* Check if someone is being silly. */
    if (!bytesRequired) {
        printf("createArray(): Cannot allocate empty array\n");
        return NULL;
    }

    temp = malloc((size_t)bytesRequired);
    if (!temp) {
        printf("createArray(): Out of memory\n");
        return NULL;
    }

    /* printf("createArray(): Allocated %d bytes at address %p\n", bytesRequired, temp); */

    /* NULL out the entire array of bytes. */
    memset(temp, '\0', bytesRequired);

    /* Return the array base address. */
    return temp;
}


/* We can delete an entire list here. Delete any array pointers first
 * then walk along deleting each node from the tail, backwards. */
static void deleteNode(SBLOCAL node) {
    if (node) {
        /* printf("deleteNode(): Deleting node '%s' at %p\n", node->variable.variableName, node); */

        /* Delete the list of LOCals. */

        /* If we have an array, delete the array's memory */
        if (node->variable.variableType > SBLOCAL_FLOAT) {
            /* printf("deleteNode(): Deleting arayPointer at %p\n", node->variable.variableValue.arrayValue); */
            free(node->variable.variableValue.arrayValue);
        }

        /* Now we can delete the node(s) we point to. */
        if (node->next) {
            /* Let's go recursive... */
            deleteNode(node->next);
        }

        /* Before returning here to delete ourself. */
        free(node);
    }
}

/* 
 * Calculate the offset into an array on 'n' dimensions. We gat called
 * from a couple of places specifically getArrayElement() and setArrayElement()
 * functions.
 * We get passed a pointer to the callers variable args, we do not call 
 * va_start or va_end here. Ever!
 */
static unsigned getArrayOffset(SBLOCAL variable, va_list args) {
    short thisDimension = 0;
    unsigned offset = 0;
    short dimensionIndex = 0;
    short elements[SB_ARRAY_MAX_DIMENSIONS - 1] = {0};
    
    /* Some shortcuts - saves typing! */
    short x = 0;
    short y = 1;
    short z = 2;
    short a = 3;
    short b = 4;
    
    short dimY = variable->variable.arrayDimensions[y] + 1;
    short dimZ = variable->variable.arrayDimensions[z] + 1;
    short dimA = variable->variable.arrayDimensions[a] + 1;
    short dimB = variable->variable.arrayDimensions[b] + 1;

    /* Extract the various dimensions, -1 ends the list. */
    while (1) {
        /* Shorts get promoted to ints, hence the following. */
        thisDimension = va_arg(args, int);

        /* Done, if negative. */
        if (thisDimension < 0) {
            break;
        }
        
        elements[dimensionIndex++] = thisDimension;        
    }
    
    /* If we created an array[3][5] we actually created array[4][6]. 
     *
     * The element of array[x][y] requested, comes in as [x][y][-1].
     * arrayDimensions[] holds 3, 5, -1, -1, -1 and are the actual sizes requested.
     *
     * MIGHT HELP?
     *
     *
     * 1. array[dimX]
     *    1-D array index for array[x] = [x].
     *
     * 2. array[dimX][dimY]
     *    1-D array index for array[x][y] = [x*dimY +y].
     *
     * 3. array[dimX][dimY][dimZ]
     *    1-D array index for array[x][y][z] = [x*dimY*dimZ + y*dimZ + z].
     *
     * 4. array[dimX][dimY][dimZ][dimA]
     *    1-D array index for array[x][y][z][a] = [x*dimY*dimZ*dimA + y*dimZ*dimA + z*dimA + a].
     *
     * 5. array[dimX][dimY][dimZ][dimA][dimB]
     *    1-D array index for array[x][y][z][a][b] = [x*dimY*dimZ*dimA*dimB + y*dimZ*dimA*dimB + z*dimA*dumB + a*dimB +b].
     */
     
     
    /* I tried to do this procedurally, but failed! :-( */
    switch (dimensionIndex) {    
        case 1: /* array[x] */
                offset = elements[x];
                break;
                
        case 2: /* array[x][y] */
                offset = elements[x] * dimY + elements[y];
                break;
                
        case 3: /* array[x][y][z] */
                offset = elements[x] * dimY * dimZ + 
                         elements[y] * dimZ +
                         elements[z];
                /* printf("Calculation: %d * %d * %d + %d * %d + %d = %d\n", elements[x], dimY, dimZ, elements[y], dimZ, elements[z], offset); */
                break;
                
        case 4: /* array[x][y][z][a] */
                offset = elements[x] * dimY * dimZ * dimA + 
                         elements[y] * dimZ * dimA + 
                         elements[z] * dimA + 
                         elements[a];
                /* printf("Calculation:  %d * %d * %d * %d * %d + %d * %d * %d + %d * %d + %d = %d\n", elements[x], dimY, dimZ, dimA, elements[y], dimZ, dimA, elements[z], dimA, elements[a], offset); */
                break;
                
        case 5: /* array[x][y][z][a][b] */
                offset = elements[x] * dimY * dimZ * dimA * dimB +
                         elements[y] * dimZ * dimA * dimB +
                         elements[z] * dimA * dimB +
                         elements[a] * dimB +
                         elements[b];
                break;
    }

    /* printf("getArrayOffset(): returning %d\n", offset); */
    return offset;
    
}


/*====================================================================PUBLIC */

/* Retrieve the FP value for a particular Local Variable from the most
 * recent scope. */
SB_FLOAT getSBLocalVariable(SBLOCAL variable) {
    double result = 0.0;

    if (variable){
        if (variable->variable.variableType == SBLOCAL_FLOAT) {
            result = variable->variable.variableValue.floatValue;
        } else {
            printf("getSBLocalVariable(): Incompatible variable types.\n");
            printf("\tExpected: %s. Found: %s.\n", sblocal_types[SBLOCAL_FLOAT],
                   sblocal_types[variable->variable.variableType]);
        }
    } else {
        printf("getSBLocalVariable(): Unknown Local Floating Point Variable.\n");
    }
    return result;
}

/* Retrieve the INTEGER value for a particular Local Variable from the most
 * recent scope. */
SB_INTEGER getSBLocalVariable_i(SBLOCAL variable) {
    short result = 0;

    if (variable){
        if (variable->variable.variableType == SBLOCAL_INTEGER) {
            result = variable->variable.variableValue.integerValue;
        } else {
            printf("getSBLocalVariable_i(): Incompatible variable types.\n");
            printf("\tExpected: %s. Found: %s.\n", sblocal_types[SBLOCAL_INTEGER],
                   sblocal_types[variable->variable.variableType]);
        }
    } else {
        printf("getSBLocalVariable_i(): Unknown Local Integer Variable.\n");
    }
    return result;
}

/* Retrieve the STRING value for a particular Local Variable from the most
 * recent scope. */
SB_CHAR *getSBLocalVariable_s(SBLOCAL variable) {
    char *result = NULL;

    if (variable){
        if (variable->variable.variableType == SBLOCAL_STRING) {
            result = (char *)variable->variable.variableValue.arrayValue;
        } else {
            printf("getSBLocalVariable_s(): Incompatible variable types.\n");
            printf("\tExpected: %s. Found: %s.\n", sblocal_types[SBLOCAL_STRING],
                   sblocal_types[variable->variable.variableType]);
        }
    } else {
        printf("getSBLocalVariable_s(): Unknown Local String Variable.\n");
    }
    return result;
}


/* Change the FP value for a particular Local Variable in the most
 * recent scope. */
void setSBLocalVariable(SBLOCAL variable, SB_FLOAT newValue) {
     if (variable){
        if (variable->variable.variableType == SBLOCAL_FLOAT) {
            variable->variable.variableValue.floatValue = newValue;
        } else {
            printf("setSBLocalVariable(): Incompatible variable types.\n");
        }
    } else {
        printf("setSBLocalVariable(): Unknown Local Floating Point Variable.\n");
    }
}

/* Change the INTEGER value for a particular Local Variable in the most
 * recent scope. */
void setSBLocalVariable_i(SBLOCAL variable, SB_INTEGER newValue) {
    if (variable){
        if (variable->variable.variableType == SBLOCAL_INTEGER) {
            variable->variable.variableValue.integerValue = newValue;
        } else {
            printf("setSBLocalVariable_i(): Incompatible variable types.\n");
        }
    } else {
        printf("setSBLocalVariable_i(): Unknown Local Integer Variable.\n");
    }
}

/* Change the STRING value for a particular Local Variable in the most
 * recent scope. */
void setSBLocalVariable_s(SBLOCAL variable, SB_CHAR *newValue) {

    char *temp;

    if (variable){
        /* BE VERY CAREFUL!!!!!!!!!
         * Strings, in C, can be quite evil.
         * We must use strncpy() to limit the size of data copied to
         * the maximum size that the string allows. Otherwise, we could
         * blat through other data that we need. Buffer overruns anyone? */
        if (variable->variable.variableType == SBLOCAL_STRING) {

            temp = (char *)variable->variable.variableValue.arrayValue;
            /* printf("setSBLocalVariable_s(): setting array at %p max %d\n", temp, variable->variable.maxLength); */

            /* Can we fit all of the new value in? */
            if (strlen(newValue) <= variable->variable.maxLength) {
                /* Yes we can. strcpy() will terminate with a '\0' */
                strcpy(temp, newValue);
            } else {
                /* No, we must truncate the new value. strncpy() will
                 * not terminate with a '\0' */
                strncpy(temp, newValue, variable->variable.maxLength);
                temp[variable->variable.maxLength] = '\0';
            }
        } else {
            printf("setSBLocalVariable_s(): Incompatible variable types.\n");
        }
    } else {
        printf("setSBLocalVariable_s(): Unknown Local String Variable.\n");
    }
}




/* Starts a new scope and returns the root pointer. This MUST be called
 * on entry to any PROCedure or FuNction with LOCal variables. */
SBLOCAL beginScope() {
    SBLOCAL temp = createNode();

    if (temp == NULL) {
        printf("beginScope(): Out of memory\n");
        exit(1);
    }

    /* Name this root, as root. */
    strcpy(temp->variable.variableName, "**ROOT**");

    /* Stack the (new) current scope; */
    pushSBLocalScope(temp);
    return temp;
}

/* Ends the current scope and deletes the items and root node. This MUST be
 * called just before exiting from a PROCedure or FuNction with LOCal variables.
 * WARNING: PROCedures or FuNctions with multiple exit points will be nasty! */
void endCurrentScope() {
    /* Get the current scope which is about to end. */
    SBLOCAL temp = popSBLocalScope();
    /* printf("endCurrentScope(): Ending scope %p.\n", temp); */
    if (temp) {
        deleteNode(temp);
    }
}

/* Create a new local variable and add to the current scope.
 * This is called directly for new local integers or floats
 * but indirectly for strings and arrays. See SBLocal.h for details. */
SBLOCAL newLocal(short variableType, char *variableName) {
    /* Allocate a new node for this variable. */

    size_t nameSize = strlen(variableName);
    size_t copySize = nameSize;
    SBLOCAL root;
    short dimn;

    /* printf("newLocal(): Creating new LOCal %s variable: '%s'\n",
           sblocal_types[variableType], variableName); */

    /* If we can't create a node, bale out. */
    SBLOCAL temp = createNode();
    if (!temp) {
        printf("newLocal(): Out of memory.n");
        exit(1);
    }

    /* Initialise the new node. */
    temp->variable.variableType = variableType;

    if (nameSize > MAX_LOCAL_NAME_SIZE) {
        printf("newLocal(): Variable name '%s' too long. Only %d characters will be used.\n",
               variableName, MAX_LOCAL_NAME_SIZE);
        copySize = MAX_LOCAL_NAME_SIZE;
    }

    if (copySize) {
        strncpy(temp->variable.variableName, variableName, copySize);
        temp->variable.variableName[copySize] = '\0';
    } else {
        /* This is done in createNode, but safety! */
        temp->variable.variableName[0] = '\0';
    }

    switch (variableType) {
        case SBLOCAL_FLOAT:
            temp->variable.variableValue.floatValue = 0.0;
            break;

        case SBLOCAL_INTEGER:
            temp->variable.variableValue.integerValue = 0;
            break;

        case SBLOCAL_STRING:
        case SBLOCAL_FLOAT_ARRAY:
        case SBLOCAL_INTEGER_ARRAY:
        case SBLOCAL_STRING_ARRAY:
            temp->variable.variableValue.arrayValue = NULL;
            temp->variable.maxLength = 0;
            break;

        default: printf("newLocal(): Unimplemented variable type for variable '%s'.\n", variableName);
    }

    /* Initialise the array dimensions to -1. */
    for (dimn = 0; dimn < SB_ARRAY_MAX_DIMENSIONS; dimn++) {
        temp->variable.arrayDimensions[dimn] = -1;
    }

    /* Add the new node to the current scope. */
    root = peekSBLocalScope();
    temp->next = root->next;
    root->next = temp;

    return temp;
}


/* Create a new local STRING variable and add to the current scope. Calls newLocal
 * to get an allocation for the SBLOCAL struct and then allocates an additional
 * space for the string contents.
 */
SBLOCAL newLocalString(char *variableName, unsigned short stringLength) {

    /* Allocate a new SBLOCAL and then some space for a string's contents */
    /* Returns, or exits on error */
    SBLOCAL temp = newLocal(SBLOCAL_STRING, variableName);
    unsigned newSize = stringLength;

    /* Check and set the required (maximum) string length. */
    if (!newSize) {
        /* Set a default string size */
        newSize = SB_DEFAULT_STRING_LENGTH;
    }
    temp->variable.maxLength = newSize;

    /* Allocate the string plus two - die horribly if we can't.
     * Why 2? One for the terminator and one because SuperBASIC strings
     * count from index 1. */
    temp->variable.variableValue.arrayValue = createArray(newSize + 2);
    if (!temp->variable.variableValue.arrayValue) {
        printf("newLocalString(): Out of memory.n");
        exit(1);
    }

    /* printf("newLocalString(): Created '%s' at %p with array at %p.\n",
              variableName, temp, temp->variable.variableValue.arrayValue); */
    return temp;
}


/* Create a new SBLOCAL and some space for a multi-dimensional array.
 * This will be called with a variable number of parameters to determine
 * the various dimensions of the array, but given how C68 arrays work, the
 * actual space will be allocated as if it was a huge single dimension of
 * the required type.
 *
 * Also, because a SuperBASIC (non-string) array can index from 0 or 1
 * then we have to add one on to each dimension. For example,
 * LOCal A%(5) can index a%(0) through A%(5) which is 6 actual elements.
 * C68 has to replicate this. This also applies to every dimension. */
SBLOCAL newLocalArray(char *variableName, short variableType, ...) {

    /* Allocate a new SBLOCAL and then some space for a string's contents */
    /* Returns, or exits on error */
    SBLOCAL temp = newLocal(variableType, variableName);

    unsigned totalSize = 0;         /* Total bytes required. */
    unsigned extraCapacity = 1;     /* Extra elements per dimension. */
    short thisDimension = 0;        /* Size of this dimension. */
    va_list args;                   /* Variable arguments. */
    void *ptr;                      /* Pointer to initialise arrays. */
    int x;                          /* Loop for initialisation. */
    unsigned short dimn;            /* Index into the arrayDimensions table. */
    void *baseAddress;              /* Base address of allocated RAM for the array. */

    /* Get our byte multiplier for later memory allocation. */
    switch (variableType) {
        /* Integers are 2 bytes (shorts) normally. */
        case SBLOCAL_INTEGER_ARRAY:
            totalSize = SB_ARRAY_INTEGER_SIZE;
            break;

        /* Floats are 8 bytes, usually. */
        case SBLOCAL_FLOAT_ARRAY:
            totalSize = SB_ARRAY_FLOAT_SIZE;
            break;

        /* Strings need one extra plus a terminator, so two. */
        case SBLOCAL_STRING_ARRAY:
            totalSize = SB_ARRAY_CHAR_SIZE;
            extraCapacity=2;
            break;

        default:
            printf("newLocalArray(): Unrecognised array type. (%d)\n", variableType);
            printf("newLocalArray(): Array creation ignored.");
            return NULL;
    }

    /* Scan the variable arguments to calculate the total bytes required to
     * allocate the array, and any extra elements required to emulate SuperBASIC. */
    va_start(args, variableType);
    dimn = 0;

    do {
        /* Although the arguments are 'short', those get promoted to
         * 'int', so I have to go up in steps of int, not short. */
        thisDimension = va_arg(args, int);

        /* Any negative value ends the list. */
        if (thisDimension < 0) {
            break;
        }

        /* Update the arrayDimensions table with this requested dimension. */
        temp->variable.arrayDimensions[dimn++] = thisDimension;

        /* Add on the extra capacity required, per dimension. What this means is that
         * while arrayDimensions[dimn] says how much the user asked for, we give an
         * extra element or two for the variable type to allow complete emulation of
         * SuperBASIC LOCal variables. */
        thisDimension += extraCapacity;

        /* Accumulate the byte count for the array.
         * Because we started with totalSize = the size of one element
         * this code 'just works' and allocates the desired number of bytes. */
        totalSize *= thisDimension;
    } while (1);

    /* Don't forget this bit! */
    va_end(args);

    /* printf("newLocalArray(): totalSize = %d.\n", totalSize); */

    /* We now have the correct number of bytes required to allocate an array
     * of the requested type and with enough extra elements to cater for any
     * terminators etc to match the behaviour of SuperBASIC. */

    /* Set the required (maximum) array length. This is in bytes not elements. */
    temp->variable.maxLength = totalSize;

    /* Allocate some RAM for the array. */
    baseAddress = createArray(totalSize);

    if (!baseAddress) {
        printf("newLocalArray(): Out of memory.n");
        exit(1);
    }

    /* All ok, store the base address. */
    temp->variable.variableValue.arrayValue = baseAddress;

    /* We have to initialise FLOAT and INTEGER arrays to 0. */
    /* Start with a pointer to the data area. */
    ptr = baseAddress;

    /* Then work out how many elements we have. */
    switch (variableType) {
        case SBLOCAL_INTEGER_ARRAY:
            for (x = 0; x < totalSize / SB_ARRAY_INTEGER_SIZE; x++) {
                /* printf("init: x=%d, ptr=%p\n", x, ptr); */
                *((SB_INTEGER *)ptr) = 0;
                ptr += SB_ARRAY_INTEGER_SIZE;
            }
            break;

        /* Floats are 8 bytes, usually. */
        case SBLOCAL_FLOAT_ARRAY:
            for (x = 0; x < totalSize / SB_ARRAY_FLOAT_SIZE; x++) {
                /* printf("init: x=%d, ptr=%p\n", x, ptr); */
                *((SB_FLOAT *)ptr) = 0.0;
                ptr += SB_ARRAY_FLOAT_SIZE;
            }
            break;
    }

    /*
    printf("newLocalArray(): Created '%s' at %p with array at %p.\n",
              variableName, temp, temp->variable.variableValue.arrayValue);
    */

    /* Finally, after all that, send the new SBLOCAL back to the caller. */
    return temp;
}


/* Fetch an element from an Integer Array on any number of dimensions. */
SB_INTEGER getArrayElement_i(SBLOCAL variable, ...) {
    va_list args;
    unsigned offset = 0;
    SB_INTEGER *iPtr;

    /* Do we actually have an integer array? */
    if (variable->variable.variableType != SBLOCAL_INTEGER_ARRAY) {
        printf("getArrayElement_i(): Variable '%s' is not an integer array.\n",
               variable->variable.variableName);
        return 0;
    }

    va_start(args, variable);
    offset = getArrayOffset(variable, args);
    va_end(args);
    /*printf("getArrayElement_i(): Offset is %d\n", offset);*/

    /* Are we in range? Convert bytes to integers. */
    if (offset > variable->variable.maxLength / SB_ARRAY_INTEGER_SIZE) {
        printf("getArrayElement_i(): Index out of range.\n");
        return 0;
    }

    /*
     * We now have an offset representing the number of elements.
     * Fetch the array pointer from the variable, and manipulate it!
     */
    iPtr = (SB_INTEGER *)variable->variable.variableValue.arrayValue;
    
    return (iPtr[offset]);
}


/* Set an Integer Array element, on any number of dimensions. */
void setArrayElement_i(SBLOCAL variable, SB_INTEGER newValue, ...) {
    va_list args;
    unsigned offset = 1;
    short thisDimension;
    SB_INTEGER *iPtr;

    /* Do we actually have an integer array? */
    if (variable->variable.variableType != SBLOCAL_INTEGER_ARRAY) {
        printf("setArrayElement_i(): Variable '%s' is not an integer array.\n",
               variable->variable.variableName);
        return;
    }

    va_start(args, newValue);
    offset = getArrayOffset(variable, args);
    va_end(args);
    /*printf("setArrayElement_i(): Offset is %d\n", offset);*/

    /* Are we in range? Convert bytes to integers. */
    if (offset > variable->variable.maxLength / SB_ARRAY_INTEGER_SIZE) {
        printf("setArrayElement_i(): Index out of range.\n");
        return;
    }

    /*
     * We now have an offset representing the number of elements.
     * Fetch the array pointer from the variable, and manipulate it!
     */
    iPtr = (SB_INTEGER *)variable->variable.variableValue.arrayValue;
    iPtr[offset] = newValue;
}


/* Fetch an element from an Integer Array on any number of dimensions. */
SB_FLOAT getArrayElement(SBLOCAL variable, ...) {
    va_list args;
    unsigned offset = 1;
    short thisDimension;
    SB_FLOAT *fPtr;

    /* Do we actually have an integer array? */
    if (variable->variable.variableType != SBLOCAL_FLOAT_ARRAY) {
        printf("getArrayElement(): Variable '%s' is not an integer array.\n",
               variable->variable.variableName);
        return 0;
    }

    va_start(args, variable);
    offset = getArrayOffset(variable, args);
    va_end(args);
    /*printf("getArrayElement(): Offset is %d\n", offset);*/

    /* Are we in range? Convert bytes to floats.*/
    if (offset > variable->variable.maxLength / SB_ARRAY_FLOAT_SIZE) {
        printf("getArrayElement(): Index out of range.\n");
        return 0;
    }

    /*
     * We now have an offset representing the number of elements.
     * Fetch the array pointer from the variable, and manipulate it!
     */
    fPtr = (SB_FLOAT *)variable->variable.variableValue.arrayValue;
    
    return (fPtr[offset]);
}


/* Set a Floating Point Array element, on any number of dimensions. */
void setArrayElement(SBLOCAL variable, SB_FLOAT newValue, ...) {
    va_list args;
    unsigned offset = 1;
    short thisDimension;
    SB_FLOAT *fPtr;

    /* Do we actually have an FP array? */
    if (variable->variable.variableType != SBLOCAL_FLOAT_ARRAY) {
        printf("setArrayElement(): Variable '%s' is not a floating point array.\n",
               variable->variable.variableName);
        return;
    }

    va_start(args, newValue);
    offset = getArrayOffset(variable, args);
    va_end(args);
    /*printf("setArrayElement(): Offset is %d\n", offset);*/

    /* Are we in range? Convert bytes to floats. */
    if (offset > variable->variable.maxLength / SB_ARRAY_FLOAT_SIZE) {
        printf("setArrayElement(): Index out of range.\n");
        return;
    }

    /*
     * We now have an offset representing the number of elements.
     * Fetch the array pointer from the variable, and manipulate it!
     */
    fPtr = (SB_FLOAT *)variable->variable.variableValue.arrayValue;
    fPtr[offset] = newValue;
}


/* Return an integer description of a LOCal variable type */
short getSBLocalVariableType(SBLOCAL variable) {
    if (variable){
        return variable->variable.variableType;
    } else {
        printf("getSBLocalVariableType(): Unknown Local Variable.\n");
        return 0;
    }
}

/* Return a textual description of a LOCal variable type */
char *getSBLocalVariableTypeName(SBLOCAL variable) {
    if (variable) {
        if (variable->variable.variableType <= max_sblocal_type) {
            return sblocal_types[variable->variable.variableType];
        } else {
            printf("getSBLocalVariableTypeName(): Variable type, %d, bigger than maximum, %d.\n",
                   variable->variable.variableType, max_sblocal_type);
            return NULL;
        }
    } else {
        printf("getSBLocalVariableTypeName(): Unknown Local Variable.\n");
        return NULL;
    }
}

