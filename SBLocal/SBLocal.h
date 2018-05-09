#ifndef __SBLOCALNODE_H__
#define __SBLOCALNODE_H__

/* What type do we have for our LOCal variables? */
#define SBLOCAL_UNDEFINED 0
#define SBLOCAL_INTEGER 1
#define SBLOCAL_FLOAT 2
/* The rest are arrays, and use pointers. */
#define SBLOCAL_STRING 3
#define SBLOCAL_INTEGER_ARRAY 4
#define SBLOCAL_FLOAT_ARRAY 5
#define SBLOCAL_STRING_ARRAY 6


/* Used to translate the above into actual SuperBASIC types. */
static char *sblocal_types[] = {
    "Undefined",                /* 0 */
    "Integer",                  /* 1 */
    "Floating Point",           /* 2 */
    "String",                   /* 3 */
    "Integer Array",            /* 4 */
    "Floating Point Array",     /* 5 */
    "String Array"              /* 6 */
};


/* These are used when allocating arrays. */
#define SB_ARRAY_MAX_DIMENSIONS 5       /* LOCAL A%(n,n,n,n,n) maximum. */
#define SB_ARRAY_CHAR_SIZE sizeof(char)
#define SB_ARRAY_FLOAT_SIZE sizeof(double)
#define SB_ARRAY_INTEGER_SIZE sizeof(short)


/* How big is the Local variable stack? If horrendously recursive calls
 * are made, this should be increased. It's only 32 bits per entry after all.
 * Plus each scope's list of variables of course! */
#define MAX_STACK_DEPTH 2048

/* How long are we allowing a LOCal variable name to be? This
 * is probably more efficient if we keep this to a power of
 * two minus 1. */
#define MAX_LOCAL_NAME_SIZE 31


/* How long will a string be, if no size is specified?
 * as in LOCal A$ etc. */
#define SB_DEFAULT_STRING_LENGTH 100


/* SuperBASIC variable types are converted to C68 variable types. It's
 * best to use the following defines to be sure you have the correct
 * type especially when dealing with pointers to SuperBASIC variables.
 * Ask me how I know! */
#define SB_FLOAT double
#define SB_INTEGER short
#define SB_CHAR char

/* The following struct is how we hold details of a
 * SuperBASIC LOCal variable.
 *
 * NOTES:
 *
 * 1. The maxLength field holds the total size of the variable's value(s)
 *    which will include additional elements for arrays to replicate the
 *    behaviour in SuperBASIC.
 *    EG. DIM a%(5) allows a$(0) through a%(5). We need one extra element
 *    in C68 to replicate this.
 *
 * 2. arrayDimensions[] holds the number of elements asked for by the user
 *    and does not include the extras added on to replicate SuperBASIC's
 *    array handling behaviour.
 *
 * 3. arrayValue points at the actual string for a LOCal string, or, to a
 *    chunk of allocated memory, maxLength bytes long, for array  types.
 *    This obviously includes the extra elements required.
 */
typedef struct SBLocalVariable {
    short variableType;         /* INTEGER, STRING, FLOAT etc */
    char variableName[MAX_LOCAL_NAME_SIZE +1];  /* No comment! */
    union variableValue {       /* Union of possible values for this LOCal */
        short integerValue;     /* Used if it's an integer */
        double floatValue;      /* Guess! */
        void *arrayValue;       /* Strings, or any array types */
    } variableValue;
    unsigned short maxLength;   /* For strings and arrays only. Total size in bytes, inc extras. */
    short arrayDimensions[SB_ARRAY_MAX_DIMENSIONS - 1];  /* For local arrays as per user's request. */
} SBLocalVariable;

/* The following struct is how we hold details of all
 * SuperBASIC LOCal variables for a PROC or FN, in a
 * linked list. There will be one linked list per scope. */
typedef struct SBLocalVariableNode {
    struct SBLocalVariableNode *next;
    struct SBLocalVariable variable;
} SBLocalVariableNode, *SBLOCAL;        /* Need to use this in proc headers below */


/*====================================================================PUBLIC */

/* Retrieve the value for a particular Local Variable from the most
 * recent scope. */
SB_FLOAT getSBLocalVariable(SBLOCAL variable);
SB_INTEGER getSBLocalVariable_i(SBLOCAL variable);
SB_CHAR *getSBLocalVariable_s(SBLOCAL variable);

/* Change the value for a particular Local Variable in the most
 * recent scope. */
void setSBLocalVariable(SBLOCAL variable, SB_FLOAT newValue);
void setSBLocalVariable_i(SBLOCAL variable, SB_INTEGER newValue);
void setSBLocalVariable_s(SBLOCAL variable, SB_CHAR *newValue);

/* Starts a new scope and returns the root pointer. */
SBLOCAL beginScope();

/* Ends the current scope and deletes the items and root node. */
void endCurrentScope();

/* Create a new local variable and add to the current scope. */
SBLOCAL newLocal(short variableType, char *variableName);

/* Create a new local string of a specific length. */
SBLOCAL newLocalString(char *variableName, unsigned short stringLength);

/* Create a new local array of a specific length. */
SBLOCAL newLocalArray(char *variableName, short variableType, ...);

/* Fetch any scope's root node from the stack. */
SBLOCAL peekSBLocalScopeLevel(unsigned short level);

/* Return a pointer to the most recent scope for a particular
 * local variable. */
SBLOCAL findSBLocalVariableByName(char *variableName);

/* Return an integer description of a LOCal variable type */
short getSBLocalVariableType(SBLOCAL variable);

/* Return a textual description of a LOCal variable type */
char *getSBLocalVariableTypeName(SBLOCAL variable);

/* Return an INTEGER array element */
SB_INTEGER getArrayElement_i(SBLOCAL variable, ...);

/* Set an Integer Array element, on any number of dimensions. */
void setArrayElement_i(SBLOCAL variable, SB_INTEGER newValue, ...);

/* Return an FLOAT array element */
SB_FLOAT getArrayElement(SBLOCAL variable, ...);

/* Set an FLOAT Array element, on any number of dimensions. */
void setArrayElement(SBLOCAL variable, SB_FLOAT newValue, ...);



/* Some defines to make things a little easier, maybe! */
#define LOCAL_INTEGER(v) newLocal(SBLOCAL_INTEGER, (v))
#define LOCAL_FLOAT(v)   newLocal(SBLOCAL_FLOAT, (v))

/* The rest are pointer based and have sizes attached. */
#define LOCAL_STRING(v, s)        newLocalString((v), (s))

/* Oh, how I wish we had ANSI compliance! (Variable parameter macros!)
 *
 * The following are defined as deep as SB_ARRAY_MAX_DIMENSIONS which is defined
 * above. If you need to make that 6, for example, you MUST add an extra definition
 * for LOCAL_ARRAY_INTEGER6 and so on below.
 */
#define LOCAL_ARRAY_INTEGER(v, d1) newLocalArray((v), SBLOCAL_INTEGER_ARRAY, (d1), -1)
#define LOCAL_ARRAY_INTEGER2(v, d1, d2) newLocalArray((v), SBLOCAL_INTEGER_ARRAY, (d1), (d2), -1)
#define LOCAL_ARRAY_INTEGER3(v, d1, d2, d3) newLocalArray((v), SBLOCAL_INTEGER_ARRAY, (d1), (d2), (d3), -1)
#define LOCAL_ARRAY_INTEGER4(v, d1, d2, d3, d4) newLocalArray((v), SBLOCAL_INTEGER_ARRAY, (d1), (d2), (d3), (d4), -1)
#define LOCAL_ARRAY_INTEGER5(v, d1, d2, d3, d4, d5) newLocalArray((v), SBLOCAL_INTEGER_ARRAY, (d1), (d2), (d3), (d4), (d5), -1)

#define GET_INTEGER_ELEMENT(v, d1) getArrayElement_i(findSBLocalVariableByName((v)), (d1), -1)
#define GET_INTEGER_ELEMENT2(v, d1, d2) getArrayElement_i(findSBLocalVariableByName((v)), (d1), (d2), -1)
#define GET_INTEGER_ELEMENT3(v, d1, d2, d3) getArrayElement_i(findSBLocalVariableByName((v)), (d1), (d2), (d3), -1)
#define GET_INTEGER_ELEMENT4(v, d1, d2, d3, d4) getArrayElement_i(findSBLocalVariableByName((v)), (d1), (d2), (d3), (d4), -1)
#define GET_INTEGER_ELEMENT5(v, d1, d2, d3, d4, d5) getArrayElement_i(findSBLocalVariableByName((v)), (d1), (d2), (d3), (d4), (d5), -1)

#define SET_INTEGER_ELEMENT(v, d1, nv) setArrayElement_i(findSBLocalVariableByName((v)), (nv), (d1), -1)
#define SET_INTEGER_ELEMENT2(v, d1, d2, nv) setArrayElement_i(findSBLocalVariableByName((v)), (nv), (d1), (d2), -1)
#define SET_INTEGER_ELEMENT3(v, d1, d2, d3, nv) setArrayElement_i(findSBLocalVariableByName((v)), (nv), (d1), (d2), (d3), -1)
#define SET_INTEGER_ELEMENT4(v, d1, d2, d3, d4, nv) setArrayElement_i(findSBLocalVariableByName((v)), (nv), (d1), (d2), (d3), (d4), -1)
#define SET_INTEGER_ELEMENT5(v, d1, d2, d3, d4, d5, nv) setArrayElement_i(findSBLocalVariableByName((v)), (nv), (d1), (d2), (d3), (d4), (d5), -1)

#define LOCAL_ARRAY_FLOAT(v, d1) newLocalArray((v), SBLOCAL_FLOAT_ARRAY, (d1), -1)
#define LOCAL_ARRAY_FLOAT2(v, d1, d2) newLocalArray((v), SBLOCAL_FLOAT_ARRAY, (d1), (d2), -1)
#define LOCAL_ARRAY_FLOAT3(v, d1, d2, d3) newLocalArray((v), SBLOCAL_FLOAT_ARRAY, (d1), (d2), (d3), -1)
#define LOCAL_ARRAY_FLOAT4(v, d1, d2, d3, d4) newLocalArray((v), SBLOCAL_FLOAT_ARRAY, (d1), (d2), (d3), (d4), -1)
#define LOCAL_ARRAY_FLOAT5(v, d1, d2, d3, d4, d5) newLocalArray((v), SBLOCAL_FLOAT_ARRAY, (d1), (d2), (d3), (d4), (d5), -1)

#define GET_FLOAT_ELEMENT(v, d1) getArrayElement(findSBLocalVariableByName((v)), (d1), -1)
#define GET_FLOAT_ELEMENT2(v, d1, d2) getArrayElement(findSBLocalVariableByName((v)), (d1), (d2), -1)
#define GET_FLOAT_ELEMENT3(v, d1, d2, d3) getArrayElement(findSBLocalVariableByName((v)), (d1), (d2), (d3), -1)
#define GET_FLOAT_ELEMENT4(v, d1, d2, d3, d4) getArrayElement(findSBLocalVariableByName((v)), (d1), (d2), (d3), (d4), -1)
#define GET_FLOAT_ELEMENT5(v, d1, d2, d3, d4, d5) getArrayElement(findSBLocalVariableByName((v)), (d1), (d2), (d3), (d4), (d5), -1)

#define SET_FLOAT_ELEMENT(v, d1, nv) setArrayElement(findSBLocalVariableByName((v)), (nv), (d1), -1)
#define SET_FLOAT_ELEMENT2(v, d1, d2, nv) setArrayElement(findSBLocalVariableByName((v)), (nv), (d1), (d2), -1)
#define SET_FLOAT_ELEMENT3(v, d1, d2, d3, nv) setArrayElement(findSBLocalVariableByName((v)), (nv), (d1), (d2), (d3), -1)
#define SET_FLOAT_ELEMENT4(v, d1, d2, d3, d4, nv) setArrayElement(findSBLocalVariableByName((v)), (nv), (d1), (d2), (d3), (d4), -1)
#define SET_FLOAT_ELEMENT5(v, d1, d2, d3, d4, d5, nv) setArrayElement(findSBLocalVariableByName((v)), (nv), (d1), (d2), (d3), (d4), (d5), -1)

/* This is probably useful too, or saves typing! */
#define FIND_LOCAL(v) findSBLocalVariableByName((v))


/* Setters and getters */
#define SET_LOCAL_INTEGER(v, new) setSBLocalVariable_i(findSBLocalVariableByName((v)), (new))
#define SET_LOCAL_FLOAT(v, new)   setSBLocalVariable(findSBLocalVariableByName((v)), (new))
#define SET_LOCAL_STRING(v, new)  setSBLocalVariable_s(findSBLocalVariableByName((v)), (new))

#define GET_LOCAL_INTEGER(v) getSBLocalVariable_i(findSBLocalVariableByName((v)))
#define GET_LOCAL_FLOAT(v)   getSBLocalVariable(findSBLocalVariableByName((v)))
#define GET_LOCAL_STRING(v)  getSBLocalVariable_s(findSBLocalVariableByName((v)))

#define LOCAL_TYPE(v)       getSBLocalVariableType(findSBLocalVariableByName((v)))
#define LOCAL_TYPE_NAME(v)  getSBLocalVariableTypeName(findSBLocalVariableByName((v)))



/* DELETE ME LATER! */
unsigned getOffset(SBLOCAL variable, short x, short y);

#endif /* __SBLOCALNODE_H__ */
