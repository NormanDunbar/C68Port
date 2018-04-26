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
 * SuperBASIC LOCal variable. */
typedef struct SBLocalVariable {
    short variableType;         /* INTEGER, STRING, FLOAT */
    char variableName[MAX_LOCAL_NAME_SIZE +1];  /* No comment! */
    union variableValue {       /* Union of possible values for this LOCal */
        short integerValue;     /* Used if it's an integer */
        double floatValue;      /* Guess! */
        void *arrayValue;       /* Strings, or any array types */
    } variableValue;
    unsigned short maxLength;   /* For strings and arrays only */
} SBLocalVariable;

/* The following struct is how we hold details of all
 * SuperBASIC LOCal variables for a PROC or FN, in a
 * linked list. There will be one linked list per scope. */
typedef struct SBLocalVariableNode {
    struct SBLocalVariable variable;
    struct SBLocalVariableNode *next;
} SBLocalVariableNode, *SBLOCAL;        /* Need to use this in proc headers below */


/*====================================================================PUBLIC */

/* Retrieve the value for a particular Local Variable from the most
 * recent scope. */
double getSBLocalVariable(SBLOCAL variable);
short getSBLocalVariable_i(SBLOCAL variable);
char *getSBLocalVariable_s(SBLOCAL variable);

/* Change the value for a particular Local Variable in the most
 * recent scope. */
void setSBLocalVariable(SBLOCAL variable, double newValue);
void setSBLocalVariable_i(SBLOCAL variable, short newValue);
void setSBLocalVariable_s(SBLOCAL variable, char *newValue);

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


/* Return a pointer to the most recent scope for a particular
 * local variable. */
SBLOCAL findSBLocalVariableByName(char *variableName);

/* Return an integer description of a LOCal variable type */
short getSBLocalVariableType(SBLOCAL variable);

/* Return a textual description of a LOCal variable type */
char *getSBLocalVariableTypeName(SBLOCAL variable);


/* Some defines to make things a little easier, maybe! */
#define LOCAL_INTEGER(v) newLocal(SBLOCAL_INTEGER, (v))
#define LOCAL_FLOAT(v)   newLocal(SBLOCAL_FLOAT, (v))

/* The rest are pointer based and have sizes attached. */
#define LOCAL_STRING(v, s)        newLocalString((v), (s))

/* Oh, how I wish we had ANSI compliance! (Variable parameter macros!) */
#define LOCAL_ARRAY_INTEGER(v, d1) newLocalArray((v), SBLOCAL_INTEGER_ARRAY, (d1), -1);
#define LOCAL_ARRAY_INTEGER2(v, d1, d2) newLocalArray((v), SBLOCAL_INTEGER_ARRAY, (d1), (d2), -1);
#define LOCAL_ARRAY_INTEGER3(v, d1, d2, d3) newLocalArray((v), SBLOCAL_INTEGER_ARRAY, (d1), (d2), (d3), -1);
#define LOCAL_ARRAY_INTEGER4(v, d1, d2, d3, d4) newLocalArray((v), SBLOCAL_INTEGER_ARRAY, (d1), (d2), (d3), (d4), -1);
#define LOCAL_ARRAY_INTEGER5(v, d1, d2, d3, d4, d5) newLocalArray((v), SBLOCAL_INTEGER_ARRAY, (d1), (d2), (d3), (d4), (d5), -1);

#define LOCAL_ARRAY_FLOAT(v, d1) newLocalArray((v), SBLOCAL_FLOAT_ARRAY, (d1), -1);
#define LOCAL_ARRAY_FLOAT2(v, d1, d2) newLocalArray((v), SBLOCAL_FLOAT_ARRAY, (d1), (d2), -1);
#define LOCAL_ARRAY_FLOAT3(v, d1, d2, d3) newLocalArray((v), SBLOCAL_FLOAT_ARRAY, (d1), (d2), (d3), -1);
#define LOCAL_ARRAY_FLOAT4(v, d1, d2, d3, d4) newLocalArray((v), SBLOCAL_FLOAT_ARRAY, (d1), (d2), (d3), (d4), -1);
#define LOCAL_ARRAY_FLOAT5(v, d1, d2, d3, d4, d5) newLocalArray((v), SBLOCAL_FLOAT_ARRAY, (d1), (d2), (d3), (d4), (d5), -1);


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


#endif /* __SBLOCALNODE_H__ */
