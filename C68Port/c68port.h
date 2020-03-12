#ifndef __C68PORT_H__
#define __C68PORT_H__



/*===========================================================================
 * DEFINES
 *===========================================================================*/
#define MAXNAMESIZE 32              /* Max allowed name size in name table. */
#define QLFP_BINARY 0               /* Binary float, A = %01011. */
#define QLFP_HEXADECIMAL 1          /* Hexadecimal float, A = $12AB. */
#define QLFP_DECIMAL 2              /* Decimal float, A = 1234. */
#define TYPE_LINENUMBER 0x8D00      /* Line Number flag */

/* Type bytes */
#define TYPE_MULTISPACE 0x80        /* Print n spaces */
#define TYPE_KEYWORD    0x81        /* Keyword */
#define TYPE_SYMBOL     0x84        /* Symbols */
#define TYPE_OPERATOR   0x85        /* Operators */
#define TYPE_MONADIC    0x86        /* Monadics */
#define TYPE_NAME       0x88        /* Names */
#define TYPE_STRING     0x8B        /* Strings, delimited */
#define TYPE_TEXT       0x8C        /* Text, no delimiters */
#define TYPE_SEPARATOR  0x8E        /* Separators */
#define TYPE_FP_BIN_MIN 0xd0        /* Lowest Binary float */
#define TYPE_FP_BIN_MAX 0xdf        /* Highest Binary float */
#define TYPE_FP_BIN_MIN 0xd0        /* Lowest Binary float */
#define TYPE_FP_BIN_MAX 0xdf        /* Highest Binary float */
#define TYPE_FP_HEX_MIN 0xe0        /* Lowest Hexadecimal float */
#define TYPE_FP_HEX_MAX 0xef        /* Highest Hexadecimal float */
#define TYPE_FP_DEC_MIN 0xf0        /* Lowest Decimal float */
#define TYPE_FP_DEC_MAX 0xff        /* Highest Decimal float */



#ifdef QDOS

#define MAXPATH 40

#else

#define MAXPATH 256

#endif

/*===========================================================================
 * TYPEDEFS
 *===========================================================================*/
typedef unsigned short ushort;
typedef unsigned long ulong;
typedef unsigned char uchar;
typedef unsigned int uint;

typedef struct {
    ushort offset;                  /* Address in file of this entry. */
    ushort nameType;                /* Name type. */
    short  lineNumber;              /* Line number of definition. */
    ushort nameLength;              /* Length of actual name. */
    uchar name[MAXNAMESIZE];        /* Bytes of name. */
} nameTableEntry;

/*===========================================================================
 * FUNCTION PROTOTYPES
 *===========================================================================*/
ushort decodeHeader(FILE *fp, ushort *entries, ushort *lines);
ushort decodeNameTable(ushort entries, FILE *fp, ulong *offset);
ushort parseProgram(FILE *fp, ulong offset);
ushort parseProgramLine(FILE *fp);
ushort parseStatement(FILE *fp);


void doMultiSpaces(FILE *fp);
void doKeywords(FILE *fp);
void doSymbols(FILE *fp);
void doOperators(FILE *fp);
void doMonadics(FILE *fp);
void doNames(FILE *fp);
void doStrings(FILE *fp);
void doText(FILE *fp);
void doSeparators(FILE *fp);
void doFloatingPoint(FILE *fp, uchar leading);

short  getWord(FILE *fp);

#ifndef QDOS

#pragma pack(push, 1)
typedef struct QLFLOAT {
    short exponent;
    long mantissa;
} QLFLOAT_t;
#pragma pack(pop)

double qlfpToDouble(QLFLOAT_t *qlfp);
void   swapExtension(char *savFile, char *newFilename, char *newExtension);

#endif
/*===========================================================================*/


/*===========================================================================*/

#endif /* __C68PORT_H__ */
