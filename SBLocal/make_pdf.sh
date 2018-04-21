#! /usr/bin/env bash
#---------------------------------------------------------------
# Builds a README.pdf from the README.md source file.
# Assumes pandoc is installed and on the PATH.
# Norman Dunbar.
# February 2017.
#---------------------------------------------------------------
# May not work on RHEL, CENTOS 7, Oracle Linux 7 etc. Even if
# pandoc is installed as it installs version 1.12.3.1. Too old!
# I *think* version 1.17 is the minimum that uses the xcolor
# LaTeX package. 
#---------------------------------------------------------------
PANDOC=`pandoc --version | head -1 | cut -d" " -f2`
PANDOC_MINOR=`echo ${PANDOC} | cut -d"." -f2`
PANDOC_COLOUR="Cool Grey"
PANDOC_PDF="${1}".pdf
PANDOC_TOC="${PANDOC_COLOUR}"

echo "Pandoc version ${PANDOC} in use."

if [ "${PANDOC_MINOR}" -lt "17" ]
then
    ## Pandoc too old, can't cope.
    PANDOC_COLOUR="blue"
    PANDOC_TOC="black"
    echo "Pandoc version ${PANDOC} is too old."
    echo "Margins in the PDF may not be at the required size."
else
    echo "Pandoc version ${PANDOC} is recent."
fi

echo "URLs and internal links will be ${PANDOC_COLOUR}."
echo "Table of contents links will be ${PANDOC_TOC}."

pandoc --from rst \
--to latex \
--output "${PANDOC_PDF}" \
--table-of-contents \
--toc-depth=3 \
--listings \
-H listings_setup.tex \
--variable fontfamily="utopia" \
--variable toccolor="${PANDOC_COLOUR}" \
--variable linkcolor="${PANDOC_COLOUR}" \
--variable urlcolor="${PANDOC_COLOUR}" \
--variable margin-top=3cm \
--variable margin-left=3cm \
--variable margin-right=3cm \
--variable margin-bottom=4cm \
"${1}".rst

if [ "${?}" -eq "0" ]
then
    echo "${PANDOC_PDF}" created.
else
    echo "${PANDOC_PDF}" failed to create.
fi

