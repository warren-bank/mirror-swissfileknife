@echo off
sfk script %~f0 -from begin %*
rem %~f0 is the absolute batch file name
rem on linux, use: 
rem    sfk script joinsfkpic.bat -from begin
GOTO xend

// required input folders:
//   sfklib\zlib
//   sfklib2\libpng
//   sfklib2\libjpg

sfk label begin -var

   +if "#(sys.slash) = /" begin
      +xex sfklib2/libjpg/jerror.h
         "/[lstart]JMESSAGE(**"**"/[parts 4,5,6],\n/"
            +setvar msgtab
   +else
      +xex sfklib2\libjpg\jerror.h
         "/[lstart]JMESSAGE(**"**"/[parts 4,5,6],\n/"
            +setvar msgtab
   +endif

   +then setvar cnt=2

   +echo -spat "
      /*
         SFKPic V1.0.3, a frozen monolithic code combining

            zlib     1.2.3
            libpng   1.2.41
            libjpeg  80

         for easiest possible compilation. Created by
         joinsfkpic.bat using the Swiss File Knife tool.

         All source files were heavily modified to allow freezing
         them into a single source code, compilable in C++ mode.
         For input source files see sfklib.zip within sfk.zip,
         but these also contain some code fixes marked with "stw".

         to compile a simple compression tool under Windows/Linux use:
            cl  -DTESTCOMP /Femycomp.exe sfkpack.cpp
            g++ -DTESTCOMP -omycomp.exe sfkpack.cpp

         to compile the sfkpic tool for image conversion use one of:
            cl -DTESTSFKPIC sfkpic.cpp
            g++ -DTESTSFKPIC -osfkpic sfkpic.cpp

         This library can be used free of charge in any project,
         based on the original library licenses, which are all 
         BSD compatible. This means you may use them also in
         closed source commercial applications, but without
         any warranty.

         If errors occur on image processing, do not ask the original
         authors of zlib, libpng or libjpeg as they have nothing to do 
         with SFKPic. Instead, download the original source codes 
         of those libraries, then compile your project with these,
         and see if it works. If so, you may be better off with the 
         original libraries. But if the error is produced just and
         only by SFKPic, and can be easily reproduced (by a sample 
         image file) you may also submit a bug report to the 
         Swiss File Knife project.
      */
      #ifdef _WIN32
       #include <windows.h>
       #include <direct.h>
       #define HAVE_BOOLEAN
      #endif
      #include <stdlib.h>
      #include <stdio.h>
      #include <string.h>
      #include <setjmp.h>
      #include <math.h>
      #include <time.h>
      #include <errno.h>
      #include <sys/stat.h>
      #define TBLS 8
      #define NO_DUMMY_DECL
      #ifdef _MSC_VER
       #ifndef _PTRDIFF_T_DEFINED
        #define _PTRDIFF_T_DEFINED
        typedef long ptrdiff_t;
       #endif
      #endif
      typedef unsigned int u4;
      #define DIST_CODE_LEN  512
      #define PNG_INTERNAL
      #define PNG_NO_PEDANTIC_WARNINGS
      #define JPEG_INTERNALS
      #define Z_BUFSIZE   (256*1024)
      #define UNZ_BUFSIZE (256*1024)
      #define NOCRYPT
      #define NOBYFOUR
      #define PNG_SKIP_SETJMP_CHECK
      #define HAVE_BZIP2
      #define BZ_NO_STDIO
      void bz_internal_error(int e)
       { printf(\qbziperr %d\\n\q,e); }

      #ifdef _WIN32
      extern "C" {
         int _fseeki64(FILE *stream, __int64 offset, int origin);
         __int64 _ftelli64(FILE * _File);
      }
      #endif
      "
      +tofile sfkpic-tmp.c

   // v103: BYFOUR is not vc14 and 64 bit compatible and
   // causes wrong crcs therefore #define NOBYFOUR above.

   +echo "
      sfklib\zlib\zconf.h
      sfklib\zlib\zlib.h
      sfklib\zlib\zutil.h
      sfklib\zlib\deflate.h
      sfklib\zlib\crc32.h
      sfklib\zlib\inftrees.h
      sfklib\zlib\inffixed.h
      sfklib\zlib\inffast.h
      sfklib\zlib\inflate.h
      sfklib\zlib\trees.h

      sfklib\zlib\deflate.c
      sfklib\zlib\adler32.c
      sfklib\zlib\compress.c
      sfklib\zlib\crc32.c      zlcrc32
      sfklib\zlib\infback.c
      sfklib\zlib\inffast.c
      sfklib\zlib\inflate.c    zlinflate
      sfklib\zlib\inftrees.c
      sfklib\zlib\trees.c      zltrees
      sfklib2\zlib\uncompr.c
      sfklib\zlib\zutil.c

      sfklib2\libpng\pngconf.h
      sfklib2\libpng\png.h

      sfklib2\libpng\png.c
      sfklib2\libpng\pngerror.c
      sfklib2\libpng\pngget.c
      sfklib2\libpng\pngmem.c     pngmem
      sfklib2\libpng\pngpread.c
      sfklib2\libpng\pngread.c
      sfklib2\libpng\pngrio.c
      sfklib2\libpng\pngrtran.c
      sfklib2\libpng\pngrutil.c
      sfklib2\libpng\pngset.c
      sfklib2\libpng\pngtrans.c
      sfklib2\libpng\pngvcrd.c
      sfklib2\libpng\pngwio.c
      sfklib2\libpng\pngwrite.c
      sfklib2\libpng\pngwtran.c
      sfklib2\libpng\pngwutil.c

      sfklib2\libjpg\jconfig.h
      sfklib2\libjpg\jmorecfg.h
      sfklib2\libjpg\jversion.h
      sfklib2\libjpg\jpeglib.h
      sfklib2\libjpg\jpegint.h
      sfklib2\libjpg\cderror.h
      sfklib2\libjpg\cdjpeg.h
      sfklib2\libjpg\jdct.h
      sfklib2\libjpg\jerror.h
      sfklib2\libjpg\jinclude.h
      sfklib2\libjpg\jmemsys.h
      sfklib2\libjpg\transupp.h

      sfklib2\libjpg\jaricom.c
      sfklib2\libjpg\jcapimin.c
      sfklib2\libjpg\jcapistd.c
      sfklib2\libjpg\jcarith.c
      sfklib2\libjpg\jccoefct.c
      sfklib2\libjpg\jcdctmgr.c
      sfklib2\libjpg\jchuff.c
      sfklib2\libjpg\jcinit.c
      sfklib2\libjpg\jcmainct.c
      sfklib2\libjpg\jcmarker.c
      sfklib2\libjpg\jcmaster.c
      sfklib2\libjpg\jcomapi.c
      sfklib2\libjpg\jcparam.c
      sfklib2\libjpg\jcprepct.c
      sfklib2\libjpg\jcsample.c
      sfklib2\libjpg\jctrans.c
      sfklib2\libjpg\jdapimin.c
      sfklib2\libjpg\jdapistd.c
      sfklib2\libjpg\jdarith.c
      sfklib2\libjpg\jdatadst.c
      sfklib2\libjpg\jdatasrc.c
      sfklib2\libjpg\jdcoefct.c
      sfklib2\libjpg\jddctmgr.c
      sfklib2\libjpg\jdhuff.c
      sfklib2\libjpg\jdinput.c
      sfklib2\libjpg\jdmainct.c
      sfklib2\libjpg\jdmarker.c   jdmarker
      sfklib2\libjpg\jdmaster.c
      sfklib2\libjpg\jdpostct.c
      sfklib2\libjpg\jdsample.c
      sfklib2\libjpg\jdtrans.c
      sfklib2\libjpg\jerror.c     jerror
      sfklib2\libjpg\jfdctflt.c
      sfklib2\libjpg\jfdctfst.c
      sfklib2\libjpg\jfdctint.c
      sfklib2\libjpg\jidctflt.c
      sfklib2\libjpg\jidctfst.c
      sfklib2\libjpg\jidctint.c
      sfklib2\libjpg\jmemmgr.c
      sfklib2\libjpg\jmemnobs.c
      sfklib2\libjpg\jquant1.c
      sfklib2\libjpg\jquant2.c    jquant2
      sfklib2\libjpg\jutils.c
      sfklib2\libjpg\jccolor.c    jpfix
      sfklib2\libjpg\jdcolor.c    jpfix
      sfklib2\libjpg\jdmerge.c    jpfix

      sfklib2\bzip2\bzlib.h       bzip2

      sfklib\zlib\contrib\minizip\ioapi.h      mzioapih
      sfklib\zlib\contrib\minizip\ioapi.c
      sfklib\zlib\contrib\minizip\zip.h        miniziph
      sfklib\zlib\contrib\minizip\zip.c        zipc
      sfklib\zlib\contrib\minizip\minizip.c    minizipc
      sfklib\zlib\contrib\minizip\unzip.h
      sfklib\zlib\contrib\minizip\miniunz.c    miniunzc
      sfklib\zlib\contrib\minizip\unzip.c      unzipc

      sfklib2\bzip2\bzlib_private.h  bzip2
      sfklib2\bzip2\blocksort.c      bzip2
      sfklib2\bzip2\crctable.c       bzip2
      sfklib2\bzip2\huffman.c        bzip2
      sfklib2\bzip2\randtable.c      bzip2
      sfklib2\bzip2\compress.c       bzip2
      sfklib2\bzip2\decompress.c     bzip2
      sfklib2\bzip2\bzlib.c          bzip2
      "

   +filter -no-empty-lines

   +perline -yes
      "call addfile sfkpic-tmp.c #text"

   +then list -size -time sfkpic-tmp.c

   +then call makecpp

   +then list -size -time sfkpic.cpp

   +end

sfk label addfile

   +setvar outfile=%1
   +setvar infile=%2
   +setvar handle=%3

   +if "#(sys.slash) = /" begin
      +getvar infile +xed "_\\_/_" +setvar infile
      +endif

   +echo "
      /* 
      :file #(infile) 
      */
      "
      +tofile -append #(outfile)

   +xed #(infile)
      "_/\*\*/__"
      "_/\*_\x10_" "_\*/_\x11_"

   // +xed
   //    "_\x10[1.10000 bytes not \x11]copyright[1.10000 bytes not \x10]\x11_[all]_"
   //    "_\x10[1.10000 bytes]\x11__"

   +xed
      "_\x10_/\*_" "_\x11_\*/_"

   +xed
      "_[lstart]METHODDEF*\r\n[ortext]\n
       _[parts 1-3] _"
      "_#define ONE**MULTIPLY16V16**#endif__"
      +filter -no-empty-lines

   +xed -case
      "/ clc_/ liclc_/"
      "/encode_mcu_DC_first/[part1]#(cnt)/"
      "/encode_mcu_AC_first/[part1]#(cnt)/"
      "/encode_mcu_DC_refine/[part1]#(cnt)/"
      "/encode_mcu_AC_refine/[part1]#(cnt)/"
      "/emit_byte/[part1]#(cnt)/"
      "/my_coef_controller/[part1]#(cnt)/"
      "/my_coef_ptr/[part1]#(cnt)/"
      "/arith_entropy_ptr/[part1]#(cnt)/"
      "/start_iMCU_row/[part1]#(cnt)/"
      "/start_pass_coef/[part1]#(cnt)/"
      "/compress_output/[part1]#(cnt)/"
      "/decode_mcu_DC_first/[part1]#(cnt)/"
      "/decode_mcu_DC_refine/[part1]#(cnt)/"
      "/decode_mcu_AC_first/[part1]#(cnt)/"
      "/decode_mcu_AC_refine/[part1]#(cnt)/"
      "/savable_state/[part1]#(cnt)/"
      "/huff_entropy_ptr/[part1]#(cnt)/"
      "/process_restart/[part1]#(cnt)/"
      "/initial_setup/[part1]#(cnt)/"
      "/per_scan_setup/[part1]#(cnt)/"
      "/my_main_controller/[part1]#(cnt)/"
      "/my_main_ptr/[part1]#(cnt)/"
      "/process_data_simple_main/[part1]#(cnt)/"
      "/my_marker_ptr/[part1]#(cnt)/"
      "/my_master_ptr/[part1]#(cnt)/"
      "/my_cquantizer/[part1]#(cnt)/"
      "/my_cconvert_ptr/[part1]#(cnt)/"
      "/my_upsampler/[part1]#(cnt)/"
      "/my_upsample_ptr/[part1]#(cnt)/"
      "/my_cquantize_ptr/[part1]#(cnt)/"
      "/build_ycc_rgb_table/[part1]#(cnt)/"
      "/grayscale_convert/[part1]#(cnt)/"
      "/null_convert/[part1]#(cnt)/"

      "/METHODDEF(void) start_pass_main\x20
       /void start_pass_main#(cnt) /"
      "/= start_pass_main;
       /= start_pass_main#(cnt);/"

      "/METHODDEF(void) start_pass\x20
       /void start_pass#(cnt) /"
      "/= start_pass;
       /= start_pass#(cnt);/"

      "/METHODDEF(void) start_input_pass\x20
       /void start_input_pass#(cnt) /"
      "/= start_input_pass;
       /= start_input_pass#(cnt);/"
      "/start_input_pass(cinfo);
       /start_input_pass#(cnt)(cinfo);/"

      "/METHODDEF(boolean) decode_mcu\x20
       /boolean decode_mcu#(cnt) /"
      "/= decode_mcu;
       /= decode_mcu#(cnt);/"

      "/#define CONST_BITS *
       /#undef CONST_BITS\n
        [all]\n
        #undef ONE\n
        #undef CONST_SCALE\n
        #undef FIX\n
        #undef DESCALE\n
        #undef MULTIPLY16C16\n
        #undef MULTIPLY16V16\n
        #define ONE ((INT32) 1)\n
        #define CONST_SCALE (ONE << CONST_BITS)\n
        #define FIX(x) ((INT32) ((x) * CONST_SCALE + 0.5))\n
        #define DESCALE(x,n)  RIGHT_SHIFT((x) + (ONE << ((n)-1)), n)\n
        #ifdef SHORTxSHORT_32\n
         #define MULTIPLY16C16(var,const)  (((INT16) (var)) * ((INT16) (const)))\n
        #endif\n
        #ifdef SHORTxLCONST_32\n
         #define MULTIPLY16C16(var,const)  (((INT16) (var)) * ((INT32) (const)))\n
        #endif\n
        #ifndef MULTIPLY16C16\n
         #define MULTIPLY16C16(var,const)  ((var) * (const))\n
        #endif\n
        #ifdef SHORTxSHORT_32\n
         #define MULTIPLY16V16(var1,var2)  (((INT16) (var1)) * ((INT16) (var2)))\n
        #endif\n
        #ifndef MULTIPLY16V16\n
         #define MULTIPLY16V16(var1,var2)  ((var1) * (var2))\n
        #endif\n/"
     "/[char not a-z0-9_]MULTIPLY[char not a-z0-9_]/[parts 1,2]#(cnt)[part3]/"
     "/[char not a-z0-9_]DEQUANTIZE[char not a-z0-9_]/[parts 1,2]#(cnt)[part3]/"
     "/[char not a-z0-9_]FIX_0_541196100[char not a-z0-9_]/[parts 1,2]#(cnt)[part3]/"
     "/[char not a-z0-9_]FIX_1_847759065[char not a-z0-9_]/[parts 1,2]#(cnt)[part3]/"
     "/const char deflate_/const char zldef01_/"
     "/const char inflate_/const char zlinf01_/"

     "/zlcrc32/crc32/"

     "/BZ2_bzCompressInit/sp_BZ2_bzCompressInit/"
     "/BZ2_bzCompress/sp_BZ2_bzCompress/"
     "/BZ2_bzCompressEnd/sp_BZ2_bzCompressEnd/"
     "/BZ2_bzDecompressInit/sp_BZ2_bzDecompressInit/"
     "/BZ2_bzDecompress/sp_BZ2_bzDecompress/"
     "/BZ2_bzDecompressEnd/sp_BZ2_bzDecompressEnd/"
     "/BZ2_bzReadOpen/sp_BZ2_bzReadOpen/"
     "/BZ2_bzReadClose/sp_BZ2_bzReadClose/"
     "/BZ2_bzReadGetUnused/sp_BZ2_bzReadGetUnused/"
     "/BZ2_bzRead/sp_BZ2_bzRead/"
     "/BZ2_bzWriteOpen/sp_BZ2_bzWriteOpen/"
     "/BZ2_bzWrite/sp_BZ2_bzWrite/"
     "/BZ2_bzWriteClose/sp_BZ2_bzWriteClose/"
     "/BZ2_bzWriteClose64/sp_BZ2_bzWriteClose64/"
     "/BZ2_bzBuffToBuffCompress/sp_BZ2_bzBuffToBuffCompress/"
     "/BZ2_bzBuffToBuffDecompress/sp_BZ2_bzBuffToBuffDecompress/"
     "/BZ2_bzlibVersion/sp_BZ2_bzlibVersion/"
     "/BZ2_bzopen/sp_BZ2_bzopen/"
     "/BZ2_bzdopen/sp_BZ2_bzdopen/"
     "/BZ2_bzread/sp_BZ2_bzread/"
     "/BZ2_bzwrite/sp_BZ2_bzwrite/"
     "/BZ2_bzflush/sp_BZ2_bzflush/"
     "/BZ2_bzclose/sp_BZ2_bzclose/"
     "/BZ2_bzerror/sp_BZ2_bzerror/"

     "/#ifdef unix || __APPLE__/#ifdef skip_unused_func/"

     // disable crashing junk outputs
     "/METHODDEF(void) output_message (j_common_ptr cinfo)[eol]{
      /[all] return;/"
     "/METHODDEF(void) emit_message (j_common_ptr cinfo, int msg_level)[eol]{
      /[all] return;/"
     "/METHODDEF(void) format_message (j_common_ptr cinfo, char * buffer)[eol]{
      /[all] return;/"

   +tif "#(handle) <> " tcall "#(handle)"

   +xed -case
      "_#*include *[eol]__"
      +tofile -append %1

   +then calc "#(cnt)+1" +setvar cnt

   +end

sfk label pngmem
   +xed -case
      "/return(png_memcpy (s1, s2, size));
       /png_memcpy (s1, s2, size);\n   return 0;/"
      "/return (png_memset (s1, value, size));
       /png_memset (s1, value, size);\n   return 0;/"
   +tend

sfk label zlcrc32
   +xed -case
      "/DO1/DO1crc/"
      "/DO8/DO8crc/"
      "/define TBLS/define TBLS2/"
   +tend

sfk label zlinflate
   +xed -case
      "/fixedtables/fxtablesinf/"
      "/PULLBYTE/PULLBYTE2/"
      "/NEEDBITS/NEEDBITS2/"
   +tend

sfk label zltrees
   +xed -case
     "/[char not a-z0-9_]code[char not a-z0-9_]
      /[part1]foocode[part3]/"
   +tend

sfk label jdmarker
   +xed -case
     "/typedef enum**JPEG_MARKER;//"
   +tend

sfk label jpfix
   +xed -case
     "/[char not a-z0-9_]FIX[char not a-z0-9_]/[parts 1,2]#(cnt)[part3]/"
   +tend

sfk label jquant2
   +xed -case
      "/typedef INT16 FSERROR;//"
      "/typedef int LOCFSERROR;//"
      "/typedef INT32 FSERROR;//"
      "/typedef INT32 LOCFSERROR;//"
      "/typedef FSERROR FAR *FSERRPTR;//"
   +tend

sfk label jerror
   +xed -case
      "/#include \qjerror.h\q*[eol]
        NULL*[eol]
       /[getvar msgtab]\nNULL\n/"
   +tend

sfk label mzioapih
   +xed -case
      "/#if (_MSC_VER >= 1400) && (!(defined(NO_MSCVER_FILE64_FUNC)))*
       /#if 1/"
   +tend

sfk label miniziph
   +xed -case
      "/#define _zip12_H
       /#define _zip12_H\n
        #ifdef _WIN32\n
        void fill_win32_filefunc OF((zlib_filefunc_def* pzlib_filefunc_def));\n
        void fill_win32_filefunc64 OF((zlib_filefunc64_def* pzlib_filefunc_def));\n
        void fill_win32_filefunc64A OF((zlib_filefunc64_def* pzlib_filefunc_def));\n
        void fill_win32_filefunc64W OF((zlib_filefunc64_def* pzlib_filefunc_def));\n
        #endif\n
       /"
   +tend

sfk label zipc
   +xed -case
      "/crc32(zi->ci.crc32,buf,(uInt)len)
       /crc32(zi->ci.crc32,(const Bytef *)buf,(uInt)len)/"
      "/zi->ci.bstream.next_in = (void\*)buf;
       /zi->ci.bstream.next_in = (char *)buf;/"
   +tend

sfk label unzipc
   +xed -case
      "/pfile_in_zip_read_info->stream.next_in = (voidpf)0;
       /pfile_in_zip_read_info->stream.next_in = (Bytef *)0;/"
   +tend

sfk label minizipc
   +xed -case
      "/#define USEWIN32IOAPI
       /#define USEWIN32IOAPI_NOT/"
      "/crc32(calculate_crc,buf,size_read)
       /crc32(calculate_crc,(const Bytef *)buf,size_read)/"
      "/filetime(filenameinzip,
       /filetime((char*)filenameinzip,/"
      "/int main(argc,
       /#ifdef TESTZIP\nint main(argc,/"
      "/[end]/#endif\n/"
   +tend

sfk label miniunzc
   +xed -case
      "/#define USEWIN32IOAPI
       /#define USEWIN32IOAPI_NOT/"
      "/WRITEBUFFERSIZE/WRITEBUFFERSIZE2/"
      "/do_banner/do_banner2/"
      "/do_help/do_help2/"
      "/int main(argc,
       /#ifdef TESTUNZIP\nint main(argc,/"
      "/[end]/#endif\n/"
      "/makedir(write_filename)
       /makedir((char*)write_filename)/"
   +tend

sfk label bzip2
   +xed -case

   "_Int32 rTPos                                 \\
    _Int32 rTPos_"

   "_s->rTPos  = 0                               \\
    _s->rTPos  = 0_"

   "xcost45 += s->len_pack\[icv\]\[2\]; \\
    xcost45 += s->len_pack\[icv\]\[2\];\n
    x"

   "/s->ll16 = BZALLOC(
    /s->ll16 = (UInt16*)BZALLOC(/"

   "/s->ll4  = BZALLOC(
    /s->ll4  = (UChar*)BZALLOC(/"

   "/s->tt  = BZALLOC(
    /s->tt  = (UInt32*)BZALLOC(/"

   "/s = BZALLOC( sizeof(EState) );
    /s = (EState*)BZALLOC( sizeof(EState) );/"

   "/s->arr1 = BZALLOC(
    /s->arr1 = (UInt32*)BZALLOC(/"

   "/s->arr2 = BZALLOC(
    /s->arr2 = (UInt32*)BZALLOC(/"

   "/s->ftab = BZALLOC(
    /s->ftab = (UInt32*)BZALLOC(/"

   "/s = BZALLOC( sizeof(DState) );
    /s = (DState*)BZALLOC( sizeof(DState) );/"

   "/EState* s = strm->state;
    /EState* s = (EState*)strm->state;
    /"

   "/ EState\* s;[1.100 bytes]s = strm->state;
    / EState\* s;[part2]s = (EState \*)strm->state;
    /"

   "/ DState\* s;[1.100 bytes]s = strm->state;
    / DState\* s;[part2]s = (DState \*)strm->state;
    /"

   "/small[1 char not (a-zA-Z)]/bsmall[part2]/"

   "/#define GET_BITS
    /#undef GET_BITS\n
     #define GET_BITS/"

   +tend

sfk label makecpp

   +filter sfkpic-tmp.c -rtrim

   +xed

      "/int lcodes, dcodes, blcodes;
       /int lcodes;\nint dcodes;\nint blcodes;/"

      "/nextbuffer = malloc(nextsize);
       /nextbuffer = (JOCTET *)malloc(nextsize);/"

      "/dest->newbuffer = *outbuffer = malloc(OUTPUT_BUF_SIZE);
       /dest->newbuffer = *outbuffer = (unsigned char*)malloc(OUTPUT_BUF_SIZE);/"

   +xed

      "/this/that/"

      "
       /[lstart]local[ortext]void[ortext]int *(*)[eol]
         [white]*;*[eol]
         {
       /[parts 2-3]
        ([part9]) {\n
       /
      "

      "
       /[lstart]local[ortext]void[ortext]int *(*)[eol]
         [white]*;*[eol]
         [white]*;*[eol]
         {
       /[parts 2-3]
        ([part9], [part14]) {\n
       /
      "

      "
       /[lstart]local[ortext]void[ortext]int *(*)[eol]
         [white]*;*[eol]
         [white]*;*[eol]
         [white]*;*[eol]
         {
       /[parts 2-3]
        ([part9], [part14], [part19]) {\n
       /
      "

      "
       /[lstart]local[ortext]void[ortext]int *(*)[eol]
         [white]*;*[eol]
         [white]*;*[eol]
         [white]*;*[eol]
         [white]*;*[eol]
         {
       /[parts 2-3]
        ([part9], [part14], [part19], [part24]) {\n
       /
      "

      "
       /[lstart]local[ortext]void[ortext]int *(*)[eol]
         [white]*;*[eol]
         [white]*;*[eol]
         [white]*;*[eol]
         [white]*;*[eol]
         [white]*;*[eol]
         {
       /[parts 2-3]
        ([part9], [part14], [part19], [part24], [part29]) {\n
       /
      "

      "
       /[lstart]local[ortext]void[ortext]int *(*)[eol]
         [white]*;*[eol]
         [white]*;*[eol]
         [white]*;*[eol]
         [white]*;*[eol]
         [white]*;*[eol]
         [white]*;*[eol]
         {
       /[parts 2-3]
        ([part9], [part14], [part19], [part24], [part29], [part34]) {\n
       /
      "

      "
       /[lstart]* zexport *(*)[eol]
         [white]*;[eol]
         {
       /[parts 2-4]
        ([part10]) {\n
       /
      "

      "
       /[lstart]* zexport *(*)[eol]
         [white]*;[eol]
         [white]*;[eol]
         {
       /[parts 2-4]
        ([part10], [part14]) {\n
       /
      "

      "
       /[lstart]* zexport *(*)[white][eol]
         [white]*;[eol]
         [white]*;[eol]
         [white]*;[eol]
         {
       /[parts 2-4]
        ([part11], [part15], [part19]) {\n
       /
      "

      "
       /[lstart]* zexport *(*)[eol]
         [white]*;[eol]
         [white]*;[eol]
         [white]*;[eol]
         [white]*;[eol]
         {
       /[parts 2-4]
        ([part10], [part14], [part18], [part22]) {\n
       /
      "
 
      "
       /[lstart]* zexport *(*)[eol]
         [white]*;[eol]
         [white]*;[eol]
         [white]*;[eol]
         [white]*;[eol]
         [white]*;[eol]
         {
       /[parts 2-4]
        ([part10], [part14], [part18], [part22], [part26]) {\n
       /
      "

      "
       /[lstart]* zexport *(**)[eol]
         [white]*;[eol]
         [white]*;[eol]
         [white]*;[eol]
         [white]*;[eol]
         [white]*;[eol]
         [white]*;[eol]
         [white]*;[eol]
         [white]*;[eol]
         {
       /[parts 2-4]
        ([part10], [part14], [part18], [part22], [part26],
         [part30], [part34], [part38]) {\n
       /
      "

      "/uLong filetime(f, tmzip, dt)**{
       /uLong filetime(char *f, tm_zip *tmzip, uLong *dt)\n{/"

   +xed 
      "_[eol][eol][eol][eol][eol]_[parts 1,2]_"
      "_[eol][eol][eol][eol]_[parts 1,2]_"
      "_[eol][eol][eol]_[parts 1,2]_"

      -tofile sfkpic.cpp

   +then filter sfkpackio.cpp +tofile -append sfkpic.cpp
   +then filter sfkpicio.cpp  +tofile -append sfkpic.cpp

   +end

:xend
