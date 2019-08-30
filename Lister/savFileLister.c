/*=============================================================================
 * C68 "_sav" file decoder and converter to C68 source. (Maybe!)
 * Reads the file on arvg[1], which should be a _sav file, created by QSAVE
 * and converts it back to SuperBASIC. This is a trial run of working out how
 * to convert the SAV file to a C source file a-la CPORT, which is old and
 * somewhat broken, and no source code exists.
 *
 * Wish me luck!
 *
 * Norman Dunbar
 * August 24 2019. (Started!)
 *===========================================================================*/

#include "c68port.h"


/*=============================================================================
 * MAIN() Start here. Expects the input file on argv[1] and writes the output
 * to stdout with messages and errors on stderr - might as well use them!
 *===========================================================================*/
int main (int argc, char *argv[]) {

    ushort nameTableEntries = 0;
    ushort programLines = 0;
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
    if (decodeNameTable(nameTableEntries, fp) != 0) {
        fprintf(stderr, "FATAL ERROR: decodeNameTable() failed.\n");
        return -1;
    }

    /* Decode the program. */
    if (decodeProgram(programLines, fp, argv[1]) != 0) {
        fprintf(stderr, "FATAL ERROR: decodeProgram() failed.\n");
        return -1;
    }

    /* All done, exit with no errors. */
    if (nameTable) {
        free(nameTable);
    }

    fclose(fp);

    return 0;
}

/*=============================================================================
 * DECODEHEADER() Decodes the header of the _save file and makes sure it's
 * valid, otherwise we abort.
 *===========================================================================*/
ushort decodeHeader(FILE *fp, ushort *entries, ushort *lines) {
    uchar  head[4];
    ushort valid;
    ushort nameTableLength = 0;
    ushort programLines = 0;
    ushort nameTableEntries = 0;

    quit = 0;

    /* Can we read the 4 byte header? */
    if (fread(head, 1, 4, fp) != 4) {
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
ushort decodeNameTable(ushort entries, FILE *fp) {
    ushort x;
    uchar ch;
    ushort size = 0;
    ushort procCount = 0;
    ushort fnCount[3] = {0,0,0};        /* FN$, FN, FN% counters */

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
            fread(nameTable[x].name, 1, size, fp);
            fprintf(stderr, "\n\nWARNING: Cannot read a name with >= %d characters. Please amend the SAV file, or, recompile C68Port.\n", MAXNAMESIZE);
            fprintf(stderr, "The offending name is '%*.*s'\n.", MAXNAMESIZE, MAXNAMESIZE, nameTable[x].name);
            return 1;
        }
    
        fread(nameTable[x].name, 1, nameTable[x].nameLength, fp);

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

    return 0;
}


/*=============================================================================
 * DECODEPROGRAM disassembles the SAV file's contants representing the program
 * listing. Eventually, this will convert the SuperBASIC to C68 style C code.
 * (I hope!)
 *===========================================================================*/
ushort decodeProgram(ushort lines, FILE *fp, char *fileName) {
    ushort x;
    uchar  typeByte;
    ushort programLine;
    ushort lineSize = 0;
    ushort flag;
    ulong offset;
    uchar endOfLine;
    char logFile[40];       /* Listing file name - big enough for a QL. */

    FILE *listingFile;

    /* The format of a program line is:
     * ushort - change in size over previous line;
     * ushort - 0x8D00 line number flag;
     * ushort - line number;
     * bytes - rest of the line.

    /* Open a listing file, replace SAV with LST. */
    if (strlen(fileName) > 39) {
        fprintf(stderr, "\n\nERROR: decodeProgram(): Cannot create listing file, filename too long.\n");
        return 1;
    }
    strncpy(logFile, fileName, 39);
    logFile[strlen(fileName) > 39 ? 39 : strlen(fileName)] = '\0';

    // Change the extension, from SAV to LST */
    if (logFile[strlen(logFile) - 4] == '_' || logFile[strlen(logFile) - 4] == '.') {
        strcpy(&logFile[strlen(logFile) - 3], "LST");
    } else {
        
    }

    listingFile = fopen(logFile, "w");
    if (!listingFile) {
        fprintf(stderr, "\n\nERROR: decodeProgram(): Cannot open listing file, 'c68port.lst'.\n");
        return 1;
    }

    for (x = 0; x < lines; x++) {
        lineSize += getWord(fp);

        if ((flag = getWord(fp)) != TYPE_LINENUMBER) {
            fprintf(stderr, "\n\nERROR: decodeProgram(): Program out of step at offset %ld ($%08lx).\n", ftell(fp), ftell(fp));
            fprintf(stderr, "Expected 0x8D00, found %xd.\n", flag);
            fclose(listingFile);
            return 1;
        }

        /* Line number */
        fprintf(listingFile, "%d ", getWord(fp));

        /* Line contents */
        while (1) {
            /* Type Byte */
            typeByte = fgetc(fp);
            endOfLine = 0;
            switch(typeByte) {
                case TYPE_MULTISPACE: doMultiSpaces(fp, listingFile); break;
                case TYPE_KEYWORD:    doKeywords(fp, listingFile); break;
                case TYPE_SYMBOL:     endOfLine = doSymbols(fp, listingFile); break;
                case TYPE_OPERATOR:   doOperators(fp, listingFile); break;
                case TYPE_MONADIC:    doMonadics(fp, listingFile); break;
                case TYPE_NAME:       doNames(fp, listingFile); break;
                case TYPE_STRING:     doStrings(fp, listingFile); break;
                case TYPE_TEXT:       doText(fp, listingFile); break;
                case TYPE_SEPARATOR:  doSeparators(fp, listingFile); break;

                /* Floats come in three formats, each with 16 leading bytes! */
                case TYPE_FP_BIN_MIN ... TYPE_FP_BIN_MIN:   
                case TYPE_FP_HEX_MIN ... TYPE_FP_HEX_MAX:
                case TYPE_FP_DEC_MIN ... TYPE_FP_DEC_MAX:   
                                      doFloatingPoint(fp, listingFile, typeByte); break;

                default: 
                    offset = ftell(fp);
                    fprintf(stderr, "\n\nERROR: decodeProgram(): At offset %ld ($%08lx), read byte %d (%c). Out of sync.", offset, offset, typeByte, (typeByte > 31 ? typeByte : '.'));
                    fclose(listingFile);
                    return 1;
            }

            /* Exit the while loop when we print an end of line character. */
            if (endOfLine) {
                fflush(listingFile);
                break;
            }
        }
    }

    return 0;
}

void doMultiSpaces(FILE *fp, FILE *listing){
    /* 0x80.nn = Print nn spaces */
    uchar nn = fgetc(fp);
    fprintf(listing, "%*.*s", nn, nn, " ");
}

void doKeywords(FILE *fp, FILE *listing){
    /* 0x81.nn = Print keywords[nn] */

    static char *keywords[] = {
        "END", "FOR", "IF", "REPeat", "SELect", "WHEN", "DEFine",
        "PROCedure", "FuNction", "GO", "TO", "SUB", "", "ERRor", "",
        "", "RESTORE", "NEXT", "EXIT", "ELSE", "ON", "RETurn",
        "REMAINDER", "DATA", "DIM", "LOCal", "LET", "THEN", "STEP",
        "REMark", "MISTAKE"
    };
    
    uchar nn = fgetc(fp) -1;
    fprintf(listing, "%s ", keywords[nn]);
}


uchar doSymbols(FILE *fp, FILE *listing){
    /* 0x84.nn = Print symbols[nn] */

    static char *symbols = "=:#,(){} \n";

    uchar nn = fgetc(fp) -1;

    fprintf(listing, "%c", symbols[nn]);

    /* Return 1 for end of line. 0 otherwise. */
    return (nn == 0x09); 
}


void doOperators(FILE *fp, FILE *listing){
    /* 0x85.nn = Print operators[nn] */

    static char *operators[] = {
        "+", "-", "*", "/", ">=", ">", "==", "=", "<>", "<=", "<",
        "||", "&&", "^^", "^", "&", "OR", "AND", "XOR", "MOD",
        "DIV", "INSTR"
    };

    uchar nn = fgetc(fp) -1;
    fprintf(listing, "%s", operators[nn]);
}


void doMonadics(FILE *fp, FILE *listing){
    /* 0x86.nn = Print monadics[nn] */

    static char *monadics[] = {
        "+", "-", "~~", "NOT"
    };

    uchar nn = fgetc(fp) -1;
    fprintf(listing, "%s", monadics[nn]);
}


void doNames(FILE *fp, FILE *listing){
    /* 0x8800 = Print name[nn] */
    uchar nn = fgetc(fp);   /* ignore */
    ushort entry = getWord(fp);
    fprintf(listing, "%*.*s", nameTable[entry].nameLength, nameTable[entry].nameLength, nameTable[entry].name);
}


void doStrings(FILE *fp, FILE *listing){
    /* 0x8B.delim.size.bytes.[padding] = Print delimited string */
    uchar delim = fgetc(fp);    /* Delimiter */
    ushort size = getWord(fp);  /* String length */
    ushort x;

    fputc(delim, listing);
    for (x = 0; x < size; x++) {
        fputc(fgetc(fp), listing);
    }

    fputc(delim, listing);

    if (size & 1)
        fgetc(fp);              /* Padding */
}


void doText(FILE *fp, FILE *listing){
    /* 0x8C00.size.bytes = Print undelimited text
    */
    uchar ignore = fgetc(fp);    /* 00 byte */
    ushort size = getWord(fp);  /* String length */
    ushort x;

    for (x = 0; x < size; x++) {
        fputc(fgetc(fp), listing);
    }

    if (size & 1)
        fgetc(fp);              /* Padding */
}


void doSeparators(FILE *fp, FILE *listing){
    /* 0x8E.nn = Print separators[nn] */

    static char *separators[] = {
        ",", ";", "\\", "!", "TO"
    };

    uchar nn = fgetc(fp) -1;
    fprintf(listing, "%s", separators[nn]);
}


void doFloatingPoint(FILE *fp, FILE *listing, uchar leading){
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
    fread(&fpVariable.buffer, 1, sizeof(QLFLOAT_t), fp);    
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

    fread(&data.val, 1, sizeof(ushort), fp);

#else    

    fread(&data.bytes[1], 1, 1, fp);
    fread(&data.bytes[0], 1, 1, fp);

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

