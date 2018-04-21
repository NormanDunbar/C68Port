#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

/* Internal helper routines to create arrays. Returns the array's address
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
    }

    /* printf("createArray(): Allocated %d bytes at address %p\n", bytesRequired, temp); */
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

/*====================================================================PUBLIC */

/* Retrieve the FP value for a particular Local Variable from the most
 * recent scope. */
double getSBLocalVariable(SBLOCAL variable) {
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
short getSBLocalVariable_i(SBLOCAL variable) {
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
char *getSBLocalVariable_s(SBLOCAL variable) {
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
void setSBLocalVariable(SBLOCAL variable, double newValue) {
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
void setSBLocalVariable_i(SBLOCAL variable, short newValue) {
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
void setSBLocalVariable_s(SBLOCAL variable, char *newValue) {

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

/* Create a new local variable and add to the current scope. */
SBLOCAL newLocal(short variableType, char *variableName) {
    /* Allocate a new node for this variable. */

    size_t nameSize = strlen(variableName);
    size_t copySize = nameSize;
    SBLOCAL root;

    /* printf("newLocal(): Creating new LOCal %s variable: '%s'\n",
           sblocal_types[variableType], variableName); */

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

    /* Add the new node to the current scope. */
    root = peekSBLocalScope();
    temp->next = root->next;
    root->next = temp;

    return temp;
}


SBLOCAL newLocalString(char *variableName, unsigned short stringLength) {
    /* Allocate a new SBLOCAL and some space for a string's contents */

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
    if (!temp) {
        printf("newLocalString(): Out of memory.n");
        exit(1);
    }

    /* printf("newLocalString(): Created '%s' at %p with array at %p.\n",
              variableName, temp, temp->variable.variableValue.arrayValue); */
    return temp;
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
