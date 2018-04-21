5 REMark C68Port by Norman Dunbar
10 REMark Why? Because there's no source code for CPort!
15 :
20 :










1000 REMark _SAV file decoder
1010 :
1020 CLS
1030 PRINT 'SAV File Decoder'\\
1040 INPUT 'Which _sav file ? ';sav$
1050 IF sav$ = '' THEN STOP: END IF
1060 :
1070 initialise
1080 decode_header
1090 IF NOT _quit
1100    decode_name_table
1110    decode_program
1120 END IF
1130 RELEASE_HEAP float_buffer
1140 CLOSE #3
1150 CLOSE #4
1160 :
1170 DEFine PROCedure decode_header
1180   LOCal head$(4), name_table_length
1190   _quit = 0
1200   OPEN_IN #3,sav$
1210   head$ = FETCH_BYTES(#3, 4)
1220   IF (head$ <> 'Q1' & CHR$(0) & CHR$(0)) AND (head$ <> 'Q1' & CHR$(2) & CHR$(192)) AND (head$ <> 'Q1'& CHR$(3) & CHR$(128)) AND (head$ <> 'Q1' & CHR$(0) & CHR$(128))
1230      PRINT #4, head$, head$(1);head$(2)!!CODE(head$(3))!CODE(head$(4))\
1240      PRINT #4, sav$ & ' is not a SAV file, or has a new flag.'
1250      CLOSE #3
1260      CLOSE #4
1270      _quit = 1
1280      RETurn
1290   END IF
1300   PRINT #4, 'FILE HEADER'
1310   PRINT #4, '==========='
1320   PRINT #4
1330   name_table_entries = GET_WORD(#3)
1340   name_table_length = GET_WORD(#3)
1350   program_lines = GET_WORD(#3)
1360   max_name_size = name_table_length - (4 * name_table_entries) / name_table_entries
1370   :
1380   PRINT #4, 'File name                    : '; sav$
1390   PRINT #4, 'Number of name table entries : '; name_table_entries
1400   PRINT #4, 'Name table length            : '; name_table_length
1410   PRINT #4, 'Number of program lines      : '; program_lines
1420   PRINT #4
1430   :
1440   DIM name_table$(name_table_entries -1, max_name_size)
1450   float_buffer = RESERVE_HEAP(6)
1460   _quit = (float_buffer < 1)
1470 END DEFine decode_header
1480 :
1490 DEFine PROCedure decode_name_table
1500   LOCal x, name_type, line_no, name_length, name$, lose_it$(1)
1510   LOCal num_procs, num_fns
1520   num_procs = 0
1530   num_fns = 0
1540   PRINT #4, 'NAME TABLE'
1550   PRINT #4, '=========='
1560   PRINT #4
1570   FOR x = 0 TO name_table_entries -1
1580     name_type = GET_WORD(#3)
1590     line_no = GET_WORD(#3)
1600     name_length = GET_WORD(#3)
1610     name$ = FETCH_BYTES(#3, name_length)
1620     IF name_length && 1
1630        lose_it$ = INKEY$(#3)
1640     END IF
1650     IF name_type = 5122 THEN num_procs = num_procs + 1
1660     IF name_type >= 5377 AND name_type <= 5379
1670        num_fns = num_fns + 1
1680     END IF
1690     PRINT #4, x;'  Name type = '; HEX$(name_type, 16) & '  ';
1700     PRINT #4, 'Line number = '; line_no & '  ';
1710     PRINT #4, 'Name length = '; name_length; '  ';
1720     PRINT #4, 'Name = <' & name$ & '>'
1730     name_table$(x) = name$
1740   END FOR x
1750   PRINT #4
1760   PRINT #4, 'There are ' & num_procs & ' PROCs'
1770   PRINT #4, 'There are ' & num_fns & ' FNs'
1780   PRINT #4
1790 END DEFine decode_name_table
1800 :
1810 :
1820 DEFine PROCedure decode_program
1830   LOCal x, type_byte, program_line
1840   :
1850   REMark WORD = size change
1860   REMark LONG = $8D00.line number
1870   REMark rest of line
1880   :
1890   PRINT #4, 'PROGRAM'
1900   PRINT #4, '======='
1910   PRINT #4
1920   :
1930   REPeat program_line
1940     IF EOF(#3) THEN EXIT program_line: END IF
1950     line_size = line_size + GET_WORD(#3)
1960     IF line_size > 65536 THEN line_size = line_size - 65536: END IF
1970     IF GET_WORD(#3) <> HEX('8d00')
1980        PRINT #4, 'Program out of step.'
1990        CLOSE #3
2000        CLOSE #4
2010        STOP
2020     END IF
2030     PRINT #4, GET_WORD(#3); ' ';
2040     line_done = 0
2050     REPeat line_contents
2060       type_byte = CODE(INKEY$(#3))
2070       SELect ON type_byte
2080         = HEX('80'): multi_spaces
2090         = HEX('81'): keywords
2100         = HEX('84'): symbols: REMark Followed by HEX('0A') = End of line
2110         = HEX('85'): operators
2120         = HEX('86'): monadics
2130         = HEX('88'): names
2140         = HEX('8B'): strings
2150         = HEX('8C'): text
2160         = HEX('8E'): separators
2170         = HEX('D0') TO HEX('DF') : floating_points 1 : REMark % binary number
2180         = HEX('E0') TO HEX('EF') : floating_points 2 : REMark $ hex number
2190         = HEX('F0') TO HEX('FF') : floating_points 3 : REMark floating point
2200       END SELect
2210       IF line_done THEN EXIT line_contents: END IF
2220     END REPeat line_contents
2230   END REPeat program_line
2240 END DEFine decode_program
2250 :
2260 :
2270 DEFine PROCedure multi_spaces
2280   :
2290   REMark $80.nn = print nn spaces
2300   :
2310   PRINT #4, FILL$(' ', GET_BYTE(#3));
2320 END DEFine multi_spaces
2330 :
2340 :
2350 DEFine PROCedure keywords
2360   :
2370   REMark $81.nn = keyword$(nn)
2380   :
2390   PRINT #4, keyword$(GET_BYTE(#3));' ';
2400 END DEFine keywords
2410 :
2420 :
2430 DEFine PROCedure symbols
2440   LOCal sym
2450   :
2460   REMark $84.nn = symbol$(nn)
2470   :
2480   sym = GET_BYTE(#3)
2490   PRINT #4, symbol$(sym);
2500   line_done = (sym = 10)
2510 END DEFine symbols
2520 :
2530 :
2540 DEFine PROCedure operators
2550   :
2560   REMark $85.nn = operator$(nn)
2570   :
2580   PRINT #4, operator$(GET_BYTE(#3));
2590 END DEFine operators
2600 :
2610 :
2620 DEFine PROCedure monadics
2630   :
2640   REMark $86.nn = monadic$(nn)
2650   :
2660   PRINT #4, monadic$(GET_BYTE(#3));
2670 END DEFine monadic
2680 :
2690 :
2700 DEFine PROCedure names
2710   LOCal ignore
2720   :
2730   REMark $8800.nnnn = name_table$(nnnn)
2740   :
2750   ignore = GET_BYTE(#3)
2760   ignore = GET_WORD(#3)
2770   IF ignore > 32768 THEN ignore = ignore - 32768: END IF
2780   PRINT #4, name_table$(ignore);
2790 END DEFine names
2800 :
2810 :
2820 DEFine PROCedure strings
2830   LOCal delim$(1), size
2840   :
2850   REMark $8B.delim.string_size = 'delim'; string; 'delim'
2860   :
2870   delim$ = INKEY$(#3)
2880   size = GET_WORD(#3)
2890   PRINT #4, delim$; FETCH_BYTES(#3, size); delim$;
2900   IF size && 1
2910      size = GET_BYTE(#3)
2920   END IF
2930 END DEFine strings
2940 :
2950 :
2960 DEFine PROCedure text
2970   LOCal size
2980   :
2990   REMark $8C00.size = text
3000   :
3010   size = GET_BYTE(#3)
3020   size = GET_WORD(#3)
3030   PRINT #4, FETCH_BYTES(#3, size);
3040   IF size && 1
3050      size = GET_BYTE(#3)
3060   END IF
3070 END DEFine text
3080 :
3090 :
3100 DEFine PROCedure separators
3110   :
3120   REMark $8E.nn = separator$(nn)
3130   :
3140   PRINT #4, separator$(GET_BYTE(#3));
3150 END DEFine separators
3160 :
3170 :
3180 DEFine PROCedure floating_points (fp_type)
3190   REMark modified for % and $ SBASIC values 22.01.10 - DJ
3200   LOCal number$(6),fpt
3210   fpt = fp_type : REMark to avoid SEL ON last parameter issue later
3220   :
3230   REMark fp_type=...
3240   REMark $Dx.xx.xx.xx.xx.xx - %binary number
3250   REMark $Ex.xx.xx.xx.xx.xx - $hex number
3260   REMark $Fx.xx.xx.xx.xx.xx - need to mask out the first $F !
3270   :
3280   MOVE_POSITION #3, -1: REMark back up to the first byte
3290   number$ = FETCH_BYTES(#3, 6)
3300   number$(1) = CHR$( CODE(number$(1)) && 15)
3310   POKE_STRING float_buffer, number$
3320   SELect ON fpt
3330     =1 : PRINT #4, '%';LTrim$(BIN$(PEEK_FLOAT(float_buffer),32));
3340     =2 : PRINT #4, '$';LTrim$(HEX$(PEEK_FLOAT(float_buffer),32));
3350     =3 : PRINT #4, PEEK_FLOAT(float_buffer);
3360   END SELect
3370 END DEFine floating_points
3380 :
3390 DEFine FuNction LTrim$(str$)
3400   REMark added 22.01.10 for % and $ values - DJ
3410   REMark remove leading zeros from binary or hex strings
3420   LOCal a,t$
3430   t$ = str$ : REMark full length by default
3440   FOR a = 1 TO LEN(t$)
3450     IF t$(a) <> '0' THEN t$ = t$(a TO LEN(t$)) : EXIT a
3460   NEXT a
3470     t$ = '0' : REMark in case it was all zeros
3480   END FOR a
3490   RETurn t$
3500 END DEFine LTrim$
3510 :
3520 DEFine PROCedure initialise
3530   LOCal x
3540   :
3550   _quit = 0
3560   last_line_size = 0
3570   line_size = 0
3580   name_table_entries = 0
3590   :
3600   RESTORE 3630
3610   DIM keyword$(31, 9)
3620   FOR x = 1 TO 31: READ keyword$(x): END FOR x
3630   DATA 'END', 'FOR', 'IF', 'REPeat', 'SELect', 'WHEN', 'DEFine'
3640   DATA 'PROCedure', 'FuNction', 'GO', 'TO', 'SUB', '', 'ERRor', ''
3650   DATA '', 'RESTORE', 'NEXT', 'EXIT', 'ELSE', 'ON', 'RETurn'
3660   DATA 'REMAINDER', 'DATA', 'DIM', 'LOCal', 'LET', 'THEN', 'STEP'
3670   DATA 'REMark', 'MISTake'
3680   :
3690   DIM symbol$(10)
3700   symbol$ =  '=:#,(){} ' & CHR$(10)
3710   :
3720   DIM operator$(22, 5)
3730   FOR x = 1 TO 22: READ operator$(x): END FOR x
3740   DATA '+', '-', '*', '/', '>=', '>', '==', '=', '<>', '<=', '<'
3750   DATA '||', '&&', '^^', '^', '&', 'OR', 'AND', 'XOR', 'MOD'
3760   DATA 'DIV', 'INSTR'
3770   :
3780   DIM monadic$(4, 3)
3790   FOR x = 1 TO 4: READ monadic$(x): END FOR x
3800   DATA '+', '-', '~~', 'NOT'
3810   :
3820   DIM separator$(5, 2)
3830   FOR x = 1 TO 5: READ separator$(x): END FOR x
3840   DATA ',', ';', '\', '!', 'TO'
3850   :
3860   INPUT 'Output file? ENTER for screen. ',out$
3870   IF out$ = '' THEN
3880      OPEN #4, scr_
3890   ELSE
3900      OPEN_OVER #4,out$
3910   END IF
3920 END DEFine initialise
3930 :
3940 :
3950 DEFine PROCedure sa
3960   SAVE 'win1_basic_decode_sav_file_bas'
3970   QSAVE 'win1_basic_decode_sav_file'
3980 END DEFine sa
3990 :
