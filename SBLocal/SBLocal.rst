=================================
C68Port - LOCal Variable Handling
=================================

The Problem
===========

SuperBASIC and C (C68 in particular here) follow different scoping rules for local variables.

A scope, is the period of the program or application, where a variable is 'visible' to other functions called from 'this' one, where the variable was initially created.

SuperBASIC
----------

In SuperBASIC, all ``LOCal`` variables can be seen by any ``PROCedure`` or ``FuNction`` which has been called from the scope where the variable was created. Not only is a variable visible, but it *can also be changed* from within any called ``PROCedure`` or ``FuNction`` further down the call stack.

For example:

..  code-block:: none

    1000 proc_1
    1010 stop
    1015 :
    1020 DEFine PROCedure proc_1
    1030   LOCal fred, barney%, wilma$
    1040   fred = 1.23: barney = 666: wilma$ = 'Flintstone'
    1050   proc_2
    1060   ...
    1070   proc_3
    1080 END DEFine proc_1

In the procedure above, all the ``LOCal`` variables are visible in ``proc_2`` when it is called at line 1050, and again when ``proc_3`` is called at line 1070.

..  code-block:: none

    2000 DEFine PROCedure proc_2
    2010   LOCal betty
    2020   betty = 4.56
    2030   proc_3
    2040   ...
    2050   fred = betty: REMark Changes fred in proc_1 above.
    2060 END DEFine proc_2

In this procedure, we can see and change if necessary, any of the ``LOCal`` variables created in ``proc_1`` above, plus, we create another one for which is only visible in this procedure, and any called ``PROCedures`` or ``FuNctions``.

..  code-block:: none

    3010 DEFine PROCedure proc_3
    3020  PRINT 'Fred = ', fred
    3030  PRINT 'Barney% = ', barney%
    3040  PRINT 'Wilma$ = ', wilma$
    3050  PRINT 'Betty = ', betty
    3060 END DEFine proc_3

Finally, in this procedure, we print out the values of all 4 ``LOCal`` variables from the call stack that started back in ``proc_1`` at line 1050. However, when called from line 1070 in ``proc_1``, the value of ``betty`` will no longer be available as that particular ``LOCal`` vanished when ``proc_2`` ended.


C68
---

C programs are completely different to the above. The (rough) equivalent of the above code would be as follows:

..  code-block:: C

    /* Function prototypes */
    void proc_1();
    void proc_2();
    void proc_3();

    int main(int argc, char *argv[]) {
        proc_1();
        return (0);
    }

    void proc_1() {
        double fred = 1.23;
        short barney = 666;
        char *wilma = "Flintstone";
     
        proc_2();
        ...
        proc_3();
    }

The code above will compile, but the three local variables defined within the procedure *are not* visible to any called functions. So there is no way that a straight conversion between SuperBASIC and C68 is ever going to work when it comes to SuperBASIC's ``LOCal`` variable handling and scope.

..  code-block:: C

    void proc_2() {
        double betty = 4.56;
        proc_3();
        ...
        fred = betty;       /* WILL NOT COMPILE */
    }

The above code simply will not compile as the variable ``fred`` is undefined within this code, unless of course, there is a ``global`` variable with the same name and (nominal) type as ``fred``. If there is a ``global`` variable which is an ``int`` then the value of ``betty`` will be truncated to fit.

..  code-block:: C

    void proc_3() {
        printf("Fred = %f\n", fred);        /* WILL NOT COMPILE */
        printf("Barney = %d\n", barney);    /* WILL NOT COMPILE */
        printf("Wilma = %s\n", wilma);      /* WILL NOT COMPILE */
        printf("Betty = %f\n", betty);      /* WILL NOT COMPILE */
        ...
    }

Again, the above code cannot compile unless suitable ``global`` variables exist. C programs are not able to see locally defined variables further back up the call stack, unlike SuperBASIC.


The Solution
============

There are two files, ``SBLocal.h`` and ``SBLocal.c``, which provide a solution. These files allow you to:

*   Create a new scope for ``LOCal`` variables;
*   Define and create different ``LOCal`` variables;
*   Assign values to them;
*   Obtain current values from them;
*   Change variable values within called program functions.
*   End a scope, thus removing all previously declared ``LOCal``s at that scope level.

Declaring Scopes
----------------

The scope of a variable is the part of a program in which the variable exists. Global variables live for the entire life of the program, whereas ``LOCal`` variables only exist within the function that they are declared in.

When converting from SuperBASIC to 68, and using ``LOCal`` variables that behave in C the way that they do in SuperBASIC, you *must* declare a new scope on entry to a converted function. (In C procedures are simply functions that do not return a value.)

..  code-block:: C

    #include "SBLocal.h"
    
    ...
    
    void proc_1() {
        SBLOCAL scope = beginScope();

        ...
        
        endCurrentScope();
    }
    
Any scope thus created, must be ended to remove that scope from the current scope stack. This is what the call to ``endCurrentScope()`` does.

The value returned into ``scope`` can be ignored - it will not leak memory

In functions with multiple exit points, it is necessary to end the current scope before each exit point. (It is sometimes frowned upon to have multiple exits from a function. I don't care, use what you like!)

..  code-block:: C

    #include "SBLocal.h"
    
    ...
    
    void proc_1() {
        SBLOCAL scope = beginScope();
        ...
        if (some condition) {
            goto endScope;
        }
        ...
        if(some other condition) {
            goto endScope;
        }
        ...
        
     
      endScope:  
        endCurrentScope();
    }

The above gives each early return point a common exit via ``endScope`` whereby the current scope will be properly ended.


Declaring Local Variables
-------------------------

In a C68 conversion of a SuperBASIC program, once you have begun a new scope, you would declare new ``LOCal`` variables as follows:

..  code-block:: C

    #include "SBLocal.h"
    
    ...
    
    void proc_1() {
        SBLOCAL temp;           /* Used below */
        
        temp = beginScope();
        
        temp = SBLOCAL_FLOAT("fred");
        temp = SBLOCAL_INTEGER("barney");
        temp = SBLOCAL_STRING("wilma"); 
        ...
        
        endCurrentScope();
    }    

First of all, we declare a temporary variable, ``temp``, as an ``SBLOCAL`` type, and will use it to create a number of new ``LOCal`` variables.

Any new ``LOCal``s created here will exist in the current scope - as previously created - and also in any called functions, and functions called from called functions etc, until the current scope is ended.

Three new ``LOCal`` variables, of different types, are then created, and added to the current scope. All ``LOCal``s are held in a linked list for the scope that they are defined in. (See below for details of the linked lists etc.)

Variables can be of the following types:

..  code-block:: C

    /* What type do we have for our LOCal variables? */
    #define SBLOCAL_INTEGER 1
    #define SBLOCAL_FLOAT 2
    #define SBLOCAL_STRING 3
    #define SBLOCAL_INTEGER_ARRAY 4
    #define SBLOCAL_FLOAT_ARRAY 5
    #define SBLOCAL_STRING_ARRAY 6

In the example above, the address of the new ``LOCal`` variable, returned into ``temp``, for each call to ``SBLOCAL_XXXX`` is thrown away by subsequent calls, but fear not, those earlier ``LOCal``s are not lost forever, nor are we leaking memory.

We are not doing it here, but we *can* use the returned address, in ``temp``, to assign values etc. More below.

The above shows how easy it is to create ``LOCal`` variables in a C program, and to get them to act as if they were actually in a SuperBASIC program.

    
Setting Values
--------------

Once a variable is created, the next thing that usually happens is that a value is assigned to that variable. There are two ways in which this can be done:

*   Directly - using the pointer returned from ``SBLOCAL_XXXX``;
*   Indirectly - using the ``SET_LOCAL_XXXX`` calls;


Direct Access
~~~~~~~~~~~~~

When a call to ``SBLOCAL_XXXX`` returns a pointer, that is the pointer to the newly created node in the linked list of ``LOCal`` variables in the current scope. This pointer can be used to access or set the different attributes of the variable - its value, in other words.

There is a full description of the nodes that are created, below, so for now I will discuss setting a value by direct access.

..  code-block:: C

    ...
    /* Create a LOCal called fred, keep its address in x. */
    x = SBLOCAL_INTEGER("fred");
    
    //* Assign fred a new value via x. */
    setSBLocalVariable_i(x, 12345);
    
This is the most efficient method of setting a value in a ``LOCal``, however, it has to be done with a pointer to that variable. Normally, the pointer is only available within the C68 function where the variable was defined - but called functions can also read and set the variable, as described below.


Indirect Access
~~~~~~~~~~~~~~~

Indirect access is where we do not have the variable's pointer (address) and we have to use a piece of code like the following to set the variable's value:

..  code-block:: C

    ...

    //* Assign fred a new value via x. */
    SET_LOCAL_INTEGER("fred", 999);
    
In this method, we must specify *exactly* the same name for the variable that we used when we created it. The variable name is case sensitive.

This method is of more use when we are in a called function, one where the variable was not defined, and we need to set it's value as we would in a SuperBASIC ``PROCedure`` or ``FuNction``.

The above method searches the current scope for a variable named "fred", if one is not found, it looks in the previous scope for "fred" and so on, until it either finds the required variable, or runs out of scopes to search - an error will be returned in that case. This is how a called function is able to change the value (or read it) for a variable defined as ``LOCal`` in a function higher up the call stack.

Sometimes, a called function needs to read or write to a ``LOCal`` variable more than once. Rather than searching all the scopes every time, we can search once and save a pointer to the desired variable, as follows:

..  code-block:: C

    ...
    SBLOCAL tempFred = findSBLocalVariableByName("fred");
    
    if (!tempFred) {
        printf("Cannot find 'fred' in the scope tree.\n");
        ...
    }
    
    /* Assign a new value to LOCal variable fred.
    setSBLocalVariable_i(tempFred, 12345);
    
Now, we have our pointer and can use it in the manner described above for direct access to the variable.



Reading Values
--------------

Once again, we have methods that allow direct and indirect access to a ``LOCal`` variable.

Direct Access
~~~~~~~~~~~~~

When a call to ``SBLOCAL_XXXX`` returns a pointer, that is the pointer to the newly created node in the linked list of ``LOCal`` variables in the current scope. This pointer can be used to access or set the different attributes of the variable - its value, in other words.

..  code-block:: C

    ...
    /* Create a LOCal called fred, keep its address in x. */
    x = SBLOCAL_INTEGER("fred");
    short xFred;
    
    ...
    
    /* Read fred's current value via x. */
    xFred = getSBLocalVariable_i(x);
    
    
This is the most efficient method of obtaining a value from a ``LOCal``, however, it has to be done with a pointer to that variable. Normally, the pointer is only available within the C68 function where the variable was defined - but called functions can also read and set the variable, as described below.


Indirect Access
~~~~~~~~~~~~~~~

Indirect access is where we do not have the variable's pointer (address) and we have to use a piece of code like the following to get the variable's value:

..  code-block:: C

    ...

    //* Read fred's value. */
    short xFred = GET_LOCAL_INTEGER("fred");
    
In this method, we must specify *exactly* the same name for the variable that we used when we created it. The variable name is case sensitive.

This method is of more use when we are in a called function, one where the variable was not defined, and we need to read it's value as we would in a SuperBASIC ``PROCedure`` or ``FuNction``.

The above method searches the current scope for a variable named "fred", if one is not found, it looks in the previous scope for "fred" and so on, until it either finds the required variable, or runs out of scopes to search - an error will be returned in that case. This is how a called function is able to read the value (or assigne a new value) for a variable defined as ``LOCal`` in a function higher up the call stack.

Sometimes, a called function needs to read or write to a ``LOCal`` variable more than once. Rather than searching all the scopes every time, we can search once and save a pointer to the desired variable, as follows:

..  code-block:: C

    ...
    SBLOCAL tempFred = findSBLocalVariableByName("fred");
    
    if (!tempFred) {
        printf("Cannot find 'fred' in the scope tree.\n");
        ...
    }
    
    /* Read fred's current value via tempFred. */
    xFred = getSBLocalVariable_i(tempFred);
    
Now, we have our pointer and can use it in the manner described above for direct access to the variable.

<<<<<<<<<<<<<<<<< YOU ARE HERE >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

The Final Conversion
--------------------

So, now that we know how to get and set values for LOCal variables, our converted SuperBASIC program will look like the following in C68, and will work as per the SuperBASIC version.

The ``main()`` function is not changed, and as such, is not shown below.

..  code-block:: C

    #include "SBLocal.h"
    
    ...
    
    void proc_1() {
        SBLOCAL temp;           /* Used below */
        
        temp = SBLOCAL_FLOAT("fred");
        temp = SBLOCAL_INTEGER("barney");
        temp = SBLOCAL_STRING("wilma");
        
        SET_LOCAL_FLOAT("fred", 1.23);
        SET_LOCAL_INTEGER("barney", 666);
        SET_LOCAL_STRING("wilma", "Flintstone");
        
        proc_2();
        ...
        proc_3();
    }    

..  code-block:: C

    void proc_2() {
        SBLOCAL temp;
        temp = SBLOCAL_FLOAT("betty");
        SET_LOCAL_FLOAT("betty", 4.56);
        
        proc_3();
        ...
        SET_LOCAL_FLOAT("fred", GET_LOCAL_FLOAT("betty");
    }
  
..  code-block:: C

    void proc_3() {
        printf("Fred = %f\n", GET_LOCAL_FLOAT("fred"));
        printf("Barney = %d\n", GET_LOCAL_INTEGER("barney"));
        printf("Wilma = %s\n", GET_LOCAL_STRING("wilma"));
        printf("Betty = %f\n", GET_LOCAL_FLOAT("betty"));
        ...
    }

You should be aware that we still have the problem that when ``proc_1()`` calls ``proc_3()``, the ``LOCal`` variable ``betty`` - defined in ``proc_2()`` will no longer exist and will display a default value, 0.0 for floats, 0 for integers or NULL for strings.

