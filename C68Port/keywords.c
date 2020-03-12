#include "keywords.h"

extern ushort level;
extern uchar indent;
extern FILE *source;
extern FILE *listing;


ushort keywordNull(FILE *fp) {

}


/*
 * If a valid program line is commented out, it's internal formatting is
 * removed and the whole line becomes nothing but plain text. Even if 
 * embedded floating point variables were there, for example, they get
 * replaced by text. A REMark statement extends to the end of the line.
 */
ushort keywordRemark(FILE *fp) {
    /* $81 $1e = REMark 
     * $8c00 = Text marker
     * Size.word = Size of text
     * Bytes = actual text
     * Byte = padding, if size is odd
     * Word $840a (End of line).
     */
    ushort x;
    ushort tempWord;
    unsigned char padding;

    fprintf(source, "%*.*s ", level*indent, level*indent, "");
    fprintf(source, "%s ", "/*");
    if ((padding = fgetc(fp)) != 0x8c) {
        fprintf(stderr, "\n\nERROR: keywordRemark(): Failed to read 'text marker' byte $8C, found $%2X.", padding);
        return 1;
    }

    doText(fp);

    /* Terminate the comment */
    fprintf(source, " */");

    return 0;
}


/*
 * If a program line is a MISTake, then it's almost like a REMark in that
 * the whole line is text until the terminator.
 */
ushort keywordMistake(FILE *fp) {
    /* $81 $1f = REMark 
     * $8c00 = Text marker
     * Size.word = Size of text
     * Bytes = actual text
     * Byte = padding, if size is odd
     * Word $840a (End of line).
     */
    ushort x;
    ushort tempWord;
    unsigned char padding;

    fprintf(source, "#error ");
    if ((padding = fgetc(fp)) != 0x8c) {
        fprintf(stderr, "\n\nERROR: keywordMistake(): Failed to read 'text marker' byte $8C, found $%2X.", padding);
        return 1;
    }

    doText(fp);
    return 0;
}


