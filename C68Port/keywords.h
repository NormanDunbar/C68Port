#ifndef __KEYWORDS_H__
#define __KEYWORDS_H__

#include <stdio.h>
#include "c68port.h"

/* Keywords */
enum  kw {
    kwEnd, 
    kwFor, 
    kwIf, 
    kwRepeat, 
    kwSelect, 
    kwWhen, 
    kwDefine,
    kwProcedure, 
    kwFunction, 
    kwGo, 
    kwTt, 
    kwSub, 
    kwNull_1, 
    kwError, 
    kwNull_2,
    kwNull_3, 
    kwRestore, 
    kwNext, 
    kwExit, 
    kwElse, 
    kwOn, 
    kwReturn,
    kwRemainder, 
    kwData, 
    kwDim, 
    kwLocal, 
    kwLet, 
    kwThen, 
    kwStep,
    kwRemark, 
    kwMistake
};

/* Functions */
typedef ushort (*KWFUNC)(FILE *fp);

ushort keywordNull(FILE *fp);
ushort keywordRemark(FILE *fp);
ushort keywordMistake(FILE *fp);



#endif /* __KEYWORDS_H__ */