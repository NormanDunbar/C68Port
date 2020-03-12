/*=============================================================================
 * C68 "_sav" file decoder and converter to C68 source. (Maybe!)
 * Reads the file on arvg[1], which should be a _sav file, created by QSAVE
 * and converts it back to a C source file a-la CPORT, which is old and
 * somewhat broken, and no source code exists.
 *
 * Wish me luck!
 *
 * Norman Dunbar
 * August 24 2019. (Started!)
 *===========================================================================*/

/*===========================================================================
 * HEADERS
 *===========================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "c68port.h"
#include "keywords.h"

/*===========================================================================
 * GLOBALS
 *===========================================================================*/

ushort quit = 0;
ushort lastLineSize = 0;
nameTableEntry *nameTable = NULL;
size_t ignore;

/* File handles for, and the output files. */
FILE *globals;          /* Globals_h */
FILE *header;           /* Filename_h */
FILE *source;           /* Filename_c */
FILE *listing;          /* Filename_bas */

char *globalFile = "globals_h";
char headerFile[MAXPATH + 1];
char sourceFile[MAXPATH + 1];
char listingFile[MAXPATH + 1];

ushort level = 0;           /* C68 source code indent level. */
const uchar indent = 4;     /* Tab stop size. */




/*=============================================================================
 * MAIN() Start here. Expects the input file on argv[1] and writes the output
 * to various files with messages and errors on stderr.
 *===========================================================================*/
int main (int argc, char *argv[]) {

    ushort nameTableEntries = 0;
    ushort programLines = 0;
    ulong  programOffset = 0;
    FILE *fp;

    if (argc != 2) {
        fprintf(stderr, "%s requires 1 argument, the SAV file name.\n", argv[0]);
        return -1;
    }

    /* Can we open the SAV file? */
    fp = fopen(argv[1], "rb");
    if (!fp) {
        fprintf(stderr, "FATAL ERROR: main(): Cannot open SAV file '%s'.\n", argv[1]);
        return -1;
    }

    fprintf(stderr, "SAV File..............: %s\n", argv[1]);

    /* Can we read the header? */
    if ((decodeHeader(fp, &nameTableEntries, &programLines)) != 0) {
        fprintf(stderr, "FATAL ERROR: decodeHeader() failed.\n");
        return -1;
    }

    /* Allocate the name table. */
    nameTable = malloc(nameTableEntries * sizeof(nameTableEntry));
    if (!nameTable) {
        fprintf(stderr, "FATAL ERROR: Cannot  allocate memory for name table. (%d entries).\n", nameTableEntries);
        return -1;
    }

    /* Fill in the name table. */
    if (decodeNameTable(nameTableEntries, fp, &programOffset) != 0) {
        fprintf(stderr, "FATAL ERROR: decodeNameTable() failed.\n");
        return -1;
    }

    /* Create the output file names. */
    printf("\n\nInput SAV (source) file..: '%s'\n", argv[1]);
    swapExtension(argv[1], headerFile, "h");
    printf("Converted header file....: '%s'\n", headerFile);

    swapExtension(argv[1], sourceFile, "c");
    printf("Converted source file....: '%s'\n", sourceFile);

    swapExtension(argv[1], listingFile, "bas");
    printf("Conversion listing.......: '%s'\n", listingFile);



    /* Convert the program:
     * This is effectively a SuperBASIC parser, in that it (should) know what to
     * expect in each source line as we have already got SuperBASIC to tokenise
     * it for us. Nice. Also, any errors in the source will have been ironed out
     * that's syntax errors, not coding problems!
     * So, it _should_ be relatively easy - famous last words - to convert to 
     * C68 source. What could possibly go wrong?
     */
    if (parseProgram(fp, programOffset) != 0) {
        fprintf(stderr, "FATAL ERROR: parseProgram() failed.\n");
        return -1;
    }

    /* Decode the program. */
    // if (decodeProgram(programLines, fp, argv[1]) != 0) {
        // fprintf(stderr, "FATAL ERROR: decodeProgram() failed.\n");
        // return -1;
    // }

    /* All done, exit with no errors. */
    if (nameTable) {
        free(nameTable);
    }

    fclose(fp);

    return 0;
}

/*=============================================================================
 * DECODEHEADER() Decodes the header of the _save file and makes sure it's
 * valid, otherwise we abort. Returns the number of entries in the name table,
 * and the number of lines in the program.
 *===========================================================================*/
ushort decodeHeader(FILE *fp, ushort *entries, ushort *lines) {
    uchar  head[4];
    ushort valid;
    ushort nameTableLength = 0;
    ushort programLines = 0;
    ushort nameTableEntries = 0;

    fprintf(stderr, "decodeHeader()\n");
    quit = 0;

    /* Can we read the 4 byte header? */
    if ((ignore = fread(head, 1, 4, fp)) != 4) {
        fprintf(stderr, "\n\nERROR: decodeHeader(): Cannot read SAV file header.\n");
        return 1;
    }

    /* Is the header valid? */
    /* Valid values are:
     * 'Q', '1', 0, 0
     * 'Q', '1', 2, 192
     * 'Q', '1', 3, 128
     * 'Q', '1', 0, 128
     */
    valid = 0;
    if (head[0] == 'Q' && head[1] == '1') {
        if (head[2] == 0 && head[3] == 0)
            valid = 1;
        
        else if (head[2] == 0 && head[3] == 128)
            valid = 1;
        
        else if (head[2] == 2 && head[3] == 192)
            valid = 1;
        
        else if (head[2] == 3 && head[3] == 128)
            valid = 1;
    }

    if (!valid) {
        fprintf(stderr, "\n\nERROR: decodeHeader(): Invalid SAV file header. [\"%c%c\",%d,%d]\n", head[0], head[1], head[2], head[3]);
        return 1;
    }

    /* Looks like a valid SAV file, read the header details. */
    nameTableEntries = getWord(fp);
    nameTableLength = getWord(fp);
    programLines = getWord(fp);

    fprintf(stderr, "\nHEADER DETAILS\n==============\n");

    fprintf(stderr, "\nName Table Entries....: %5d", nameTableEntries);
    fprintf(stderr, "\nName Table length.....: %5d", nameTableLength);
    fprintf(stderr, "\nProgram Lines.........: %5d\n", programLines);
    fflush(stderr);
    
    *entries = nameTableEntries;
    *lines = programLines;

    return 0;
}


/*=============================================================================
 * DECODENAMETABLE() Builds the internal name table by reading the SAV file's
 * name table. 
 *===========================================================================*/
ushort decodeNameTable(ushort entries, FILE *fp, ulong *offset) {
    ushort x;
    uchar ch;
    ushort size = 0;
    ushort procCount = 0;
    ushort fnCount[3] = {0,0,0};        /* FN$, FN, FN% counters */

    fprintf(stderr, "decodeNameTable()\n");

    for (x = 0; x < entries; x++) {
        nameTable[x].offset = ftell(fp);
        nameTable[x].nameType = getWord(fp);
        nameTable[x].lineNumber = getWord(fp);        
        nameTable[x].nameLength = getWord(fp);
        
        /* Count up the details. */
        switch (nameTable[x].nameType) {
            case 0x1402: procCount++; break;
            case 0x1501:
            case 0x1502:
            case 0x1503: fnCount[nameTable[x].nameType - 0x1501]++; break;
        }

        /* Do we need a recompile? */
        if (nameTable[x].nameLength >= MAXNAMESIZE) {
            size = MAXNAMESIZE - 1;
            ignore = fread(nameTable[x].name, 1, size, fp);
            fprintf(stderr, "\n\nWARNING: Cannot read a name with >= %d characters. Please amend the SAV file, or, recompile C68Port.\n", MAXNAMESIZE);
            fprintf(stderr, "The offending name is '%*.*s'\n.", MAXNAMESIZE, MAXNAMESIZE, nameTable[x].name);
            return 1;
        }
    
        ignore = fread(nameTable[x].name, 1, nameTable[x].nameLength, fp);

        /* Odd length names are padded. */
        if (nameTable[x].nameLength & 1)
            ch = fgetc(fp);
    }

    fprintf(stderr, "\nNAME TABLE\n==========\n");

    fprintf(stderr, "\nNumber of Procedures..: %4d", procCount);
    fprintf(stderr, "\nNumber of Function$...: %4d", fnCount[0]);
    fprintf(stderr, "\nNumber of Function....: %4d", fnCount[1]);
    fprintf(stderr, "\nNumber of Function%%...: %4d\n", fnCount[2]);

    for (x = 0; x < entries; x++) {
        fprintf(stderr, "\n%4.4X: ", nameTable[x].offset);
        fprintf(stderr, "NameTable[%4d]: ", x);
        fprintf(stderr, "Name Type: %4d ($%4.4X), ", nameTable[x].nameType, nameTable[x].nameType);
        fprintf(stderr, "Line Number: %5d, ", nameTable[x].lineNumber);
        fprintf(stderr, "Name: Size = %3d, ", nameTable[x].nameLength);
        fprintf(stderr, "%*.*s", nameTable[x].nameLength, nameTable[x].nameLength, nameTable[x].name);
    }

    fflush(stderr);

    /* Return the program offset */
    *offset = ftell(fp);

    return 0;
}


/*=============================================================================
 * PARSEPROGRAM() is the top level program parser. It will open the files and
 * call out repeatedly to parseProgramLine() to do the needful.
 *===========================================================================*/
ushort parseProgram(FILE *fp, ulong offset) {
    fprintf(stderr, "parseProgram()\n");

    /* Open output files */
    header = fopen(headerFile, "w");
    source = fopen(sourceFile, "w");
    globals = fopen(globalFile, "w");
    listing = fopen(listingFile, "w");

    /* Position at correct location */
    fseek(fp, offset, SEEK_SET);
    if (ftell(fp) != offset) {
        fprintf(stderr, "\n\nERROR: parseProgram() cannot seek to start of program lines at position %ld\n.", offset);
        return 1;
    }

    /* Parse Program Lines */
    while (1) {
        if (feof(fp)) 
            break;

        if (parseProgramLine(fp) != 0) {
            fprintf(stderr, "\n\nERROR: parseProgramLine() failed.\n");
            return 1;
        }
    }


    /* Close Output Files */
    fclose(header);
    fclose(source);
    fclose(globals);
    fclose(listing);
}


/*=============================================================================
 * PARSEPROGRAMLINE() is the low level program parser. It will parse and 
 * convert one line of the source program at a time. A line is made up of:
 * 
 * Word: Line length change from previous line.
 * Word: 0x8D00 (TYPE_LINENUMBER) Indicates a line number to follow.
 * Word: The line number.
 * Bytes: The content of the line. (LineSize in size, obviously!)
 * Byte: 0x84 (TYPE_SYMBOL) Indicates a linefeed to follow.
 * Byte: 0x0A The end of the line.
 * 
 * Lines may, of course, have multiple statements, but the last one is always
 * terminated by a 0x840a word. (Well, pair of bytes!)
 * 
 * BEWARE, lines can only start with:
 * 
 * Keywords - but not $8107 (PROCedure), $8108 (FuNction) or $810D (ERRor),
 * Names - but only type $0800 (Procedures) or $1402 (SuperBASIC procedure),
 * Separators - but only a colon (Symbol $8402),
 * Multispaces - $80nn.
 *===========================================================================*/
ushort parseProgramLine(FILE *fp) {
    static ushort lineSize = 0;         /* Current line size */
    ushort flag;                        /* Line number coming indicator */
    ushort lineNumber;                  /* Guess! */

    /* 
     * Read the change in lineSize. We might be at EOF though
     * so check this and return if so. we are done.
     */
    lineSize += getWord(fp);
    if (feof(fp))
        return 0;

    if ((flag = getWord(fp)) != TYPE_LINENUMBER) {
        fprintf(stderr, "\n\nERROR: parseProgramLine(): Program out of step at offset %ld ($%08lx).\n", ftell(fp), ftell(fp));
        fprintf(stderr, "Expected 0x8D00, found 0x%X.\n", flag);
        return 1;
    }

    /* Now the line number */
    lineNumber = getWord(fp);
    fprintf(listing, "%5d ", lineNumber);
    printf("parseProgramLine(%d)\n", lineNumber);

    /* And the rest of the line - the statements */
    while (1) {
        /* Parse one statement and check for end of line */
        if ((flag = parseStatement(fp)) == 10) {
            return 0;
        }

        /* If an error was found, bale out. */
        if (flag != 0) {
            fprintf(stderr, "\n\nERROR: parseProgramLine(): parseStatement() Failed.\n");
            return flag;
        }
    }
}



/*=============================================================================
 * PARSESTATEMENT() is the very low level program parser. It will parse and 
 * convert one statement of the source program at a time. This is made up of:
 * 
 * Byte: To determine the statements type.
 * Byte: Parameter for the type byte.
 * Bytes: The content of the statements.
 * EITHER:
 * Byte: 0x84 (TYPE_SYMBOL) Indicates a linefeed to follow.
 * Byte: 0x0A The end of the line.
 * OR:
 * Byte: 0x84 (TYPE_SYMBOL) Indicates a symbol to follow.
 * Byte: 0x02 End of statement, a colon.
 * 
 * Returns 10 if end of line found.
 *          2 if end of statement found.
 *          1 if error.
 *===========================================================================*/
ushort parseStatement(FILE *fp) {
    uchar  typeByte;                    /* What are we processing? */
    ulong offset;                       /* Where are we in the file? */
    uchar endOfLine;                    /* Colon? End of Line? */

    fprintf(stderr, "parseStatement()\n");

    /* Type Byte */
    typeByte = fgetc(fp);
    endOfLine = 0;

    /*-------------------------------------------------------------------------
     * OK, there's a problem here. When we exit from whatever code we call to
     * parse something, we need to be holding the next type byte. We exit back
     * to parseProgram() when we hit (0x84 0x0A) which is a doSymbol() call
     * technically.
     * ----------------------------------------------------------------------*/
    switch(typeByte) {
        case TYPE_MULTISPACE: doMultiSpaces(fp); break;
        case TYPE_KEYWORD:    doKeywords(fp); break;
        case TYPE_SYMBOL:     doSymbols(fp); break;
        case TYPE_OPERATOR:   doOperators(fp); break;
        case TYPE_MONADIC:    doMonadics(fp); break;
        case TYPE_NAME:       doNames(fp); break;
        case TYPE_STRING:     doStrings(fp); break;
        case TYPE_TEXT:       doText(fp); break;
        case TYPE_SEPARATOR:  doSeparators(fp); break;

        /* Floats come in three formats, each with 16 leading bytes! */
        case TYPE_FP_BIN_MIN ... TYPE_FP_BIN_MIN:   
        case TYPE_FP_HEX_MIN ... TYPE_FP_HEX_MAX:
        case TYPE_FP_DEC_MIN ... TYPE_FP_DEC_MAX:   
                                doFloatingPoint(fp, typeByte); break;

        default: 
            offset = ftell(fp);
            fprintf(stderr, "\n\nERROR: parseStatement(): At offset %ld ($%08lx), read byte %d (%c). Out of sync.", offset, offset, typeByte, (typeByte > 31 ? typeByte : '.'));
            return 1;
    }

    /*
     * At this point, we should be looking for either an end of statement 0x8402
     * or and end of line 0x840a word. In either case we should check for it and
     * call out to doSymbol() to print the details to the BAS listing and also
     * to end the line in the C source file.
     */

    typeByte = fgetc(fp);
    if (typeByte != 0x84) {
        fprintf(stderr, "\n\nERROR: parseStatement(): At offset %ld ($%08lx), read byte %d (%c). Out of sync.", offset, offset, typeByte, (typeByte > 31 ? typeByte : '.'));
        return 1;
    }

    /*
     * Fetch the terminator byte then shove it back! 
     */
    endOfLine = fgetc(fp);
    ungetc(endOfLine, fp);

    doSymbols(fp);

    switch (endOfLine) {
        case 2: return 0; 
                break;               /* Another statement to process */

        case 10: return endOfLine;
                 break;              /* End of the current line */

        default: return 1; 
                 break;             /* Oops! */
    }
}




void doMultiSpaces(FILE *fp){
    /* 0x80.nn = Print nn spaces */
    uchar nn = fgetc(fp);
    fprintf(listing, "%*.*s", nn, nn, " ");
}

/*=============================================================================
 * DOKEYWORDS() Call here when a keyword is read from the SAV file. This will
 * print the appropriate codes to the listing file, and then process the 
 * keyword into something resembling C68 source code. I hope! 
 *===========================================================================*/
void doKeywords(FILE *fp){
    /* 0x81.nn = Print keywords[nn] */

    static char *keywords[] = {
        "END", "FOR", "IF", "REPeat", "SELect", "WHEN", "DEFine",
        "PROCedure", "FuNction", "GO", "TO", "SUB", "", "ERRor", "",
        "", "RESTORE", "NEXT", "EXIT", "ELSE", "ON", "RETurn",
        "REMAINDER", "DATA", "DIM", "LOCal", "LET", "THEN", "STEP",
        "REMark", "MISTAKE"
    };

    static KWFUNC kwFunctions[] = {
        keywordNull, //kwEnd, 
        keywordNull, //kwFor, 
        keywordNull, //kwIf, 
        keywordNull, //kwRepeat, 
        keywordNull, //kwSelect, 
        keywordNull, //kwWhen, 
        keywordNull, //kwDefine,
        keywordNull, //kwProcedure,
        keywordNull, //kwFunction, 
        keywordNull, //kwGo, 
        keywordNull, //kwTt, 
        keywordNull, //kwSub, 
        keywordNull, //kwNull_1, 
        keywordNull, //kwError, 
        keywordNull, //kwNull_2,
        keywordNull, //kwNull_3, 
        keywordNull, //kwRestore, 
        keywordNull, //kwNext, 
        keywordNull, //kwExit, 
        keywordNull, //kwElse, 
        keywordNull, //kwOn, 
        keywordNull, //kwReturn,
        keywordNull, //kwRemainder,
        keywordNull, //kwData, 
        keywordNull, //kwDim, 
        keywordNull, //kwLocal, 
        keywordNull, //kwLet, 
        keywordNull, //kwThen, 
        keywordNull, //kwStep,
        keywordRemark, //kwRemark, 
        keywordMistake, //kwMistake
    };
    
    /* Read the keyword index, update the listing file, then
     * call the appropriate helper routine in keywords.c to
     * do the conversion to C68 source code.
     */
    fprintf(stderr, "doKeywords()\n");
    uchar nn = fgetc(fp) -1;
    fprintf(listing, "%s ", keywords[nn]);
    (kwFunctions[nn])(fp);
}


void doSymbols(FILE *fp){
    /* 0x84.nn = Print symbols[nn] */

    static char *symbols = "=:#,(){} \n";

    fprintf(stderr, "doSymbols()\n");
    uchar nn = fgetc(fp) -1;

    fprintf(listing, "%c", symbols[nn]);

    /* For EOL, we need to terminate the C code line */
    if (nn == 9)
        fprintf(source, "\n");
}


void doOperators(FILE *fp){
    /* 0x85.nn = Print operators[nn] */

    static char *operators[] = {
        "+", "-", "*", "/", ">=", ">", "==", "=", "<>", "<=", "<",
        "||", "&&", "^^", "^", "&", "OR", "AND", "XOR", "MOD",
        "DIV", "INSTR"
    };

    fprintf(stderr, "doOperators()\n");
    uchar nn = fgetc(fp) -1;
    fprintf(listing, "%s", operators[nn]);
}


void doMonadics(FILE *fp){
    /* 0x86.nn = Print monadics[nn] */

    static char *monadics[] = {
        "+", "-", "~~", "NOT"
    };

    fprintf(stderr, "doMonadics()\n");
    uchar nn = fgetc(fp) -1;
    fprintf(listing, "%s", monadics[nn]);
}


void doNames(FILE *fp){
    /* 0x8800.0xnnnn = Print name[0xnnnn] */
    uchar nn = fgetc(fp);   /* ignore */
    ushort entry = getWord(fp);
    fprintf(stderr, "doNames()\n");
    fprintf(listing, "%*.*s", nameTable[entry].nameLength, nameTable[entry].nameLength, nameTable[entry].name);
}


void doStrings(FILE *fp){
    /* 0x8B.delim.size.bytes.[padding] = Print delimited string */
    uchar delim = fgetc(fp);    /* Delimiter */
    ushort size = getWord(fp);  /* String length */
    ushort x;

    fprintf(stderr, "doStrings()\n");
    fputc(delim, listing);
    for (x = 0; x < size; x++) {
        fputc(fgetc(fp), listing);
    }

    fputc(delim, listing);

    if (size & 1)
        fgetc(fp);              /* Padding */
}


void doText(FILE *fp){
    /* 0x8C00.size.bytes = Print undelimited text
    */
    uchar ignore = fgetc(fp);    /* 00 byte */
    ushort size = getWord(fp);  /* String length */
    ushort x;

    fprintf(stderr, "doText()\n");
    for (x = 0; x < size; x++) {
        ignore = fgetc(fp);
        fputc(ignore, listing);
        fputc(ignore, source);
    }

    if (size & 1)
        fgetc(fp);              /* Padding byte*/
}


void doSeparators(FILE *fp){
    /* 0x8E.nn = Print separators[nn] */

    static char *separators[] = {
        ",", ";", "\\", "!", "TO"
    };

    uchar nn = fgetc(fp) -1;
    fprintf(stderr, "doSeparators()\n");
    fprintf(listing, "%s", separators[nn]);
}


void doFloatingPoint(FILE *fp, uchar leading){
    /* Floating points come in three variations:
     * 0xDn = % Binary
     * 0xEn = $ Hexadecimal
     * 0xFn = Normal.
     *
     * Each one is followed by 5 bytes.
     */
    ushort x;
    uchar fpType = ((leading & 0xF0) >> 4) - 13;
    char fpPrefix = (fpType == 0 ? '%' : fpType == 1 ? '$' : ' ');

    /* 6 byte QL Float structure */
    union {
        QLFLOAT_t buffer;
        ushort sh[3];
    } fpVariable;

    fprintf(stderr, "doFloatingPoints()\n");

    /* Print the Float prefix character, if necessary. */
    if (fpType == 0 || fpType == 1) {
        fprintf(listing, "%c", fpPrefix);
    }

    /* Backup up one byte. We can read the whole FP then. */
    fseek(fp, -1, SEEK_CUR);

#ifndef QDOS

    /* This reads a QL Float for wrong endian systems! */
    fpVariable.sh[0] = getWord(fp);
    fpVariable.sh[2] = getWord(fp);
    fpVariable.sh[1] = getWord(fp);

    fprintf(listing, "%f", qlfpToDouble(&fpVariable.buffer));

#else

    /* But the QL is correct endian! */
    ignore = fread(&fpVariable.buffer, 1, sizeof(QLFLOAT_t), fp);    
    fprintf(listing, "%f", qlfp_to_d(&fpVariable.buffer));

#endif     

}




/*=============================================================================
 * GETWORD() Reads a signed short value from the SAV file. On Linux, where I
 * tested, I have to reverse the order of the two bytes. Linux is "wrong"
 * endian, unlike QDOS.
 *===========================================================================*/
short getWord(FILE *fp) {
    union {
        ushort val;
        uchar  bytes[2];
    } data;

/* I tested on Linux, which is wrong endian! */
#ifdef QDOS

    ignore = fread(&data.val, 1, sizeof(ushort), fp);

#else    

    ignore = fread(&data.bytes[1], 1, 1, fp);
    ignore = fread(&data.bytes[0], 1, 1, fp);

#endif

    return data.val;
}


#ifndef QDOS

/*=============================================================================
 * QLFPTODOUBLE() Converts a QL 6 byte floating point value to a double in IEEE
 * format. This is a C conversion of the assembly routine qlfp_to_d() from the
 * C68 source code. There are three styles of float:
 *
 * 0xDx xx xx xx xx xx = Binary %101010101 etc.
 * 0xEx xx xx xx xx xx = Hexadecimal $abcdef etc.
 * 0xFx xx xx xx xx xx = Decimal 12345 etc.
 *
 * The top nibble, D, E or F, must be masked out before converting but will
 * return the correct type in the fpType parameter.
 *
 * According to https://en.wikipedia.org/wiki/Double-precision_floating-point_format
 * an IEEE double is:
 *
 * 1 sign bit
 * 11 exponent bits offset by 1023
 * 1 implied plus 52 fractional bits.
 *
 * The number is usually:
 * 
 * -1^^Sign * 2^^(exp-2013) * 1.Fractional Bits
 *
 * Where the 1 is the implied bit.
 *===========================================================================*/
double qlfpToDouble(QLFLOAT_t *qlfp) {
    ulong sign;
    ulong exponent = qlfp->exponent & 0x0fff;
    ulong mantissa = qlfp->mantissa;
    ulong temp;
    struct ieeeFloat {
        ulong ieeeExponent;
        ulong ieeeMantissa;
    } result;

    /* Lose the type marker. */
    exponent &= 0xFFFF0FFFL;

    /* Simple case first, is it zero? */
    if (exponent == 0 && mantissa == 0) {
        return (double)0;
    }

    /* Positive or negative? */
    sign = (mantissa & 0x80000000L);
    if (sign) {
        /* Negative. */
        exponent *= -1;
        if (mantissa & 0x40000000L) {
            exponent++;
        }

        /* Then drop in below. */
    }

    /* Positive. */
    exponent -= 0x402;
    exponent <<= 4;
    exponent <<= 16;
    exponent |= sign;

    mantissa <<= 2;
    temp = (short)mantissa;
    temp <<= 4;
    temp <<= 16;

    mantissa >>= 12;
    mantissa |= exponent;

    /* Exponent bits = 1 sign, 11 exponent, (1 implied), 20 fractionals
     * Mantissa bits = 32 bits of fractional */
    result.ieeeExponent = (long unsigned)temp;
    result.ieeeMantissa = (long unsigned)mantissa;
    return *((double *)(&result));
 }

#endif


/*=============================================================================
 * SWAPEXTENSION() Given a SAV filename, a buffer for the new filename and the
 * new extension required, string SAV from the end of the SAV file and create
 * a new filename with the same root but the required extension.
/*===========================================================================*/
void swapExtension(char *savFile, char *newFilename, char *newExtension) {
    ushort savSize = strlen(savFile);
    ushort extSize = strlen(newExtension);

    strncpy(newFilename, savFile, savSize - 3);
    strncpy(newFilename + savSize - 3, newExtension, extSize);
    newFilename[savSize - 3 + extSize] = '\0';
}