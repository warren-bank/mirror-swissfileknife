/*
   swiss file knife

   known issues:
   -  syncto: no empty target files supported yet.
   -  not every option can be specified everywhere.
   -  patch, inst: option -keep-dates only works
      with Win32 yet (uses xcopy command).
   -  unix handling of files >2GB and timestamps >2038
      is not yet tested and may require further adaptions.
   -  ntfs handling of links is not yet tested.
   -  unix bash interprets $, ! and probably other chars.

   1.1.2
   -  add: linux: run: using # instead of $ due to bash.
   -  add: linux: all: using : instead of ! due to bash.
      in general, sfk for linux is still alpha.
   -  fix: freezeto: wrong warning that files are not
      contained in archive if starting with ./
   -  add: sfk simple tcp file server, get, put.
   -  add: sfk md5 for quick single file check.
   -  fix: patch: changed error message on pattern mismatch,
      now telling the real from block number.
   -  add: sfk filter may now also read from file i/o stdin.
   -  add: sfk deblank, replace blanks in filenames.
   -  fix: sview usability fixes, see sview.cpp.
   -  add: simple ftp server, ftp client.
   -  fix: md5check linux path char compatibility.
   -  fix: linux: patch function, inst function port.

   1.1.1
   -  fix: massive linux reworks. no longer following
      symbolic links. some functions unusable so far
      now have at least alpha status.
   -  chg: complete rework of mirrorto backup function,
      which is now renamed to "freezeto", as it is
      primarily intended to create zipped archives.
      -  chg: per-dir zip names changed from 01-arc.zip
         to 01-arc-part.zip for better uniqueness.
         If you used mirrorto previously, cleanup your
         archive tree directories before next use!
      -  add: checksums of copied files now also included
         in md5-arc.txt, to ensure everything's covered
         in verification batch.
      -  fix: freezeto: 09-freeze-log contained only last error,
         now appending all zip messages by double ">".
      -  add: freezeto: local zip header scan while building
         checksums, to verify that files were really added.
   -  fix: diverse sview fixes, see sview.cpp.
   -  add: sfk list: options -time, -size, -size=digits.
   -  fix: now really using 64 bit timestamps with msvc,
      which requires the latest visual compiler.
      added compiler version check, and 32 bit warning.
   -  fix: minor: cleanup of parameter handling.
   -  fix: compile: variable scope in execDirFreeze

   1.1.0
   -  add: sfk strings: extract strings from binary file.
   -  fix: sfk patch: now also supporting target filenames
      without any path contained. this obsoletes the error msg:
      "no path character in (null), cannot create backup dir."
   -  add: sview: shift+lmouse to copy word and line combinations
      to clipboard.
   -  add: manual.html documentation

   1.0.8
   -  fix: mirrorto: dir max time comparison switched off.
      the file time is not a sufficient criteria for backup.
      if user installed old files from an archive, he wants
      them being backed up as well, although dir maxtime
      is not changed. therefore we now always call zip.
   -  fix: mirrorto: empty dirs: always calling zip, copy.
   -  fix: dir time maximum building. some dirs were listed
      as being newer then they are. fixed by resetting the
      local maxtime after every execSingleDir use.
   -  fix: win: now reading file sizes>2GB using _stati64.
      fix: win: now reading time stamps>2038 using _stati64.
      this requires non-portable 64-bit calls, therefore
      these issues may still be unsolved under unix.
   -  add: snapto: option -wrap=n to auto-wrap long lines.
   -  add: snap file format: ,wrap=n in header line.
   -  fix: combined paths now w/o double path char.

   1.0.7
   -  FIX: parallel write of editor and read of sfk on the
      same cluster file led to sync-break of sfk. now, sfk
      searches for ":cluster-end", and retries until complete.
   -  syncto:  cluster format revision 1.0.7, obsoleting 1.0
   -  snapto: snapfile format revision 1.0.7, obsoleting 1.0
   -  add: sfk snapview tool, windows realtime text browser.
   -  fix: syncto: notes section now written only on demand
   -  fix: snapfile header now listing full [l]prefix,
           to give snapview better infos about the format.

   1.0.6
   -  add: syncto: free text notes section
   -  fix: now using 64 bit for sizes and time stamps.
   -  fix: files like base.and.type are now treated as having 
      extension ".type" instead of ".and.type".
   -  fix: now issuing error if reflist called with >2 root dirs
   -  add: run with only $path supplied now runs on dirs only
   -  add: full support for remark lines in -from files
   -  add: general options can now be specified everywhere.
   -  add: md5check: fast spot checking by -skip option.
   -  fix: md5gen files now more similar to md5sum.
      md5sum: \671032bb3eb7877491df816f592472f4 *testfiles\\readme.txt
      sfk   :  671032bb3eb7877491df816f592472f4 *testfiles\readme.txt
      sfk will not write the same slash format, however, windows' cygwin
      md5sum is now able of verifying sfk md5 files, and vice versa.
   -  add: sfk run with quoted and unquoted expressions.
   -  add: sfk run -ifiles, -idirs.
   -  add: general -i option, yet listed for "stat" only.

   1.0.5
   -  fix: directory mask handling: for example, on
         -dir a1 a2 a3 !m1 -dir b1 b2 b3 !m2
      you expect that negative mask m1 is applied
      just to dir trees a1, a2, a3. actually !m1
      was applied to every dir tree (also b1 b2 b3).
      e.g. this can cause "mirrorto" to exclude too many files.
      fixed with 1.0.5, upgrade highly recommended for everyone.

   1.0.4
   -  add: inst: c++ source instrumentation support

   1.0.3
   -  fix: mapto: line number calculation
   -  add: run: support for $path $base $ext
   -  fix: twinfilescanner: mem leak, added shutdown
   -  add: general prefix options support

   1.0.2
   -  fix: minor fixes concerning parameter check
   -  fix: autocompletion of dir lists: adding empty command
   -  added: addhead, addtail w/ optional -noblank
   -  added: -norec option to avoid dir recursion
   -  fix: commands w/o any parms now get default dir "."
   -  fix: commands with just -file specified now get default dir "."
   -  added: mapto support.
   -  fix: filter: rework of positive/negative filter matching.
   -  added: command sfk run
   -  added: crlf conversion functions

   1.0.1
   -  pszGlblClusterFileStamp now written correctly
   -  basic unix support (path chars), short test of snapto, syncto
   -  compile fixes (multiple symbols in .cpp's)
   -  fix: snapfiles now also excluding clusters from input.
   -  fix: syncto: stopped if target files were deleted.
      such targets are now dropped from snapshots.
   -  fix: syncto: stopped if file was added containing
      unallowed keywords. now such files are ignored.
   -  exclude output file from input fileset
   -  auto-remove dot slash on filenames, dirnames
   -  support for -quiet option on md5check
   -  snapto default prefix now :file
   -  detect recursive collection of file clusters, exclude content
   -  execSnapAdd now continues after excluded files
*/

// NOTE: if you change the source and create your own derivate,
// fill in the following infos before releasing your version of sfk.
#define SFK_BRANCH   "yourlabel"
#define SFK_VERSION  "yourversion"
#define SFK_PROVIDER "yourname"

// in case of linking problems concerning libsocket etc.,
// you may out-comment this to compile without tcp support:
#define WITH_TCP

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <assert.h>
#include <time.h>
#include <string.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
  #include <windows.h>
  #include <eh.h>
  #include <sys/timeb.h>
  #include <time.h>
  #include <process.h>
  #define getpid() _getpid()
  #include <errno.h>
  #include <direct.h>
  #include <io.h>
  #ifndef socklen_t
   #define socklen_t int
  #endif
#else
  #include <unistd.h>
  #include <dirent.h>
  #include <sys/stat.h>
  #include <netinet/in.h>
  #include <sys/socket.h>
  #include <sys/ioctl.h>
  #include <sys/time.h>
  #include <arpa/inet.h>
  #include <netdb.h>
  #include <errno.h>
  #include <sched.h>
  #ifndef  _S_IFDIR
   #define _S_IFDIR 0040000 // = 0x4000
  #endif
  #ifndef  _S_IREAD
   #ifdef S_IREAD
    #define _S_IREAD  S_IREAD
    #define _S_IWRITE S_IWRITE
   #else
    #define _S_IREAD  __S_IREAD  // by owner
   #define  _S_IWRITE __S_IWRITE // by owner
   #endif
  #endif
  typedef int SOCKET;
  #define INVALID_SOCKET -1
  #define SOCKET_ERROR   -1
  #define closesocket(n) close(n)
  #define ioctlsocket ioctl
  #define WSAEWOULDBLOCK EWOULDBLOCK
#endif

#define uchar unsigned char
#define ulong unsigned long
#define bool  unsigned char

#define WITH_FN_INST
#define SFK_BOTH_RUNCHARS

#ifdef _WIN32
 #include "memdeb.cpp"
#endif

#include "sfkmd5.cpp"
#include "binpack.cpp"

#if defined(_WIN32) && defined(SFK_WINPOPUP_SUPPORT)
 #include "winpop.cpp"
#endif

#ifdef _WIN32
static const char  glblPathChar    = '\\';
static const char  glblWrongPChar  = '/';
static const char *glblPathStr     = "\\";
static const char *glblAddWildCard = "*";
static const char *glblDotSlash    = ".\\";
static const char  glblNotChar     = '!';
static const char  glblRunChar     = '$';
#define SFK_FILT_NOT1 "-!"
#define SFK_FILT_NOT2 "-ls!"
#define EXE_EXT ".exe"
#else
static const char  glblPathChar    = '/';
static const char  glblWrongPChar  = '\\';
static const char *glblPathStr     = "/";
static const char *glblAddWildCard = "";
static const char *glblDotSlash    = "./";
static const char  glblNotChar     = ':';
static const char  glblRunChar     = '#';
#define SFK_FILT_NOT1 "-:"
#define SFK_FILT_NOT2 "-ls:"
#define EXE_EXT ""
#endif

// ========== 64 bit abstraction layer begin ==========

#ifdef _WIN32
 #if _MSC_VER >= 1310
  #define SFK_W64
 #endif
#endif

#ifdef _WIN32
 typedef __int64 num;
 #ifdef SFK_W64
 typedef __time64_t mytime_t;
 #else
 typedef time_t mytime_t;
 #endif
#else
 typedef long long num;
 typedef time_t mytime_t;
#endif

static char *numtostr(num n, int nDigits, char *pszBuf, int nRadix)
{
   static char szBuf[100];
   if (!pszBuf)
        pszBuf = szBuf;

   #ifdef _WIN32
   if (nRadix == 10)
      sprintf(pszBuf, "%0*I64d", nDigits, n);
   else
      sprintf(pszBuf, "%0*I64x", nDigits, n);
   return pszBuf;
   #else
   if (nRadix == 10)
      sprintf(pszBuf, "%0*lld", nDigits, n);
   else
      sprintf(pszBuf, "%0*llx", nDigits, n);
   return pszBuf;
   #endif
}

static char *numtoa_blank(num n, int nDigits=12)
{
   static char szBuf2[100];
   #ifdef _WIN32
   sprintf(szBuf2, "% *I64d", nDigits, n);
   return szBuf2;
   #else
   sprintf(szBuf2, "% *lld", nDigits, n);
   return szBuf2;
   #endif
}

char *numtoa(num n, int nDigits=1, char *pszBuf=0) {
   return numtostr(n, nDigits, pszBuf, 10);
}

char *numtohex(num n, int nDigits=1, char *pszBuf=0) {
   return numtostr(n, nDigits, pszBuf, 0x10);
}

num atonum(char *psz)
{
   #ifdef _WIN32
   return _atoi64(psz);
   #else
   return atoll(psz);
   #endif
}

mytime_t getSystemTime()
{
   static mytime_t stSysTime = 0;
   #ifdef SFK_W64
   return _time64(&stSysTime);
   #else
   return time(&stSysTime);
   #endif
}

// ========== 64 bit abstraction layer end ============

void doSleep(long nmsec) 
{
   #ifdef _WIN32
   Sleep(nmsec);
   #else
   sleep(1);
   #endif
}

enum eWalkTreeFuncs {
   eFunc_MD5Write = 1,
   eFunc_JamFile     ,
   eFunc_Detab       ,
   eFunc_JamIndex    ,
   eFunc_SnapAdd     ,
   eFunc_BinPatch    ,
   eFunc_FileStat    ,
   eFunc_Grep        ,
   eFunc_Mirror      ,
   eFunc_Run         ,
   eFunc_FormConv    ,
   eFunc_Inst        ,
   eFunc_RefColDst   ,  // collect reflist targets
   eFunc_RefProcSrc  ,  // process reflist sources
   eFunc_Deblank     ,
   #ifdef WITH_TCP
   eFunc_FTPList     ,
   #endif
};

enum eConvTargetFormats {
   eConvFormat_LF     = 1,
   eConvFormat_CRLF   = 2
};

long bGlblDebug    = 0;
long nGlblVerbose  = 0; // 0, 1, 2
long nGlblFunc     = 0;
long nGlblFiles    = 0;
long nGlblNoFiles  = 0; // fnames that failed to stat etc.
long nGlblLines    = 0;
long nGlblTabSize  = 0;
long nGlblTabsDone = 0;
long nGlblTabFiles = 0;
long bGlblScanTabs = 0;
char *pszGlblOutFile = 0;  // if set, some funcs will take care not to read this file
FILE *fGlblOut     = 0; // general use
FILE *fGlblCont    = 0; // freezeto: content list
FILE *fGlblMD5Arc  = 0;
FILE *fGlblMD5Org  = 0;
FILE *fGlblBatch   = 0;
FILE *fGlblTimes   = 0;
unsigned char abBuf[100000]; // must be far greater than szLineBuf
#define MAX_LINE_LEN 4096
char szLineBuf[MAX_LINE_LEN+10];
char szLineBuf2[MAX_LINE_LEN+10];
char *pszGlblJamPrefix = 0;
char *pszGlblJamRoot = 0;
bool  bGlblJamPure = 0;
#define MAX_JAM_TARGETS 1000
char *apJamTargets[MAX_JAM_TARGETS];
num   alJamTargetTime[MAX_JAM_TARGETS];
num   nJamSnapTime = -1;
long  nJamTargets  = 0;
char *pszGlblRepSrc = 0;
char *pszGlblRepDst = 0;
char *pszGlblBlank =
   "                                                "
   "                                                ";
num  nGlblStartTime = 0;
long nGlblListMinSize = 0; // in mbytes
long nGlblListMode = 0;    // 1==stat 2==list
ulong nGlblListForm = 0;
int  nGlblListDigits = 12;
bool bGlblEscape = 0;
char *pszGlblDstRoot = 0;
num nGlblBytes = 0;
char *pszGlblDirTimes = 0;
char *pszGlblTurn = "\\|/-";
long  nGlblTurnCnt = 0;
bool  bGlblQuiet = 0; // do not show file count while processing
bool  bGlblSim   = 0; // just simulate command
bool  bGlblRecurseDirs = 1;
char *pszGlblRunCmd = "";
bool  bGlblWalkJustDirs = false;
ulong nGlblConvTarget = 0; // see eConvTargetFormats
#ifdef WITH_FN_INST
bool  bGlblInstRevoke = 0;
bool  bGlblInstRedo   = 0;
char *pszGlblInstInc  = "";
char *pszGlblInstMac  = "";
static bool bGlblTouchOnRevoke = 1;
#endif
char *pszGlblSnapFileStamp    = ":snapfile sfk,1.0.7,lprefix=";
char *pszGlblClusterFileStamp = ":cluster sfk,1.0.7,prefix=:";
bool  bGlblRefRelCmp    = 1;
bool  bGlblRefBaseCmp   = 0;
bool  bGlblRefWideInfo  = 0;
long  nGlblRefMaxSrc    = 10;
bool  bGlblStdInAny     = 0;  // all cmd except run: take list from stdin
bool  bGlblStdInFiles   = 0;  // run only: take filename list from stdin
bool  bGlblStdInDirs    = 0;  // run only: take directory list from stdin
long  nGlblMD5Skip      = 0;
long  nGlblSnapWrap     = 0;  // if >0, auto-wrap lines in snapfile
bool  bGlblMirrorByDate = 0;  // inofficial, might be removed.
bool  bGlblHiddenFiles  = 0;  // include also system and hidden files
char *pszGlblXCopyCmd   = 0;
char *pszGlblZipCmd     = 0;
char *pszGlblUnzipCmd   = 0;
char *pszGlblSFKCmd     = 0;
long nGlblZipVersionHi  = 0;
long nGlblZipVersionLo  = 0;
long nGlblUnzipVersionHi = 0;
long nGlblUnzipVersionLo = 0;
long nGlblTCPMaxSizeMB   = 500;
bool bGlblYes            = 0;
#ifdef WITH_TCP
SOCKET hGlblTCPOutSocket = 0;
bool bGlblFTPReadWrite   = 0;
#endif

long nGlblFzMisArcFiles = 0;
long nGlblFzConArcFiles = 0;
long nGlblFzConArchives = 0;
long nGlblFzMisCopFiles = 0;
long nGlblFzConCopFiles = 0;

#define _ { printf("[%d]\n",__LINE__); fflush(stdout); }

class FileList;

long execDetab       (char *pszFile);
long execDirMirror   (char *pszName, long  lLevel, FileList &oDirFiles, num &ntime1, num &ntime2);
long execFileMirror  (char *pszFile, num &ntime1, num &ntime2, long nDirFileCnt);
long execDirXCopy    (char *pszName, long  lLevel, FileList &oDirFiles, num &ntime1, num &ntime2);
long execFileXCopy   (char *pszFile, num &ntime1, num &ntime2, long nDirFileCnt);
long execDirFreeze   (char *pszName, long  lLevel, FileList &oDirFiles, num &ntime1, num &ntime2);
long execFileFreeze  (char *pszFile, num &ntime1, num &ntime2, long nDirFileCnt);
long getFileMD5      (char *pszFile, SFKMD5 &md5, bool bSilent=0);
long execFormConv    (char *pszFile);
long walkFiles       (char *pszIn  , long lLevel, long &nGlobFiles, FileList &oDirFiles, long &lDirs, num &lBytes, num &ntime1, num &ntime2);
long execSingleFile  (char *pszFile, long lLevel, long &lGlobFiles, long nDirFileCnt, long &lDirs, num &lBytes, num &ntime1, num &ntime2);
long execSingleDir   (char *pszName, long lLevel, long &lGlobFiles, FileList &oDirFiles, long &lDirs, num &lBytes, num &ntime1, num &ntime2);
num  getFileSize     (char *pszName);

#ifndef _WIN32
char *readDirEntry(DIR *d) {
  struct dirent *e;
  e = readdir(d);
  return e == NULL ? (char *) NULL : e->d_name;
}
#endif

// uses szLineBuf:
num getOldDirTime(char *pszName) 
{
   // printf("gdt %s\n",pszName);
   if (!pszGlblDirTimes)
      return 0;
   // direct search of entry in text file
   sprintf(szLineBuf, " %s\r", pszName);
   char *psz1 = strstr(pszGlblDirTimes, szLineBuf);
   if (!psz1)
      return 0;
   // step back to start of line, with timestamp
   while ((psz1 > pszGlblDirTimes) && (*psz1 != '\n'))
      psz1--;
   // skip LF, if not in very first line
   if (psz1 > pszGlblDirTimes)
      psz1++;
   num nTime = atonum(psz1);
   // printf("-> TIME %u\n",nTime);
   return nTime;
}

bool equalFileName(char *psz1, char *psz2) {
   #ifdef _WIN32
   // Windows: expect case-insensitive filenames
   return (!_stricmp(psz1, psz2)) ? 1 : 0;
   #else
   // Unix: expect case-sensitive filenames
   return (!strcmp(psz1, psz2)) ? 1 : 0;
   #endif
}

class InfoCounter {
public:
   InfoCounter   ( );
   ulong count   ( );  // RC > 0 says print the counter now
   ulong value   ( ) { return nClUnits; }
private:
   ulong nClUnits;
   ulong nClLastTold;
   ulong nClTellSteps;
};

InfoCounter::InfoCounter() {
   nClUnits     = 0;
   nClLastTold  = 0;
   nClTellSteps = 1;
}

ulong InfoCounter::count() {
   nClUnits++;
   if (nClUnits >= (nClLastTold+nClTellSteps)) {
      nClLastTold = nClUnits;
      nClTellSteps++;
      return nClUnits;
   }
   return 0;
}

InfoCounter glblFileCount;

class StringTable {
friend class Array;
public:
   StringTable          ( );
  ~StringTable          ( );
   long addEntry        (char *psz);
   long numberOfEntries ( );
   char *getEntry       (long iIndex, int nTraceLine);
   void resetEntries    ( );
   bool isSet           (long iIndex);
   void dump            (long nIndent=0);
private:
   long addEntryPrefixed(char *psz, char cPrefix);
   long setEntryPrefixed(long iIndex, char *psz, char cPrefix);
   long expand          (long nSoMuch);
   long nClArraySize;
   long nClArrayUsed;
   char **apClArray;
};

StringTable::StringTable() {
   nClArraySize = 0;
   nClArrayUsed = 0;
   apClArray    = 0;
}

StringTable::~StringTable() {
   resetEntries();
}

void StringTable::dump(long nIndent) {
   printf("] %.*sstringtable %p, %d entries:\n",nIndent,pszGlblBlank,this,nClArrayUsed);
   for (long i=0; i<nClArrayUsed; i++) {
      printf("]   %.*s%s\n", nIndent,pszGlblBlank,apClArray[i]);
   }
}

void StringTable::resetEntries() {
   for (long i=0; i<nClArrayUsed; i++) {
      delete [] apClArray[i];
      apClArray[i] = 0;
   }
   nClArrayUsed = 0;
   if (apClArray)
      delete [] apClArray;
   apClArray = 0;
}

long StringTable::numberOfEntries() {
   return nClArrayUsed;
}

bool StringTable::isSet(long iIndex) {
   if (iIndex < 0) { fprintf(stderr, "warning: illegal index: %d\n", iIndex); return 0; }
   return (iIndex < nClArrayUsed) ? 1 : 0;
}

long StringTable::expand(long nSoMuch) {
   char **apTmp = new char*[nClArraySize+nSoMuch];
   if (!apTmp) return 9;
   if (apClArray) {
      memcpy(apTmp, apClArray, nClArraySize*sizeof(char*));
      delete [] apClArray;
   }
   apClArray = apTmp;
   nClArraySize += nSoMuch;
   return 0;
}

long StringTable::addEntry(char *psz) {
   if (nClArrayUsed == nClArraySize) {
      if (nClArraySize == 0) {
         if (expand(10)) return 9;
      } else {
         if (expand(nClArraySize)) return 9;
      }
   }
   apClArray[nClArrayUsed++] = strdup(psz);
   return 0;
}

long StringTable::addEntryPrefixed(char *psz, char cPrefix) {
   if (nClArrayUsed == nClArraySize) {
      if (nClArraySize == 0) {
         if (expand(10)) return 9;
      } else {
         if (expand(nClArraySize)) return 9;
      }
   }
   // create extended copy with prefix at beginning
   long nLen = strlen(psz);
   char *pszCopy = new char[nLen+2];
   pszCopy[0] = cPrefix;
   strcpy(pszCopy+1, psz);
   // add this copy
   apClArray[nClArrayUsed++] = pszCopy;
   if (bGlblDebug) printf("]  stringtable %p added %s, have %d\n",this,pszCopy,nClArrayUsed);
   return 0;
}

long StringTable::setEntryPrefixed(long nIndex, char *psz, char cPrefix) {
   if (nIndex >= nClArrayUsed) {
      fprintf(stderr, "error: illegal set index: %d\n", nIndex);
      return 9;
   }
   // create extended copy with prefix at beginning
   long nLen = strlen(psz);
   char *pszCopy = new char[nLen+2];
   pszCopy[0] = cPrefix;
   strcpy(pszCopy+1, psz);
   // delete old entry, if any
   if (apClArray[nIndex])
      delete [] apClArray[nIndex];
   // set new copy
   apClArray[nIndex] = pszCopy;
   if (bGlblDebug) printf("]  stringtable %p set %s, have %d\n",this,pszCopy,nClArrayUsed);
   return 0;
}

char *StringTable::getEntry(long nIndex, int nTraceLine) {
   if (nIndex >= 0 && nIndex < nClArrayUsed)
      return apClArray[nIndex];
   fprintf(stderr, "error: illegal StringTable index: %d tline %d\n", nIndex, nTraceLine);
   return 0;
}

StringTable glblErrorLog;

void logError(const char *format, ...)
{
   va_list arg_ptr;
   va_start(arg_ptr, format);

   char szTmpBuf[4096];
   vsprintf(szTmpBuf, format, arg_ptr);

   glblErrorLog.addEntry(szTmpBuf);
   printf("%s\n", szTmpBuf);
}

// ============================================================

class Array {
public:
   Array                (const char *pszID); // one-dimensional by default
  ~Array                ( );
   long addString       (char *psz);   // to current row (0 by default)
   long addString       (long lRow, char *psz);
   long setString       (long lRow, long iIndex, char *psz);
   char *getString      (long iIndex); // use isStringSet() before
   char *getString      (long lRow, long iIndex);
   long addLong         (long nValue, int nTraceLine); // to current row (0 by default)
   long addLong         (long lRow, long nValue, int nTraceLine);
   long getLong         (long iIndex); // use isLongSet() before
   long getLong         (long lRow, long iIndex, int nTraceLine);
   long setLong         (long lRow, long iIndex, long nValue, int nTraceLine);
   long numberOfEntries ( );           // in current row (0 by default)
   long numberOfEntries (long lRow);
   bool isLongSet       (long lRow, long iIndex); // index and type test
   bool isStringSet     (long iIndex); // index and type test
   bool isStringSet     (long lRow, long iIndex);
   long addRow          (int nTraceLine); // add empty row, set as current
   long setRow          (long iCurRow, int nTraceLine);// set current row
   bool hasRow          (long iRow);   // tell if row exists
   void reset           ( );           // removes all entries
   void dump            ( );
private:
   bool isSet           (long iIndex); // in current row (0 by default)
   long ensureBase      ( );  // make sure at least one row exists
   long expand          (long nSoMuch);
   long nClRowsSize;
   long nClRowsUsed;
   StringTable **apClRows;
   long nClCurRow;
   const char *pszClID;
};

Array::Array(const char *pszID) {
   nClRowsSize = 0;
   nClRowsUsed = 0;
   apClRows    = 0;
   nClCurRow   = 0;
   pszClID     = pszID;
}

Array::~Array() {
   reset();
}

void Array::dump() {
   printf("] array %s dump:\n",pszClID);
   for (long iRow=0; iRow<nClRowsUsed; iRow++) {
      StringTable *pRow = apClRows[iRow];
      printf("]   row %p:\n",pRow);
      pRow->dump(5);
   }
}

// internal: expand row array
long Array::expand(long nSoMuch) {
   StringTable **apTmp = new StringTable*[nClRowsSize+nSoMuch];
   if (!apTmp) return 9;
   if (apClRows) {
      memcpy(apTmp, apClRows, nClRowsSize*sizeof(StringTable*));
      delete [] apClRows;
   }
   apClRows = apTmp;
   nClRowsSize += nSoMuch;
   return 0;
}

// internal: make sure at least one row exists
long Array::ensureBase( ) {
   if (!nClRowsSize) {
      // if (bGlblDebug) printf("] array %p crt initial\n",this);
      // create initial row, for one-dimensional mode
      if (expand(1)) return 9;
      StringTable *pFirst = new StringTable();
      if (!pFirst) return 9;
      apClRows[nClRowsUsed++] = pFirst;
   }
   return 0;
}

// public: remove everything
void Array::reset() {
   // if (bGlblDebug) printf("] array %p RESET\n",this);
   for (long i=0; i<nClRowsUsed; i++) {
      delete apClRows[i];
      apClRows[i] = 0;
   }
   nClRowsUsed = 0;
   if (apClRows) {
      delete [] apClRows;
      apClRows = 0;
   }
}

// public: return number of columns in current row
long Array::numberOfEntries() {
   if (ensureBase()) return 0;
   return apClRows[nClCurRow]->numberOfEntries();
}

long Array::numberOfEntries(long lRow) {
   if (ensureBase()) return 0;
   if (lRow < 0 || lRow > nClRowsUsed) { fprintf(stderr, "error: %s: illegal row %d\n",pszClID,lRow); return 9; }
   return apClRows[lRow]->numberOfEntries();
}

// public: add string object to current row
long Array::addString(char *psz) {
   if (ensureBase()) return 0;
   if (bGlblDebug) printf("] array %s: add to row %d entry %s. [%d rows total]\n",pszClID,nClCurRow,psz,nClRowsUsed);
   long lRC = apClRows[nClCurRow]->addEntryPrefixed(psz, 's');
   return lRC;
}

long Array::addString(long lRow, char *psz) {
   if (ensureBase()) return 0;
   if (lRow < 0 || lRow > nClRowsUsed) { fprintf(stderr, "error: %s: illegal row %d\n",pszClID,lRow); return 9; }
   if (bGlblDebug) printf("] array %s: add to row %d entry %s. [%d rows total]\n",pszClID,lRow,psz,nClRowsUsed);
   long lRC = apClRows[lRow]->addEntryPrefixed(psz, 's');
   return lRC;
}

long Array::setString(long lRow, long nIndex, char *psz) {
   if (ensureBase()) return 0;
   if (lRow < 0 || lRow > nClRowsUsed) { fprintf(stderr, "error: %s: illegal row %d\n",pszClID,lRow); return 9; }
   if (bGlblDebug) printf("] array %s: set row %d:%d entry %s. [%d rows total]\n",pszClID,lRow,nIndex,psz,nClRowsUsed);
   long lRC = apClRows[lRow]->setEntryPrefixed(nIndex, psz, 's');
   return lRC;
}

// public: get string object from current row
char *Array::getString(long nIndex) {
   if (ensureBase()) return 0;
   char *pszRaw = apClRows[nClCurRow]->getEntry(nIndex, __LINE__);
   if (!pszRaw)
      return 0;
   if (pszRaw[0] != 's') { fprintf(stderr, "error: %s: no string entry type at %u %u\n", pszClID, nClCurRow, nIndex); return 0; }
   return pszRaw+1;
}

char *Array::getString(long lRow, long nIndex) {
   if (ensureBase()) return 0;
   if (lRow < 0 || lRow >= nClRowsUsed) { fprintf(stderr, "error: %s: illegal row %d\n",pszClID,lRow); return 0; }
   char *pszRaw = apClRows[lRow]->getEntry(nIndex, __LINE__);
   if (!pszRaw)
      return 0;
   if (pszRaw[0] != 's') { fprintf(stderr, "error: %s: no string entry type at %u %u\n", pszClID, lRow, nIndex); return 0; }
   return pszRaw+1;
}

// public: add integer to current row, internally encoded as string
long Array::addLong(long nValue, int nTraceLine) {
   if (ensureBase()) return 0;
   if (bGlblDebug) printf("] array %s: add to row %d entry %d [tline %d]\n",pszClID,nClCurRow,nValue,nTraceLine);
   char szBuf[100];
   ulong uValue = (ulong)nValue;
   // char *_ultoa( unsigned long value, char *string, int radix );
   #ifdef _WIN32
   _ultoa(uValue, szBuf, 16);
   #else
   sprintf(szBuf, "%lx", uValue);
   #endif
   return apClRows[nClCurRow]->addEntryPrefixed(szBuf, 'i');
}

long Array::addLong(long lRow, long nValue, int nTraceLine) {
   if (ensureBase()) return 0;
   if (bGlblDebug) printf("] array %s: add to row %d entry %d [tline %d]\n",pszClID,lRow,nValue,nTraceLine);
   if (lRow < 0 || lRow > nClRowsUsed) { fprintf(stderr, "error: %s: illegal row %d\n",pszClID,lRow); return 9; }
   char szBuf[100];
   ulong uValue = (ulong)nValue;
   #ifdef _WIN32
   _ultoa(uValue, szBuf, 16);
   #else
   sprintf(szBuf, "%lx", uValue);
   #endif
   return apClRows[lRow]->addEntryPrefixed(szBuf, 'i');
}

long Array::getLong(long nIndex) {
   if (ensureBase()) return 0;
   char *pszRaw = apClRows[nClCurRow]->getEntry(nIndex, __LINE__);
   if (!pszRaw)
      return 0;
   if (pszRaw[0] != 'i') { fprintf(stderr, "error: no long entry type at row %u col %u\n", nClCurRow, nIndex); return 0; }
   // unsigned long strtoul( const char *nptr, char **endptr, int base );
   ulong uValue = strtoul(pszRaw+1, 0, 16);
   return (long)uValue;
}

long Array::getLong(long lRow, long nIndex, int nTraceLine) {
   if (ensureBase()) return 0;
   if (lRow < 0 || lRow >= nClRowsUsed) { fprintf(stderr, "error: %s: illegal row %d\n",pszClID,lRow); return 0; }
   char *pszRaw = apClRows[lRow]->getEntry(nIndex, nTraceLine);
   if (!pszRaw)
      return 0;
   if (pszRaw[0] != 'i') { fprintf(stderr, "error: no long entry type at row %u col %u\n", lRow, nIndex); return 0; }
   // unsigned long strtoul( const char *nptr, char **endptr, int base );
   ulong uValue = strtoul(pszRaw+1, 0, 16);
   return (long)uValue;
}

long Array::setLong(long lRow, long nIndex, long nValue, int nTraceLine) {
   if (ensureBase()) return 0;
   if (lRow < 0 || lRow >= nClRowsUsed) { fprintf(stderr, "error: %s: illegal row %d\n",pszClID,lRow); return 0; }
   char szBuf[100];
   ulong uValue = (ulong)nValue;
   // char *_ultoa( unsigned long value, char *string, int radix );
   #ifdef _WIN32
   _ultoa(uValue, szBuf, 16);
   #else
   sprintf(szBuf, "%lx", uValue);
   #endif
   return apClRows[lRow]->setEntryPrefixed(nIndex, szBuf, 'i');
}

// public: create new row. shouldn't be called for first row.
long Array::addRow(int nTraceLine) {
   long nOldRows = nClRowsSize;
   if (ensureBase()) return 0;
   // if anyone calls this before adding first data, accept it.
   if (!nOldRows)
      return 0;
   // for all further rows, check if row table is full.
   // if so, expand in exponential steps.
   if (nClRowsUsed == nClRowsSize) {
      if (expand(nClRowsSize)) return 9;
   }
   // finally, add new row object
   StringTable *pFirst = new StringTable();
   if (!pFirst) return 9;
   apClRows[nClRowsUsed++] = pFirst;
   nClCurRow = nClRowsUsed-1;
   if (bGlblDebug) printf("array %s extended to %d rows [tln %d]\n",pszClID,nClRowsUsed,nTraceLine);
   return 0;
}

// public: select current row
long Array::setRow(long iCurRow, int nTraceLine) {
   if (ensureBase()) return 9;
   if (iCurRow < 0 || iCurRow >= nClRowsUsed)
   {  fprintf(stderr, "error: %s: illegal row index: %d on setRow, tline %d\n",pszClID,iCurRow,nTraceLine); return 9; }
   nClCurRow = iCurRow;
   if (bGlblDebug) printf("array %s setrow %d\n",pszClID,nClCurRow);
   return 0;
}

// public: tell if index in current row is set, used for loops
bool Array::isSet(long iIndex) {
   if (ensureBase()) return 0;
   return apClRows[nClCurRow]->isSet(iIndex);
}

// public: tell if index in current row is set with a string
bool Array::isStringSet(long iIndex) {
   if (ensureBase()) return 0;
   if (!apClRows[nClCurRow]->isSet(iIndex)) {
      // if (bGlblDebug) printf("] %s: no string set at %d:%d\n",pszClID,nClCurRow,iIndex);
      return 0;
   }
   char *pszRaw = apClRows[nClCurRow]->getEntry(iIndex, __LINE__);
   // printf("xxx %s:%d:%d %s\n",pszClID,nClCurRow,iIndex,pszRaw);
   if (!pszRaw) {
      if (bGlblDebug) {
         printf("error: no entry at index %d, table %p, within array %p\n",iIndex,apClRows[nClCurRow],this);
         apClRows[nClCurRow]->dump();
      }
      return 0;
   }
   if (pszRaw[0] != 's') {
      fprintf(stderr, "error: no string type at %s:%d:%d\n",pszClID,nClCurRow,iIndex);
      return 0;
   }
   return 1;
}

bool Array::isStringSet(long lRow, long iIndex) {
   if (ensureBase()) return 0;
   if (lRow < 0 || lRow >= nClRowsUsed) { fprintf(stderr, "error: %s: illegal row %d\n",pszClID,lRow); return 0; }
   if (!apClRows[lRow]->isSet(iIndex))
      return 0;
   char *pszRaw = apClRows[lRow]->getEntry(iIndex, __LINE__);
   if (!pszRaw) {
      if (bGlblDebug) {
         printf("error: no entry at index %d, table %p, within array %p\n",iIndex,apClRows[lRow],this);
         apClRows[lRow]->dump();
      }
      return 0;
   }
   return (pszRaw[0] == 's') ? 1 : 0;
}

// public: tell if index in current row is set with a long
bool Array::isLongSet(long lRow, long iIndex) {
   if (ensureBase()) return 0;
   if (lRow < 0 || lRow >= nClRowsUsed) { fprintf(stderr, "error: %s: illegal row %d\n",pszClID,lRow); return 0; }
   if (!apClRows[lRow]->isSet(iIndex))
      return 0;
   char *pszRaw = apClRows[lRow]->getEntry(iIndex, __LINE__);
   if (!pszRaw)
      return 0;
   return (pszRaw[0] == 'i') ? 1 : 0;
}

// public: tell if row exists
bool Array::hasRow(long iRow) {
   if (ensureBase()) return 0;
   if (iRow < 0) {  fprintf(stderr, "error: %s: illegal row index: %d on hasRow\n",pszClID,iRow); return 9; }
   return (iRow < nClRowsUsed) ? 1 : 0;
}

// ============================================================

class LongTable {
public:
   LongTable            ( );
  ~LongTable            ( );
   long addEntry        (long nValue);
   long numberOfEntries ( );
   long getEntry        (long iIndex, int nTraceLine);
   void resetEntries    ( );
private:
   long expand          (long nSoMuch);
   long nClArraySize;
   long nClArrayUsed;
   long *pClArray;
};

LongTable::LongTable() {
   nClArraySize = 0;
   nClArrayUsed = 0;
   pClArray     = 0;
}

LongTable::~LongTable() {
   resetEntries();
}

void LongTable::resetEntries() {
   nClArrayUsed = 0;
   if (pClArray)
      delete [] pClArray;
   pClArray = 0;
}

long LongTable::numberOfEntries() {
   return nClArrayUsed;
}

long LongTable::expand(long nSoMuch) {
   long *apTmp = new long[nClArraySize+nSoMuch];
   if (!apTmp) return 9;
   if (pClArray) {
      memcpy(apTmp, pClArray, nClArraySize*sizeof(long));
      delete [] pClArray;
   }
   pClArray = apTmp;
   nClArraySize += nSoMuch;
   return 0;
}

long LongTable::addEntry(long nValue) {
   if (nClArrayUsed == nClArraySize) {
      if (nClArraySize == 0) {
         if (expand(10)) return 9;
      } else {
         if (expand(nClArraySize)) return 9;
      }
   }
   pClArray[nClArrayUsed++] = nValue;
   return 0;
}

long LongTable::getEntry(long nIndex, int nTraceLine) {
   if (nIndex >= 0 && nIndex < nClArrayUsed)
      return pClArray[nIndex];
   fprintf(stderr, "error: illegal LongTable index: %d %d tline %d\n", nIndex, nClArrayUsed, nTraceLine);
   return -1;
}

// ======================================================

class NumTable {
public:
   NumTable            ( );
  ~NumTable            ( );
   long addEntry        (num nValue);
   long numberOfEntries ( );
   num  getEntry        (long iIndex, int nTraceLine);
   void resetEntries    ( );
private:
   long expand          (long nSoMuch);
   long nClArraySize;
   long nClArrayUsed;
   num  *pClArray;
};

NumTable::NumTable() {
   nClArraySize = 0;
   nClArrayUsed = 0;
   pClArray     = 0;
}

NumTable::~NumTable() {
   resetEntries();
}

void NumTable::resetEntries() {
   nClArrayUsed = 0;
   if (pClArray)
      delete [] pClArray;
   pClArray = 0;
}

long NumTable::numberOfEntries() {
   return nClArrayUsed;
}

long NumTable::expand(long nSoMuch) {
   num *apTmp = new num[nClArraySize+nSoMuch];
   if (!apTmp) return 9;
   if (pClArray) {
      memcpy(apTmp, pClArray, nClArraySize*sizeof(num));
      delete [] pClArray;
   }
   pClArray = apTmp;
   nClArraySize += nSoMuch;
   return 0;
}

long NumTable::addEntry(num nValue) {
   if (nClArrayUsed == nClArraySize) {
      if (nClArraySize == 0) {
         if (expand(10)) return 9;
      } else {
         if (expand(nClArraySize)) return 9;
      }
   }
   pClArray[nClArrayUsed++] = nValue;
   return 0;
}

num NumTable::getEntry(long nIndex, int nTraceLine) {
   if (nIndex >= 0 && nIndex < nClArrayUsed)
      return pClArray[nIndex];
   fprintf(stderr, "error: illegal NumTable index: %d %d tline %d\n", nIndex, nClArrayUsed, nTraceLine);
   return -1;
}

// ==========================================================

class FileList {
public:
   FileList       ( );
  ~FileList       ( );
   long  addFile        (char *pszAbsName, num nTimeStamp, num nSize);
   long  checkAndMark   (char *pszName, num nSize);
   StringTable clNames;
   NumTable    clTimes;
   NumTable    clSizes;
};

FileList::FileList()  { }
FileList::~FileList() { }

long FileList::addFile(char *pszAbsName, num nTimeStamp, num nSize)
{
   // IF filename starts with ".\\", skip this part
   if (!strncmp(pszAbsName, glblDotSlash, strlen(glblDotSlash)))
      pszAbsName += strlen(glblDotSlash);

   if (clNames.addEntry(pszAbsName)) return 9;
   if (clTimes.addEntry(nTimeStamp)) return 9;
   if (clSizes.addEntry(nSize)) return 9;
   return 0;
}

// checkAndMark checks the filename and size,
// but NOT the rather complicated timestamps.
long FileList::checkAndMark(char *pszName, num nSize) {
   long nEntries = clNames.numberOfEntries();
   for (long i=0; i<nEntries; i++) {
      char *psz = clNames.getEntry(i, __LINE__);
      if ((psz[0] != 0) && !strcmp(psz, pszName)) {
         num nSize2 = clSizes.getEntry(i, __LINE__);
         if (nSize != nSize2)
            return 2; // on size mismatch, do NOT mark as done.
         psz[0] = '\0';
         return 0;
      }
   }
   return 1;
}

// ====================================================

class TwinFileScanner {
public:
   TwinFileScanner   ( );
   long  addFile     (char *psz);
   long  analyze     ( );
   void  shutdown    ( );
private:
   StringTable clNames;
   LongTable   clHashes;
   InfoCounter clCnt;
};

TwinFileScanner::TwinFileScanner() {
}

void TwinFileScanner::shutdown() {
   clNames.resetEntries();
   clHashes.resetEntries();
}

long TwinFileScanner::addFile(char *pszFile) 
{
   clNames.addEntry(pszFile);

   // create file hash
   SFKMD5 md5;
   if (getFileMD5(pszFile, md5))
      return 9;
   unsigned char *pmd5 = md5.digest();
   // create short hash from 16 bytes long hash
   uchar aHash[4];
   aHash[0] = aHash[1] = aHash[2] = aHash[3] = 0;
   for (ulong i1=0; i1<4; i1++) {
      aHash[0] ^= pmd5[i1*4+0];
      aHash[1] ^= pmd5[i1*4+1];
      aHash[2] ^= pmd5[i1*4+2];
      aHash[3] ^= pmd5[i1*4+3];
   }
   ulong nHash =
         ((ulong)aHash[0] << 24)
      |  ((ulong)aHash[1] << 16)
      |  ((ulong)aHash[2] <<  8)
      |  ((ulong)aHash[3] <<  0);
   long lHash = (long)nHash;

   // add our own hash
   clHashes.addEntry((long)nHash);
   if (clCnt.count()) {
      fprintf(stderr, "scanning %u files ...   \r",clHashes.numberOfEntries());
      fflush(stderr);
   }

   return 0;
}

long TwinFileScanner::analyze()
{
   ulong nDouble = 0;
   num nTotalBytes = 0;

   ulong nFiles = clHashes.numberOfEntries();
   for (ulong isrc=0; isrc<nFiles; isrc++)
   {
      bool bToldSrc = 0;
      for (ulong idst=isrc+1; idst<nFiles; idst++)
      {
         if (clHashes.getEntry(isrc, __LINE__) == clHashes.getEntry(idst, __LINE__))
         {
            // this MIGHT be identical. verify deep.
            char *psz1 = clNames.getEntry(isrc, __LINE__);
            char *psz2 = clNames.getEntry(idst, __LINE__);
            if (*psz1 && *psz2)
            {
               SFKMD5 bigHash1, bigHash2;
               if (getFileMD5(psz1, bigHash1))
                  return 9;
               if (getFileMD5(psz2, bigHash2))
                  return 9;
               uchar *pHash1 = bigHash1.digest();
               uchar *pHash2 = bigHash2.digest();
               if (!memcmp(pHash1, pHash2, 16))
               {
                  if (!bToldSrc) {
                     bToldSrc = 1;
                     printf("%s\n", psz1);
                  }
                  printf("   %s\n", psz2);
                  nDouble++;
                  nTotalBytes += getFileSize(psz1);
                  // unset 2nd entry, to avoid further re-listing
                  *psz2 = '\0';
               }
            }
         }
      }
   }

   ulong nMBytes = nTotalBytes / 1000000;
   printf("%u identical files, %lu mbytes, %lu bytes\n", nDouble, nMBytes, nTotalBytes);

   return 0;
}

TwinFileScanner glblTwinScan;

enum eDirCommands {
   eCmd_CopyDir      = 1,
   eCmd_FreezeDir    = 2,
   eCmd_Undefined    = 0xFF
};

class FileSet {
public:
   FileSet  ( );
  ~FileSet  ( );
   long  beginLayer     (bool bWithEmptyCommand);
   long  addRootDir     (char *pszName, int nTraceLine, bool bNoCmdFillup);
   long  addDirMask     (char *pszName);
   long  addDirCommand  (long);
   long  addFileMask    (char *pszName);
   long  autoCompleteFileMasks   ( );
   void  setBaseLayer   ( );
   void  reset ( );
   bool  hasRoot        (long iIndex);
   char* setCurrentRoot (long iIndex);
   char* getCurrentRoot ( );
   long  numberOfRootDirs ( );
   Array &dirMasks      ( ) { return clDirMasks; }
   Array &fileMasks     ( ) { return clFileMasks; }
   bool  anythingAdded  ( );
   void  dump           ( );
   long  getDirCommand  ( );  // of current root
   bool  isAnyExtensionMaskSet   ( );
private:
   long  ensureBase     (int nTraceLine);
   Array clRootDirs;    // row 0: names/str, row 1: command/int, row 2: layer/int
   Array clDirMasks;    // row == layer
   Array clFileMasks;   // row == layer
   long nClCurDir;
   long nClCurLayer;
};

FileSet::FileSet() 
 : clRootDirs("RootDirs"),
   clDirMasks("DirMasks"),
   clFileMasks("FileMasks")
{
   nClCurDir   =  0;
   nClCurLayer = -1;
}

FileSet::~FileSet() {
   reset();
}

bool FileSet::isAnyExtensionMaskSet() 
{
   for (int i=0; clFileMasks.isStringSet(i); i++) 
   {
      char *pszMask = clFileMasks.getString(i);
      if (pszMask[0] == '.')
         return true;
   }
   return false;
}

long FileSet::getDirCommand() {
   return clRootDirs.getLong(1, nClCurDir, __LINE__);
}

void FileSet::dump() 
{
   printf("=== fileset begin ===\n");

   for (long i1=0; clRootDirs.isStringSet(0,i1); i1++)
   {
      long nLayer = clRootDirs.getLong(2,i1, __LINE__);
      long nCmd   = 0;
      if (clRootDirs.isLongSet(1,i1))
           nCmd   = clRootDirs.getLong(1,i1, __LINE__);
      printf("] ROOT %s -> layer %d, cmd %d\n",
         clRootDirs.getString(0,i1),
         nLayer,
         nCmd
         );
   }

   for (long iLayer=0; clDirMasks.hasRow(iLayer); iLayer++) 
   {
      printf("] layer %d:\n", iLayer);

      printf("]  dmsk:\n");
      for (long i5=0; clDirMasks.isStringSet(iLayer, i5); i5++)
         printf("]   %s\n",clDirMasks.getString(iLayer, i5));

      printf("]  fmsk:\n]   ");
      for (long i6=0; clFileMasks.isStringSet(iLayer, i6); i6++)
         printf("%s, ",clFileMasks.getString(iLayer, i6));
      printf("\n");
   }

   printf("=== fileset end ===\n");
}

bool FileSet::anythingAdded() {
   return (clRootDirs.numberOfEntries() > 0) ? 1 : 0;
}

bool FileSet::hasRoot(long iIndex) {
   bool bRC = clRootDirs.isStringSet(iIndex);
   if (bGlblDebug) printf("] %d = hasRoot(%d)\n", bRC, iIndex);
   return bRC;
}

char* FileSet::setCurrentRoot(long iIndex) {
   if (bGlblDebug) printf("] select root %d\n",iIndex);
   nClCurDir = iIndex;
   // the root dir no. iIndex selects a specific layer.
   char *pszDirName = clRootDirs.getString(0, iIndex);
   nClCurLayer      = clRootDirs.getLong(2, iIndex, __LINE__);
   clRootDirs.setRow(0, __LINE__);
   // fully switch to current layer
   if (bGlblDebug) printf("]  select layer %d\n",nClCurLayer);
   clDirMasks.setRow(nClCurLayer, __LINE__);
   clFileMasks.setRow(nClCurLayer, __LINE__);
   return pszDirName;
}

char* FileSet::getCurrentRoot( ) {
   return clRootDirs.getString(0, nClCurDir);
}

long FileSet::numberOfRootDirs() {
   return clRootDirs.numberOfEntries(0);
}

long FileSet::ensureBase(int nTraceLine) {
   // clRootDirs must have 3 rows
   while (!clRootDirs.hasRow(2))
      if (clRootDirs.addRow(nTraceLine))
         return 9;
   return 0;
}

void FileSet::reset() {
   clRootDirs.reset();
   clDirMasks.reset();
   clFileMasks.reset();
   nClCurLayer = -1;
}

// a layer is one set of dir masks and file masks.
// - dir masks is a row in clDirMasks.
// - file masks is a row in clFileMasks.
// - command is 1:1 per directory root,
//   therefore cmd is stored in clRootDirs.
// - clRootDirs row 0 is the directory name.
// - clRootDirs row 1 is the directory command (zip, copy).
// - a directory tree references a layer.

long FileSet::beginLayer(bool bWithEmptyCommand) 
{
   // ensure clRootDirs has two columns
   if (ensureBase(__LINE__)) return 9;
   // make space for new layer
   clDirMasks.addRow(__LINE__);
   clFileMasks.addRow(__LINE__);
   // count new layer
   nClCurLayer++;
   // printf("] BEGINLAYER: new current=%d [\n",nClCurLayer);

   // a "command" is "-copy" or "-zip", used only with freezeto.
   // all other commands will just fill the command column with dummys.
   if (bWithEmptyCommand) {
      // set dummy command "0" in row 1 of clRootDirs
      clRootDirs.addLong(1, 0, __LINE__);
   }
   return 0;
}

long FileSet::addRootDir(char *pszRoot, int nTraceLine, bool bNoCmdFillup) {
   if (bGlblDebug) printf("] add root dir: %s, referencing layer: %d [tline %d]\n", pszRoot, nClCurLayer, nTraceLine);
   if (ensureBase(__LINE__)) return 9;
   if (nClCurLayer == -1) {
      if (bGlblDebug) printf("] impl. create first fileset layer\n");
      beginLayer(true);
   }
   // complete the current column
   long nMax = clRootDirs.numberOfEntries(0);
   if (!bNoCmdFillup)
      if (clRootDirs.numberOfEntries(1) < nMax)
          clRootDirs.addLong(1, 0, __LINE__); // add empty command
   if (clRootDirs.numberOfEntries(2) < nMax)
   {  fprintf(stderr, "error: internal #20\n"); return 9; }
   // add another root dir, forward-referencing
   // the current layer in which masks will follow.
   clRootDirs.addString(0,pszRoot);
   clRootDirs.addLong(2,nClCurLayer, __LINE__);
   return 0;
}

long FileSet::addDirCommand(long lCmd)
{
   if (ensureBase(__LINE__)) return 9;
   // one single -cmd may map to several root dirs.
   // example: -dir a1 a2 a3 -copy
   // therefore fill up until root dir names number reached.
   long nMax = clRootDirs.numberOfEntries(0);  
   while (clRootDirs.numberOfEntries(1) < nMax)
      clRootDirs.addLong(1, lCmd, __LINE__);
   return 0;
}

long FileSet::addDirMask(char *pszMask) {
   if (ensureBase(__LINE__)) return 9;
   clDirMasks.addString(pszMask);
   return 0;
}

long FileSet::addFileMask(char *pszMask) {
   // if (bGlblDebug) printf("] addFileMask %s\n", pszMask);
   if (ensureBase(__LINE__)) return 9;
   clFileMasks.addString(pszMask);
   return 0;
}

long FileSet::autoCompleteFileMasks() {
   if (bGlblDebug) printf("] autocomplete:\n");
   for (long irow=0; clFileMasks.hasRow(irow); irow++) {
      if (clFileMasks.setRow(irow, __LINE__)) return 9;
      if (clFileMasks.numberOfEntries() == 0) {
         if (bGlblDebug) printf("]  yes, at layer %d\n",irow);
         if (clFileMasks.addString("*")) return 9;
      }
   }
   if (clRootDirs.numberOfEntries() == 0) {
      if (bGlblDebug) printf("] adding dir .\n");
      addRootDir(".", __LINE__, false);
   }
   return 0;
}

void FileSet::setBaseLayer() {
   clRootDirs.setRow(0, __LINE__);
   clDirMasks.setRow(0, __LINE__);
   clFileMasks.setRow(0, __LINE__);
}

FileSet glblFileSet;

Array glblGrepPat("grep");

// only for processDirParms and walkAllTrees:
long bGlblError = 0;
StringTable *pGlblFileParms = 0;
char **apGlblFileParms = 0;
int  nGlblFileParms = 0;

void skipSpaceRem(char **pszInOut)
{
   char *psz1 = *pszInOut;

   // skip all whitespaces and remark lines
   while (*psz1) 
   {
      if (*psz1==' ' || *psz1=='\t' || *psz1=='\r' || *psz1=='\n')
         { psz1++; continue; }

      if (*psz1=='#') {
         while (*psz1 && *psz1!='\n')
            psz1++;
         if (*psz1)
            psz1++; // skip LF
      }
      else
         break;
   }

   *pszInOut = psz1;
}

bool setGeneralOption(char *psz1) 
{
   if (!strcmp(psz1, "-debug"))     { bGlblDebug = 1; return true; }
   if (!strcmp(psz1, "-quiet"))     { bGlblQuiet = 1; return true; }
   if (!strcmp(psz1, "-sim"))       { bGlblSim = 1; return true; }
   if (!strcmp(psz1, "-norec"))     { bGlblRecurseDirs = 0; return true; }
   if (!strcmp(psz1, "-i"))         { bGlblStdInAny = 1; return true; }
   if (!strcmp(psz1, "-verbose"))   { nGlblVerbose = 1; return true; }
   if (!strcmp(psz1, "-verbose=2")) { nGlblVerbose = 2; return true; }
   if (!strcmp(psz1, "-verbose=3")) { nGlblVerbose = 3; return true; }
   if (!strcmp(psz1, "-hidden"))    { bGlblHiddenFiles = 1; return true; }
   if (!strcmp(psz1, "-yes"))       { bGlblYes = 1; return true; }
   return false;
}

long processDirParms(char *pszCmd, int argc, char *argv[], int iDir, bool bAutoCompleteMasks)
{
   long checkMask(char *pszMask);
   char *loadFile(char *pszFile, int nLine);

   // fetch prefixed general options
   while (iDir < argc) {
      if (setGeneralOption(argv[iDir]))
         iDir++;
      else
         break;
   }

   if (iDir < argc)
   {
      char *pszFirstParm = argv[iDir];

      if (!strncmp(pszFirstParm, "-from=", strlen("-from="))) 
      {
         // re-create parameter array from input file
         pszFirstParm += strlen("-from=");
         char *pszParmFile = loadFile(pszFirstParm,__LINE__);
         if (!pszParmFile) { fprintf(stderr, "error: unable to read: %s\n",pszFirstParm); return 9; }
         pGlblFileParms = new StringTable();
         char *psz1 = pszParmFile;
         while (*psz1) 
         {
            // find next token.
            char *pszo = psz1;
            skipSpaceRem(&psz1);

            // find end of token. support "-parm with blanks".
            char *psz2 = psz1;
            if (*psz2 == '"')
               while (*psz2 && *psz2!='"')
                  psz2++;
            while (*psz2 && *psz2!=' ' && *psz2!='\t' && *psz2!='\r' && *psz2!='\n')
               psz2++;

            // isolate token
            if (*psz2)
               *psz2++ = '\0';

            // line-end may lead to empty entry, therefore
            if (strlen(psz1)) // only if not line-end
               pGlblFileParms->addEntry(psz1);

            // continue with next token, if any
            psz1 = psz2;
         }
         delete [] pszParmFile;

         // pGlblFileParms now holds all parms
         long nParms = pGlblFileParms->numberOfEntries();
         apGlblFileParms = new char*[nParms];
         for (long i=0; i<nParms; i++)
            apGlblFileParms[i] = pGlblFileParms->getEntry(i, __LINE__);
         nGlblFileParms = nParms;

         // restart through recursion
         // printf("[%d parms from file]\n", nParms);
         return processDirParms(pszCmd, nParms, apGlblFileParms, 0, bAutoCompleteMasks);
      }
  
      // init fileset
      if (glblFileSet.beginLayer(false))
         return 9;

      // fetch further prefix options, if any
      while (!strncmp(pszFirstParm, "-", 1)) {
         char *psz1 = pszFirstParm;
         if (!setGeneralOption(psz1))
            break; // unknown option, fall through to further processing
         pszFirstParm = (iDir < argc-1) ? argv[++iDir] : (char*)"";
      }

      if (*pszFirstParm == '-') 
      {
         if (bGlblDebug) printf("process long format\n");

         // process long format, allowing multiple dirs:
         // sfk cmd -dir dir1 dir2 !dir3 !dir4 -copy -file .hpp .cpp

         // we now have:
         //   clRootDirs , two rows, DirName(empty) and Cmd(empty)
         // one layer:
         //   clDirMasks , one row , empty
         //   clFileMasks, one row , empty

         long lState = 0;
         for (;iDir < argc; iDir++)
         {
            char *psz1 = argv[iDir];
   
            if (bGlblDebug) printf("] \"%s\" [\n",psz1);

            if (!strcmp(psz1, "-debug")) {
               bGlblDebug = 1;
               continue;
            }
   
            if (!strcmp(psz1, "-dir")) {
               // on change from non-dir list to dir list
               if (lState != 0) {
                  if (bGlblDebug) printf("] -dir: begin layer\n");
                  // not the very first -dir parm: add another layer
                  if (glblFileSet.beginLayer(false))
                     return 9;
               } else {
                  if (bGlblDebug) printf("] -dir: no begin layer\n");
               }
               lState = 1;
               continue;
            }

            if (psz1[0] == glblNotChar) {
               if (lState == 1)
                  glblFileSet.addDirMask(psz1);
               else
                  glblFileSet.addFileMask(psz1);
               continue;
            }

            if (!strcmp(psz1, "-copy")) {
               glblFileSet.addDirCommand(1);
               continue;
            }
            if (!strcmp(psz1, "-zip")) {
               glblFileSet.addDirCommand(2);
               continue;
            }

            if (!strcmp(psz1, "-file")) {
               if (!strncmp(pszCmd, "freezeto=", strlen("freezeto=")))
                  { fprintf(stderr, "error: no -file masks supported with freezeto command.\n"); return 9; }
               lState = 2;
               continue;
            }

            // used only with grep:
            if (!strcmp(psz1, "-pat")) {
               lState = 3;
               continue;
            }

            // general option specified inbetween:
            if (setGeneralOption(psz1))
               continue;

            if (*psz1 == '-') { fprintf(stderr, "error: unknown option: %s\n", psz1); return 9; }

            // handle all non-command parameters
            switch (lState) {
               case 1: glblFileSet.addRootDir(psz1, __LINE__, true);  break;
               case 2: glblFileSet.addFileMask(psz1); break;
               case 3: glblGrepPat.addString(argv[iDir]); break;
            }
         }
      }
      else 
      if (strlen(pszFirstParm))
      {
         // process short format with single dir:
         //    . .cpp .hpp
         // no dir masks and commands supported then.

         // fetch dir
         glblFileSet.addRootDir(pszFirstParm, __LINE__, false);
         iDir++;

         // fetch masks
         while (iDir < argc) 
         {
            // also care about postfix/inbetween options
            char *psz1 = argv[iDir];
            if (setGeneralOption(psz1)) {
               iDir++;
               continue;
            }
            if (glblFileSet.addFileMask(argv[iDir++]))
               return 9;
         }
      }
   }
   else
   {
      // no parms at all supplied
      if (bAutoCompleteMasks)
         glblFileSet.addRootDir(".", __LINE__, false);
      // default file masks added below
   }
   
   if (bAutoCompleteMasks)
      glblFileSet.autoCompleteFileMasks();

   if (bGlblDebug) glblFileSet.dump();

   glblFileSet.setBaseLayer();

   return 0;
}

long setProcessCurrentDir()
{
   // init fileset
   if (glblFileSet.beginLayer(true))
      return 9;

   glblFileSet.addRootDir(".", __LINE__, false);
   glblFileSet.autoCompleteFileMasks();

   if (bGlblDebug) glblFileSet.dump();

   glblFileSet.setBaseLayer();

   return 0;
}

void removeCRLF(char *pszBuf) {
   char *pszLF = strchr(pszBuf, '\n');
   if (pszLF) *pszLF = '\0';
   char *pszCR = strchr(pszBuf, '\r');
   if (pszCR) *pszCR = '\0';
}

void fixPathChars(char *pszBuf) {
   // if path contains foreign path chars, change to local
   char *psz = pszBuf;
   for (; *psz; psz++)
      if (*psz == glblWrongPChar)
          *psz = glblPathChar;
}

// just for sfk run: process text list with file- OR dirnames
long walkStdInListFlat(long nFunc, long &rlFiles, num &rlBytes)
{
   if (bGlblError)
      return 9;

   nGlblFunc = nFunc;

   long lDirs = 0;
   FileList oLocDirFiles;

   while (fgets(szLineBuf, sizeof(szLineBuf)-10, stdin))
   {
      removeCRLF(szLineBuf);

      num nLocalMaxTime = 0, nTreeMaxTime  = 0;

      // skip remark and empty lines
      if (szLineBuf[0] == '#') continue;
      if (!strlen(szLineBuf))  continue;

      if (bGlblStdInFiles)
      {
         char *pszSub = strdup(szLineBuf);
         long lRC = execSingleFile(pszSub, 0,
                        rlFiles, oLocDirFiles.clNames.numberOfEntries(),
                        lDirs, rlBytes,
                        nLocalMaxTime, nTreeMaxTime);
         if (!lRC) {
            rlFiles++;
            // oLocDirFiles.addFile(pszSub, 0);
         }
         delete [] pszSub;
         if (lRC >= 9)
            return lRC;
      }

      if (bGlblStdInDirs)
      {
         rlFiles = 0; // reset tree file count

         char *pszSub = strdup(szLineBuf);
         long rlDirs = 0;
         num  nLocalBytes = 0;
         long lRC = execSingleDir(pszSub, 0, rlFiles, oLocDirFiles, rlDirs, nLocalBytes,
                                  nLocalMaxTime, nTreeMaxTime);
         nLocalMaxTime = 0; // reset after use in execSingleDir
         rlBytes += nLocalBytes;
         delete [] pszSub;
         if (lRC >= 9)
            return lRC;
      }
   }

   return 0;
}

// all commands except sfk run:
// take list with mixed file- and dirnames.
// in case of dirs, process each as root tree.
long walkStdInListDeep(long nFunc, long &rlFiles, num &rlBytes)
{
   long isDir(char *pszName);

   if (bGlblError)
      return 9;

   nGlblFunc = nFunc;

   long lDirs = 0;
   num nLocalMaxTime = 0, nTreeMaxTime = 0;

   while (fgets(szLineBuf, sizeof(szLineBuf)-10, stdin))
   {
      removeCRLF(szLineBuf);

      // skip remark and empty lines
      if (szLineBuf[0] == '#') continue;
      if (!strlen(szLineBuf))  continue;

      // now holding a file- OR a dirname.
      // select the default mask set.
      glblFileSet.setCurrentRoot(0);

      long lDirs = 0;
      FileList oLocDirFiles;
      num  nLocalBytes = 0, nLocalMaxTime = 0, ntime2 = 0;

      char *pszSub = strdup(szLineBuf);
      if (bGlblDebug) printf("] siw: %s\n", pszSub);
      long lRC = walkFiles(pszSub, 0, rlFiles, oLocDirFiles, lDirs, nLocalBytes, nLocalMaxTime, ntime2);
      if (!lRC && isDir(pszSub))
      {
         rlFiles = 0; // reset tree file count

         // if this succeeded, treat the tree as another dir.
         if (bGlblDebug) printf("] sid: %s\n", pszSub);
         if (execSingleDir(pszSub, 0, rlFiles, oLocDirFiles, lDirs, nLocalBytes, nLocalMaxTime, ntime2))
         {
            nLocalMaxTime = 0;
            bGlblError = 1;
            delete [] pszSub;
            return 9;
         }
         nLocalMaxTime = 0;
      }
      rlBytes += nLocalBytes;
      delete [] pszSub;

      if (lRC >= 9)
         return lRC;

      if (bGlblEscape)
         break;
   }

   return 0;
}

long walkAllTrees(long nFunc, long &rlTreeFiles, long &rlDirs, num &rlBytes) 
{
   if (bGlblError)
      return 9;

   if (bGlblStdInAny)
      return walkStdInListDeep(nFunc, rlTreeFiles, rlBytes);

   nGlblFunc = nFunc;

   for (long nDir=0; glblFileSet.hasRoot(nDir); nDir++)
   {
      // reset tree file count:
      rlTreeFiles = 0;

      FileList oDirFiles;
      num nLocalMaxTime = 0, nTreeMaxTime  = 0;

      // process one of the trees given at commandline.
      char *pszTree = glblFileSet.setCurrentRoot(nDir);
      if (bGlblDebug) printf("] wtr: %s\n", pszTree);
      if (walkFiles(pszTree, 0, rlTreeFiles, oDirFiles, rlDirs, rlBytes, nLocalMaxTime, nTreeMaxTime))
      {
         bGlblError = 1;
         return 9;
      }
      else
      {
         // if this succeeded, treat the tree as another dir.
         if (bGlblDebug) printf("] esr: %s\n", pszTree);
         if (execSingleDir(pszTree, 0, rlTreeFiles, oDirFiles, rlDirs, rlBytes, nLocalMaxTime, nTreeMaxTime)) 
         {
            nLocalMaxTime = 0;
            bGlblError = 1;
            return 9;
         }
         nLocalMaxTime = 0;
      }

      if (bGlblEscape)
         break;
   }

   return 0;
}

long containsWildCards(char *pszName)
{
   if (strchr(pszName, '*'))
      return 1;
   if (strchr(pszName, '?'))
      return 1;
   return 0;
}

long lastCharIsBackSlash(char *pszName)
{
   char *psz = strrchr(pszName, glblPathChar);
   if (psz && ((psz-pszName) == strlen(pszName)-1))
      return 1;
   return 0;
}

#ifndef _MSC_VER
 #define _stat stat
#endif

long isDir(char *pszName)
{
   if (containsWildCards(pszName))
      return 0;
   #ifdef _MSC_VER
   DWORD nAttrib = GetFileAttributes(pszName);
   if (nAttrib == 0xFFFFFFFF) // "INVALID_FILE_ATTRIBUTES"
      return 0;
   if (nAttrib & FILE_ATTRIBUTE_DIRECTORY)
      return 1;
   #else
   struct _stat buf;
   if (_stat(pszName, &buf)) {
      return 0;
   }
   if (buf.st_mode & _S_IFDIR )
      return 1;
   #endif
   return 0;
}

long getFileStat( // RC == 0 if exists anything
   char  *pszName,
   long   &rbIsDirectory,
   long   &rbCanRead,
   long   &rbCanWrite,
   num    &rlFileTime,
   num    &rlFileSize
 )
{
   #ifdef _MSC_VER
   // using MSC specific 64-bit filesize and time stamp infos
   #ifdef SFK_W64
   struct __stat64 buf;
   if (_stat64(pszName, &buf))
      return -1;
   #else
   struct _stati64 buf;
   if (_stati64(pszName, &buf))
      return -1;
   #endif
   rbIsDirectory = (buf.st_mode & _S_IFDIR ) ? 1 : 0;
   rbCanRead     = (buf.st_mode & _S_IREAD ) ? 1 : 0;
   rbCanWrite    = (buf.st_mode & _S_IWRITE) ? 1 : 0;
   // on old msvc, this may be 0xFFFF... for timestamps > 2038:
   rlFileTime    =  buf.st_mtime;
   rlFileSize    =  buf.st_size;
   return 0;
   #else
   // generic: may not support 64-bit filesizes and time stamps
   struct _stat buf;
   if (_stat(pszName, &buf)) {
      return -1;
   }
   rbIsDirectory = (buf.st_mode & _S_IFDIR ) ? 1 : 0;
   rlFileTime    =  buf.st_mtime;
   rlFileSize    =  buf.st_size;
   rbCanRead     = (buf.st_mode & _S_IREAD ) ? 1 : 0;
   rbCanWrite    = (buf.st_mode & _S_IWRITE) ? 1 : 0;
   return 0;
   #endif
}

num getFileSize(char *pszName) 
{
   long bIsDir    = 0;
   long bCanRead  = 1;
   long bCanWrite = 1;
   num  nFileTime = 0;
   num  nFileSize = 0;
   if (getFileStat(pszName, bIsDir, bCanRead, bCanWrite, nFileTime, nFileSize))
      return -1;
   return nFileSize;
}

num getFileTime(char *pszName)
{
   long bIsDir    = 0;
   long bCanRead  = 1;
   long bCanWrite = 1;
   num  nFileTime = 0;
   num  nFileSize = 0;
   if (getFileStat(pszName, bIsDir, bCanRead, bCanWrite, nFileTime, nFileSize))
      return 0;
   return nFileTime;
}

bool canWriteFile(char *pszName, bool bTryCreate)
{
   long bIsDir    = 0;
   long bCanRead  = 0;
   long bCanWrite = 0;
   num  nFileTime = 0;
   num  nFileSize = 0;
   if (!getFileStat(pszName, bIsDir, bCanRead, bCanWrite, nFileTime, nFileSize)) {
      return bCanWrite ? 1 : 0;
   }
   if (bTryCreate) {
      // file does not exist yet: try creation
      FILE *fout = fopen(pszName, "wb");
      if (!fout) return 0;
      fclose(fout);
      remove(pszName);
      return 1;
   }
   return 1;
}

char *timeAsString(num nTime, bool bFlat=0)
{
   static char szTimeStrBuf[200];

   // nTime may be 0xFFFF... in case of times > 2038.

   struct tm *pLocTime = 0;
   mytime_t nTime2 = (mytime_t)nTime;

   #ifdef SFK_W64
   pLocTime = _localtime64(&nTime2);   // may be NULL
   #else
   pLocTime = localtime(&nTime2);      // may be NULL
   #endif

   // size_t strftime( char *strDest, size_t maxsize, const char *format, const struct tm *timeptr );
   szTimeStrBuf[0] = '\0';
   if (pLocTime) {
      if (bFlat)
         strftime(szTimeStrBuf, sizeof(szTimeStrBuf)-10, "%Y%m%d%H%M%S", pLocTime);
      else
         strftime(szTimeStrBuf, sizeof(szTimeStrBuf)-10, "%Y-%m-%d %H:%M:%S", pLocTime);
   } else {
      if (bFlat)
         strcpy(szTimeStrBuf, "99991231235959");
      else
         strcpy(szTimeStrBuf, "9999-12-31 23:59:59");
   }

   return szTimeStrBuf;
}

static long fileExists(char *pszName) {
   FILE *ftmp = fopen(pszName, "r");
   if (!ftmp) return 0;
   fclose(ftmp);
   return 1;
}

num getCurrentTime()
{
   #ifdef _WIN32
   return (num)GetTickCount();
   #else
   struct timeval tv;
   gettimeofday(&tv, NULL);
   return tv.tv_sec * 1000 + tv.tv_usec / 1000;
   #endif
}

ulong currentElapsedMSec() {
   return getCurrentTime() - nGlblStartTime;
}

ulong currentKBPerSec() {
   ulong lMSElapsed = currentElapsedMSec();
   if (lMSElapsed == 0) lMSElapsed = 1;
   return (ulong)(nGlblBytes / lMSElapsed);
}

char szCmpBuf1[4096];
char szCmpBuf2[4096];
long mystrstri(char *psz1, char *psz2)
{
   strncpy(szCmpBuf1, psz1, sizeof(szCmpBuf1)-10);
   szCmpBuf1[sizeof(szCmpBuf1)-10] = '\0';

   strncpy(szCmpBuf2, psz2, sizeof(szCmpBuf2)-10);
   szCmpBuf2[sizeof(szCmpBuf2)-10] = '\0';

   long slen1 = strlen(szCmpBuf1);
   long slen2 = strlen(szCmpBuf2);

   for (long i1=0; i1<slen1; i1++)
      szCmpBuf1[i1] = tolower(szCmpBuf1[i1]);

   for (long i2=0; i2<slen2; i2++)
      szCmpBuf2[i2] = tolower(szCmpBuf2[i2]);

   return (strstr(szCmpBuf1, szCmpBuf2) != 0) ? 1 : 0;
}

#ifdef _WIN32
long getKeyPress()
{
   DWORD dwNumEvents, dwEventsPeeked, dwInputEvents;
   DWORD iEvent, iPrevEvent;
   INPUT_RECORD aInputBuffer[1];

   HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);

   if (!GetNumberOfConsoleInputEvents(hStdIn, &dwNumEvents))
      return -1;

   if (dwNumEvents <= 0)
      return -1;

   if (!PeekConsoleInput(hStdIn, aInputBuffer, 1, &dwEventsPeeked))
      return -1;
   
   if (!ReadConsoleInput(hStdIn, &aInputBuffer[0], 1, &dwInputEvents))
      return -1;

   if (dwInputEvents != 1)
      return -1;

   if (aInputBuffer[0].EventType == KEY_EVENT)
      return aInputBuffer[0].Event.KeyEvent.wVirtualKeyCode;

   return -1;
}

bool userInterrupt() {
   if (bGlblEscape)
      return 1;
   static bool bTold = 0;
   if (getKeyPress() == VK_ESCAPE) {
      if (!bTold) {
         bTold = 1;
         printf("[aborted by user]%.50s\n", pszGlblBlank);
      }
      bGlblEscape = 1;
      return 1;
   }
   return 0;
}
#else
bool userInterrupt() { return 0; }
#endif

class TextFile {
public:
   TextFile (char *pszInFileName);
  ~TextFile ( );
   long  loadFromFile      ( );
   long  writeToFile       ( );
   long  createFromMemory  (char *pszInTextMemBlock);
   char  *getFileName      ( ) { return pszClFileName; }
   long  dumpTo            (FILE *fout);
   TextFile *clone         ( );
   bool  equals            (TextFile *pOther);
   void  setTouched        (bool b) { bClTouched = b; }
   bool  isTouched         ( )      { return bClTouched; }
   bool  fileStatChanged   ( );  // 1==yes, 2==gone
   bool  isContentValid    ( );  // i.e., contains no :create etc.
   long  dataSize          ( )      { return nClDataSize; }
   long  numberOfLines     ( )      { return nClLines; }
   bool  isReadOnly        ( )      { return bClReadOnly; }
   void  setReadOnly       (bool b) { bClReadOnly = b; }
private:
   void  checkIntegrity    ( );
   char  *pszClFileName;
   long  nClLines;
   char  **apClLines;
   char  *pClData;
   long  nClDataSize;
   num   lClFileTime;
   bool  bClTouched;
   bool  bClReadOnly;
};

// all keywords of sfk and patch files
char *apBlockedKeys[] = 
{
   // input files containing these keys might break a cluster file's syntax. 
   // therefore input files containing them must be skipped.
   ":snapfile ", ":cluster ", ":create ",
   ":done\n", ":done\r\n",
   ":cluster-end\n", ":cluster-end\r\n",

   // these keys are less relevant, but also blocked by default.
   // if sfk refuses to integrate some files into a cluster,
   // you may try to ease restrictions by setting this flag:
   #ifndef SFK_LESS_STRICT_KEY_BLOCKING
   ":file:\n", ":file:\r\n",
   ":skip-begin\n" , ":skip-begin\r\n"
   ":skip-end\n"   , ":skip-end\r\n",
   ":patch ", ":info ", ":root ", ":file ",
   ":from\n", ":from\r\n",
   ":to\n", ":to\r\n",
   ":mkdir ", ":select-replace ", ":set ",
   ":edit ", ":READ ",
   #endif

   0 // EOT
};

bool TextFile::isContentValid() {
   if (!pClData || !apClLines) { fprintf(stderr, "error: internal #40\n"); return 9; }
   for (long i1=0; i1<nClLines; i1++) {
      char *psz1 = apClLines[i1];
      for (long i2=0; apBlockedKeys[i2]; i2++) {
         if (!strncmp(psz1, apBlockedKeys[i2], strlen(apBlockedKeys[i2]))) {
            fprintf(stderr, "info : excluding file from input: %s\n", pszClFileName);
            fprintf(stderr, "info : contains line beginning with %s\n", apBlockedKeys[i2]);
         // fprintf(stderr, "info : ... will not recursively collect cluster files.\n");
            return 0; // == false
         }
      }
   }
   return 1; // == true
}

bool TextFile::fileStatChanged() {
   long bIsDir    = 0;
   long bCanRead  = 1;
   long bCanWrite = 1;
   num  lFileTime = 0;
   num  nFileSize = 0;
   if (getFileStat(pszClFileName, bIsDir, bCanRead, bCanWrite, lFileTime, nFileSize)) {
      // fprintf(stderr, "error: unable to re-read file: %s\n", pszClFileName);
      return 2;
   }
   // change in file attributes?
   long bReadOnly = 1 - bCanWrite;
   if (bReadOnly != bClReadOnly)
      return 1;
   // change in file time?
   if (lFileTime != lClFileTime)
      return 1;
   return 0;
}

TextFile::TextFile(char *pszFileName) {
   pszClFileName  = strdup(pszFileName);
   nClLines       = 0;
   apClLines      = 0;
   pClData        = 0;
   nClDataSize    = 0;
   lClFileTime    = 0;
   bClTouched     = 0;
   bClReadOnly    = 0;
}

TextFile::~TextFile() {
   if (pszClFileName)   delete [] pszClFileName;
   if (pClData)         delete [] pClData;
   if (apClLines)       delete [] apClLines;
}

bool TextFile::equals(TextFile *pOther) {
   checkIntegrity();
   pOther->checkIntegrity();
   if (strcmp(pszClFileName, pOther->pszClFileName)) return 0;
   if (nClLines != pOther->nClLines) return 0;
   for (long i=0; i<nClLines; i++)
      if (strcmp(apClLines[i], pOther->apClLines[i]))
         return 0;
   if (pOther->isReadOnly() != isReadOnly())
      return 0;
   return 1;
}

void TextFile::checkIntegrity() {
   if (!pszClFileName) { fprintf(stderr, "error: internal #20\n"); exit(1); }
   if (!nClLines     ) { return; } // fprintf(stderr, "error: internal #21\n"); exit(1); }
   if (!apClLines    ) { fprintf(stderr, "error: internal #22\n"); exit(1); }
   if (!pClData      ) { fprintf(stderr, "error: internal #23\n"); exit(1); }
   for (long i=0; i<nClLines; i++)
      if (!apClLines[i])
         { fprintf(stderr, "error: internal #24\n"); exit(1); }
}

TextFile *TextFile::clone() {
   checkIntegrity();
   TextFile *pNew = new TextFile(pszClFileName);
   if (!(pNew->pClData = new char[nClDataSize+10]))
      return 0;
   pNew->nClDataSize = nClDataSize;
   memcpy(pNew->pClData, pClData, nClDataSize);
   pNew->pClData[nClDataSize] = '\0';
   pNew->apClLines = new char*[nClLines];
   for (long i=0; i<nClLines; i++)
      pNew->apClLines[i] = apClLines[i] - pClData + pNew->pClData;
   pNew->nClLines = nClLines;
   pNew->lClFileTime = lClFileTime;
   pNew->bClReadOnly = bClReadOnly;
   // pszClFileName was done in new's ctr
   return pNew;
}

long TextFile::dumpTo(FILE *fout) {
   // size_t fwrite( const void *buffer, size_t size, size_t count, FILE *stream );
   // int fputs( const char *string, FILE *stream );
   for (long i=0; i<nClLines; i++) {
      fputs(apClLines[i], fout);
      fputc('\n', fout);
   }
   return 0;
}

long fileSize(char *pszFile) {
   struct stat sinfo;
   if (stat(pszFile, &sinfo))
      return -1;
   return sinfo.st_size;
}

char *loadFile(char *pszFile, int nLine) 
{
   long lFileSize = fileSize(pszFile);
   if (lFileSize < 0)
      return 0;
   char *pOut = new char[lFileSize+10];
   // printf("loadFile %p %d\n", pOut, nLine);
   FILE *fin = fopen(pszFile, "rb");
   if (!fin) { fprintf(stderr, "error: cannot read: %s\n", pszFile); return 0; }
   long nRead = fread(pOut, 1, lFileSize, fin);
   fclose(fin);
   if (nRead != lFileSize) {
      fprintf(stderr, "error: cannot read: %s (%d %d)\n", pszFile, nRead, lFileSize);
      delete [] pOut;
      return 0;
   }
   // printf("%u bytes read from %s\n", nRead, pszFile);
   pOut[lFileSize] = '\0';
   return pOut;
}

uchar *loadBinaryFile(char *pszFile, long &rnFileSize) {
   long lFileSize = fileSize(pszFile);
   if (lFileSize < 0)
      return 0;
   char *pOut = new char[lFileSize+10];
   FILE *fin = fopen(pszFile, "rb");
   if (!fin) {
      fprintf(stderr, "error: cannot read: %s\n", pszFile); 
      delete [] pOut; 
      return 0; 
   }
   long nRead = fread(pOut, 1, lFileSize, fin);
   fclose(fin);
   if (nRead != lFileSize) {
      fprintf(stderr, "error: cannot read: %s (%d %d)\n", pszFile, nRead, lFileSize);
      delete [] pOut;
      return 0;
   }
   pOut[lFileSize] = '\0';
   rnFileSize = lFileSize;
   return (uchar*)pOut;
}

long TextFile::loadFromFile() 
{
   if (fileSize(pszClFileName) == 0) {
      printf("warning: zero-sized file, skipping: %s\n", pszClFileName);
      return 1;
   }

   // get all infos about the file to load
   long bIsDir    = 0;
   long bCanRead  = 1;
   long bCanWrite = 1;
   num  lFileTime = 0;
   num  nFileSize = 0;
   if (getFileStat(pszClFileName, bIsDir, bCanRead, bCanWrite, lFileTime, nFileSize)) {
      printf("warning: cannot read file, skipping: %s\n", pszClFileName);
      return 1;
   }
   // remember only the stats we care about
   lClFileTime  = lFileTime;
   bClReadOnly  = 1 - bCanWrite;
   // printf("%d on %s\n",bClReadOnly,pszClFileName);

   char *pszRaw = loadFile(pszClFileName, __LINE__);
   if (!pszRaw)
      return 9;

   long lRC = createFromMemory(pszRaw);
   delete [] pszRaw;
   return lRC;
}

long TextFile::writeToFile() {
   FILE *fout = fopen(pszClFileName, "w");
   if (!fout) { fprintf(stderr, "error: unable to write: %s\n", pszClFileName); return 9; }
   dumpTo(fout);
   fclose(fout);
   lClFileTime = getFileTime(pszClFileName);
   return 0;
}

long TextFile::createFromMemory(char *pszRaw) 
{
   // delete old data, if any
   if (pClData) delete [] pClData;
   if (apClLines) delete [] apClLines;

   // copy new raw data
   nClDataSize = strlen(pszRaw);
   pClData     = new char[nClDataSize+10];
   memcpy(pClData, pszRaw, nClDataSize);
   pClData[nClDataSize] = '\0';

   // create line index
   apClLines   = 0;

   // determine number of lines in raw data.
   long nLineFeeds = 0;
   for (long i1=0; i1<nClDataSize; i1++) {
      if (pClData[i1] == '\n')
         nLineFeeds++;
   }

   // if the very last content has no linefeed,
   // it counts as a line without linefeed.
   if (pClData[nClDataSize-1] != '\n')
      nClLines = nLineFeeds+1;
   else
      nClLines = nLineFeeds;

   // convert raw data into array of lines
   apClLines = new char*[nClLines+2];
   long iLine = 0;
   char *psz1 = pClData;
   while (psz1 && *psz1 && (iLine < nClLines)) {
      char *psz2 = strchr(psz1, '\n');
      if (psz2) *psz2++  = '\0'; // remove LF, skip past LF
      apClLines[iLine++] = psz1;
      char *pszCR = strchr(psz1, '\r');
      if (pszCR) *pszCR = '\0';
      psz1 = psz2;
   }
   if (iLine != nClLines) { fprintf(stderr, "error: internal error #10 %d %d %p\n",iLine,nClLines,psz1); return 9; }

   // printf("[%d lines read, %s]\n", nClLines, pszClFileName);
   if (!isContentValid())
      return 9;

   return 0;
}

#define MAX_SYNC_INFO     10
#define MAX_NOTES_LINES 1000

class SnapShot {
public:
   SnapShot (const char *pszID);
  ~SnapShot ( );
   long  addTarget   (char *pszFileName);
   long  addTarget   (TextFile *pTarget);
   void  setFileName (char *pszFileName);
   char *getFileName ( ) { return pClFileName; }
   long  writeToFile ( );
   long  readFromFile(long &rbDroppedAny, long bForceBuildMatch);
   void  addNotesLine(char *pszLine);
   void  setRootDir  (char *pszDirName);
   long  copyTargetsFrom   (SnapShot &rFrom);
   long  numberOfTargets   ( ) { return nClTargets; }
   long  syncDownTargets   (SnapShot &oMaster, long &nSync);
   long  syncUpTargets     (SnapShot &oSrc, long &nSynced);
   long  mirrorTargetsFrom (SnapShot &oSrc, long &nMissing); // copies filenames, but up-loads
   long  checkLoadTargets  ( );
   long  hasTarget   (char *pszFileName); // based on the real target list
   long  dropTarget  (char *pszFileName); // just the entry, doesn't delete the file
   void  dumpTargets ( );
   void  shutdown    ( );
   void  setAllTouched     ( );
   void  resetLastSync     ( );
   void  registerLastSync  (char *pszInfo);
   char *getLastSyncInfo   (unsigned long iIndex);
   void  mapCompilerOutput (bool bMix, char *pszCmd);
protected:
   void  expandTargets     (long lSoMuch);
   long  removeTargetEntry (long n);
   void  adjustNamePadding ( );
   void  resetTargets      (const char *pszInfo);
   void  resetNotes        ( );
   TextFile **apClTargets;
   long  nClMaxTargets;
   long  nClTargets;
   char  *pClFileName;
   char  *pClRootName;
   long  nClNamePadding;
   long  lClLastSavedBuild;
   long  lClLoadedRevision;
   char  *apLastSync[MAX_SYNC_INFO];
   char  *apNotes[MAX_NOTES_LINES+10]; // used only in FileSnap
   long  nClNotes;
   const char *pszClID;
};

SnapShot glblMemSnap("mem");
SnapShot glblFileSnap("fil");

SnapShot::SnapShot(const char *pszID) {
   apClTargets       =  0;
   nClMaxTargets     =  0;
   nClTargets        =  0;
   pClFileName       =  0;
   pClRootName       =  0;
   nClNamePadding    = 30;
   lClLastSavedBuild = 0;
   for (int i=0; i<MAX_SYNC_INFO; i++)
      apLastSync[i]  = 0;
   memset(apNotes, 0, sizeof(apNotes));
   nClNotes          = 0;
   pszClID           = pszID;
   lClLoadedRevision = 0;
}

SnapShot::~SnapShot() {
   shutdown();
}

void SnapShot::addNotesLine(char *pszLine) {
   if (nClNotes < MAX_NOTES_LINES) {
      apNotes[nClNotes++] = strdup(pszLine);
   } else {
      fprintf(stderr, "warning: max. number of notes lines exceeded (%ld), ignoring\n", (long)MAX_NOTES_LINES);
   }
}

void SnapShot::shutdown() {
   resetTargets("shutdown");
   resetLastSync();
   if (pClFileName)  { delete [] pClFileName; pClFileName = 0; }
   if (pClRootName)  { delete [] pClRootName; pClRootName = 0; }
   resetNotes();
}

void SnapShot::resetNotes() {
   for (int i=0; i<nClNotes; i++)
      delete [] apNotes[i];
   memset(apNotes, 0, sizeof(apNotes));
   nClNotes = 0;
}

void SnapShot::resetLastSync() {
   for (int i=0; i<MAX_SYNC_INFO; i++)
      if (apLastSync[i]) {
         delete [] apLastSync[i];
         apLastSync[i] = 0;
      }
}

void SnapShot::registerLastSync(char *pszFileName) {
   for (int i=0; i<MAX_SYNC_INFO; i++)
      if (!apLastSync[i]) {
         apLastSync[i] = strdup(pszFileName);
         break;
      }
}

char *SnapShot::getLastSyncInfo(unsigned long iIndex) {
   if (iIndex < MAX_SYNC_INFO)
      return apLastSync[iIndex];
   return 0;
}

char szPadBuf[1024];
char *padString(char *psz1, long lLen) {
   long lBaseLen = strlen(psz1);
   long lMaxLen  = sizeof(szPadBuf)-10;
   if (lBaseLen > lMaxLen)
       lBaseLen = lMaxLen;
   if (lLen > lMaxLen)
       lLen = lMaxLen;
   strncpy(szPadBuf, psz1, lBaseLen);
   while (lBaseLen < lLen)
      szPadBuf[lBaseLen++] = ' ';
   szPadBuf[lBaseLen] = '\0';
   return szPadBuf;
}

/*
   /models/DeviceControllerSystemModel/SYS/sources/BHDiagManager.cpp:109: #error testError01
   cc: C:/Programme/QNX630BH/host/win32/x86/usr/lib/gcc-lib/ntosh/2.95.3/cpp0 caught signal 33
*/

void SnapShot::mapCompilerOutput(bool bMixMappedWithUnmappedOutput, char *pszCmd)
{
   // read compiler errors from stdin
   long nMaxLineLen = sizeof(szLineBuf)-10;
   long nLine = 0;
   long nMaps = 0;
   while (fgets(szLineBuf, nMaxLineLen, stdin))
   {
      nLine++;

      char *psz1 = 0;
      if (psz1 = strchr(szLineBuf, '\n'))
         *psz1 = '\0';
      if (psz1 = strchr(szLineBuf, '\r'))
         *psz1 = '\0';

      // any indication for an error or warning?
      // if (!strstr(szLineBuf, "err") && !strstr(szLineBuf, "warn")) {
      //    // no: skip input, copy-through if required
      //    if (bMixMappedWithUnmappedOutput)
      //       printf("] %s\n", szLineBuf);
      //    continue;
      // }

      // map every potential path info to our path char
      while (psz1 = strchr(szLineBuf, glblWrongPChar))
         *psz1 = glblPathChar;

      // calc line number of first target's :create
      ulong nBaseLine = 0;
      nBaseLine += 5;            // header before index
      nBaseLine += nClTargets;   // index lines
      nBaseLine += 2;            // header after index

      // now search for matching patch expressions
      char *pszHit = 0;
      TextFile *pTarget = 0;
      char *pszTargName = 0;
      for (long i=0; i<nClTargets && !pszHit; i++) {
         nBaseLine++; // skip :create, now on 1st line of content
         pTarget = apClTargets[i];
         pszTargName = pTarget->getFileName();
         if (!(pszHit = strstr(szLineBuf, pszTargName))) {
            // adjust line start number within index
            nBaseLine += pTarget->numberOfLines();
            nBaseLine += 2; // skip :done and blank line
         }
      }

      char szLineNum[100];
      ulong iLineNum =  0;
      ulong iMaxSeek = 10; // search a max. of 10 chars for line number, past filename
      if (pszHit) {
         // a filename from the cluster appeared.
         // can we identify a line number nearby behind?
         for (char *psz2 = pszHit+strlen(pszTargName);
              *psz2 && (iLineNum < sizeof(szLineNum)-10) && (iMaxSeek-- > 0);
              psz2++)
         {
            if (*psz2 >= '0' && *psz2 <= '9')
               szLineNum[iLineNum++] = *psz2;
            else
            if (iLineNum > 0) // end of number stream
               break;
         }
      }
      szLineNum[iLineNum] = '\0';

      if ((iLineNum > 0) && pTarget && pszTargName) 
      {
         // dump mapped output.
         printf("* %s\n", szLineBuf);
         // it seems we have a filename and a line number.
         ulong aRelLine = (ulong)atol(szLineNum);
         printf("* ===> %s %u\n", getFileName(), nBaseLine+aRelLine);
         // if this is the first hit, exec optional command
         if (nMaps==0 && pszCmd!=0) {
            sprintf(szLineBuf2, "%s %s %d", pszCmd, getFileName(), nBaseLine+aRelLine);
            // printf("RUN: %s\n", szLineBuf2);
            int iRC = system(szLineBuf2);
            if (iRC) { fprintf(stderr, "error: unable to run: %s, rc %d\n", szLineBuf2, iRC); }
         }
         nMaps++;
      }
      else 
      if (bMixMappedWithUnmappedOutput)
      {
         // copy-through compiler output
         printf("] %s\n", szLineBuf);
      }
   }
   // printf("] %u mappings for %s, %u lines\n", nMaps, getFileName(), nLine);
}

long SnapShot::hasTarget(char *pszTargetName) {
   for (long i=0; i<nClTargets; i++) {
      TextFile *pTarget = apClTargets[i];
      if (!strcmp(pTarget->getFileName(), pszTargetName))
         return 1;
   }
   return 0;
}

long SnapShot::dropTarget(char *pszFileName) {
   // remove from list of targets
   long i2, bDone=0;
   for (i2=0; i2<nClTargets; i2++) {
      TextFile *pTarget = apClTargets[i2];
      if (!strcmp(pTarget->getFileName(), pszFileName)) {
         removeTargetEntry(i2);
         bDone = 1;
         break;
      }
   }
   if (!bDone) {
      fprintf(stderr, "warning: not in target list: %s\n", pszFileName);
      dumpTargets();
   }
   return 0;
}

void SnapShot::dumpTargets() {
   for (long i=0; i<nClTargets; i++)
      printf("... %s\n",apClTargets[i]->getFileName());
}

void SnapShot::setAllTouched() {
   for (long i=0; i<nClTargets; i++)
      apClTargets[i]->setTouched(1);
}

long SnapShot::removeTargetEntry(long n) {
   if (n >= nClTargets) { fprintf(stderr, "error: internal #53\n"); return 9; }
   delete apClTargets[n];
   for (long k=n; k<nClTargets-1; k++)
      apClTargets[k] = apClTargets[k+1];
   nClTargets--;
   // printf("[ dropped target entry %d, %d remaining ]\n", n, nClTargets);
   return 0;
}

long SnapShot::syncDownTargets(SnapShot &oMaster, long &rnSync)
{
   resetLastSync();

   if (nClTargets != oMaster.nClTargets) {
      fprintf(stderr, "error: target number differs (%s %d, %s %d)\n", oMaster.pszClID, oMaster.nClTargets, pszClID, nClTargets);
      return 9;
   }

   long nSynced = 0;

   // on every difference to master, take master's target
   if (nClTargets != oMaster.nClTargets) { fprintf(stderr, "error: number of targets changed (%u %u)\n", nClTargets, oMaster.nClTargets); return 9; }
   // we expect an absolutely identical list of targets, with same sequence
   for (long iTarg=0; iTarg<nClTargets; iTarg++) {
      TextFile *pMemTarget    = apClTargets[iTarg];
      TextFile *pMasterTarget = oMaster.apClTargets[iTarg];
      if (strcmp(pMemTarget->getFileName(), pMasterTarget->getFileName())) {
         fprintf(stderr, "error: change in target names or sequence:\n%s\n%s\n",pMemTarget->getFileName(), pMasterTarget->getFileName());
         return 9;
      }
      // printf("[check: %s]\n",pMemTarget->getFileName());
      if (!pMemTarget->equals(pMasterTarget)) {
         // pre-check: is target writeable at all?
         if (!pMemTarget->isReadOnly()) {
            // on every difference, take master's target data
            // first, collect some difference stats
            long lSizeDiff = pMasterTarget->dataSize() - pMemTarget->dataSize(); 
            long lLineDiff = pMasterTarget->numberOfLines() - pMemTarget->numberOfLines(); 
            // printf("[internal-replace: %s]\n", oMaster.getFileName());
            TextFile *pDownClone = oMaster.apClTargets[iTarg]->clone();
            if (!pDownClone) { fprintf(stderr, "error: internal #11\n"); return 9; }
            delete apClTargets[iTarg];
            apClTargets[iTarg] = pDownClone;
            // and write down to file
            // printf("[write: %s]\n",pDownClone->getFileName());
            printf("[ WRITE: %s %s%d size %s%d lines]\n",
               padString(pDownClone->getFileName(), nClNamePadding),
               (lSizeDiff >= 0) ? "+":"", lSizeDiff,
               (lLineDiff >= 0) ? "+":"", lLineDiff
               );
            if (pDownClone->writeToFile())
               return 9;
            nSynced++;
            registerLastSync(pDownClone->getFileName());
         } else {
            // conflict: target is read-only
            printf("[ *!*!*: %s is READ-ONLY, will not write.]\n",
                  padString(pMemTarget->getFileName(), nClNamePadding)
               );
         }
      }
   }
   rnSync = nSynced;
   return 0;
}

long SnapShot::mirrorTargetsFrom(SnapShot &oMaster, long &nMissing)
{
   resetTargets("mirror");

   nClMaxTargets  = oMaster.nClMaxTargets;
   apClTargets    = new TextFile*[nClMaxTargets];
   nClTargets     = oMaster.nClTargets;

   long lMissing = 0;
   for (long iTarg=0; iTarg<nClTargets; iTarg++) 
   {
      TextFile *pMasterTarget = oMaster.apClTargets[iTarg];
      TextFile *pCopy = new TextFile(pMasterTarget->getFileName());
      apClTargets[iTarg] = pCopy;
      if (pCopy->loadFromFile()) {
         // fprintf(stderr, "[ -n/a-: %s\n", pCopy->getFileName());
         lMissing++;
      }
   }

   adjustNamePadding();

   nMissing = lMissing;
   return 0;
}

long SnapShot::syncUpTargets(SnapShot &oMaster, long &rnSynced) {
   long nSynced = 0;
   // on every difference to master, take master's target
   if (nClTargets != oMaster.nClTargets) { fprintf(stderr, "error: number of targets changed (%u %u)\n", nClTargets, oMaster.nClTargets); return 9; }
   // we expect an absolutely identical list of targets, with same sequence
   for (long iTarg=0; iTarg<nClTargets; iTarg++) {
      TextFile *pMemTarget      = oMaster.apClTargets[iTarg];
      TextFile *pSnapFileTarget = apClTargets[iTarg];
      if (strcmp(pMemTarget->getFileName(), pSnapFileTarget->getFileName())) {
         fprintf(stderr, "error: change in target names or sequence:\n%s\n%s\n",pMemTarget->getFileName(), pSnapFileTarget->getFileName());
         return 9;
      }
      if (!pMemTarget->isTouched())
         continue;
      pMemTarget->setTouched(0);
      // printf("[check: %s]\n",pMemTarget->getFileName());
      if (pMemTarget->equals(pSnapFileTarget)) {
         printf("[ NODIF: %s ]\n", pMemTarget->getFileName());
      } else {
         // on every difference, take master's target data
         long lSizeDiff = pMemTarget->dataSize() - pSnapFileTarget->dataSize();
         long lLineDiff = pMemTarget->numberOfLines() - pSnapFileTarget->numberOfLines();
         // printf("[internal-replace: %s]\n", oMaster.getFileName());
         TextFile *pUpClone = pMemTarget->clone();
         if (!pUpClone) { fprintf(stderr, "error: internal #30\n"); return 9; }
         delete apClTargets[iTarg];
         apClTargets[iTarg] = pUpClone;
         printf("[ READ : %s %s%d size %s%d lines]\n", 
            padString(pUpClone->getFileName(), nClNamePadding),
            (lSizeDiff >= 0) ? "+":"", lSizeDiff,
            (lLineDiff >= 0) ? "+":"", lLineDiff
            );
         nSynced++;
      }
   }
   // if any content really synced, re-write whole snapfile
   if (nSynced > 0) {
      // printf("[ WRITE: %s ]\n", getFileName());
      writeToFile();
   }
   rnSynced = nSynced;
   return 0;
}

long SnapShot::checkLoadTargets() {
   long lRC = 0;
   bool bReRun = false;
   do {
    bReRun = false;
    for (long iTarg=0; iTarg<nClTargets; iTarg++) {
      TextFile *pMemTarget = apClTargets[iTarg];
      long lStatRC = 0;
      if (lStatRC = pMemTarget->fileStatChanged()) {
         // printf("[ RELOAD: %s ]\n", pMemTarget->getFileName());
         if ((lStatRC==2) || pMemTarget->loadFromFile()) {
            // a target was probably deleted
            char *pszGoneFile = pMemTarget->getFileName();
            printf("[ DROP  : %s - file unreadable ]\n", pszGoneFile);
            // remove from both snapshots
            glblFileSnap.dropTarget(pszGoneFile);
            removeTargetEntry(iTarg);
            // must re-run our loop, iTarg's now invalid
            bReRun = true;
            lRC |= 2;
            break;
         } else {
            // file load succeded
            pMemTarget->setTouched(1);
            lRC |= 1;
         }
      }
    } // endfor
   }
   while (bReRun);
   return lRC;
}

void SnapShot::resetTargets(const char *pszInfo) 
{
   if (bGlblDebug) printf("%s reset tlist due to %s\n", pszClID, pszInfo);
   if (apClTargets) {
      for (long i=0; i<nClTargets; i++)
         delete apClTargets[i];
      delete [] apClTargets;
      apClTargets   = 0;
      nClMaxTargets = 0;
      nClTargets    = 0;
   }
}

long SnapShot::copyTargetsFrom(SnapShot &oSrc) 
{
   resetTargets("copy");
   nClMaxTargets = oSrc.nClMaxTargets;
   if (!(apClTargets = new TextFile*[nClMaxTargets]))
      return -1;
   nClTargets    = oSrc.nClTargets;
   for (long i=0; i<nClTargets; i++) {
      // printf("[cloning %s]\n",oSrc.apClTargets[i]->getFileName());
      if (!(apClTargets[i] = oSrc.apClTargets[i]->clone()))
         return -1;
   }
   // printf("[%d targets cloned]\n",nClTargets);
   return 0;
}

void SnapShot::setFileName(char *pszFileName) {
   pClFileName = strdup(pszFileName);
}

void SnapShot::setRootDir(char *pszDirName) {
   pClRootName = strdup(pszDirName);
}

long SnapShot::writeToFile() 
{
   if (!pClFileName || !pClRootName) { fprintf(stderr, "error: missing filename, or root\n"); return 9; }

   FILE *fout = fopen(pClFileName, "w");

   fprintf(fout,
      "%s,build=%u\n\n"
      ":root %s\n\n",
      pszGlblClusterFileStamp,
      ++lClLastSavedBuild,
      pClRootName);

   // write notes, if any
   if (nClNotes > 0)
   {
      fprintf(fout, ":notes-begin\n");
      for (long i0=0; i0<nClNotes; i0++) {
         fprintf(fout, "%s\n", apNotes[i0]);
      }
      fprintf(fout, ":notes-end\n\n");
   }

   // write target index
   fprintf(fout, ":# ----- %d target files -----\n", nClTargets);
   for (long i1=0; i1<nClTargets; i1++) {
      TextFile *pTarget = apClTargets[i1];
      if (pTarget->isReadOnly())
         fprintf(fout, ":READ %s\n", pTarget->getFileName());
      else
         fprintf(fout, ":edit %s\n", pTarget->getFileName());
   }
   fprintf(fout, ":# ----- target index end -----\n\n");
   
   // write all targets
   for (long i2=0; i2<nClTargets; i2++) {
      TextFile *pTarget = apClTargets[i2];
      fprintf(fout, ":create %s\n", pTarget->getFileName());
      pTarget->dumpTo(fout); // writes line by line, with guranteed LF at end
      fprintf(fout, ":done\n\n");
   }

   // write epilogue
   fprintf(fout, ":cluster-end\n");

   fclose(fout);
   return 0;
}

long SnapShot::readFromFile(long &rbDroppedAny, long bForceBuildMatch) 
{
   resetTargets("read");
   resetNotes();

   if (!pClFileName) { fprintf(stderr, "error: missing filename, or root\n"); return 9; }

   char *pszRaw = 0;
   long lRetryCnt = 0;
   long lOldLen  = -1;
   long lOldBail = 0;
   while (true)
   {
      // load snapfile in one block
      pszRaw = loadFile(pClFileName, __LINE__);
      if (!pszRaw) return 9;
   
      // NOTE: if the editor is writing to the cluster RIGHT NOW,
      //       we get incomplete data. therefore:
      if (   !strstr(pszRaw, ":cluster-end\n")
          && !strstr(pszRaw, ":cluster-end\r\n")
         ) 
      {
         // keep some compat to old-format clusters:

         // direct revision check, if available
         if (!strncmp(pszRaw, ":cluster sfk,1.0,", strlen(":cluster sfk,1.0,"))) {
            printf("info : old-format cluster detected, loaded.\n");
            printf("info : please add \":cluster-end\" line at end of file.\n");
            break;
         }

         // if size doesn't change over 3 sec, load although
         long lNewLen = strlen(pszRaw);
         if (lOldLen == lNewLen) {
            if (++lOldBail >= 3) {
               printf("info : probably old-format cluster, loaded.\n");
               printf("info : please add \":cluster-end\" line at end of file.\n");
               break;
            }
         }
         lOldLen = lNewLen;

         // else drop current load, retry
         delete [] pszRaw;
         lRetryCnt++;
         printf("info : cluster probably locked, retrying (%d) \r", lRetryCnt);
         fflush(stdout);
         doSleep(1000);
      }
      else
      {
         if (lRetryCnt) printf("info : cluster read completed.               \n");
         break; // full load done, continue
      }
      if (userInterrupt())
         return 9;
   }

   long lRawSize = strlen(pszRaw);
   if (lRawSize < 100) { fprintf(stderr, "error: insufficient bytes from %s, %u\n", pClFileName, lRawSize); return 9; }

   // pass 1: parse control block, build target index
   char *psz1 = pszRaw;

   long nIndexSize = 0;
   char **apIndex  = 0;
   long nIndexUsed = 0;

   rbDroppedAny = 0;
   bool bWithinNotes = 0;

   while (psz1)
   {
      // printf("] \"%.10s\"\n", psz1);

      // fetch another control line
      char *psz2 = strchr(psz1, '\n');
      if (psz2)
      {
         char *pszContinue = psz2+1;
         int nLineLen = psz2 - psz1;
         if (nLineLen > MAX_LINE_LEN) nLineLen = MAX_LINE_LEN;
         strncpy(szLineBuf, psz1, nLineLen);
         szLineBuf[nLineLen] = '\0';
         // finish control line
         char *pszCR = strchr(szLineBuf, '\r');
         if (pszCR) *pszCR = '\0';

         // parse control line
         if (!strcmp(szLineBuf, ":notes-begin")) {
            bWithinNotes = true;
         }
         else
         if (bWithinNotes) {
            if (!strcmp(szLineBuf, ":notes-end")) {
               bWithinNotes = false;
            }
            else
               addNotesLine(szLineBuf);
         }
         else
         if (!strncmp(szLineBuf, ":cluster ", strlen(":cluster "))) 
         {
            // process header line
            char *psz1 = strstr(szLineBuf, ",build=");
            if (psz1) {
               // verify if user forgot to reload the fileset
               psz1 += strlen(",build=");
               long lFileBuild = atol(psz1);
               if (bForceBuildMatch) {
                  if (lFileBuild != lClLastSavedBuild) {
                     printf("[ ERROR: YOU FORGOT TO RELOAD THE CLUSTER, AND TRY TO SAVE CHANGES !! ]\n");
                     return 9;
                  }
               } else {
                  lClLastSavedBuild = lFileBuild;
               }
            }
            // retrieve revision from ":cluster sfk,1.0,prefix=:"
            char *psz2 = strstr(szLineBuf, "sfk,");
            if (psz2) {
               // so far, there are just a few possible revisions
               psz2 += strlen("sfk,");
               if (!strncmp(psz2, "1.0,", strlen("1.0,")))
                  lClLoadedRevision = 0x010000;
               else
               if (!strncmp(psz2, "1.0.7,", strlen("1.0.7,")))
                  lClLoadedRevision = 0x010007;
               else
               {
                  static bool bWarned = 0;
                  if (!bWarned) {
                     bWarned = 1;
                     fprintf(stderr, "warn : cluster syntax may be too new for this sfk.\n");
                     lClLoadedRevision = 0x010007;
                  }
               }
            }
            // if (bGlblDebug) printf("fetched :cluster\n");
         }
         else
         if (!strncmp(szLineBuf, ":root ", strlen(":root "))) {
            // verify root of cluster
            char *pszTmpRoot = &szLineBuf[strlen(":root ")];
            if (!pClRootName) { fprintf(stderr, "error: internal #50\n"); return 9; }
            if (strcmp(pszTmpRoot, pClRootName)) { fprintf(stderr, "error: root mismatch: make sure you are within the directory: %s\n", pszTmpRoot); return 9; }
            // if (bGlblDebug) printf("fetched :root\n");
         }
         else
         if (   !strncmp(szLineBuf, ":edit ", strlen(":edit "))
             || !strncmp(szLineBuf, ":READ ", strlen(":READ "))
            ) 
         {
            // build temporary index table
            if (nIndexUsed == nIndexSize) {
               // expand table
               long nAddSize = (nIndexSize == 0) ? 2 : nIndexSize;
               long nNewSize = nIndexSize + nAddSize;
               char **apNew  = new char*[nNewSize+10];
               if (nIndexUsed > 0)
                  memcpy(apNew, apIndex, sizeof(char*) * nIndexUsed);
               delete [] apIndex;
               apIndex    = apNew;
               nIndexSize = nNewSize;
               // printf("index expanded:\n");
               // for (long i=0; i<nIndexUsed; i++)
               //    printf("   %s\n", apIndex[i]);
            }
            // add next entry, INCLUDING the :edit or :READ statement!
            apIndex[nIndexUsed++] = strdup(szLineBuf);
            // printf("add-index %s\n",szLineBuf);
         }
         else
         if (!strncmp(szLineBuf, ":# ", strlen(":# "))) {
            // todo: store user comments
         }
         else
         if (!strncmp(szLineBuf, ":create ", strlen(":create "))) {
            // end of control block is marked by first target content, if any
            pszContinue = 0;
         }
         psz1 = pszContinue;
      } else {
         if (strlen(psz1) > 10) {
            fprintf(stderr, "error: unexpected content in %s:\n", pClFileName);
            fprintf(stderr, "\"%.100s\"\n", psz1);
            return 9;
         }
         psz1 = 0;
      }
   }  // endwhile psz1

   long lPreFix  = strlen(":edit "); // and :READ

   // pass 2: isolate target contents
   psz1 = pszRaw;
   while (psz1) 
   {
      char *psz2 = strstr(psz1, "\n:create ");
      if (psz2)
      {
         // found another :create, isolate target name
         char *pszTargetName = psz2 + strlen("\n:create ");
         char *psz3 = strchr(pszTargetName, '\n');
         if (!psz3) { fprintf(stderr, "error: wrong syntax: \"%.100s\"\n", pszTargetName); return 9; }
         *psz3++ = '\0'; // remove and skip LF, go to start of content
         char *psz3b = strchr(pszTargetName, '\r');
         if (psz3b) *psz3b = '\0'; // remove possible CR
         // printf("create %s\n", pszTargetName);

         // find end of content
         char *pszContentBegin = psz3;
         char *pszDone = strstr(pszContentBegin, "\n:done\n");
         if (!pszDone) pszDone = strstr(pszContentBegin, "\n:done\r\n");
         if (!pszDone) { fprintf(stderr, "error: missing :done after :create in %s\n\"%.100s\"\n",pszTargetName,pszContentBegin); return 9; }
         char *pszContentEnd = pszDone+1;
         *pszContentEnd = '\0'; // set end of content, including last LF
         char *pszContinue = pszContentEnd+1; // not perfect, but simple

         // is target listed in index?
         long bInIndex  = 0;
         long bReadOnly = 0;
         for (long i=0; i<nIndexUsed; i++)
            if (!strcmp(&apIndex[i][lPreFix], pszTargetName)) {
               bInIndex = 1;
               // check if it's read-only
               if (!strncmp(apIndex[i], ":READ ", strlen(":READ ")))
                  bReadOnly = 1;
               // and immediately mark index entry as used
               char *psz1 = &apIndex[i][lPreFix];
               *psz1 = '\0';
               break;
            }

         if (bInIndex) {
            // create Target entry and add
            TextFile *pTarget = new TextFile(pszTargetName);
            if (pTarget->createFromMemory(pszContentBegin))
               return 9;
            pTarget->setReadOnly(bReadOnly);
            if (addTarget(pTarget)) {
               printf("RFF returns 9.1\n");
               return 9;
            }
         } else {
            // drop target, by not loading
            printf("[ DROP : %s ] %.20s\n", pszTargetName, pszGlblBlank);
            // at mem snapshot as well
            // NOTE: if user forgot to reload snapfile, target
            //       was already dropped before at mem-snap.
            if (glblMemSnap.hasTarget(pszTargetName))
               glblMemSnap.dropTarget(pszTargetName);
            else
               printf("[ WARN : reload the cluster! ] %.20s\n", pszTargetName, pszGlblBlank);
            rbDroppedAny = 1;
         }

         // continue in snapfile
         psz1 = pszContinue;
      }
      else 
      {
         // last :done, with :cluster-end
         if (strlen(psz1) > (strlen("done\r\n\n:cluster-end\r\n")+5)) {
            fprintf(stderr, "warn : unexpected content in %s:\n", pClFileName);
            fprintf(stderr, "\"%.100s\"\n", psz1);
            // return 9;
         }
         psz1 = 0;
      }
   }

   // pass 3: add all files of index not yet in snapfile
   for (long i5=0; i5<nIndexUsed; i5++) 
   {
      if (strlen(&apIndex[i5][lPreFix])) 
      {
         char *pszTargetName = &apIndex[i5][lPreFix];
         if (fileExists(pszTargetName)) 
         {
            // create and load to file-snap
            TextFile *pTarget = new TextFile(pszTargetName);
            if (pTarget->loadFromFile()) {
               // loading failed, e.g. due to keywords contained
               printf("[ SKIP : %s ] %.20s\n", pszTargetName, pszGlblBlank);
               delete pTarget;
            } else {
               // loading ok: add to file snapshot (this)
               printf("[ ADD  : %s ] %.20s\n", pszTargetName, pszGlblBlank);
               if (addTarget(pTarget)) {
                  printf("RFF returns 9.3\n");
                  return 9;
               }
               // and add to mem-snap
               TextFile *pClone = pTarget->clone();
               glblMemSnap.addTarget(pClone);
               // tell caller have to reload
               rbDroppedAny = 1;
            }
         } else {
            printf("[ ERROR: %s ] no such file\n", pszTargetName);
         }
      }
   }

   // pass 4: cleanup
   if (apIndex && (nIndexSize > 0)) {
      for (long i=0; i<nIndexUsed; i++)
         delete [] apIndex[i]; // char array
      delete [] apIndex;
   }

   adjustNamePadding();

   // all done, free temporary stuff
   delete [] pszRaw;

   // printf("%s re-read, %u targets\n", pszClID, numberOfTargets());
   if (numberOfTargets() < 1) { fprintf(stderr, "error: no targets found after cluster re-read\n"); return 9; }

   return 0;
}

void SnapShot::expandTargets(long lSoMuch) {
   // printf("[expand target array from %d to %d]\n",nClMaxTargets,nClMaxTargets+lSoMuch);
   TextFile **apTmp = new TextFile*[nClMaxTargets+lSoMuch];
   if (apClTargets) {
      memcpy(apTmp, apClTargets, nClMaxTargets*sizeof(TextFile*));
      delete [] apClTargets;
   }
   apClTargets = apTmp;
   nClMaxTargets += lSoMuch;
}

long SnapShot::addTarget(char *pszFileName) {
   // create new target entry and add
   TextFile *pTarget = new TextFile(pszFileName);
   long lRC = pTarget->loadFromFile();
   if (lRC == 1) {
      delete pTarget;
      return 0;
   }
   if (lRC > 1) {
      if (bGlblDebug) printf("SAT returns 9\n");
      delete pTarget;
      return 9;
   }
   return addTarget(pTarget);
}

long SnapShot::addTarget(TextFile *pTarget) {
   // is array still large enough? if not, expand
   if (nClTargets > nClMaxTargets-2)
      if (nClMaxTargets==0)
         expandTargets(100);
      else
         expandTargets(nClMaxTargets);
   // add new target entry
   apClTargets[nClTargets++] = pTarget;
   // update statistics
   adjustNamePadding();
   return 0;
}

void SnapShot::adjustNamePadding() {
   // determine max length over all filenames, calc a padding value
   long lMaxLen = 20; // always at least this minimum
   for (long iTarg=0; iTarg<nClTargets; iTarg++) {
      char *psz1 = apClTargets[iTarg]->getFileName();
      long lLen  = strlen(psz1);
      if (lLen > lMaxLen)
          lMaxLen = lLen;
   }
   if (lMaxLen > 20)
       lMaxLen = 20;
   lMaxLen = (lMaxLen / 5) * 5; // force alignment
   nClNamePadding = lMaxLen;
}

long matchesFileMask(char *pszStr)
{
   long lMatched = 0;

   Array &rMasks = glblFileSet.fileMasks();
   for (int i=0; rMasks.isStringSet(i); i++) 
   {
      char *pszMask = rMasks.getString(i);

      if (!strcmp(pszMask, "*")) {
         lMatched=1;
         continue;
      }

      if (pszMask[0] == glblNotChar) {
         if (mystrstri(pszStr, pszMask+1))
            return 0;
         continue;
      }

      if (pszMask[0] == '.') {
         // masks starting with '.' indicate an exact filetype match
         char *psz1 = strrchr(pszStr, '.');
         if (!psz1) {
            // printf("FAIL.1 %s %s\n",pszStr,pszMask);
            continue;
         }
         // ensure same length
         long lLen1 = strlen(psz1);
         long lLen2 = strlen(pszMask);
         if (lLen1 != lLen2) {
            // printf("FAIL.2 %s %s\n",psz1,pszMask);
            continue;
         }
         // now strstr does an exact comparison
         if (mystrstri(psz1, pszMask)) {
            // printf("MATCH  %s %s\n",psz1,pszMask);
            lMatched=1;
            continue;
         }
      }
      else
      if (mystrstri(pszStr, pszMask)) {
         lMatched=1;
         continue;
      }

   }
   if (lMatched)
      return 1;
   return 0;
}

long matchesDirMask(char *pszStr)
{
   // just a check against negative masks
   Array &rMasks = glblFileSet.dirMasks();
   for (int i=0; rMasks.isStringSet(i); i++) 
   {
      char *pszMask = rMasks.getString(i);
      if (pszMask[0] == glblNotChar)
      {
         if (mystrstri(pszStr, pszMask+1)) {
            if (bGlblDebug) printf("MDM-NO  %s %s\n", pszStr, pszMask+1);
            return 0;
         } else {
            if (bGlblDebug) printf("MDM-YES %s %s\n", pszStr, pszMask+1);
         }
      }
   }
   return 1;
}

long execGrep(char *pszFileName) 
{
   FILE *fin = fopen(pszFileName, "r");
   if (!fin) { fprintf(stderr, "warning: cannot read: %s\n", pszFileName); return 0; }
   long nMaxLineLen = sizeof(szLineBuf)-10; // YES, szLineBuf
   memset(abBuf, 0, nMaxLineLen+2); // yes, abBuf is larger by far

   long nLocalLines = 0;
   bool bDumpedFileName = 0;
   while (fgets((char*)abBuf, nMaxLineLen, fin)) // yes, exact len
   {
      nGlblLines++;
      nLocalLines++;

      if (strlen((char*)abBuf) == nMaxLineLen)
         fprintf(stderr, "warning: max line length %d reached, splitting. file %s, line %d\n", nMaxLineLen, pszFileName, nLocalLines);

      removeCRLF((char*)abBuf);

      long nMatch = 0;
      long nGrepPat = glblGrepPat.numberOfEntries();
      for (long i=0; (nMatch < nGrepPat) && (i<nGrepPat); i++)
         if (mystrstri((char*)abBuf, glblGrepPat.getString(i)))
            nMatch++;

      if (nMatch == nGrepPat) {
         if (!bDumpedFileName) {
            bDumpedFileName = 1;
            if (!strncmp(pszFileName, glblDotSlash, 2))
               pszFileName += 2;
            printf("%s :\n", pszFileName);
         }
         printf("   %s\n", abBuf);
      }
   }

   fclose(fin);

   return 0;
}

long execFileStat(char *pszFileName, long lLevel, long &lFiles, long &lDirs, num &lBytes, num &nLocalMaxTime, num &nTreeMaxTime) 
{
   long bIsDir    = 0;
   long bCanRead  = 1;
   long bCanWrite = 1;
   num  nFileTime = 0;
   num  nFileSize = 0;
   getFileStat(pszFileName, bIsDir, bCanRead, bCanWrite, nFileTime, nFileSize);

   if (!strncmp(pszFileName, glblDotSlash, 2))
      pszFileName += 2;

   switch (nGlblListMode)
   {
      case 1:
         if (!bGlblQuiet && (nGlblListMinSize > 0))
         {
            long lMBytes = (long)(nFileSize / 1000000L);
            if (lMBytes >= nGlblListMinSize)
            {
               int nIndent = (int)lLevel;
               if (nIndent > strlen(pszGlblBlank)) nIndent = strlen(pszGlblBlank);
               if (nIndent > 10) nIndent = 10;
                 
               printf("%5ld mb,              %.*s%s\n", lMBytes, nIndent, pszGlblBlank, pszFileName);
            }
         }
         break;

      case 2:
         if (!bGlblQuiet) {
            switch (nGlblListForm)
            {
               // plain filename, nothing else:
               case 0: printf("%s\n", pszFileName); break;
               // size and filename:
               case 1: printf("%s %s\n", numtoa_blank(nFileSize, nGlblListDigits), pszFileName); break;
               // time and filename:
               case 2: printf("%s %s\n", timeAsString(nFileTime), pszFileName); break;

               // size time filename:
               case 0x0102: printf("%s %s %s\n", numtoa_blank(nFileSize, nGlblListDigits), timeAsString(nFileTime), pszFileName); break;
               // time size filename:
               case 0x0201: printf("%s %s %s\n", timeAsString(nFileTime), numtoa_blank(nFileSize, nGlblListDigits), pszFileName); break;
            }
         }
         break;

      case 3:
         glblTwinScan.addFile(pszFileName);
         break;
   }

   // update maxtimes
   if (nFileTime > nLocalMaxTime)
       nLocalMaxTime = nFileTime;
   if (nFileTime > nTreeMaxTime)
       nTreeMaxTime = nFileTime;

   lBytes += nFileSize;
   return 0;
}

#ifdef WITH_FN_INST
long execInst(char *pszFileName, long lLevel, long &lFiles, long &lDirs, num &lBytes) 
{
   extern int sfkInstrument(char *pszFile, char *pszInc, char *pszMac, bool bRevoke, bool bRedo, bool bTouchOnRevoke);

   // source code automatic instrumentation
   if (!strncmp(pszFileName, glblDotSlash, 2))
      pszFileName += 2;

   long nRC = sfkInstrument(pszFileName, pszGlblInstInc, pszGlblInstMac, bGlblInstRevoke, bGlblInstRedo, bGlblTouchOnRevoke);

   if (nRC < 9)
      return 0;

   return nRC;
}
#endif

enum eRunExpressions 
{
   erun_path      = 0,
   erun_file      = 1,
   erun_file_     = 2,
   erun_relfile   = 3,
   erun_base      = 4,
   erun_ext       = 5
};

const char *apRunTokens[] =
{
   "purepath"    , "ppath"  , "quotpath"     , "qpath"  ,
   "purefile"    , "pfile"  , "quotfile"     , "qfile"  ,
   "pure_file"   , "p_file" , "quot_file"    , "q_file" ,
   "purerelfile" , "prfile" , "quotrelfile"  , "qrfile" ,
   "purebase"    , "pbase"  , "quotbase"     , "qbase"  ,
   "pureext"     , "pext"   , "quotext"      , "qext"   ,
};

// tell if a supplied user command references single files.
// if not, it will be applied on directories only.
bool anyFileInRunCmd(char *pszCmd) 
{
   char abToken[100];
   for (ulong i=4; i<(sizeof(apRunTokens)/sizeof(apRunTokens[0])); i++) {
      strcpy(&abToken[1], apRunTokens[i]);
      abToken[0] = '#';
      if (strstr(pszCmd, abToken))
         return true;
      #ifdef SFK_BOTH_RUNCHARS
      abToken[0] = '$';
      if (strstr(pszCmd, abToken))
         return true;
      #endif
   }
   return false;
}

long onRunExpression(char *psz1, long &lExpLength, bool &bquot)
{
   char abToken[100];
   ulong nPtrs = (sizeof(apRunTokens)/sizeof(apRunTokens[0]));
   ulong nRows = nPtrs/4;
   for (ulong irow=0; irow<nRows; irow++)
   {
      for (ulong icol=0; icol<4; icol++)
      {
         strcpy(&abToken[1], apRunTokens[irow*4+icol]);
         abToken[0] = '#';
         char *psz2 = abToken;
         if (!strncmp(psz1, psz2, strlen(psz2)))
         {
            lExpLength = strlen(psz2);
            bquot = (icol >= 2) ? true : false;
            if (bGlblDebug) printf("orp %u %u - %s\n", lExpLength, bquot, psz2);
            return irow;
         }
         #ifdef SFK_BOTH_RUNCHARS
         else {
          abToken[0] = '$';
          if (!strncmp(psz1, psz2, strlen(psz2)))
          {
            lExpLength = strlen(psz2);
            bquot = (icol >= 2) ? true : false;
            return irow;
          }
         }
         #endif
      }
   }
   return -1;
}

long execRunFile(char *pszFileName, long lLevel, long &lFiles, long &lDirs, num &lBytes) 
{
   if (!strncmp(pszFileName, glblDotSlash, 2))
      pszFileName += 2;

   // copy command template to command buffer
   strcpy(szLineBuf, pszGlblRunCmd);

   // preparations
   char *pszRelFilename = strrchr(pszFileName, glblPathChar);
   if (!pszRelFilename)
      pszRelFilename = pszFileName;
   else
      pszRelFilename++; // skip path char

   bool bDoneAny = 0;
   char *psz1 = strchr(szLineBuf, '#');
   #ifdef SFK_BOTH_RUNCHARS
   if (!psz1) psz1 = strchr(szLineBuf, '$');
   #endif
   while (psz1)
   {
      long lTokenLen = 0;
      bool bQuoted   = false;
      switch (onRunExpression(psz1, lTokenLen, bQuoted))
      {
         case erun_file:
         {
            // replace absolute filename
            memset(szLineBuf2, 0, sizeof(szLineBuf2));
            strncpy(szLineBuf2, szLineBuf, psz1-szLineBuf);
            // middle
            char *psz2 = psz1+lTokenLen;
            if (bQuoted) strcat(szLineBuf2, "\"");
             strcat(szLineBuf2, pszFileName);
            if (bQuoted) strcat(szLineBuf2, "\"");
            // remember position past insert
            psz1 = szLineBuf+strlen(szLineBuf2);
            // right
            strcat(szLineBuf2, psz2);
            // copy back result
            strncpy(szLineBuf, szLineBuf2, sizeof(szLineBuf));
            bDoneAny = 1;
            break;
         }

         case erun_file_:
         {
            // absolute filename with blanks replaced by "_".
            // alpha - not yet official.
            memset(szLineBuf2, 0, sizeof(szLineBuf2));
            strncpy(szLineBuf2, szLineBuf, psz1-szLineBuf);
            // middle
            char *psz2 = psz1+lTokenLen;
            char *psz3 = szLineBuf2+strlen(szLineBuf2);
            if (bQuoted) strcat(szLineBuf2, "\"");
             strcat(szLineBuf2, pszFileName);
            if (bQuoted) strcat(szLineBuf2, "\"");
            // replace blanks, within target buffer
            while (*psz3) {
               if (*psz3 == ' ')
                   *psz3 = '_';
               psz3++;
            }
            // remember position past insert
            psz1 = szLineBuf+strlen(szLineBuf2);
            // right
            strcat(szLineBuf2, psz2);
            // copy back result
            strncpy(szLineBuf, szLineBuf2, sizeof(szLineBuf));
            bDoneAny = 1;
            break;
         }

         case erun_relfile:
         {
            // replace relative filename
            memset(szLineBuf2, 0, sizeof(szLineBuf2));
            strncpy(szLineBuf2, szLineBuf, psz1-szLineBuf);
            // middle
            char *psz2 = psz1+lTokenLen;
            if (bQuoted) strcat(szLineBuf2, "\"");
             strcat(szLineBuf2, pszRelFilename);
            if (bQuoted) strcat(szLineBuf2, "\"");
            // remember position past insert
            psz1 = szLineBuf+strlen(szLineBuf2);
            // right
            strcat(szLineBuf2, psz2);
            // copy back result
            strncpy(szLineBuf, szLineBuf2, sizeof(szLineBuf));
            bDoneAny = 1;
            break;
         }

         case erun_path:
         {
            // replace filename path
            memset(szLineBuf2, 0, sizeof(szLineBuf2));
            strncpy(szLineBuf2, szLineBuf, psz1-szLineBuf);
            // middle
            char *psz2 = psz1+lTokenLen;
            if (bQuoted) strcat(szLineBuf2, "\"");
            char *psz3 = strrchr(pszFileName, glblPathChar);
            if (psz3 && (psz3-pszFileName)>0)
               strncat(szLineBuf2, pszFileName, psz3-pszFileName);
            else
               strcat(szLineBuf2, ".");
            if (bQuoted) strcat(szLineBuf2, "\"");
            // remember position past insert
            psz1 = szLineBuf+strlen(szLineBuf2);
            // right
            strcat(szLineBuf2, psz2);
            // copy back result
            strncpy(szLineBuf, szLineBuf2, sizeof(szLineBuf));
            bDoneAny = 1;
            break;
         }

         case erun_base:
         {
            // operations working with extensions should have a file filter (?)
            // if (!bGlblQuiet && !glblFileSet.isAnyExtensionMaskSet()) {
            //    static bool bWarned=0;
            //    if (!bWarned) { printf("warning: $base used but no -file .ext specified\n"); bWarned=1; }
            // }
            // replace file base name, without ".ext"
            // have to use relative filename for this.
            // note: ".afile" has ".afile" as base
            // note: "afile.long.longext" has ".longext" as ext
            memset(szLineBuf2, 0, sizeof(szLineBuf2));
            strncpy(szLineBuf2, szLineBuf, psz1-szLineBuf);
            // middle
            char *psz2 = psz1+lTokenLen;
            if (bQuoted) strcat(szLineBuf2, "\"");
            char *psz3 = strrchr(pszRelFilename, '.');
            if (psz3 && (psz3 > pszRelFilename)) {
               // can identify extension
               strncat(szLineBuf2, pszRelFilename, psz3-pszRelFilename);
            } else {
               // cannot identify extension
               strcat(szLineBuf2, pszRelFilename);
            }
            if (bQuoted) strcat(szLineBuf2, "\"");
            // remember position past insert
            psz1 = szLineBuf+strlen(szLineBuf2);
            // right
            strcat(szLineBuf2, psz2);
            // copy back result
            strncpy(szLineBuf, szLineBuf2, sizeof(szLineBuf));
            bDoneAny = 1;
            break;
         }

         case erun_ext:
         {
            // operations working with extensions should have a file filter (?)
            // if (!bGlblQuiet && !glblFileSet.isAnyExtensionMaskSet()) {
            //    static bool bWarned=0;
            //    if (!bWarned) { printf("warning: $ext used but no -file .ext specified\n"); bWarned=1; }
            // }
            // replace file extension
            // have to use relative filename for this.
            // note: ".afile" has ".afile" as base
            // note: "afile.long.longext" has ".longext" as ext
            // note: "afile." has "" as extension
            memset(szLineBuf2, 0, sizeof(szLineBuf2));
            strncpy(szLineBuf2, szLineBuf, psz1-szLineBuf);
            // middle
            char *psz2 = psz1+lTokenLen;
            if (bQuoted) strcat(szLineBuf2, "\"");
            char *psz3 = strrchr(pszRelFilename, '.');
            if (psz3 && (psz3 > pszRelFilename)) {
               // can identify extension, zero length accepted
               strcat(szLineBuf2, psz3+1);
            } else {
               // cannot identify extension, leave empty
            }
            if (bQuoted) strcat(szLineBuf2, "\"");
            // remember position past insert
            psz1 = szLineBuf+strlen(szLineBuf2);
            // right
            strcat(szLineBuf2, psz2);
            // copy back result
            strncpy(szLineBuf, szLineBuf2, sizeof(szLineBuf));
            bDoneAny = 1;
            break;
         }

         default:
            psz1++;
            break;

      }  // end switch

      // find next potential token, if any
      #ifdef SFK_BOTH_RUNCHARS
      if (strchr(psz1, '#'))
         psz1 = strchr(psz1, '#');
      else
         psz1 = strchr(psz1, '$');
      #else
      psz1 = strchr(psz1, '#');
      #endif
   }

   if (!bDoneAny) 
   {
      fprintf(stderr, "error: no valid token in run command. type \"sfk run\" for help.\n");
      return 9;
      // strcat(szLineBuf, " ");
      // strcat(szLineBuf, pszFileName);
   }

   if (!bGlblQuiet && !bGlblSim) {
      printf("%s\n", szLineBuf);
      fflush(stdout); 
   }

   int iRC = 0;

   if (bGlblSim) {
      // special case: just dump resulting command to terminal
      printf("%s\n", szLineBuf);
   } else {
      iRC = system(szLineBuf);
   }

   if (!bGlblQuiet && !bGlblSim) {
      if (iRC) {
         printf("... error, rc %d\n", iRC);
         fflush(stdout);
      }
   }

   return 0;
}

long execRunDir(char *pszFileName, long lLevel, long &lFiles, long &lDirs, num &lBytes) 
{
   if (!strncmp(pszFileName, glblDotSlash, 2))
      pszFileName += 2;

   // copy command template to command buffer
   strcpy(szLineBuf, pszGlblRunCmd);

   bool bDoneAny = 0;
   char *psz1 = strchr(szLineBuf, '$');
   while (psz1)
   {
      long lTokenLen = 0;
      bool bQuoted   = false;
      switch (onRunExpression(psz1, lTokenLen, bQuoted))
      {
         case erun_path:
         {
            // replace filename path
            memset(szLineBuf2, 0, sizeof(szLineBuf2));
            strncpy(szLineBuf2, szLineBuf, psz1-szLineBuf);
            // middle
            char *psz2 = psz1+lTokenLen;
            if (bQuoted) strcat(szLineBuf2, "\"");
             strcat(szLineBuf2, pszFileName);
            if (bQuoted) strcat(szLineBuf2, "\"");
            // remember position past insert
            psz1 = szLineBuf+strlen(szLineBuf2);
            // right
            strcat(szLineBuf2, psz2);
            // copy back result
            strncpy(szLineBuf, szLineBuf2, sizeof(szLineBuf));
            bDoneAny = 1;
            break;
         }

         default:
            psz1++;
            break;
      }

      // find next potential token, if any
      psz1 = strchr(psz1, '$');
   }

   if (!bDoneAny) 
   {
      fprintf(stderr, "error: no valid token in run command. type \"sfk run\" for help.\n");
      return 9;
   }

   if (!bGlblQuiet && !bGlblSim) {
      printf("%s\n", szLineBuf);
      fflush(stdout); 
   }

   int iRC = 0;

   if (bGlblSim) {
      // special case: just dump resulting command to terminal
      printf("%s\n", szLineBuf);
   } else {
      iRC = system(szLineBuf);
   }

   if (!bGlblQuiet && !bGlblSim) {
      if (iRC) {
         printf("... error, rc %d\n", iRC);
         fflush(stdout);
      }
   }

   return 0;
}

long execDirStat(char *pszDir, long lLevel, long lFiles, long lDirs, num lBytes, num &nLocalMaxTime, num &ntime2)
{
   if (nGlblListMode == 1)
   {
      int nIndent = (int)lLevel;
      if (nIndent > strlen(pszGlblBlank)) nIndent = strlen(pszGlblBlank);
      if (nIndent > 10) nIndent = 10;
   
      if (!strncmp(pszDir, glblDotSlash, 2))
         pszDir += 2;
   
      unsigned long lMBytes = (unsigned long)lBytes / 1000000UL;
   
      if (bGlblQuiet) {
         if (lLevel == 0)
            printf("%5ld mb %s\n", lMBytes, pszDir);
      } else {
         if (lMBytes >= nGlblListMinSize)
            printf("%5ld mb, %5ld files, %.*s%s\n", lMBytes, lFiles, nIndent, pszGlblBlank, pszDir);
      }
   }
   else
   if (nGlblListMode == 2)
   {
      // printf("DIR %s %s\n", numtoa(nLocalMaxTime), pszDir);
   }

   return 0;
}

long walkFiles(
   char *pszIn, long lLevel,
   long &nGlobFiles, FileList &rTopDirFiles,
   long &lDirs, num &lBytes,
   num &nLocalMaxTime, num &nTreeMaxTime
 )
{
   if (bGlblEscape)
      return 0;

   long lSize1       = strlen(pszIn);
   char *pszPattern  = new char[lSize1+10];
   char *pszBasePath = new char[lSize1+10];

   long lRC = 0;

   // pszIn might be
   // -  a single file
   // -  a directory, with or w/o slash at end
   // -  a pattern expression: dir\a*b??.cpp
   char *pszLastSlash = strrchr(pszIn, glblPathChar);

   if (!pszLastSlash) {
      if (isDir(pszIn)) {
         // expression contains no nested path, and it's probably a dir.
         // we operate on this whole directory.
         strcpy(pszBasePath, pszIn);
         sprintf(pszPattern, "%s%c%s", pszIn, glblPathChar, glblAddWildCard);
      } else {
         // expression contains no nested path, and it's not a dir.
         strcpy(pszBasePath, "");
         strcpy(pszPattern, pszIn);
      }
   } else {
      if (isDir(pszIn)) {
         // expression contains a path, and it's probably a dir.
         // if (bGlblDebug) printf("ISDIR  : %s\n", pszIn);
         sprintf(pszPattern, "%s%c%s", pszIn, glblPathChar, glblAddWildCard);
         strcpy(pszBasePath, pszIn);
      } else {
         // expression contains a path, and it's not a dir.
         strcpy(pszPattern, pszIn);
         strncpy(pszBasePath, pszIn, pszLastSlash-pszIn);
         pszBasePath[pszLastSlash-pszIn] = '\0';
      }
   }

   // NOTE: while walking through a directory,
   // we also expect to get all SYSTEM and HIDDEN files listed.

   #ifdef _WIN32 // --------- Windows directory walking code ----------

   // yes, this is looking awful, but i want sfk compileable both
   // on the latest msvc with 64-bit timestamps, as well as on older ones.
   #ifdef SFK_W64
   __finddata64_t myfdat;
   intptr_t myfdh = _findfirst64(pszPattern, &myfdat);
   #else
    #ifndef _INTPTR_T_DEFINED
     typedef long intptr_t;
    #endif
   _finddata_t myfdat;
   intptr_t myfdh = _findfirst(pszPattern, &myfdat);
   #endif

   if (myfdh == -1) {
      // printf("%s not found\n", pszPattern);
      delete [] pszPattern;
      delete [] pszBasePath;
      return 0; // may happen on some empty dirs
   }

   do
   {

   #else // ----------- unix directory walking code -------------

   struct SfkFindData {
      char *name;
      int   attrib;
      num   time_write;
      num   size;
   } myfdat;

   DIR *myfdh = opendir(pszPattern);

   if (!myfdh) {
      // printf("%s not found\n", pszPattern);
      delete [] pszPattern;
      delete [] pszBasePath;
      return 0;
   }

   while (1)
   {
      struct dirent *e = readdir(myfdh);
      if (e == NULL)
         break; // while

      myfdat.name   = e->d_name;
      myfdat.attrib = 0;

   #endif // _WIN32
   
      char *pszFile = myfdat.name;

      if (   !strcmp(pszFile, ".")
          || !strcmp(pszFile, ".."))
         continue;

      long lSize2   = strlen(myfdat.name);
      char *pszSub  = new char[lSize1+lSize2+10];
      long lBaseLen = strlen(pszBasePath);
      if ((lBaseLen > 0) && (pszBasePath[lBaseLen-1] == glblPathChar))
         sprintf(pszSub, "%s%s", pszBasePath, myfdat.name);
      else
         sprintf(pszSub, "%s%c%s", pszBasePath, glblPathChar, myfdat.name);

   #ifndef _WIN32

      // get further dir/file statistics. no 32 bit compat here -
      // getting the latest g++ for linux shouldn't be too difficult.
      struct stat64 hStat1;
      if (stat64(pszSub, &hStat1)) {
         if (nGlblVerbose) printf("nostat: %s (non-regular file)\n", pszSub);
         nGlblNoFiles++;
         continue;
      }

      bool bIsLink = 0;

      #ifdef DT_LNK
      // linux
      if (e->d_type == DT_LNK)
         bIsLink = 1; // cannot tell here if dir or file link
      else
      if (e->d_type == DT_DIR)
         myfdat.attrib = 0x10; // dir
      else
      if (e->d_type == DT_REG)
         myfdat.attrib = 0x00; // regular file
      else {
         if (nGlblVerbose) printf("nofile: %s (non-regular file)\n", myfdat.name);
         nGlblNoFiles++;
         delete [] pszSub;
         continue;
      }
      #else
      // weirdux
      #ifdef S_IFLNK
      if ((hStat1.st_mode & S_IFLNK) == S_IFLNK)
         bIsLink = 1;
      #endif
      // no else here.
      if ((hStat1.st_mode & S_IFDIR) == S_IFDIR)
         myfdat.attrib = 0x10; // dir
      else
      if ((hStat1.st_mode & S_IFREG) == S_IFREG)
         myfdat.attrib = 0x00; // regular file
      else {
         if (nGlblVerbose) printf("nofile: %s (non-regular file)\n", myfdat.name);
         nGlblNoFiles++;
         delete [] pszSub;
         continue;
      }
      #endif

      /*
         NOTE: these are OCTAL VALUES, NOT hexadecimal.
         __S_IFDIR   0040000  // Directory.
         __S_IFCHR   0020000  // Character device.
         __S_IFBLK   0060000  // Block device.
         __S_IFREG   0100000  // Regular file.
         __S_IFIFO   0010000  // FIFO.
         __S_IFLNK   0120000  // Symbolic link.
         __S_IFSOCK  0140000  // Socket.
      */

      if (bIsLink && ((hStat1.st_mode & _S_IFDIR) == _S_IFDIR)) {
         // symbolic directory link: do not follow
         delete [] pszSub;
         continue;
      }

      // may also set 0x02 here for hidden files.
      // may also set 0x04 here for system files.

      myfdat.time_write = hStat1.st_mtime;
      myfdat.size       = hStat1.st_size;

   #endif

     if ((myfdat.attrib & 0x06) && (!bGlblHiddenFiles))
     {
         // hidden or system file or dir,
         // but inclusion of hidden not selected: skip
     }
     else
     {
      if (myfdat.attrib & 0x10)
      {
         // subdirectory
         long nTreeFileCnt = 0;

         FileList oLocDirFiles;
         long nDirDirs  = 0;
         num  nDirBytes = 0, nDirLocalMaxTime = 0, nDirTreeMaxTime = 0;

         // temporary attach '\\' char for exact path matching
         strcat(pszSub, glblPathStr);
         bool bMatch = matchesDirMask(pszSub);
         pszSub[strlen(pszSub)-1] = '\0';

         long lRC = 0;

         if (bGlblRecurseDirs && bMatch)
         {
            lRC = walkFiles(pszSub, lLevel+1, nTreeFileCnt, oLocDirFiles, nDirDirs, nDirBytes,
                            nDirLocalMaxTime, nDirTreeMaxTime);

            if (bGlblDebug) printf("] esd: %d %s\n", lLevel, pszSub);
            if (execSingleDir(pszSub, lLevel+1, nTreeFileCnt, oLocDirFiles, nDirDirs, nDirBytes,
                              nDirLocalMaxTime, nDirTreeMaxTime))
            {
               nDirLocalMaxTime = 0;
               delete [] pszSub; // falling past delete below
               break; // while
            }
            nDirLocalMaxTime = 0;

            // NOTE: local maxtime is NOT promoted, it was used w/in execSingleDir.
            //       we only promote the tree max time:
            if (nDirTreeMaxTime > nTreeMaxTime)
                nTreeMaxTime = nDirTreeMaxTime;

            // add sub tree file count to next-higher level
            nGlobFiles += nTreeFileCnt;
         }

         lDirs  += nDirDirs ;
         lBytes += nDirBytes;

         if (lRC)
         {
            delete [] pszSub; // falling past delete below
            break; // while
         }
      }
      else
      {
         // normal file: check mask against file name WITHOUT path
         if (!bGlblWalkJustDirs && matchesFileMask(myfdat.name))
         {
            if (lRC = execSingleFile(pszSub, lLevel+1,
                           nGlobFiles, rTopDirFiles.clNames.numberOfEntries(),
                           lDirs, lBytes,
                           nLocalMaxTime, nTreeMaxTime)
               )
            {
               delete [] pszSub; // falling past delete below
               break; // while
            }
            else
            {
               // count file as processed.
               nGlobFiles++;
               if (rTopDirFiles.addFile(pszSub, myfdat.time_write, myfdat.size))
                  return 9; // shouldn't happen (outofmem)
            }
         }
      } // endelse dir or file
     } // endelse hidden

      delete [] pszSub;

      if (userInterrupt())
         break;
   }
   #ifdef _WIN32
    #ifdef SFK_W64
    while (!_findnext64(myfdh, &myfdat));
    #else
    while (!_findnext(myfdh, &myfdat));
    #endif
   _findclose(myfdh);
   #else
   closedir(myfdh);
   #endif

   delete [] pszPattern;
   delete [] pszBasePath;

   return lRC;
}

long dumpHexBlock(uchar *pCur, long lSize) {
   for (long i=0; i<lSize; i++) {
      if (pCur[i])
         fprintf(fGlblOut, "0x%x,", pCur[i]);
      else
         fprintf(fGlblOut, "0,", pCur[i]);
   }
   fprintf(fGlblOut,"\n");
   return 0;
}

long execBin2Src(uchar *pIn, long lInSize, bool bPack)
{
   long lOldInSize = lInSize;

   if (bPack) {
      ulong nPackSize = 0;
      // binPack alloc's mem block with packed data
      pIn = binPack(pIn, lInSize, nPackSize);
      lInSize = (long)nPackSize;
   }

   fprintf(fGlblOut,"unsigned long nBlockSize = %u;\n\n", lOldInSize);
   fprintf(fGlblOut,"unsigned char abRawBlock[%u] = {\n", lInSize);
   long lRemain = lInSize;
   uchar *pCur  = pIn;
   while (lRemain > 32) {
      dumpHexBlock(pCur, 32);
      lRemain -= 32;
      pCur += 32;
   }
   if (lRemain > 0)
      dumpHexBlock(pCur, lRemain);
   fprintf(fGlblOut,"};\n");

   if (bPack) {
fprintf(fGlblOut,
   "\n"
   "long fskGetBlock(uchar *pOut, ulong nOutSize)\n"
   "{\n"
   "   uchar *pCur    = abRawBlock;\n"
   "   ulong nInSize  = %lu;\n"
   "   uchar *pMax    = pCur + nInSize;\n"
   "   uchar *pOutMax = pOut + nOutSize;\n"
   "   while (pCur < pMax)\n"
   "   {\n"
   "      uchar ncmd = *pCur++;\n"
   "      if (ncmd >= 64) {\n"
   "         uchar nsiz = ncmd >> 6;\n"
   "         uchar nrep = ncmd & 0x3F;\n"
   "         for (; nrep > 0; nrep--)\n"
   "            for (uchar i1=0; i1<nsiz; i1++)\n"
   "               *pOut++ = *(pCur+i1);\n"
   "         pCur += nsiz;\n"
   "      } else {\n"
   "         uchar nrep = ncmd;\n"
   "         for (; (nrep > 0) && (pCur < pMax); nrep--)\n"
   "            *pOut++ = *pCur++;\n"
   "      }\n"
   "   }\n"
   "   return 0;\n"
   "}\n",
   lInSize
      );
      delete [] pIn;
   }

   return 0;
}

long getFileMD5(char *pszFile, SFKMD5 &md5, bool bSilent)
{
   FILE *fin = fopen(pszFile, "rb");
   if (!fin) { if (!bSilent) fprintf(stderr, "error: cannot read: %s\n", pszFile); return 9; }
   size_t nRead = 0;
   while ((nRead = fread(abBuf,1,sizeof(abBuf)-10,fin)) > 0) {
      md5.update(abBuf,nRead);
      nGlblBytes += nRead;
   }
   fclose(fin);
   return 0;
}

ulong getLong(uchar ab[], ulong noffs) {
   return  (((ulong)ab[noffs+3])<<24)
          |(((ulong)ab[noffs+2])<<16)
          |(((ulong)ab[noffs+1])<< 8)
          |(((ulong)ab[noffs+0])<< 0);
}

ulong getShort(uchar ab[], ulong noffs) {
   return  (((ulong)ab[noffs+1])<< 8)
          |(((ulong)ab[noffs+0])<< 0);
}

// offsets taken from InfoZIP's UnZip 5.52:
#define L_VERSION_NEEDED_TO_EXTRACT_0  0 
#define L_VERSION_NEEDED_TO_EXTRACT_1  1 
#define L_GENERAL_PURPOSE_BIT_FLAG     2 
#define L_COMPRESSION_METHOD           4 
#define L_LAST_MOD_DOS_DATETIME        6 
#define L_CRC32                        10 
#define L_COMPRESSED_SIZE              14 // 32-bit 
#define L_UNCOMPRESSED_SIZE            18 
#define L_FILENAME_LENGTH              22 // 16-bit 
#define L_EXTRA_FIELD_LENGTH           24 // 16-bit

#define L_LOCAL_HEADER_SIZE (L_EXTRA_FIELD_LENGTH+2)

// build md5 over a zip file AND:
// -  read all it's filenames
// -  check them against provided list

// USES: szLineBuf2
long getZipMD5(char *pszFile, SFKMD5 &md5, FileList &rFileList, bool bMakeList=0)
{
   FILE *fin = fopen(pszFile, "rb");
   if (!fin) return 9;

   size_t nRead = 0;

   while (1)
   {
      uchar abLocHdrPre[4+4];
      uchar abLocHdrPreTpl[] = { 0x50, 0x4b, 0x03, 0x04 };
      uchar abCntDirHdrTpl[] = { 0x50, 0x4b, 0x01, 0x02 };
   
      // read local file header magic.
      nRead = fread(abLocHdrPre,1,4,fin);
      if (nRead <= 0)
         break;   // EOD
      if (nRead != 4) { fclose(fin); return 9; }

      md5.update(abLocHdrPre, 4);
      nGlblBytes += nRead;
      if (!memcmp(abLocHdrPre, abCntDirHdrTpl, 4))
         break;   // central directory begin: don't parse further
      if (memcmp(abLocHdrPre, abLocHdrPreTpl, 4)) { fclose(fin); return 8; }
   
      // local file header.
      uchar abLocHdr[L_LOCAL_HEADER_SIZE+4];
      if ((nRead = fread(abLocHdr,1,L_LOCAL_HEADER_SIZE,fin)) != L_LOCAL_HEADER_SIZE) { fclose(fin); return 9; }
      md5.update(abLocHdr, L_LOCAL_HEADER_SIZE);
      nGlblBytes += nRead;
   
      ulong nCmpSize     = getLong (abLocHdr, L_COMPRESSED_SIZE);
      ulong nFileNameLen = getShort(abLocHdr, L_FILENAME_LENGTH);
      ulong nExtraLen    = getShort(abLocHdr, L_EXTRA_FIELD_LENGTH);

      // PkZip format seems not to support 64-bit sizes and timestamps.
      // num nTimeStamp = getLong (abLocHdr, L_LAST_MOD_DOS_DATETIME);
      num nFileSize  = getLong (abLocHdr, L_UNCOMPRESSED_SIZE);
   
      // filename.
      if (nFileNameLen > sizeof(szLineBuf2)-10) { fclose(fin); return 9; }
      if ((nRead = fread(szLineBuf2, 1, nFileNameLen, fin)) != nFileNameLen) { fclose(fin); return 9; }
      md5.update((uchar*)szLineBuf2, nFileNameLen);
      nGlblBytes += nRead;
   
      // printf("file: %.*s %ld\n", (int)nFileNameLen, szLineBuf2, nFileNameLen);

      szLineBuf2[nFileNameLen] = '\0';

      #ifdef _WIN32
      // convert path chars from zip format '/' to local
      char *psz1 = 0;
      while (psz1 = strchr(szLineBuf2, glblWrongPChar))
         *psz1 = glblPathChar;
      #endif

      if (bMakeList)
         rFileList.addFile(szLineBuf2, 0, 0);
      else {
         long lRC = rFileList.checkAndMark(szLineBuf2, nFileSize);
         if (lRC == 1) {
            static bool bInfoDone = 0;
            logError("info   : outdated file \"%s\" in archive \"%s\"", szLineBuf2, pszFile);
            if (!bInfoDone) {
               bInfoDone = 1;
               logError("info   : you may want to cleanup your archive tree.");
            }
         }
         if (lRC == 2) {
            static bool bInfoDone2 = 0;
            logError("error  : size mismatch of \"%s\" in archive \"%s\"", szLineBuf2, pszFile);
            if (!bInfoDone2) {
               bInfoDone2 = 1;
               logError("error  : maybe the file is too large, empty, unaccessible, or not zip compatible.");
            }
         }
      }

      // skip to next local file header.
      num nRemain = nExtraLen + nCmpSize;
      while (nRemain > 0) {
         size_t nBlockLen = sizeof(abBuf)-10;
         if (nBlockLen > nRemain) nBlockLen = nRemain;
         nRead = fread(abBuf,1,nBlockLen,fin);
         if (nRead <= 0)
            break;
         md5.update(abBuf,nRead);
         nGlblBytes += nRead;
         nRemain -= nRead;
      }
   }

   // read remaining data (central dir) as black box
   while ((nRead = fread(abBuf,1,sizeof(abBuf)-10,fin)) > 0) {
      md5.update(abBuf,nRead);
      nGlblBytes += nRead;
   }

   // unsigned char *pmd5 = md5.digest();
   // for (int i=0; i<16; i++)
   //    fprintf(stdout,"%02x",pmd5[i]);
   // printf("\n");

   fclose(fin);
   return 0;
}

long execMD5write(char *pszFile)
{
   SFKMD5 md5;
   if (getFileMD5(pszFile, md5))
      return 9;
   unsigned char *pmd5 = md5.digest();
   for (int i=0; i<16; i++)
      fprintf(fGlblOut,"%02x",pmd5[i]);
   fprintf(fGlblOut," *%s\n",pszFile); // md5sum similar
   if (glblFileCount.count()) {
      printf("[%u files, %u kb/sec] \r", glblFileCount.value(), currentKBPerSec()); fflush(stdout);
   }
   return 0;
}

long execMD5check(char *pIn)
{
   long nLine=0;
   long nError=0;
   char *pszLine=pIn;
   char *pszNext=0;
   ulong nListSize = strlen(pIn);
   if (nListSize==0) nListSize=1;   ulong nOldPerc = 0;
   long nSkipCnt = 0;

   // prerun: determine approx. number of targets
   ulong nLF = 0;
   for (ulong i=0; i<nListSize; i++)
      if (pIn[i] == '\n')
         nLF++;

   if (nGlblMD5Skip > 0) {
      ulong nCover = 100/(1+nGlblMD5Skip);
      printf("spot-checking %02u%% of files (skip=%u).\n", nCover, nGlblMD5Skip);
   }

   // main run, check md5, optionally sped up through skips
   for (;pszLine;pszLine=pszNext)
   {
      pszNext = strchr(pszLine, '\n');
      if (pszNext) *pszNext++ = 0;
      strncpy(szLineBuf, pszLine, sizeof(szLineBuf)-10);
      nLine++;
      if (!strlen(szLineBuf))
         continue;
      if (nGlblMD5Skip > 0) {
         if (nSkipCnt <= 0) {
            nSkipCnt = nGlblMD5Skip;
            // and fall through
         } else {
            nSkipCnt--;
            continue;
         }
      }
      char *psz = strstr(szLineBuf, " *"); // md5sum format
      if (!psz) {
         fprintf(stderr, "error: illegal format in line %d:\n\"%s\"\n",nLine,szLineBuf);
         nError++; 
         continue;
      }
      *psz = 0;
      char *pszHex  = szLineBuf;
      if (*pszHex == '\\') pszHex++; // support for md5sum files
      char *pszFile = psz+2;  // skip " *"
      // fix filename ending and path chars
      if (psz = strchr(pszFile, '\r')) *psz = 0;
      if (psz = strchr(pszFile, '\n')) *psz = 0;
      fixPathChars(pszFile);
      // printf("try: %s\n", pszFile);
      {
         SFKMD5 md5;      // auto instanciate
         // NOTE: this uses abBuf, so we shouldn't use abBuf here.
         if (getFileMD5(pszFile, md5)) {
            nError++;   // but continue
         }
         else
         {
            unsigned char *pmd5 = md5.digest();
            char szBuf2[100];
            for (int i=0; i<16; i++)
               sprintf(szBuf2+i*2,"%02x",pmd5[i]);
            if (strcmp(pszHex,szBuf2)) {
               fprintf(stderr, "error: MD5 mismatch: %s\n", pszFile); // ,*%s*,*%s*\n", pszFile, pszHex, szBuf2);
               nError++;
            }            
         }
      }
      if (glblFileCount.count() && !bGlblQuiet) {
         if (!nLF) nLF = 1;
         ulong nperc = nLine * 100 / nLF;
         if (nGlblMD5Skip > 0) {
            printf("[%02u%%, %u files, %u mb, %u kb/sec, skip %u] \r", nperc, glblFileCount.value(), (ulong)(nGlblBytes/1000000UL), currentKBPerSec(), nGlblMD5Skip); fflush(stdout);
         } else {
            printf("[%02u%%, %u files, %u mb, %u kb/sec] \r", nperc, glblFileCount.value(), (ulong)(nGlblBytes/1000000UL), currentKBPerSec()); fflush(stdout);
         }
      }
      if (userInterrupt())
         break;
   }
   if (nError) {
      fprintf(stderr, "error: %u files of %u failed verification.\n", nError, glblFileCount.value());
      fprintf(stdout, "info : %u files checked, %u mb, %u sec, %u kb/sec.\n", glblFileCount.value(), (ulong)(nGlblBytes/1000000UL), currentElapsedMSec()/1000, currentKBPerSec());
   } else {
      fprintf(stdout, "OK. %u files checked, all checksums matched. %u mb, %u sec, %u kb/sec.\n", glblFileCount.value(), (ulong)(nGlblBytes/1000000UL), currentElapsedMSec()/1000, currentKBPerSec());
   }
   fflush(stderr);
   fflush(stdout);
   return nError ? 9 : 0;
}

long execJamIndex(char *pszFile)
{
   // strip ".\" at start, if any
   char *psz1 = pszFile;
   if (!strncmp(psz1, glblDotSlash, 2))
         psz1 += 2;
   fprintf(fGlblOut, ":# %s\n", psz1);

   // remember in array of targets
   if (nJamTargets < MAX_JAM_TARGETS-10) {
      apJamTargets[nJamTargets++] = strdup(psz1);
   } else {
      fprintf(stderr, "error: too many snapfile targets\n");
      return 9;
   }

   return 0;
}

long execSnapAdd(char *pszFile)
{
   // strip ".\" at start, if any
   char *psz1 = pszFile;
   if (!strncmp(psz1, glblDotSlash, 2))
         psz1 += 2;
   long lRC = glblMemSnap.addTarget(psz1);
   // if (lRC != 0) printf("ESA RC %d, continue\n", lRC);
   return 0; // continue scan!
}

long execJamFile(char *pszFile)
{
   FILE *fin = 0;

   // add file content, check for illegal entries
   if (!fin) fin = fopen(pszFile, "r");
   if (!fin) { fprintf(stderr, "warning: cannot read: %s\n", pszFile); return 0; }
   long nMaxLineLen = sizeof(szLineBuf)-10; // YES, szLineBuf
   memset(abBuf, 0, nMaxLineLen+2); // yes, abBuf is larger by far
   long nLocalLines = 0;
   bool bWrapMode = (nGlblSnapWrap > 0) ? 1 : 0;
   while (fgets((char*)abBuf, nMaxLineLen, fin)) // yes, exact len
   {
      nGlblLines++;
      nLocalLines++;

      if (strlen((char*)abBuf) == nMaxLineLen)
         fprintf(stderr, "warning: max line length %d reached, splitting. file %s, line %d\n", nMaxLineLen, pszFile, nLocalLines);

      if (   (!strncmp((char*)abBuf, ":snapfile sfk,", strlen(":snapfile sfk,")))
          || (!strncmp((char*)abBuf, ":cluster sfk,", strlen(":cluster sfk,")))
         )
      {
         // we have either detected that we're re-reading our own output as input,
         // or that we're reading an older file collection somewhere down the tree.
         // exclude the content - collections shall not contain collections.
         break;
      }

      if (nLocalLines == 1) {
         // first local line: also write header
         if (pszGlblJamPrefix)
            fprintf(fGlblOut, "%s\n%s\n", pszGlblJamPrefix, pszFile);
         else
         if (!bGlblJamPure)
            fprintf(fGlblOut, ":file:\n%s\n", pszFile);
      }

      if (bWrapMode && (strlen((char*)abBuf) > nGlblSnapWrap))
      {
         // auto-wrap input line into many smaller output lines
         char *psz1 = (char*)abBuf;
         char *pszOld = 0;
         while (*psz1)
         {
            pszOld = psz1;
            long icnt = 0;
            char *pszGap = 0;
            // step until overflow or eod, remember last whitespace
            while (*psz1 && (icnt < nGlblSnapWrap)) {
               char c = *psz1;
               switch (c) {
                  case ' ':
                  case '\t':
                     pszGap = psz1;
                     break;
               }
               psz1++;
               icnt++;
            }
            // if overflow, go back past whitespace. if no whitespace,
            // make a word break at that point (splitting very long words).
            if (*psz1) {
               if (pszGap)
                  psz1 = pszGap+1;
            }
            int nLen = psz1-pszOld;
            fprintf(fGlblOut, "%.*s\n", nLen, pszOld);
            // continue past whitespace or word break.
         }
      } else {
         fputs((char*)abBuf, fGlblOut);
      }

      nGlblBytes += strlen((char*)abBuf);
      abBuf[nMaxLineLen] = '\0';
   }
   fclose(fin);

   // trailer
   fprintf(fGlblOut, "\n");

   if (glblFileCount.count()) {
      if (nGlblBytes > 1000000)
         printf("[collected %u files, %u lines, %u mb]  \r", glblFileCount.value(), nGlblLines, (ulong)(nGlblBytes/1000000UL));
      else
         printf("[collected %u files, %u lines, %u kb] \r", glblFileCount.value(), nGlblLines, (ulong)(nGlblBytes/1000UL));
      fflush(stdout);
   }
   return 0;
}

long execBinPatch(char *pszFile) 
{
   long nFileSize = 0;
   uchar *pInFile = loadBinaryFile(pszFile, nFileSize);
   if (!pInFile) { fprintf(stderr, "error: cannot read %s\n", pszFile); return 9; }

   uchar *pszSrc = (uchar*)pszGlblRepSrc;
   uchar *pszDst = (uchar*)pszGlblRepDst;
   long lPatLen  = strlen(pszGlblRepSrc);

   // printf("check: %s, %d bytes\n", pszFile, nFileSize);

   // find all occurrences of src, replace by dst
   uchar *psz1 = pInFile;
   long nHits = 0;
   long i,k;
   for (i=0; i<nFileSize; i++) 
   {
      if (psz1[i] == pszSrc[0])
      {
         for (k=1; (k<lPatLen) && ((i+k)<nFileSize); k++)
            if (psz1[i+k] != pszSrc[k])
               break;

         if (k==lPatLen)
         {
            // have a match: replace
            for (k=0; k<lPatLen; k++)
               psz1[i+k] = pszDst[k];
            i += lPatLen-1; // -1 due to i++
            nHits++;
         }
      }
   }

   if (nHits > 0) {
      FILE *fout = fopen(pszFile, "wb");
      if (!fout) { fprintf(stderr, "error: cannot write %s\n", pszFile); delete [] pInFile; return 9; }
      long nWritten = fwrite(pInFile, 1, nFileSize, fout);
      fclose(fout);
      if (nWritten != nFileSize) { fprintf(stderr, "error: unable to fully rewrite %s\n", pszFile); delete [] pInFile; return 9; }
      nGlblFiles++;
      // printf("patched: %s, %d changes, \"%s\" => \"%s\"\n", pszFile, nHits, pszGlblRepSrc, pszGlblRepDst);
      printf("patched: %s, %d changes.\n", pszFile, nHits);
   }

   delete [] pInFile; 

   return 0;
}

Array glblRefDst("RefDst");

long execRefColDst(char *pszFile)
{
   if (!bGlblQuiet)
      printf("%05u: %.50s   \r", glblRefDst.numberOfEntries(0)+1, pszFile);

   strncpy(szLineBuf, pszFile, sizeof(szLineBuf)-10);
   szLineBuf[sizeof(szLineBuf)-10] = '\0';

   // force path chars to system path char.
   // the same is done in the source contents to ease comparison.
   char *psz2 = szLineBuf;
   while (*psz2) {
      if (*psz2 == glblWrongPChar)
         *psz2 = glblPathChar;
      psz2++;
   }

   #ifdef _WIN32
   // create lowercase filename, we want to compare case-insensitive
   char *psz1 = szLineBuf;
   while (*psz1) {
      if (isalpha(*psz1))
         *psz1 = tolower(*psz1);
      psz1++;
   }
   #endif

   glblRefDst.addString(0, szLineBuf);
   glblRefDst.addLong(1, 0, __LINE__);
   for (long i=0; i<nGlblRefMaxSrc; i++)
      glblRefDst.addString(2+i, "");
   return 0;
}

long nGlblRefSrcCnt = 0;

bool isFileNameChar(char c) {
   if (isalnum(c)) return true;
   switch (c) {
      case glblPathChar: return true;
      case glblWrongPChar: return true;
      case '_': return true;
      case ':': return true;
      case '.': return true;
      case '-': return true;
      case '#': return true;
   }
   return false;
}

long execRefProcSrc(char *pszFile)
{
   nGlblRefSrcCnt++;

   if (!bGlblQuiet)
      printf("%05u: %.50s   \r", nGlblRefSrcCnt, pszFile);

   if (bGlblDebug)
      printf("\n");

   num nFileSize = getFileSize(pszFile);
   if (nFileSize <= 0)
      return 0;
   if (nFileSize >= 100 * 1048576) {
      fprintf(stderr, "warn: file too large, skipping: %s\n", pszFile);
      return 0;
   }

   // load native file content
   char *pInFile = new char[nFileSize+10];
   FILE *fin = fopen(pszFile, "rb");
   if (!fin) { 
      fprintf(stderr, "error: cannot read: %s\n", pszFile); 
      delete [] pInFile; 
      return 0; 
   }
   unsigned long nRead = fread(pInFile, 1, nFileSize, fin);
   fclose(fin);
   if (nRead != nFileSize) {
      fprintf(stderr, "error: cannot read: %s (%d %d)\n", pszFile, nRead, nFileSize);
      delete [] pInFile;
      return 0;
   }
   pInFile[nFileSize] = '\0';

   // convert content to allow strstr.
   // in case of windows prepare case-insensitive comparison.
   // force all path chars to use system path char.
   for (ulong i=0; i<nFileSize; i++) {
      char c = pInFile[i];
      if (c == '\0')
         pInFile[i] = ' ';
      else
      if (c == glblWrongPChar)
         pInFile[i] = glblPathChar;
      #ifdef _WIN32
      else
      if (isalpha(c))
         pInFile[i] = tolower(c);
      #endif
   }

   // for all dst entries
   for (long idst=0; idst<glblRefDst.numberOfEntries(0); idst++)
   {
      char *pszDst = glblRefDst.getString(0, idst);

      // check for any occurrence of relativized pszDst
      if (bGlblRefRelCmp) {
         char *pszRel = strrchr(pszDst, glblPathChar);
         if (pszRel) pszDst = pszRel+1;
         if (bGlblDebug) printf("REL *%s* \n", pszDst);
      }
      // use only basename of pszDst?
      if (bGlblRefBaseCmp) {
         strncpy(szLineBuf, pszDst, sizeof(szLineBuf)-10);
         szLineBuf[sizeof(szLineBuf)-10] = '\0';
         char *pszDot = strrchr(szLineBuf, '.');
         if (pszDot) *pszDot = '\0';
         pszDst = szLineBuf;
         if (bGlblDebug) printf("BAS *%s* \n", pszDst);
      }

      char *pszHit = 0;
      if (pszHit = strstr(pInFile, pszDst))
      {
         // increment reference count for target
         long iCnt = glblRefDst.getLong(1, idst, __LINE__);
         glblRefDst.setLong(1, idst, iCnt+1, __LINE__);

         // remember (some) source infos referencing this target
         char *pszPre = pszHit;
         while (pszPre > pInFile) {
            #ifdef _WIN32
            bool bColon = (*pszPre == ':');
            #endif
            if (!isFileNameChar(*(pszPre-1)))
               break;
            pszPre--;
            #ifdef _WIN32
            if (bColon)
               break;
            #endif
         }
         char *pszPost = pszHit;
         while (pszPost < pInFile+nFileSize-1) {
            if (!isFileNameChar(*(pszPost+1)))
               break;
            pszPost++;
         }
         sprintf(szLineBuf, "%s \"%.*s\"", pszFile, (int)(pszPost-pszPre+1), pszPre);

         // we store up to 10 source infos in the extended rows
         if (iCnt < nGlblRefMaxSrc) {
            if (bGlblRefWideInfo)
               glblRefDst.setString(2+iCnt, idst, szLineBuf);
            else
               glblRefDst.setString(2+iCnt, idst, pszFile);
         }
      }
      else {
         // printf("nohit %s: \n", pszFile);
         // sfkmem_hexdump(pInFile, (long)nFileSize);
      }
   }

   delete [] pInFile; 

   // printf("rps: %s done\n", pszFile);

   return 0;
}

long execDeblank(char *pszFile)
{
   // replace all blanks in filename by '_'
   strncpy(szLineBuf, pszFile, MAX_LINE_LEN);
   szLineBuf[MAX_LINE_LEN] = '\0';
   
   char *psz = 0;
   while (psz = strchr(szLineBuf, ' '))
      *psz = '_';

   if (!strcmp(szLineBuf, pszFile))
      return 0; // nothing to do

   if (!bGlblQuiet)
      printf("%s -> %s\n", pszFile, szLineBuf);

   if (bGlblYes) {
      int nRC = rename(pszFile, szLineBuf);
      if (nRC) { fprintf(stderr, "error: rename failed on %s\n", pszFile); return 1; }
   }

   return 0;
}

#ifdef WITH_TCP
long sendLine(SOCKET hSock, char *psz, bool bQuiet=0);

long execFTPList(char *pszFileName)
{
   long bIsDir    = 0;
   long bCanRead  = 1;
   long bCanWrite = 1;
   num  nFTimePre = 0;
   num  nFileSize = 0;
   getFileStat(pszFileName, bIsDir, bCanRead, bCanWrite, nFTimePre, nFileSize);

   // -rw-r--r-- 1 ftp ftp         102808 Nov 20  2005 lslr
   // -rw-r--r-- 1 ftp ftp              4 May 27 23:34 tmp1.dat

   mytime_t nFileTime   = (mytime_t)nFTimePre;   // may be 0xFFFF
   mytime_t nSysTime    = getSystemTime();       // may be 0xFFFF
   struct tm *pSysTime  = 0;
   struct tm *pFileTime = 0;
   ulong nSysYear = 0;

   #ifdef SFK_W64
   pSysTime  = _localtime64(&nSysTime);    // may be NULL
   nSysYear  = (pSysTime != 0) ? pSysTime->tm_year : 0;
   pFileTime = _localtime64(&nFileTime);   // OVERWRITES pSysTime!
   pSysTime  = 0;
   #else
   pSysTime  = localtime(&nSysTime);       // may be NULL
   nSysYear  = (pSysTime != 0) ? pSysTime->tm_year : 0;
   pFileTime = localtime(&nFileTime);      // OVERWRITES pSysTime!
   pSysTime  = 0;
   #endif

   char abTimeStamp[100];
   abTimeStamp[0] = '\0';

   // files of current year get time added on listing, else list year
   if (pFileTime != 0) {
      if (nSysYear == (ulong)pFileTime->tm_year)
         strftime(abTimeStamp, sizeof(abTimeStamp)-10, "%b %d %H:%M", pFileTime);
      else
         strftime(abTimeStamp, sizeof(abTimeStamp)-10, "%b %d %Y", pFileTime);
   } else {
      strcpy(abTimeStamp, "Dez 31 9999");
   }

   sprintf(szLineBuf2, "%s 1 ftp ftp %s %s %s",
      bGlblFTPReadWrite ? "-rw-rw-rw-":"-r--r--r--",
      numtoa_blank(nFileSize), abTimeStamp, pszFileName);

   if (sendLine(hGlblTCPOutSocket, szLineBuf2, 1)) // uses szLineBuf
      return 9;
   return 0;
}
#endif

long execSingleFile(char *pszFile, long lLevel, long &lFiles, long nDirFileCnt, long &lDirs, num &lBytes, num &nLocalMaxTime, num &ntime2)
{
   if (userInterrupt())
      return 9;

   // skip initial dot slash which might be returned by dir scanning
   if (!strncmp(pszFile, glblDotSlash, 2))
      pszFile += 2;

   // if set, exclude output filename from input fileset,
   // to avoid endless recursions e.g. on snapto function.
   // see also checks for FileCollectionStamp.
   if (pszGlblOutFile)
      if (equalFileName(pszGlblOutFile,pszFile))
         return 0;

   switch (nGlblFunc) 
   {
      case eFunc_MD5Write: return execMD5write(pszFile);   break;
      case eFunc_JamFile : return execJamFile(pszFile);    break;
      case eFunc_Detab   : return execDetab(pszFile);      break;
      case eFunc_JamIndex: return execJamIndex(pszFile);   break;
      case eFunc_SnapAdd : return execSnapAdd(pszFile);    break;
      case eFunc_BinPatch: return execBinPatch(pszFile);   break;
      case eFunc_FileStat: return execFileStat(pszFile, lLevel, lFiles, lDirs, lBytes, nLocalMaxTime, ntime2);  break;
      case eFunc_Grep    : return execGrep(pszFile);       break;
      case eFunc_Mirror  : return execFileMirror(pszFile, nLocalMaxTime, ntime2, nDirFileCnt); break;
      case eFunc_Run     : return execRunFile(pszFile, lLevel, lFiles, lDirs, lBytes);  break;
      case eFunc_FormConv: return execFormConv(pszFile);   break;
      #ifdef WITH_FN_INST
      case eFunc_Inst    : return execInst(pszFile, lLevel, lFiles, lDirs, lBytes);  break;
      #endif
      case eFunc_RefColDst : return execRefColDst(pszFile);  break;
      case eFunc_RefProcSrc: return execRefProcSrc(pszFile); break;
      case eFunc_Deblank : return execDeblank(pszFile); break;
      #ifdef WITH_TCP
      case eFunc_FTPList  : return execFTPList(pszFile); break;
      #endif
      default: break;
   }
   return 0;
}

char szMirStatBuf[200];
void showMirrorStatus(const char *pszAction, const char *pszStatus,
   char *pszObject, bool bIsFile, long nCount, bool bLF=0)
{
   // if pszObject is a filename with path, show just path.

   int nLen = strlen(pszObject);
   if (bIsFile) {
      char *psz1 = strrchr(pszObject, glblPathChar);
      if (psz1) nLen = psz1-pszObject;
   }
   if (nLen > 40) { pszObject += (nLen-40); nLen = 40; }

   sprintf(szMirStatBuf, "% 4lu %4.4s %4.4s %03lu %.*s ",
      glblFileCount.value(),
      pszAction, pszStatus,
      nCount, nLen, pszObject
      );

   // fill up with blanks to 76 chars
   nLen = strlen(szMirStatBuf);
   while (nLen < 76)
      szMirStatBuf[nLen++] = ' ';
   szMirStatBuf[nLen] = '\0';

   if ((nGlblVerbose==2) && strcmp(pszAction, "scan") && strcmp(pszStatus, "----"))
      bLF = 1;

   printf("%s%c", szMirStatBuf, bLF ? '\n' : '\r');
   fflush(stdout);
}

// uses szLineBuf, abBuf
long createSubDirTree(char *pszDstRoot, char *pszDirTree)
{
   // create all needed target directories
   sprintf(szLineBuf, "%s%s", pszDstRoot, pszDirTree);
   char *psz1 = szLineBuf;
   char *psz2 = 0;
   if (strlen(psz1))
      psz2 = strchr(psz1+1, glblPathChar);
   while (psz2) {
      strncpy((char*)abBuf, psz1, psz2-psz1);
      abBuf[psz2-psz1] = 0;
      char *pszDir = (char*)abBuf;
      if (!isDir(pszDir)) {
         if (bGlblDebug) printf("mkdir: %s\n", pszDir);
         #ifdef _WIN32
         if (_mkdir(pszDir))
         #else
         if (mkdir(pszDir, S_IREAD | S_IWRITE | S_IEXEC))
         #endif
         {
            fprintf(stderr, "error: cannot create dir: %s\n", pszDir);
            return 9;
         }
      }
      psz2 = strchr(psz2+1, glblPathChar);
   }
   char *pszDir = szLineBuf;
   if (!isDir(pszDir)) {
      if (bGlblDebug) printf("mkdir: %s\n", pszDir);
      #ifdef _WIN32
      if (_mkdir(pszDir))
      #else
      if (mkdir(pszDir, S_IREAD | S_IWRITE | S_IEXEC))
      #endif
      {
         fprintf(stderr, "error: cannot create dir: %s\n", pszDir);
         return 9;
      }
   }
   return 0;
}

long execDirFreeze(char *pszName, long lLevel, FileList &oDirFiles, num &nLocalMaxTime, num &nTreeMaxTime)
{
   long nLocFiles = oDirFiles.clNames.numberOfEntries();

   // in: pszGlblSrcRoot, DstRoot, pszName
   if (!strncmp(pszName, glblDotSlash, 2))
      pszName += 2;

   if (!nLocFiles) {
      // zero files found: do not call zip
      if (nGlblVerbose)
         showMirrorStatus("zip ", "skip", pszName, 0, 0);
      return 0;
   }

   // create all needed target directories
   if (createSubDirTree(pszGlblDstRoot, pszName))
      return 9;

   // check file time maxima: any need to call zip?
   sprintf(szLineBuf, "%s%s%c01-arc-part.zip", pszGlblDstRoot, pszName, glblPathChar);
   num nZipTime = getFileTime(szLineBuf);

   // FIX: whole time comparison now OPTIONAL (bGlblMirrorByDate).
   //      if user installs old files, e.g. from an archive,
   //      he surely wants them back-uped as well, although
   //      they do NOT update the directory max time.

   // IF we want to fully optimize the backup process, we have to check
   // 1. if a directory contains files not yet listed there
   // 2. if the existing files have a different md5 checksum
   // but we shall NOT simply rely on the file times!

   bool bNoChange = 0;

   if (   // (nLocalMaxTime == 0) || // i.e. no files at all contained
       (bGlblMirrorByDate && (nZipTime != 0) && (nLocalMaxTime <= nZipTime))
      )
   {
      // skip, don't bother zip
   }
   else
   {
      // else create or update zip archive
      sprintf(szLineBuf, 
              "%s -u -D %s\"%s%s%c01-arc-part.zip\" \"%s\"%c* "
              "2>>%s09-freeze-log.txt",
              pszGlblZipCmd,
              bGlblHiddenFiles ? "-S " : "",
              pszGlblDstRoot, pszName, glblPathChar,
              pszName,
              glblPathChar,
              pszGlblDstRoot
             );

      if (!bGlblQuiet) {
         if (nGlblVerbose >= 3)
            printf("%s\n", szLineBuf);
         else
         if (nGlblVerbose)
            showMirrorStatus("zip ", "----", pszName, 0, nLocFiles);
      }

      long lRC = 0;
      if (lRC = system(szLineBuf))
      {
         // zip RC 12, 3072: nothing to do
         if (lRC == 12 || lRC == 3072)
         {
            // if (nLocFiles > 0) {
            //    logError("warning: there seem to be %lu files, but ZIP says \"nothing to do\" in dir:", nLocFiles);
            //    logError("warning: %s", pszName);
            // }
            // return 0;
            bNoChange = 1;
         }
         else
         {
            // all other RC's: real error, like cannot access file
            logError("error: RC %d while running command:\n   %s", lRC, szLineBuf);
            // continue, list errors when finishing
            return 0;
         }
      }
   }

   // recreate target archive name
   sprintf(szLineBuf, "%s%s%c01-arc-part.zip", pszGlblDstRoot, pszName, glblPathChar);

   // 1. write single file entry:
   // not done here, but in execFileFreeze.

   // 2. write md5 of archive zip:
   SFKMD5 md5;
   // if (getFileMD5(szLineBuf, md5, 1))
   if (getZipMD5(szLineBuf, md5, oDirFiles))
   {
      // this is valid only if there are no files w/in dir.
      if (nLocFiles > 0) {
         nGlblFzMisArcFiles += nLocFiles;
         logError("warning: there seem to be %lu files, but ZIP created no archive for dir:", nLocFiles);
         logError("warning: %s", pszName);
      }
      // logError("error: no archive created:\n   %s", szLineBuf);
      if (nGlblVerbose)
         showMirrorStatus("zip ", "skip", pszName, 0, nLocFiles);
      return 0;
   }
   unsigned char *pmd5 = md5.digest();
   int i=0;
   for (i=0; i<16; i++)
      fprintf(fGlblMD5Arc,"%02x",pmd5[i]);
   fprintf(fGlblMD5Arc," *%s%c01-arc-part.zip\n", pszName, glblPathChar); // md5sum similar

   // 3. extend unpack batch:
   fprintf(fGlblBatch, "05-arc-tools%cunzip \"%s%c01-arc-part.zip\"\n", glblPathChar, pszName, glblPathChar);

   // remember dir maxtime in dir-times.txt
   char szTimeBuf[100];
   if (fGlblTimes != 0) {
      fprintf(fGlblTimes, "%s %s%c01-arc-part.zip\n", numtoa(nLocalMaxTime,15,szTimeBuf), pszName, glblPathChar);
      fflush(fGlblTimes);
   }

   // getZipMD5 matched the input filenamelist against the directory
   // within the zipfile. check if anything's remaining:
   long nNames = oDirFiles.clNames.numberOfEntries();
   for (i=0; i<nNames; i++) {
      char *psz = oDirFiles.clNames.getEntry(i, __LINE__);
      if (psz[0] != 0) {
         nGlblFzMisArcFiles++;
         logError("warning: file was not added to archive %s:", szLineBuf);
         logError("warning: %s", psz);
         logError("warning: maybe it was inaccessible, or using wrong zip version.");
      }
      else
         nGlblFzConArcFiles++;
   }
   nGlblFzConArchives++;

   if (bNoChange) {
      if (nGlblVerbose)
         showMirrorStatus("zip ", "-==-", pszName, 0, nLocFiles, 0);
   } else {
      showMirrorStatus("zip ", "done", pszName, 0, nLocFiles, 1);
   }

   return 0;
}

long execFileMirror(char *pszName, num &nLocalMaxTime, num &ntime2, long nDirFileCnt)
{
   if (glblFileSet.getDirCommand() == eCmd_CopyDir)
      return execFileXCopy(pszName, nLocalMaxTime, ntime2, nDirFileCnt);
   else
   if (glblFileSet.getDirCommand() == eCmd_FreezeDir)
      return execFileFreeze(pszName, nLocalMaxTime, ntime2, nDirFileCnt);
   fprintf(stderr, "error: no command supplied for directory\n");
   return 9;
}

long execDirMirror(char *pszName, long lLevel, FileList &oDirFiles, num &nLocalMaxTime, num &ntime2)
{
   if (glblFileSet.getDirCommand() == eCmd_CopyDir)
      return execDirXCopy(pszName, lLevel, oDirFiles, nLocalMaxTime, ntime2);
   else
   if (glblFileSet.getDirCommand() == eCmd_FreezeDir)
      return execDirFreeze(pszName, lLevel, oDirFiles, nLocalMaxTime, ntime2);
   fprintf(stderr, "error: no command supplied for directory\n");
   return 9;
}

long execFileFreeze(char *pszName, num &nLocalMaxTime, num &nTreeMaxTime, long nDirFileCnt)
{
   // doesn't actually freeze the single file, but

   // writes the file's md5 hash to md5org
   SFKMD5 md5;
   if (getFileMD5(pszName, md5)) {
      logError("error: failed to read:\n   %s", pszName);
      return 0;
   }
   unsigned char *pmd5 = md5.digest();
   for (int i=0; i<16; i++)
      fprintf(fGlblMD5Org,"%02x",pmd5[i]);
   fprintf(fGlblMD5Org," *%s\n",pszName); // md5sum similar

   // get file stats
   long bIsDir    = 0;
   long bCanRead  = 1;
   long bCanWrite = 1;
   num  nFileTime = 0;
   num  nFileSize = 0;
   if (getFileStat(pszName, bIsDir, bCanRead, bCanWrite, nFileTime, nFileSize)) {
      logError("error: failed to stat:\n   %s", pszName);
      return 0;
   }

   // add file to global inventory
   char szTimeBuf[100];
   char szSizeBuf[100];
   fprintf(fGlblCont, "%s %s %s\n",
      numtoa(nFileTime, 15, szTimeBuf),
      numtoa(nFileSize, 15, szSizeBuf),
      pszName
      );

   // update maxtimes
   if (nFileTime > nLocalMaxTime)
       nLocalMaxTime = nFileTime;
   if (nFileTime > nTreeMaxTime)
       nTreeMaxTime = nFileTime;

   // show some stats
   if (glblFileCount.count()) {
      if (!bGlblQuiet)
         showMirrorStatus("scan", "----", pszName, 1, nDirFileCnt);
   }

   return 0;
}

long execDirXCopy(char *pszName, long lLevel, FileList &oDirFiles, num &nLocalMaxTime, num &nTreeMaxTime)
{
   long nLocFiles = oDirFiles.clNames.numberOfEntries();

   // copy current dir or whole subtree?
   bool bAnyMasksGiven = glblFileSet.dirMasks().isStringSet(0,0);
   char *pszMode = "";
   num  nMaxTime = nLocalMaxTime;

   if (bAnyMasksGiven)
   {
      if (!nLocFiles) {
         // zero files found: do not call xcopy
         if (nGlblVerbose)
            showMirrorStatus("copy", "skip", pszName, 0, 0);
         return 0;
      }
   }
   else
   {
      // running in whole-tree mode: only toplevel acts
      if (lLevel > 0)
         return 0;
      // tell xcopy to copy whole subtree:
      #ifdef _WIN32
      pszMode = " /S";
      #else
      pszMode = "-R";
      #endif
      nMaxTime = nTreeMaxTime;
   }

   // in: pszGlblSrcRoot, DstRoot, pszName
   if (!strncmp(pszName, glblDotSlash, 2))
      pszName += 2;

   // this uses szLineBuf:
   num nOldDirTime = getOldDirTime(pszName);

   // do we have to call xcopy at all?
   if (   // (nMaxTime == 0) || // i.e. no files at all contained
       (bGlblMirrorByDate && (nOldDirTime != 0) && (nMaxTime <= nOldDirTime))
      )
   {
      // skip, don't bother xcopy
   }
   else
   {
      // copy a dir, excluding subdirectories (NO /S option),
      // except if pszMode was set other above.
      #ifdef _WIN32
      sprintf(szLineBuf, "%s \"%s\" \"%s%s\" %s/I /R /K /Y /D%s",
              pszGlblXCopyCmd, pszName, pszGlblDstRoot, pszName,
              bGlblHiddenFiles ? "/H " : "", pszMode
             );
      #else
      if (createSubDirTree(pszGlblDstRoot, pszName))
         return 9;

      sprintf(szLineBuf, "cp -p -d -u %s \"%s%c\"* \"%s%s\"",
              pszMode, pszName, glblPathChar, pszGlblDstRoot, pszName
             );
      #endif

      if (!bGlblQuiet) {
         if (nGlblVerbose)
            showMirrorStatus("copy", bAnyMasksGiven ? "dir " : "tree", pszName, 0, nLocFiles);
      }

      if (system(szLineBuf)) {
         logError("error: failed to fully execute command:\n   %s", szLineBuf);
         // continue anyway, list errors when finishing.
      }
   }

   // remember dir maxtime in dir-times.txt
   char szTimeBuf[100];
   if (fGlblTimes != 0) {
      fprintf(fGlblTimes, "%s %s\n", numtoa(nLocalMaxTime,15,szTimeBuf), pszName);
      fflush(fGlblTimes);
   }

   // VERIFY if all files do really exist in the target tree:
   long nFiles = oDirFiles.clNames.numberOfEntries();
   for (long i=0; i<nFiles; i++) 
   {
      char *pszInFileName = oDirFiles.clNames.getEntry(i, __LINE__);
      num nInFileSize = oDirFiles.clSizes.getEntry(i, __LINE__);

      sprintf(szLineBuf, "%s%s", pszGlblDstRoot, pszInFileName);

      long bIsDir    = 0;
      long bCanRead  = 1;
      long bCanWrite = 1;
      num  nFileTime = 0;
      num  nFileSize = 0;
      if (getFileStat(szLineBuf, bIsDir, bCanRead, bCanWrite, nFileTime, nFileSize)) {
         logError("error: archive not found after copy: %s", szLineBuf);
         nGlblFzMisCopFiles++;
      } else {
         if (nFileSize != nInFileSize) {
            logError("error: size mismatch after copy: %s", szLineBuf);
            nGlblFzMisCopFiles++;
         }
         else
            nGlblFzConCopFiles++;
      }
   }

   return 0;
}

long execFileXCopy(char *pszName, num &nLocalMaxTime, num &nTreeMaxTime, long nDirFileCnt)
{
   // this doesn't actually copy, but

   // writes the file's md5 hash
   SFKMD5 md5;
   if (getFileMD5(pszName, md5)) {
      logError("error: failed to read:\n   %s", pszName);
      return 0;
   }
   unsigned char *pmd5 = md5.digest();

   // BOTH to md5org ...
   int i=0;
   for (i=0; i<16; i++)
      fprintf(fGlblMD5Org,"%02x",pmd5[i]);
   fprintf(fGlblMD5Org," *%s\n",pszName); // md5sum similar

   // AND to md5arc to ensure everything's covered by verify batch.
   for (i=0; i<16; i++)
      fprintf(fGlblMD5Arc,"%02x",pmd5[i]);
   fprintf(fGlblMD5Arc," *%s\n",pszName); // md5sum similar

   // get file stats
   long bIsDir    = 0;
   long bCanRead  = 1;
   long bCanWrite = 1;
   num  nFileTime = 0;
   num  nFileSize = 0;
   if (getFileStat(pszName, bIsDir, bCanRead, bCanWrite, nFileTime, nFileSize)) {
      logError("error: failed to stat:\n   %s", pszName);
      return 0;
   }

   // add file to global inventory
   char szTimeBuf[100];
   char szSizeBuf[100];
   fprintf(fGlblCont, "%s %s %s\n",
      numtoa(nFileTime, 15, szTimeBuf),
      numtoa(nFileSize, 15, szSizeBuf),
      pszName
      );

   // update max times
   if (nFileTime > nLocalMaxTime)
      nLocalMaxTime = nFileTime;
   if (nFileTime > nTreeMaxTime)
      nTreeMaxTime = nFileTime;

   // show some stats
   if (glblFileCount.count()) {
      if (!bGlblQuiet)
         showMirrorStatus("scan", "----", pszName, 1, nDirFileCnt);
   }

   return 0;
}

long execSingleDir(char *pszName, long lLevel, long &lGlobFiles, FileList &oDirFiles, long &lDirs, num &lBytes, num &nLocalMaxTime, num &ntime2)
{
   if (userInterrupt())
      return 9;

   // skip initial dot slash which might be returned by dir scanning
   if (!strncmp(pszName, glblDotSlash, 2))
      pszName += 2;

   long lRC = 0;
   switch (nGlblFunc) 
   {
      case eFunc_FileStat:
           lRC = execDirStat(pszName, lLevel, lGlobFiles, lDirs, lBytes, nLocalMaxTime, ntime2);
           break;
      case eFunc_Mirror  :
           lRC = execDirMirror(pszName, lLevel, oDirFiles, nLocalMaxTime, ntime2);
           break;
      case eFunc_Run     :
           if (bGlblWalkJustDirs)
              lRC = execRunDir(pszName, lLevel, lGlobFiles, lDirs, lBytes);
           break;
      default:
           break;
   }

   // localMaxTime was used w/in above methods, reset now
   // to avoid potential further use in other dir trees.
   nLocalMaxTime = 0;

   return lRC;
}

long checkMask(char *pszMask) {
   if (!strcmp(pszMask, "*"))
      return 1;
   if (strchr(pszMask, '*')) { fprintf(stderr, "error: no masks with * supported, except \"*\" alone.\n"); return 0; }
   return 1;
}

long execTextJoinLines(char *pIn) {
   // join a text file with lines broken by mailing

   // 1. pre-scan for line length maximum
   char *psz = pIn;
   int nLineChars = 0;
   int nLineCharMax = 0;
   while (*psz)
   {
      char c = *psz++;
      if (c == '\r')
         continue;
      if (c == '\n') {
         if (nLineChars > nLineCharMax)
            nLineCharMax = nLineChars;
         nLineChars = 0;
         continue;
      }
      nLineChars++;
   }
   printf("[line break near %d]\n",nLineCharMax);

   // 2. join lines which are broken, pass-through others
   psz = pIn;
   nLineChars = 0;
   while (*psz)
   {
      char c = *psz++;

      if (c == '\r') // drop CR. LF-mapping is done by runtime.
         continue;

      if (c == '\n') {
         // line collected. what to do?
         if (   (nLineChars < nLineCharMax-1)
             || (nLineChars > nLineCharMax)
            )
         {
            fputc(c, fGlblOut);  // not near threshold: do not join
         }
         nLineChars = 0;
         continue;
      } else {
         fputc(c, fGlblOut);
      }

      nLineChars++;
   }
   
   return 0;
}

long execDetab(char *pszFile)
{
   // printf("probe1: %s\n", pszFile);

   char *pInFile = loadFile(pszFile, __LINE__);
   if (!pInFile) { fprintf(stderr, "error: cannot read %s\n", pszFile); return 9; }

   // printf("probe2: %s, %d\n", pszFile, strlen(pInFile));

   nGlblFiles++;

   // any tabs at all contained?
   if (!strchr(pInFile, '\t')) {
      delete [] pInFile;
      return 0;
   }

   nGlblTabFiles++;

   if (bGlblScanTabs) {
      if (nGlblTabFiles==1)
         printf("list of files containing tabs:\n");
      if (!strncmp(pszFile, glblDotSlash, 2))
         pszFile += 2;
      printf("%s\n", pszFile);
      delete [] pInFile;
      return 0;
   }

   printf("detab: %s\n", pszFile);

   // if so, overwrite.
   FILE *fOut = fopen(pszFile, "w");
   if (!fOut) { fprintf(stderr, "error: cannot overwrite %s\n", pszFile); return 9; }

   char *pCur   = pInFile;
   int bBail    = 0;
   while (!bBail && *pCur)
   {
      char *pNext = strchr(pCur, '\n');

      if (pNext)
         *pNext++ = 0; // remove LF on current line
      else
          bBail   = 1; // last line

      // truncate CR on current line, if any
      char *psz   = strchr(pCur, '\r');
      if (psz) *psz = 0;

      // detab a single line
      int nInsert=0, iout=0;
      for (int icol=0; pCur[icol]; icol++)
      {
         char c1 = pCur[icol];
         if (c1 == '\t') 
         {
            nInsert = nGlblTabSize - (iout % nGlblTabSize);
            for (int i2=0; i2<nInsert; i2++) {
               fputc(' ', fOut);
               iout++;
            }
            nGlblTabsDone++;
         }
         else {
            fputc(c1, fOut);
            iout++;
         }
      }

      fputc('\n', fOut);

      pCur = pNext;
   }

   fclose(fOut);

   delete [] pInFile;

   return 0;
}

long execFormConv(char *pszFile)
{
   char *pInFile = loadFile(pszFile, __LINE__);
   if (!pInFile) { fprintf(stderr, "error: cannot read %s\n", pszFile); return 9; }

   nGlblFiles++;

   printf("conv : %s\n", pszFile);

   // if so, overwrite in BINARY mode
   FILE *fOut = fopen(pszFile, "wb");
   if (!fOut) { fprintf(stderr, "error: cannot overwrite %s\n", pszFile); return 9; }

   uchar abCRLF[] = { 0xD, 0xA };

   char *pCur   = pInFile;
   int bBail    = 0;
   while (!bBail && *pCur)
   {
      char *pNext = strchr(pCur, '\n');

      if (pNext)
         *pNext++ = 0; // remove LF on current line
      else
          bBail   = 1; // last line

      // truncate CR on current line, if any
      char *psz   = strchr(pCur, '\r');
      if (psz) *psz = 0;

      // now we have a clean line. write in target format.
      fwrite(pCur, 1, strlen(pCur), fOut);
      if (nGlblConvTarget & eConvFormat_LF) {
         fwrite(&abCRLF[1], 1, 1, fOut);
      }
      else
      if (nGlblConvTarget & eConvFormat_CRLF) {
         fwrite(&abCRLF[0], 1, 2, fOut);
      }

      pCur = pNext;
   }

   fclose(fOut);

   delete [] pInFile;

   return 0;
}

long checkArgCnt(long argc, long lMinCnt) {
   if (argc < lMinCnt) {
      fprintf(stderr, "error: missing arguments. type \"sfk\" without parms for help.\n");
      return 9;
   }
   return 0;
}

bool isWriteable(char *pszTmpFile) {
   FILE *fout = fopen(pszTmpFile, "w");
   if (!fout) return 0;
   fclose(fout);
   return 1;
}

// uses szLineBuf, also for result!
char *findPathLocation(char *pszCmd)
{
   char *pszPath = getenv("PATH");
   if (!pszPath) { fprintf(stderr, "error: no PATH variable found.\n"); return 0; }
   char *psz1 = pszPath;
   while (*psz1) {
      char *psz2 = psz1;
      #ifdef _WIN32
      while (*psz2 && (*psz2 != ';'))
         psz2++;
      #else
      while (*psz2 && (*psz2 != ':'))
         psz2++;
      #endif
      // isolate single directory from path.
      int nLen = psz2-psz1;
      strncpy(szLineBuf, psz1, nLen);
      szLineBuf[nLen] = '\0';
      // now holding single dir in szLineBuf.
      if (bGlblDebug)
         printf("probe: %s\n", szLineBuf);
      strcat(szLineBuf, glblPathStr);
      strcat(szLineBuf, pszCmd);
      if (fileExists(szLineBuf)) {
         if (bGlblDebug)
            printf("hit: %s\n", szLineBuf);
         return szLineBuf;
      }
      // step to next subpath
      if (*psz2)
         psz2++;
      psz1 = psz2;
   }
   return 0;
}

// uses szLineBuf
long checkXCopy(char *pszTmpFile, char *pszMask)
{
   char *pszCmd = findPathLocation("xcopy.exe");
   if (!pszCmd) return 9;
   pszGlblXCopyCmd = strdup(pszCmd);

   sprintf(szLineBuf, "%s /? >%s 2>&1", pszGlblXCopyCmd, pszTmpFile);
   if (bGlblDebug) printf("%s\n", szLineBuf);
   long lRC = system(szLineBuf);
   if (lRC) return lRC;

   // read status output of tool, search for mask
   lRC = 1;
   FILE *fin = fopen(pszTmpFile, "r");
   if (fin) {
      while (fgets(szLineBuf, sizeof(szLineBuf)-10, fin)) {
         removeCRLF(szLineBuf);
         if (strstr(szLineBuf, pszMask))
            lRC = 0;
      }
      fclose(fin);
   }

   return lRC;
}

#ifdef _WIN32
 #define STR_FROM_NUL "<nul"
#else
 #define STR_FROM_NUL ""
#endif

// uses szLineBuf
long checkZipVersion(char *pszTmpFile)
{
   char *pszCmd = findPathLocation("zip" EXE_EXT);
   if (!pszCmd) { fprintf(stderr, "error: no zip" EXE_EXT " found within PATH.\n"); return 9; }
   pszGlblZipCmd = strdup(pszCmd);

   // the following command will cause a proper zip 2.31
   // to really tell it's version. zip 2.0 however will
   // try to compress stdin, therefore the <nul.
   sprintf(szLineBuf, "%s -v " STR_FROM_NUL " >%s 2>&1", pszGlblZipCmd, pszTmpFile);
   long lRC = system(szLineBuf);
   if (lRC) {
      fprintf(stderr, "error: ZIP RC: %ld - %s probably too old\n", lRC, pszGlblZipCmd);
      return lRC;
   }
 
   // read status output of tool, search for mask
   FILE *fin = fopen(pszTmpFile, "r");
   if (fin) {
      while (fgets(szLineBuf, sizeof(szLineBuf)-10, fin)) {
         removeCRLF(szLineBuf);
         if (!strncmp(szLineBuf, "This is Zip ", strlen("This is Zip "))) {
            char *pszVerHi = szLineBuf+strlen("This is Zip ");
            char *pszVerLo = pszVerHi;
            while (*pszVerLo && (*pszVerLo != '.'))
               pszVerLo++;
            if (*pszVerLo)
               pszVerLo++;
            nGlblZipVersionHi = atol(pszVerHi);
            nGlblZipVersionLo = atol(pszVerLo);
            break;
         }
      }
      fclose(fin);
   }

   if (nGlblZipVersionHi > 2)
      return 0; // OK: Zip 3.x or higher

   if (nGlblZipVersionHi < 2) {
      fprintf(stderr, "error: %s version too old (%ld.%ld)\n", pszGlblZipCmd, nGlblZipVersionHi, nGlblZipVersionLo);
      return 9; // NOK, stone-old zip 1.x
   }

   if (nGlblZipVersionLo >= 31)
      return 0; // OK: Zip 2.31

   // else below 2.31: too old
   fprintf(stderr, "error: %s version too old (%ld.%ld)\n", pszGlblZipCmd, nGlblZipVersionHi, nGlblZipVersionLo);
   return 9;
}

// uses szLineBuf
long checkUnzipVersion(char *pszTmpFile)
{
   char *pszCmd = findPathLocation("unzip" EXE_EXT);
   if (!pszCmd) { fprintf(stderr, "error: no unzip" EXE_EXT " found within PATH.\n"); return 9; }
   pszGlblUnzipCmd = strdup(pszCmd);
 
   // the following command will cause a proper unzip 5.50 to tell it's version.
   sprintf(szLineBuf, "%s -v " STR_FROM_NUL " >%s 2>&1", pszGlblUnzipCmd, pszTmpFile);
   long lRC = system(szLineBuf);
   if (lRC) {
      fprintf(stderr, "error: UNZIP RC: %ld - %s probably too old\n", lRC, pszGlblUnzipCmd);
      return lRC;
   }
 
   // read status output of tool, search for mask
   FILE *fin = fopen(pszTmpFile, "r");
   if (fin) {
      while (fgets(szLineBuf, sizeof(szLineBuf)-10, fin)) {
         removeCRLF(szLineBuf);
         if (!strncmp(szLineBuf, "UnZip ", strlen("UnZip "))) {
            char *pszVerHi = szLineBuf+strlen("UnZip ");
            char *pszVerLo = pszVerHi;
            while (*pszVerLo && (*pszVerLo != '.'))
               pszVerLo++;
            if (*pszVerLo)
               pszVerLo++;
            nGlblUnzipVersionHi = atol(pszVerHi);
            nGlblUnzipVersionLo = atol(pszVerLo);
            break;
         }
      }
      fclose(fin);
   }

   if (nGlblUnzipVersionHi > 5)
      return 0; // OK: Unzip 6.x or higher

   if (nGlblUnzipVersionHi < 5) {
      fprintf(stderr, "error: %s version too old (%ld.%ld)\n", pszGlblUnzipCmd, nGlblUnzipVersionHi, nGlblUnzipVersionLo);
      return 9; // NOK, stone-old unzip 4.x
   }

   if (nGlblUnzipVersionLo >= 50)
      return 0; // OK: Unzip 5.50

   // else below 5.50: too old
   fprintf(stderr, "error: %s version too old (%ld.%ld)\n", pszGlblUnzipCmd, nGlblUnzipVersionHi, nGlblUnzipVersionLo);
   return 9;
}

#ifdef WITH_TCP

void setBlocking(SOCKET &hSock, bool bYesNo)
{
   // make accept non-blocking:
   unsigned long ulParm = bYesNo ? 0 : 1;
   ioctlsocket(hSock, FIONBIO, &ulParm);
}

bool hasData(SOCKET &hSock, long lTimeoutMS)
{
   #ifdef _WIN32
   struct timeval tv;
   tv.tv_sec  = 0;
   tv.tv_usec = lTimeoutMS * 1000;

   fd_set fds1, fds2, fds3;
   fds1.fd_count    = 1;
   fds1.fd_array[0] = hSock;
   fds2.fd_count    = 0;
   fds3.fd_count    = 0;

   return select(0, &fds1, &fds2, &fds3, &tv) == 1;
   #else
   struct timeval tv;
   tv.tv_sec  = 0;
   tv.tv_usec = lTimeoutMS * 1000;

   fd_set fds;
   FD_ZERO(&fds);
   FD_SET(hSock, &fds);

   int nrc = select(hSock+1, &fds, NULL, NULL, &tv);
   return nrc > 0;
   #endif
}

// by default, recv() returns as much bytes as there are.
// this function forces receival of full length block.
long receiveBlock(SOCKET hSock, uchar *pBlock, ulong nLen, char *pszInfo)
{
   long nRemain = nLen;
   long nCursor = 0;
   while (nRemain > 0) {
      long nRead = recv(hSock, (char*)pBlock+nCursor, nRemain, 0);
      if (nRead <= 0) {
         if (pszInfo) // else silent mode
            fprintf(stderr, "error: failed to receive %s\n", pszInfo);
         return -1;
      }
      nRemain -= nRead;
      nCursor += nRead;
   }
   return 0;
}

long sendLine(SOCKET hSock, char *psz, bool bQuiet)
{
   strncpy(szLineBuf, psz, MAX_LINE_LEN-10);
   szLineBuf[MAX_LINE_LEN-10] = '\0';
   strcat(szLineBuf, "\r\n");
   if (!bQuiet) printf("< %s", szLineBuf);
   long nSent = send(hSock, szLineBuf, strlen(szLineBuf), 0);
   if (nSent != strlen(szLineBuf)) return 9;
   return 0;
}

long readLine(SOCKET hSock, char *pszLineBuf = szLineBuf) 
{
  while (1)
  {
   long nCursor = 0;
   long nRemain = MAX_LINE_LEN;
   pszLineBuf[0] = '\0';
   // switching from readLine mode to readBinary is tricky,
   // therefore read char by char to exactly get the point
   // of CRLF, from which on we may switch to binary.
   while (nRemain > 10) {
      // recv blocks until at least 1 byte is available.
      long nRead = recv(hSock, pszLineBuf+nCursor, 1, 0);
      if (nRead <= 0) {
         // fprintf(stderr, "error: readLine failed %ld %ld", nCursor, nRead);
         return 9;
      }
      nCursor += nRead;
      nRemain -= nRead;
      pszLineBuf[nCursor] = '\0';
      if (strstr(pszLineBuf, "\r\n"))
         break;
   }
   long nLen = strlen(szLineBuf);
   for (long i=0; i<nLen; i++)
      if (isprint(pszLineBuf[i]))
         printf("%c", pszLineBuf[i]);
   printf("\n");
   fflush(stdout);
   // reading a block of continued lines?
   if (pszLineBuf[3] != '-')
      break; // no: return (last) line
  }
   return 0;
}

bool isPathTraversal(char *pszFile) {
   if (!strlen(pszFile)) return 1;
   if (!strncmp(pszFile, ".", 1)) return 1;
   if (strstr(pszFile, "..")) return 1;
   if (strstr(pszFile, "/")) return 1;
   if (strstr(pszFile, "\\")) return 1;
   return 0;
}

// uses abBuf
long readLong(SOCKET hSock, ulong &rOut, char *pszInfo)
{
   if (receiveBlock(hSock, abBuf, 4, pszInfo)) return 9;
   ulong nLen =   (((ulong)abBuf[3])<<24)
                | (((ulong)abBuf[2])<<16)
                | (((ulong)abBuf[1])<< 8)
                | (((ulong)abBuf[0])<< 0);
   rOut = nLen;
   return 0;
}

// uses abBuf
long sendLong(SOCKET hSock, ulong nOut, char *pszInfo)
{
   abBuf[3] = ((uchar)(nOut >> 24));
   abBuf[2] = ((uchar)(nOut >> 16));
   abBuf[1] = ((uchar)(nOut >>  8));
   abBuf[0] = ((uchar)(nOut >>  0));
   long nSent = send(hSock, (char*)abBuf, 4, 0);
   if (nSent != 4) { fprintf(stderr, "error: failed to send %s\n", pszInfo); return 9; }
   return 0;
}

// uses abBuf
long readNum(SOCKET hSock, num &rOut, char *pszInfo)
{
   if (receiveBlock(hSock, abBuf, 8, pszInfo)) return 9;
   num nOut = 0;
   for (long i=0; i<8; i++) {
      nOut <<= 8;
      nOut |= (ulong)abBuf[i];
   }
   rOut = nOut;
   return 0;
}

// uses abBuf
long sendNum(SOCKET hSock, num nOut, char *pszInfo)
{
   // this may fail with num's >= 2 up 63.
   for (long i=7; i>=0; i--) {
      abBuf[i] = (uchar)(nOut & 0xFF);
      nOut >>= 8;
   }
   long nSent = send(hSock, (char*)abBuf, 8, 0);
   if (nSent != 8) { fprintf(stderr, "error: failed to send %s\n", pszInfo); return 9; }
   return 0;
}

long sendFileRaw(SOCKET hSock, char *pszFile)
{
   num nLen = getFileSize(pszFile);

   FILE *fin = fopen(pszFile, "rb");
   if (!fin) { fprintf(stderr, "error: cannot read %s\n", pszFile); return 9; }

   num nLen2 = 0;
   num nTellStep = 10;
   num nTellNext = nTellStep;
   while (nLen2 < nLen) {
      int nRead = fread(abBuf, 1, sizeof(abBuf)-10, fin);
      if (nRead <= 0) { fprintf(stderr, "error: cannot fully read %s\n", pszFile); return 9; }
      if (send(hSock, (char*)abBuf, nRead, 0) != nRead) { fprintf(stderr, "error: while sending %s\n", pszFile); return 9; }
      nLen2 += nRead;
      if (nLen2 >= nTellNext) {
         nTellNext += nTellStep;
         nTellStep += nTellStep;
         printf("%02d%% sending %s ... \r", (int)(nLen2 * 100 / (nLen+1)), pszFile);
         fflush(stdout);
      }
   }
   fclose(fin);

   printf("%lu bytes sent raw\n", (ulong)nLen2);

   return 0;
}

// INCLUDES ackReceive past file send.
long putFileBySFT(SOCKET hSock, char *pszFile)
{
   num nLen = getFileSize(pszFile);
   if (nLen <= 0)
      { fprintf(stderr, "error: cannot read %s\n", pszFile); return 9; }

   ulong nMetaSize = 8+16;
   if (sendLong(hSock, nMetaSize, "metalen")) return 9;

   // meta 1: 8 bytes filesize
   if (sendNum(hSock, nLen, "len")) return 9;

   // meta 2: 16 bytes md5
   SFKMD5 md5;
   if (getFileMD5(pszFile, md5))
      return 9;
   unsigned char *pmd5 = md5.digest();
   long nSent = send(hSock, (char*)pmd5, 16, 0);
   if (nSent != 16) { fprintf(stderr, "error: failed to send md5\n"); return 9; }

   sendFileRaw(hSock, pszFile);

   // wait until receival of ack, to avoid transmission break by premature close.
   // printf("> wait for ack\n");
   receiveBlock(hSock, abBuf, 4, 0); // 0 == silent mode
   // we do IGNORE the rc here.

   printf("] SEND  %s done, %lu bytes.\n", pszFile, nLen);

   return 0;
}

// MODE 1: receive until END OF DATA (nMaxBytes < 0)
// MODE 2: receive until nMaxBytes   (nMaxBytes > 0)
long receiveFileRaw(SOCKET hSock, char *pszFile, num nMaxBytes)
{
   FILE *fout = fopen(pszFile, "wb");
   if (!fout) { fprintf(stderr, "error: cannot write to %s\n", pszFile); return 9; }

   num nLen2 = 0;
   num nTellStep = 10;
   num nTellNext = nTellStep;
   long nRead = 0;
   while (1) 
   {
      if ((nMaxBytes >= 0) && (nLen2 >= nMaxBytes))
         break;
      if ((nRead = recv(hSock, (char*)abBuf, sizeof(abBuf)-10, 0)) <= 0)
         break; // EOD
      nLen2 += nRead;
      fwrite(abBuf, 1, nRead, fout);
      if (nLen2 >= nTellNext) {
         nTellNext += nTellStep;
         nTellStep += nTellStep;
         printf("%lu receiving %s ... \r", (ulong)nLen2, pszFile);
         fflush(stdout);
      }
   }
   fclose(fout);

   printf("%lu bytes written to %s\n", (ulong)nLen2, pszFile);

   return 0;
}

// INCLUDES ack send past file transfer
long getFileBySFT(SOCKET hSock, char *pszFile)
{
   ulong nMetaSize = 0;
   if (readLong(hSock, nMetaSize, "metalen")) return 9;

   if (nMetaSize != 8+16) {
      fprintf(stderr, "error: unsupported SFT protocol version\n");
      return 9;
   }

   // meta 1: 8 bytes filesize
   num nLen = 0;
   if (readNum(hSock, nLen, "len")) return 9;
   if (nGlblTCPMaxSizeMB)
      if (nLen > nGlblTCPMaxSizeMB * 1000000)
         { fprintf(stderr, "error: illegal length received, %s\n", numtoa(nLen)); return 9; }
   printf("< len %s\n", numtoa(nLen));

   // meta 2: 16 bytes md5
   uchar abMD5[100];
   if (receiveBlock(hSock, abMD5, 16, "md5")) return 9;

   receiveFileRaw(hSock, pszFile, nLen);

   // send short confirmation when finished, so client can safely close socket.
   long nSent = send(hSock, (char*)"ok\n\n", 4, 0);
   if (nSent != 4) { fprintf(stderr, "error: failed to send reply, %ld\n", nSent); return 9; }

   SFKMD5 md5;
   if (getFileMD5(pszFile, md5))
      return 9;
   unsigned char *pmd5 = md5.digest();

   if (memcmp(pmd5, abMD5, 16)) { fprintf(stderr, "error: md5 mismatch - transfered file corrupted.\n"); return 9; }

   return 0;
}

long ftpLogin(char *pszHost, ulong nPort, SOCKET &hSock, bool &bSFT)
{
   #ifdef _WIN32
   WORD wVersionRequested = MAKEWORD(1,1);
   WSADATA wsaData;
   if (WSAStartup(wVersionRequested, &wsaData)!=0)
      { fprintf(stderr, "error: WSAStartup failed\n"); return 9; }
   #endif

   struct hostent *pTarget;
   struct sockaddr_in sock;
   hSock = socket(AF_INET, SOCK_STREAM, 0);
   if (hSock == INVALID_SOCKET) { fprintf(stderr, "error: cannot create socket\n"); return 9; }

   if ((pTarget = gethostbyname(pszHost)) == NULL)
      { fprintf(stderr, "error: cannot get host\n"); return 9; }

   memcpy(&sock.sin_addr.s_addr, pTarget->h_addr, pTarget->h_length);
   sock.sin_family = AF_INET;
   sock.sin_port = htons((unsigned short)nPort);

   if ((connect(hSock, (struct sockaddr *)&sock, sizeof(sock))) == -1) {
      #ifdef _WIN32
      int nErr = WSAGetLastError();
      fprintf(stderr, "error: cannot connect to %s:%lu, rc %d\n", pszHost, nPort, nErr);
      #else
      fprintf(stderr, "error: cannot connect to %s:%lu\n", pszHost, nPort);
      #endif
      return 9;
   }

   // SFT: first handle FTP handshake
   if (readLine(hSock)) return 9; // 220
   if (strstr(szLineBuf, "sft 100")) {
      bSFT = 1;
      printf("> using sft 1.0\n");
   }
   if (sendLine(hSock, "USER anonymous")) return 9;
   if (readLine(hSock)) return 9; // 331
   if (sendLine(hSock, "PASS sfk@")) return 9;
   if (readLine(hSock)) return 9; // 230 login done
   if (sendLine(hSock, "TYPE I")) return 9;
   if (readLine(hSock)) return 9; // 200 OK

   return 0;
}

long connectSocket(char *pszHost, ulong nPort, struct sockaddr_in &ClntAdr, SOCKET &hSock, char *pszInfo);

long setPassive(SOCKET &hSock, struct sockaddr_in &SoAdr, SOCKET &hData)
{
   if (hData != INVALID_SOCKET)
      return 0; // already done, reuse hData

   if (sendLine(hSock, "PASV")) return 9;
   if (readLine(hSock)) return 9;
   // 227 Entering Passive Mode (127,0,0,1,117,246)
   char *psz = strchr(szLineBuf, '(');
   if (!psz) return 9;
   psz++; 
   uchar n[6];
   for (int i=0; i<6; i++) {
      n[i] = atol(psz);
      psz = strchr(psz+1, ',');
      if (psz) psz++; else break;
   }
   char szIP[50];
   sprintf(szIP, "%d.%d.%d.%d",n[0],n[1],n[2],n[3]);
   ulong nPort = (((ulong)n[4])<<8)|((ulong)n[5]);
   if (connectSocket(szIP, nPort, SoAdr, hData, "pasv data")) return 9;

   return 0;
}

long ftpClient(char *pszHost, ulong nPort, char *pszCmd=0)
{
   SOCKET hSock = 0;
   bool bSFT = 0;
   if (ftpLogin(pszHost, nPort, hSock, bSFT)) return 9;

   struct sockaddr_in DataAdr;
   SOCKET hData = INVALID_SOCKET;

   for (bool bLoop=1; bLoop;)
   {
      if (pszCmd) {
         strcpy(szLineBuf, pszCmd);
         bLoop = 0;
      } else {
         printf("> ");
         fflush(stdout);
         if (!(fgets(szLineBuf, sizeof(szLineBuf)-10, stdin)))
            break;
         removeCRLF(szLineBuf);
      }

      if (!strncmp(szLineBuf, "cd ", 3)) {
         char *pszDir = strdup(szLineBuf+3);
         sprintf(szLineBuf2, "CWD %s", pszDir);
         if (sendLine(hSock, szLineBuf2)) break;
         if (readLine(hSock)) break; // 200 OK
         delete [] pszDir;
      }
      else
      if (!strcmp(szLineBuf, "dir")) {
         if (!bSFT) {
            // either create pasv connection or reuse existing
            if (setPassive(hSock, DataAdr, hData)) break;
            if (sendLine(hSock, "LIST")) break;
            if (readLine(hSock)) break; // 150 Listing
            while (readLine(hData) == 0)
               if (!strncmp(szLineBuf, "226 ", 4))
                  break;
            if (readLine(hSock)) break; // 226 Closing
            closesocket(hData); hData = INVALID_SOCKET;
         } else {
            sendLine(hSock, "SLST");
            while (readLine(hSock) == 0)
               if (!strncmp(szLineBuf, "226 ", 4))
                  break;
         }
      }
      else
      if (!strncmp(szLineBuf, "get ", 4)) {
         char *pszFileName = strdup(szLineBuf+4);
         if (!bSFT) {
            if (setPassive(hSock, DataAdr, hData)) break;
            sprintf(szLineBuf2, "RETR %s", pszFileName);
            if (sendLine(hSock, szLineBuf2)) break;
            if (readLine(hSock)) break; // 150 Sending
            if (receiveFileRaw(hData, pszFileName, -1)) break;
            if (readLine(hSock)) break; // 226 Closing
            closesocket(hData); hData = INVALID_SOCKET;         
         } else {
            sprintf(szLineBuf2, "SGET %s", pszFileName);
            if (sendLine(hSock, szLineBuf2)) break;
            if (readLine(hSock)) break; // 200 OK
            if (!strncmp(szLineBuf, "200", 3))
               if (getFileBySFT(hSock, pszFileName))
                  break;
            // ack send was done above. keep socket open.
         }
         delete [] pszFileName;
      }
      else
      if (!strncmp(szLineBuf, "put ", 4)) {
         char *pszFileName = strdup(szLineBuf+4);
         if (!bSFT) {
            if (setPassive(hSock, DataAdr, hData)) break;
            sprintf(szLineBuf2, "STOR %s", pszFileName);
            if (sendLine(hSock, szLineBuf2)) break;
            if (readLine(hSock)) break; // 150 Receiving
            if (sendFileRaw(hData, pszFileName)) break;
            closesocket(hData); hData = INVALID_SOCKET;         
            if (readLine(hSock)) break; // 226 Closing
         } else {
            sprintf(szLineBuf2, "SPUT %s", pszFileName);
            if (sendLine(hSock, szLineBuf2)) break;
            if (readLine(hSock)) break; // 200 OK, 500 Error
            if (!strncmp(szLineBuf, "200", 3))
               if (putFileBySFT(hSock, pszFileName))
                  break;
            // ack receive was done above. keep socket open.
         }
         delete [] pszFileName;
      }
      else
      if (!strcmp(szLineBuf, "bye")) {
         break;
      }
      else {
         if (sendLine(hSock, szLineBuf)) break;
         if (readLine(hSock)) break; // 200 OK, 500 Error
      }
   }

   if (!pszCmd)
      printf("connection closed.\n");

   closesocket(hSock);

   #ifdef _WIN32
   WSACleanup();
   #endif

   return 0;
}

long makeServerSocket(ulong &nNewPort, struct sockaddr_in &ServerAdr, SOCKET &hServSock, char *pszInfo)
{
   ulong nPort = nNewPort;

   socklen_t nSockAdrSize = sizeof(sockaddr_in);

   ServerAdr.sin_family      = AF_INET;
   ServerAdr.sin_addr.s_addr = htonl(INADDR_ANY);
   ServerAdr.sin_port        = htons((unsigned short)nPort);

   hServSock = socket(AF_INET, SOCK_STREAM, 0);
   if (hServSock == INVALID_SOCKET)
      { fprintf(stderr, "error: cannot create %s (%lu)\n", pszInfo, nPort); return 9; }

   if (bind(hServSock, (struct sockaddr *)&ServerAdr, sizeof(sockaddr_in)) == SOCKET_ERROR) {
      fprintf(stderr, "error: cannot bind %s (%lu)\n", pszInfo, nPort);
      fprintf(stderr, "info : maybe a different app is running, or firewall blocks access.\n");
      fprintf(stderr, "info : you may retry with a different port, e.g. -port=30199.\n");
      return 9; 
   }

   int nerr = getsockname(hServSock, (struct sockaddr *)&ServerAdr, &nSockAdrSize);
   if (nerr == SOCKET_ERROR) {
      #ifdef _WIN32
      nerr = WSAGetLastError();
      #else
      nerr = errno;
      #endif
      fprintf(stderr, "error: getsockname failed, %d\n", nerr);
      return 9;
   }

   bool bTell = (nPort == 0);
   nPort    = (ulong)ntohs(ServerAdr.sin_port);
   nNewPort = nPort;
   if (bTell)
      printf("< local port %lu (%lu, %lu)\n", nPort, (nPort>>8), nPort&0xFF);

   // make accept non-blocking:
   setBlocking(hServSock, 0);

   if (listen(hServSock, 4) == SOCKET_ERROR)
      { fprintf(stderr, "error: cannot listen on %s (%lu)\n", pszInfo, nPort); return 9; }

   return 0;
}

long connectSocket(char *pszHost, ulong nPort, struct sockaddr_in &ClntAdr, SOCKET &hSock, char *pszInfo)
{
   hSock = socket(AF_INET, SOCK_STREAM, 0);
   if (hSock == INVALID_SOCKET)
      { fprintf(stderr, "error: cannot create %s\n", pszInfo); return 9; }

   struct hostent *pTarget = 0;
   if ((pTarget = gethostbyname(pszHost)) == NULL)
      { fprintf(stderr, "error: cannot get host for %s\n", pszInfo); return 9; }

   memcpy(&ClntAdr.sin_addr.s_addr, pTarget->h_addr, pTarget->h_length);
   ClntAdr.sin_family = AF_INET;
   ClntAdr.sin_port = htons((unsigned short)nPort);
   printf("< connect to %s:%lu\n", pszHost, nPort);
   if ((connect(hSock, (struct sockaddr *)&ClntAdr, sizeof(struct sockaddr_in))) == -1)
      { fprintf(stderr, "error: cannot establish connection for %s\n", pszInfo); return 9; }

   return 0;
}

long ftpServ(ulong nPort, bool bRW, char *pszPath)
{
   #ifdef _WIN32
   WORD wVersionRequested = MAKEWORD(1,1);
   WSADATA wsaData;
   if (WSAStartup(wVersionRequested, &wsaData)!=0)
      { fprintf(stderr, "error: WSAStartup failed\n"); return 9; }
   #endif

   bGlblFTPReadWrite = bRW;

   struct sockaddr_in ServerAdr;
   struct sockaddr_in PasServAdr;
   struct sockaddr_in ClientAdr;
   struct sockaddr_in DataAdr;
   socklen_t nSoLen = sizeof(sockaddr_in);
   SOCKET hServer   = INVALID_SOCKET;
   SOCKET hClient   = INVALID_SOCKET;
   SOCKET hPasServ  = INVALID_SOCKET;
   SOCKET hData     = INVALID_SOCKET;
   ulong  nPasPort  = 0;
   if (makeServerSocket(nPort, ServerAdr, hServer, "server main port")) return 9;

   if (bRW)
      printf("waiting on port %lu. write allowed, %lu MB limit per file.\n", nPort, nGlblTCPMaxSizeMB);
   else
      printf("waiting on port %lu. only read allowed.\n", nPort);

   long lRC = 0;
   while (!userInterrupt())
   {
      hClient = accept(hServer, (struct sockaddr *)&ClientAdr, &nSoLen);
      if (hClient == INVALID_SOCKET) {
         #ifdef _WIN32
         int nerr = WSAGetLastError();
         #else
         int nerr = errno;
         #endif
         if (nerr == WSAEWOULDBLOCK) { doSleep(500); continue; }
         fprintf(stderr, "error: accept on server main port failed\n");
         lRC = 9;
         break;
      }
      setBlocking(hClient, 1);

      // login, pseudo-authentication
      if (sendLine(hClient, "220 sfk instant ftp, 15 sec timeout. sft 100.")) { closesocket(hClient); continue; }
      if (readLine(hClient)) { closesocket(hClient); continue; } // > USER username
      if (sendLine(hClient, "331 User name ok, need password")) { closesocket(hClient); continue; }
      if (readLine(hClient)) { closesocket(hClient); continue; } // > PASS passwd
      if (sendLine(hClient, "230 User logged in")) { closesocket(hClient); continue; }

      // read client commands
      while (1)
      {
         long lTimeout = 15000;
         num t1 = getCurrentTime();
         while (getCurrentTime() < t1 + lTimeout)
         {
            if (hasData(hClient, 500))
               break;
            doSleep(500);
         }
         if (getCurrentTime() >= t1 + lTimeout) {
            sprintf(szLineBuf2, "500 inactivity timeout (%ld sec)", lTimeout/1000);
            sendLine(hClient, szLineBuf2);
            // doSleep(500);
            break;
         }

         if (readLine(hClient)) break;
         removeCRLF(szLineBuf);

         if (!strncmp(szLineBuf, "SYST", 4)) {
            if (sendLine(hClient, "215 UNIX emulated by SFK.")) break;
            continue;
         }
         else
         if (!strncmp(szLineBuf, "TYPE ", 5)) {
            if (sendLine(hClient, "200 Command OK")) break;
            continue;
         }
         else
         if (!strncmp(szLineBuf, "PWD", 3)) {
            if (sendLine(hClient, "257 \"/\" is current directory.")) break;
            continue;
         }
         else
         if (!strncmp(szLineBuf, "SIZE ", 5)) {
            char *pszFile = szLineBuf+5;
            if (pszFile[0] == '/')
                pszFile++;
            if (!isPathTraversal(pszFile)) {
               num nFileSize = getFileSize(pszFile);
               sprintf(szLineBuf2, "213 %s", numtoa(nFileSize));
               if (sendLine(hClient, szLineBuf2)) break;
            } else {
               if (sendLine(hClient, "550 File not found")) break;
            }
            continue;
         }
         else
         if (!strncmp(szLineBuf, "MDTM ", 5)) {
            // MDTM 20060604111037
            char *pszFile = szLineBuf+5;
            if (pszFile[0] == '/')
                pszFile++;
            if (!isPathTraversal(pszFile)) {
               num nFileTime = getFileTime(pszFile);
               sprintf(szLineBuf2, "213 %s", timeAsString(nFileTime, 1));
               if (sendLine(hClient, szLineBuf2)) break;
            } else {
               if (sendLine(hClient, "550 File not found")) break;
            }
            continue;
         }
         else
         if (!strcmp(szLineBuf, "RETR /")) {
            if (sendLine(hClient, "550 File not found")) break;
            closesocket(hData); hData = INVALID_SOCKET;
            continue;
         }
         else
         if (!strncmp(szLineBuf, "CWD ", 4)) {
            char *pszPath = szLineBuf+4;
            if (!strcmp(pszPath, "/")) {
               if (sendLine(hClient, "250 CWD OK")) break;
            } else {
               if (sendLine(hClient, "500 Not allowed")) break;
            }
            continue;
         }
         else
         if (!strncmp(szLineBuf, "PASV", 4)) 
         {
            // establish passive data connection server on demand
            if (hPasServ == INVALID_SOCKET) {
               nPasPort = 0; // find new local port
               if (makeServerSocket(nPasPort, PasServAdr, hPasServ, "passive server port"))
                  break;
            }
            sprintf(szLineBuf2, "227 Entering Passive Mode (127,0,0,1,%u,%u)",(nPasPort>>8),(nPasPort&0xFF));
            if (sendLine(hClient, szLineBuf2)) break;
            if (hData != INVALID_SOCKET)
               closesocket(hData);

            printf("> wait for accept\n");
            long k=0;
            for (k=0; k<6; k++)
            {
               nSoLen = sizeof(sockaddr_in);
               hData  = accept(hPasServ, (struct sockaddr *)&ClientAdr, &nSoLen);
               if (hData == INVALID_SOCKET) {
                  #ifdef _WIN32
                  int nerr = WSAGetLastError();
                  #else
                  int nerr = errno;
                  #endif
                  if (nerr == WSAEWOULDBLOCK) { doSleep(500); continue; }
                  fprintf(stderr, "error: accept on passive port failed\n");
                  break;
               }
               else
                  break;
            }
            if (k==6) { fprintf(stderr, "error: accept timeout for passive port\n"); break; }
            setBlocking(hData, 1);
            printf("> accept done\n");

            if (hData == INVALID_SOCKET)
               { fprintf(stderr, "error: passive accept failed\n"); break; }
         }
         else
         if (!strncmp(szLineBuf, "PORT ", 5)) 
         {
            // establish active data connection
            uchar n[6];
            char *psz = szLineBuf+strlen("PORT ");
            for (int i=0; i<6; i++) {
               n[i] = atol(psz);
               psz = strchr(psz+1, ',');
               if (psz) psz++; else break;
            }
            char szIP[50];
            sprintf(szIP, "%d.%d.%d.%d",n[0],n[1],n[2],n[3]);
            ulong nPort = (((ulong)n[4])<<8)|((ulong)n[5]);
            if (connectSocket(szIP, nPort, DataAdr, hData, "active data")) break;
            if (sendLine(hClient, "200 Okay")) break;
         }
         else
         if (!strncmp(szLineBuf, "LIST", 4))
         {
            if (hData == INVALID_SOCKET) {
               sendLine(hClient, "500 internal error 1");
               continue;
            }
            // path is ignored. list only current dir.
            strcpy(szLineBuf2, szLineBuf);
            if (sendLine(hClient, "150 Listing Directory")) break;
            long lFiles=0, lDirs=0, lRC=0;
            num nBytes=0;
            // if (strlen(szLineBuf2) > 8)
            //    sendLine(hData, "Path ignored - listing only current dir.");
            // sendLine(hData, "300: ftp://localhost/");
            // sendLine(hData, "200: filename"); // content-length last-modified");
            hGlblTCPOutSocket = hData;
            lRC = walkAllTrees(eFunc_FTPList, lFiles, lDirs, nBytes);
            closesocket(hData); hData = INVALID_SOCKET;
            printf("dir done, RC %ld\n", lRC);
            if (sendLine(hClient, "226 Closing data connection")) break; // 226, 250
         }
         else
         if (!strncmp(szLineBuf, "RETR ", 5)) 
         {
            if (hData == INVALID_SOCKET) {
               sendLine(hClient, "500 internal error 1");
               continue;
            }
            // send file
            strcpy(szLineBuf2, szLineBuf);
            if (sendLine(hClient, "150 Sending File")) break;
            char *pszFile = szLineBuf2+5;
            if (pszFile[0] == '/')
                pszFile++;
            if (isPathTraversal(pszFile)) {
               sendLine(hClient, "500 forbidden path");
            } else {
               printf("send file: \"%s\"\n", pszFile);
               long lRC = sendFileRaw(hData, pszFile);
               printf("send file done, RC %ld\n", lRC);
            }
            closesocket(hData); hData = INVALID_SOCKET;
            if (sendLine(hClient, "226 Closing data connection")) break; // 226, 250
         }
         else
         if (!strncmp(szLineBuf, "STOR ", 5)) 
         {
            if (hData == INVALID_SOCKET) {
               sendLine(hClient, "500 internal error 1");
               continue;
            }
            strcpy(szLineBuf2, szLineBuf);
            if (!bRW)
               sendLine(hClient, "500 write forbidden");
            else {
               // receive file
               char *pszFile = szLineBuf2+5;
               if (isPathTraversal(pszFile)) {
                  sendLine(hClient, "500 forbidden path");
               } else {
                  if (sendLine(hClient, "150 Receiving File")) break;
                  printf("recv file: \"%s\"\n", pszFile);
                  long lRC = receiveFileRaw(hData, pszFile, -1);
                  printf("recv file done, RC %ld\n", lRC);
               }
            }
            closesocket(hData); hData = INVALID_SOCKET;
            if (sendLine(hClient, "226 Closing data connection")) break; // 226, 250
         }
         else
         if (!strncmp(szLineBuf, "QUIT", 4)) {
            break;
         }
         else
         if (!strncmp(szLineBuf, "SGET ", 5)) {
            // sft file retrieve, within control connection
            strcpy(szLineBuf2, szLineBuf);
            char *pszFile = szLineBuf2+5;
            if (isPathTraversal(pszFile)) {
               sendLine(hClient, "500 forbidden path");
            } else {
               sendLine(hClient, "200 OK, data follows");
               printf("send sft file: \"%s\"\n", pszFile);
               long lRC = putFileBySFT(hClient, pszFile);
               printf("send sft file done, RC %ld\n", lRC);
            }
            // putFileBySFT includes ack receive
         }
         else
         if (!strncmp(szLineBuf, "SPUT ", 5)) {
            if (!bRW)
               sendLine(hClient, "500 write forbidden");
            else {
               // sft receive file, via control connection
               strcpy(szLineBuf2, szLineBuf);
               char *pszFile = szLineBuf2+5;
               if (isPathTraversal(pszFile)) {
                  sendLine(hClient, "500 forbidden path");
                  break; // block potential content coming
               } else {
                  sendLine(hClient, "200 OK, send data");
                  printf("recv sft file: \"%s\"\n", pszFile);
                  long lRC = getFileBySFT(hClient, pszFile);
                  printf("recv sft file done, RC %ld\n", lRC);
                  if (lRC) break; // block potential content remainder
               }
            }
            // getFileBySFT includes ack send
         }
         else
         if (!strncmp(szLineBuf, "SLST", 4)) {
            // path is ignored. list only current dir.
            if (sendLine(hClient, "150 Listing Directory")) break;
            long lFiles=0, lDirs=0, lRC=0;
            num nBytes=0;
            if (strlen(szLineBuf2) > 7)
                sendLine(hClient, "Path ignored - listing only current dir.");
            hGlblTCPOutSocket = hClient;
            lRC = walkAllTrees(eFunc_FTPList, lFiles, lDirs, nBytes);
            printf("dir done, RC %ld\n", lRC);
            if (sendLine(hClient, "226 Listing Done")) break; // 226, 250
         }
         else {
            if (sendLine(hClient, "500 not supported")) break;
            // break; // block potential content past unknown command
         }
      }  // endwhile (1)

      printf("disconnecting client\n");
      closesocket(hClient);
   }

   closesocket(hServer);

   #ifdef _WIN32
   WSACleanup();
   #endif

   return 0;
}

#endif // WITH_TCP

extern int patchMain(int argc, char *argv[], int noffs);

int main(int argc, char *argv[])
{
   long lFiles=0, lDirs=0;
   num nBytes=0;
   long lRC = 0;
   bool bDone = 0;
   long bFatal = 0;

   if (argc < 2) 
   {
      printf("sfk/" SFK_BRANCH " " SFK_VERSION " - a swiss file knife derivate.\n"); // [patch-id]
      printf("this version provided by " SFK_PROVIDER ".\n");
      printf("based on the free sfk 1.1.2 by stahlworks art & technology.\n");
      printf("Distributed for free under the GNU General Public License,\n"
             "without any warranty. Use with care and on your own risk.\n");
      printf("\n"
             "usage:\n"
             "   sfk snapto=outfile [-pure] [-norec] -dir mydir1 -file .ext1 .ext2\n"
             "       collect many files into one large text file.\n"
             "       -pure: don't insert filenames. -prefix=x: insert this before every block.\n"
             "       -norec: do not recurse into subdirs. -stat: show time stats at end.\n"
             "       -wrap=n: auto-wrap long lines near column n, e.g. -wrap=80\n"
             "       many dirs may be specified. names beginning with %c are excluded.\n"
             "          sfk snapto=all-src.cpp . .cpp .hpp\n"
             "          sfk snapto=all-src.cpp -dir src1 src2 %csrc2%cold -file .cpp .hpp .xml\n",
             glblNotChar, glblNotChar, glblPathChar
            );
      printf("   sfk syncto=dbfile dir mask [%cmask2]\n"
             "       gather files into one large editable clusterfile.\n"
             "       changes in the cluster are written back to sourcefiles automatically.\n"
             "       changes in the source files are synced into the cluster automatically.\n"
             "          sfk syncto=cluster.cpp . .cpp .hpp\n", glblNotChar
            );
      printf("   sfk syncto=dbfile [-up]\n"
             "       reuse an existing clusterfile.\n"
             "       on start, by default, cluster diffs are written to the sourcefiles.\n"
             "       on start, with -up, sourcefile diffs are written into the cluster.\n"
             "          sfk syncto=cluster.cpp\n"
             "   sfk syncto=dbfile -from=myconfig.sfk\n"
             "       read command line parameters from config file, e.g.\n"
             "          -dir\n"
             "             foosys\\bar1\\include\n"
             "             foosys\\bar1\\source\n"
             "             %csave_patch\n"
             "          -file\n"
             "             .cpp .hpp\n"
             "   mapping of compiler error output line numbers:\n"
             "       make yoursys.mak >err.txt 2>&1\n"
             "       sfk mapto=yourcluster.cpp [-nomix] [-cmd=...] <err.txt\n"
             "          nomix: list only the mapped output lines. cmd: on first mapped line,\n"
             "          call supplied command, with clustername and line as parameters.\n"
             "\n", glblNotChar
             );
      printf("   sfk detab=tabsize dir mask [%cmask2]\n"
             "       replace tabs by spaces within file(s).\n"
             "          sfk detab=3 . .cpp .hpp\n"
             "          sfk detab=3 singleFileName.txt\n"
             "   sfk scantab dir mask [%cmask2]\n"
             "       check if files contain tabs.\n"
             "          sfk scantab -dir src1 src2 -file .cpp .hpp\n",
             glblNotChar, glblNotChar
             );
      printf("   sfk lf-to-crlf or crlf-to-lf [or addcrlf, remcrlf]\n"
             "       just like detab, for CRLF conversion\n"
             "   sfk text-join-lines infile outfile\n"
             "       for text with lines split by email reformatting.\n"
             "   sfk stat [-minsize=mb] [dir] [-i]\n"
             "       show directory size statistics in mbytes.\n"
             "       minsize: list only dirs and files >= minsize mbytes.\n"
             "       -i: read list of files and directories from stdin.\n"
             "          sfk stat -minsize=10m\n"
             "          type dirlist.txt | sfk stat -quiet -i\n"
             "   sfk list [-twinscan] [-time] [-size|-size=digits] dir [mask]\n"
             "       plain listing of files within directory tree.\n"
             "       twinscan: list only identical files, if any.\n"
             "       quiet: suppress some msg. verbose: list non-regular files.\n"
             "          sfk list -dir src1 -file .cpp -dir src2 -file .hpp\n"
             "       to find most recent or largest files of directory tree:\n"
             "          sfk list -time -size=10 | sort\n"
             "          sfk list -size=10 -time | sort\n"
             "   sfk grep [-pat] pattern [pattern2] [-dir] [dir1] [mask1] ...\n"
             "       very simple, case-insensitive pattern search for text files.\n"
             "       only lines containing all patterns are listed.\n"
             "          sfk grep mytext . .hpp\n"
             "          sfk grep -pat text1 text2 -dir src1 src2 -file .cpp .hpp\n"
             "   sfk bin-to-src [-pack] infile outfile\n"
             "       create sourcefile containing a binary object\n"
             "          sfk bin-to-src myimg.dat imgsrc.cpp\n"
             );
      printf("   sfk filter <input >output [-lnum] [-c] -+orpat [++andpat] [-%cnopat] [...]\n"
             "   sfk filter infile -+pattern\n"
             "       filter lines from standard input, case-insensitive.\n"
             "       -+   this pattern MAY  be  part of a result line\n"
             "       ++   this pattern MUST be  part of a result line\n"
             "       +ls+ this pattern MUST be start of a result line\n"
             "       -%c   this pattern must NOT be  part of a result line\n"
             "       -ls%c this pattern must NOT be start of a result line\n"
             "       -no-empty-lines removes all empty lines from stream\n"
             "       -lnum preceed all result lines by input line number\n"
             "       -c   compare case-sensitive (not default)\n"
             "          anyprog | sfk filter -+mypat -%cotherpat\n",
             glblNotChar, glblNotChar, glblNotChar, glblNotChar
             );
      printf("   sfk addhead <in >out [-noblank] string1 string2 ...\n"
             "   sfk addtail <in >out [-noblank] string1 string2 ...\n"
             "       add string(s) at start or end of lines.\n"
             "       with noblank specified, does not add blank char.\n"
             "   sfk strings filename\n"
             "       extract strings from a binary file.\n"
             "          sfk strings test.exe | sfk filter -+VersionInfo\n"
             "   sfk pathfind anycmd.exe | sfk where anycmd.exe\n"
             "       tells location of anycmd.exe within PATH.\n"
             "   sfk deblank [-yes]\n"
             "       replace blanks in filenames by \"_\" character.\n"
             "       command only simulates. specify -yes to really rename.\n"
             "\n"
             );
      printf("   sfk md5gento=outfile dir mask [mask2] [%cmask3] [...]\n"
             "   sfk md5gento=outfile -dir dir1 dir2 -file mask1 mask2 %cmask3 [...]\n"
             "       create list of md5 checksums over all files.\n"
             "          sfk md5gento=md5.dat . *\n"
             "   sfk md5check=infile [-skip=n] [-skip n]\n"
             "       verify list of md5 checksums. to speed up verifys by spot checking,\n"
             "       specify -skip=n: after every checked file, n files will be skipped.\n"
             "          sfk md5check=md5.dat\n"
             "   use \"sfk md5 filename\" to create md5 for a single file.\n"
             "\n", glblNotChar, glblNotChar
             );

         printf(
             "   sfk run \"your command %cpfile [%cqfile] [...]\" [-quiet] [-sim]\n"
             "       run self-defined command on files or directories.\n"
             "       type \"sfk run\" for help.\n", glblRunChar, glblRunChar
             );

         printf(
             #ifdef _WIN32 // uses Window-specific XCopy command
             "\n"
             "   sfk freezeto=targetdir [-quiet][-hidden][-verbose] -dir src1 -copy|zip\n"
             "       create self-verifying archive tree, prepared for dvd burning.\n"
             "       type \"sfk freezeto\" for help.\n"
             #endif

             "\n"
             "   sfk reflist [-abs] [-wide] -dir tdir -file .text -dir sdir -file .sext\n"
             "       find file references and dependencies. \"sfk reflist\" for help.\n"

             #ifdef WITH_TCP
             "\n"
             "   sfk ftpserv [-h|-help] [-port=nport] [-rw] [-maxsize=n]\n"
             "       run simple ftp server, providing access to current directory.\n"
             "       type \"sfk ftpserv -help\" for help.\n"
             "\n"
             "   sfk ftp host[:port] [put|get filename]\n"
             "       simple ftp client. if connecting to sfk server, this client uses\n"
             "       sfk/sft protocol, which should always work even if ftp doesn't.\n"
             "       type \"sfk ftp\" for help.\n"
             #endif

             "\n"
             "   sfk patch [...]\n"
             "       dynamic source file patching. type \"sfk patch\" for help.\n"

             #ifdef WITH_FN_INST
             "\n"
             "   sfk inst [...]\n"
             "       instrument c++ source code with calls to micro tracing kernel.\n"
             "       type \"sfk inst\" for help.\n"
             #endif

             "\n"
             "   general option -norec avoids recursion into subdirectories.\n"
             "   general option -quiet shows less output on some commands.\n"

             #ifdef _WIN32
             "   general option -hidden includes hidden and system files and dirs.\n"
             #endif
            );

      #ifdef _WIN32
      if (sizeof(mytime_t) < 8) {
         // if you see this warning, sfk list -time may fail on file dates > 2038.
         // 64-bit times with unix are an sfk issue left open, so there is no warning.
         printf("   \ninfo: compile with latest msvc to make sfk list y2040 compliant.\n");
      }
      #endif

      return 0;
   }

   nGlblStartTime = getCurrentTime();

   char *pszCmd = argv[1];

   if (!strncmp(pszCmd, "md5gento=", strlen("md5gento="))) 
   {
      pszGlblOutFile = pszCmd+strlen("md5gento=");
      fGlblOut = fopen(pszGlblOutFile, "w");
      if (!fGlblOut) { fprintf(stderr, "error: cannot write %s\n", pszGlblOutFile); return 9; }

      processDirParms(pszCmd, argc, argv, 2, 1);
      lRC = walkAllTrees(eFunc_MD5Write, lFiles, lDirs, nBytes);

      fclose(fGlblOut);
      printf("%u files hashed into %s. %u kb/sec\n", glblFileCount.value(), pszGlblOutFile, currentKBPerSec());
      bDone = 1;
   }

   if (!strncmp(pszCmd, "md5check=", strlen("md5check="))) 
   {
      char *pszInFile = pszCmd+strlen("md5check=");
      char *pInFile = loadFile(pszInFile, __LINE__);
      if (!pInFile) { fprintf(stderr, "error: cannot read %s\n", pszInFile); return 9; }

      int iDir = 2;
      while (iDir < argc) 
      {
         if (!strncmp(argv[iDir],"-skip=",strlen("-skip="))) {
            nGlblMD5Skip = atol(argv[iDir]+strlen("-skip="));
            iDir++;
         }
         else
         if (!strcmp(argv[iDir],"-skip")) {
            iDir++;
            if (iDir < argc)
               nGlblMD5Skip = atol(argv[iDir]);
            else
               { fprintf(stderr, "error: missing parm after -skip\n"); return 9; }
         }
         else
         if (!strcmp(argv[iDir],"-quiet")) {
            bGlblQuiet = 1;
            iDir++;
         }
         else
            break;
      }
      lRC = execMD5check(pInFile);

      delete [] pInFile;
      bDone = 1;
   }

   if (!strcmp(pszCmd, "md5"))
   {
      if (checkArgCnt(argc, 3)) return 9;

      SFKMD5 md5;
      if (getFileMD5(argv[2], md5))
         return 9;

      unsigned char *pmd5 = md5.digest();
      for (int i=0; i<16; i++)
         printf("%02x",pmd5[i]);
      printf("\n");

      bDone = 1;
   }

   if (!strncmp(pszCmd, "snapto=", strlen("snapto="))) 
   {
      pszGlblOutFile = pszCmd+strlen("snapto=");
      fGlblOut = fopen(pszGlblOutFile, "w");
      if (!fGlblOut) { fprintf(stderr, "error: cannot write %s\n", pszGlblOutFile); return 9; }

      bool bstat = 0;
      int iDir = 2;
      while (iDir < argc) {
         if (!strncmp(argv[iDir],"-prefix=",strlen("-prefix="))) {
            pszGlblJamPrefix = strdup(argv[iDir]+strlen("-prefix="));
            iDir++;
         }
         else
         if (!strcmp(argv[iDir],"-pure")) {
            bGlblJamPure = 1;
            iDir++;
         }
         else
         if (!strcmp(argv[iDir],"-stat")) {
            bstat = 1;
            iDir++;
         }
         else
         if (!strncmp(argv[iDir],"-wrap=",strlen("-wrap="))) {
            nGlblSnapWrap = atol(argv[iDir]+strlen("-wrap="));
            iDir++;
         }
         else
            break;
      }
      if (!pszGlblJamPrefix)
           pszGlblJamPrefix = strdup(":file:");

      // collect dir and mask parms
      processDirParms(pszCmd, argc, argv, iDir, 1);

      // write global file header
      fprintf(fGlblOut, "%s%s", pszGlblSnapFileStamp, pszGlblJamPrefix);
      if (nGlblSnapWrap > 0)
         fprintf(fGlblOut, ",wrap=%u", nGlblSnapWrap);
      fprintf(fGlblOut,"\n\n");
      // we will scan the input if we see this content, and exclude it

      lRC = walkAllTrees(eFunc_JamFile, lFiles, lDirs, nBytes);

      fclose(fGlblOut);

      ulong nmbytes = (ulong)(nGlblBytes/1000000UL);
      ulong nkbytes = (ulong)(nGlblBytes/1000UL);
      ulong nkbsec  = currentKBPerSec();
      ulong nmbsec  = nkbsec / 1000UL;
      ulong nmsec   = (ulong)(getCurrentTime() - nGlblStartTime);
      ulong nsec    = nmsec / 1000UL;

      if (bstat)
      printf("%u files collected into %s, %d lines, %u %s, %u %s, %u %s/sec\n",
         glblFileCount.value(), pszGlblOutFile, nGlblLines,
         (nmbytes>0)?nmbytes:nkbytes, (nmbytes>0)?"mb":"kb",
         (nsec>=10)?nsec:nmsec, (nsec>=10)?"sec":"msec",
         (nmbsec>=10)?nmbsec:nkbsec, (nmbsec>=10)?"mb":"kb");
      else
      printf("%u files collected into %s, %d lines, %u %s\n",
         glblFileCount.value(), pszGlblOutFile, nGlblLines,
         (nmbytes>0)?nmbytes:nkbytes, (nmbytes>0)?"mb":"kb");

      bDone = 1;
   }

   if (!strcmp(pszCmd, "text-join-lines"))
   {
      if (checkArgCnt(argc, 4)) return 9;

      char *pszInFile  = argv[2];
      char *pszOutFile = argv[3];

      char *pInFile = loadFile(pszInFile, __LINE__);
      if (!pInFile) { fprintf(stderr, "error: cannot read %s\n", pszInFile); return 9; }

      fGlblOut = fopen(pszOutFile, "w");
      if (!fGlblOut) {
         fprintf(stderr, "error: cannot write %s\n", pszOutFile);
         delete [] pInFile;
         return 9; 
      }

      lRC = execTextJoinLines(pInFile);

      fclose(fGlblOut);
      delete [] pInFile;

      bDone = 1;
   }

   if (!strcmp(pszCmd, "scantab")) 
   {
      bGlblScanTabs = 1;
      processDirParms(pszCmd, argc, argv, 2, 1);
      lRC = walkAllTrees(eFunc_Detab, lFiles, lDirs, nBytes);
      printf("%d files of %d contain tabs.\n", nGlblTabFiles, nGlblFiles);
      bDone = 1;
   }

   if (!strncmp(pszCmd, "detab=", strlen("detab="))) 
   {
      if (checkArgCnt(argc, 3)) return 9;

      char *pszTabSize = pszCmd+strlen("detab=");
      if (!(nGlblTabSize = atol(pszTabSize))) { fprintf(stderr, "error: invalid tab size\n"); return 9; }
      bGlblScanTabs = 0;
      if (argc == 3 && strcmp(argv[2], "-i")) {
         // single file name given
         char *pszFile = argv[2];
         if (execDetab(pszFile))
            return 9;
         printf("%d files checked, %d detabbed, %d tabs in total.\n", nGlblFiles, nGlblTabFiles, nGlblTabsDone);
      } else {
         processDirParms(pszCmd, argc, argv, 2, 1);
         lRC = walkAllTrees(eFunc_Detab, lFiles, lDirs, nBytes);
         printf("%d files checked, %d detabbed, %d tabs in total.\n", nGlblFiles, nGlblTabFiles, nGlblTabsDone);
      }
      bDone = 1;
   }

   if (!strcmp(pszCmd, "lf-to-crlf") || !strcmp(pszCmd, "addcrlf"))
   {
      if (checkArgCnt(argc, 3)) return 9;
      nGlblConvTarget = eConvFormat_CRLF;
      if (argc == 3) {
         // single file name given
         char *pszFile = argv[2];
         if (execFormConv(pszFile))
            return 9;
      } else {
         processDirParms(pszCmd, argc, argv, 2, 1);
         lRC = walkAllTrees(eFunc_FormConv, lFiles, lDirs, nBytes);
      }
      printf("%d files converted.\n", nGlblFiles);
      bDone = 1;
   }

   if (!strcmp(pszCmd, "crlf-to-lf") || !strcmp(pszCmd, "remcrlf"))
   {
      if (checkArgCnt(argc, 3)) return 9;
      nGlblConvTarget = eConvFormat_LF;
      if (argc == 3) {
         // single file name given
         char *pszFile = argv[2];
         if (execFormConv(pszFile))
            return 9;
      } else {
         processDirParms(pszCmd, argc, argv, 2, 1);
         lRC = walkAllTrees(eFunc_FormConv, lFiles, lDirs, nBytes);
      }
      printf("%d files converted.\n", nGlblFiles);
      bDone = 1;
   }

   if (!strcmp(pszCmd, "binpatch")) 
   {
      // patch binary files by exact word replacement.
      // just one replacement pattern yet supported.
      if (checkArgCnt(argc, 4)) return 9;

      char *pszRep = argv[2];
      // parse format: /src/dest/
      char cDelim  = *pszRep++;
      char *pszSrc = pszRep;
      char *psz1   = pszSrc;
      while (*psz1 && (*psz1 != cDelim))
         psz1++;
      if (*psz1 != cDelim) { fprintf(stderr, "illegal format, use /src/dest/\n"); return 9; }
      *psz1++ = '\0';
      char *pszDst = psz1;
      char *psz2   = pszDst;
      while (*psz2 && (*psz2 != cDelim))
         psz2++;
      if (*psz2 != cDelim) { fprintf(stderr, "illegal format, use /src/dest/\n"); return 9; }
      *psz2 = '\0';
      // now we hold src and dest in pszSrc, pszDst
      if (strlen(pszSrc) != strlen(pszDst)) { fprintf(stderr, "error: src must have the same length as dst\n"); return 9; }
      pszGlblRepSrc = strdup(pszSrc);
      pszGlblRepDst = strdup(pszDst);
      // printf("replace: \"%s\" by \"%s\"\n",pszGlblRepSrc,pszGlblRepDst);

      if (argc == 4) {
         // single file name given
         char *pszFile = argv[3];
         lRC = execBinPatch(pszFile);
      } else {
         processDirParms(pszCmd, argc, argv, 3, 1);
         lRC = walkAllTrees(eFunc_BinPatch, lFiles, lDirs, nBytes);
         printf("%d files patched.\n", nGlblFiles);
      }

      delete [] pszGlblRepSrc;
      delete [] pszGlblRepDst;

      bDone = 1;
   }

   if (!strncmp(pszCmd, "syncto=", strlen("syncto="))) 
   {
      char *pszSnapFile = pszCmd+strlen("syncto=");
      glblFileSnap.setFileName(pszSnapFile);
      pszGlblOutFile = pszSnapFile; // to avoid inclusion of this in input

      // determine current root dir
      #ifdef _WIN32
      _getcwd(szLineBuf,sizeof(szLineBuf)-10);
      #else
      getcwd(szLineBuf,sizeof(szLineBuf)-10);
      #endif
      char *psz = strrchr(szLineBuf, glblPathChar);
      if (!psz) {
         fprintf(stderr, "error: you can do this only from within a directory, not from root.\n");
         return 9;
      }
      psz++;
      pszGlblJamRoot = strdup(psz);
      glblFileSnap.setRootDir(pszGlblJamRoot);

      bool bInitialUpSync = 0;
      int iDir = 2;
      while (iDir < argc) {
         if (!strcmp(argv[iDir], "-up")) {
            iDir++;
            bInitialUpSync = 1;
         }
         else
            break;
      }

      // collect dir and mask parms
      processDirParms(pszCmd, argc, argv, iDir, 0); // no autocomplete!

      long nReloadInfo = 0;

      // if any target file information is given
      if (glblFileSet.anythingAdded())
      {
         // read and add targets to memory snapshot
         walkAllTrees(eFunc_SnapAdd, lFiles, lDirs, nBytes);
         // now, glblMemSnap contains all targets.

         // create and write the snapfile
         glblFileSnap.copyTargetsFrom(glblMemSnap);
         if (glblFileSnap.writeToFile())
            return 9;
         printf("> created: %s\n", glblFileSnap.getFileName());
      }
      else
      {
         // else load the snapfile directly
         printf("> load : %s\n", glblFileSnap.getFileName());
         long bDroppedAny = 0;
         if (glblFileSnap.readFromFile(bDroppedAny, 0))
            return 9;
         // and force initial sync
         long lMissing = 0;
         if (glblMemSnap.mirrorTargetsFrom(glblFileSnap, lMissing))
            return 9;
         if (lMissing > 0) {
            fprintf(stderr, "> info: %d files do not exist, will re-create\n", lMissing);
         }
         if (bInitialUpSync) {
            // user selected initial up-sync
            printf("> re-reading all targets ...\n");
            long lSynced = 0;
            glblMemSnap.setAllTouched();
            if (glblFileSnap.syncUpTargets(glblMemSnap, lSynced))
               return 9;
            // tell initially to reload snapfile
            if (lSynced > 0)
               nReloadInfo = 7;
         } else {
            // initial down-sync (default)
            printf("> sync : %s\n", glblFileSnap.getFileName());
            long nSynced = 0;
            if (glblMemSnap.syncDownTargets(glblFileSnap, nSynced))
               return 9;
            printf("> done : %d targets (re)written [%u,%u]\n", nSynced, glblFileSnap.numberOfTargets(), glblMemSnap.numberOfTargets());
         }
      }

      // wait for changes and sync them
      int iBlink = 0;
      char *pszBlink = "-\\|/";
      char *pszInfo  = "";
      num  lSnapFileTime = getFileTime(glblFileSnap.getFileName());
      long bLFDone = 0;
      long bbail = 0;
      while (!bbail)
      {
         #ifdef _WIN32
         for (long iDelay=0; iDelay<10; iDelay++) {
            if (userInterrupt()) {
               bbail = 1;
               break;
            }
            Sleep(100);
         }
         #else
         sleep(1);
         #endif

         pszInfo = "                   ";

         if (bFatal) {
            if (!bLFDone) { bLFDone=1; printf("\n"); }      
            iBlink++;
            if (iBlink & 1) printf("* * * S Y N C   A B O R T E D * * *   \r");
            else            printf("                                      \r");
            fflush(stdout);
            continue;
         }

         // check for updates in the snapfile
         num lSnapTimeNew = getFileTime(glblFileSnap.getFileName());
         if (lSnapTimeNew != lSnapFileTime) 
         {
            // remember new filetime
            lSnapFileTime = lSnapTimeNew;
            // re-read the snapfile
            long bDroppedAny = 0;
            if (glblFileSnap.readFromFile(bDroppedAny, 1))
               { bFatal=1; continue; }
            if (bDroppedAny) {
               // target list changed. have to re-write and reload.
               if (glblFileSnap.writeToFile()) return 9;
               bDroppedAny = 0;
               if (glblFileSnap.readFromFile(bDroppedAny, 1)) return 9;
               nReloadInfo = 7;
            }
            // check for changed snapfile targets
            long nSynced = 0;
            if (glblMemSnap.syncDownTargets(glblFileSnap, nSynced))
               { bFatal=1; continue; }
            if (nSynced == 0 && nReloadInfo == 0) {
               printf("[ NODIF: %s ]                       \n", glblFileSnap.getFileName());
            } else {
               #ifdef SFK_WINPOPUP_SUPPORT
               // collect short infos about written files
               szLineBuf[0] = '\0';
               for (int i=0; i<MAX_SYNC_INFO; i++) {
                  char *psz1 = glblMemSnap.getLastSyncInfo(i);
                  if (psz1) {
                     char *psz2 = strrchr(psz1, glblPathChar);
                     if (psz2) psz1 = psz2+1;
                     if ((strlen(psz1)+strlen(szLineBuf)) < sizeof(szLineBuf)-10) {
                        strcat(szLineBuf, psz1);
                        strcat(szLineBuf, " ");
                     }
                  }
               }
               // e.g. on file dropping, this might be empty
               if (strlen(szLineBuf) > 0)
                  showInfoPopup(szLineBuf);
               #endif
            }
            pszInfo = (nSynced > 0) ? (char*)"loaded" : (char*)"no change";
         }

         // check for updates in the target files
         long lRC = glblMemSnap.checkLoadTargets();
         if (lRC > 3)
           { bFatal=1; continue; }
         long lSynced = 0;
         if (lRC & 1) {
            // updates have been reloaded. 
            // reloaded targets have the touched() flag set.
            // we expect user knows what's going on,
            // therefore sync up into the snapfile.
            if (glblFileSnap.syncUpTargets(glblMemSnap, lSynced))
               { bFatal=1; continue; }
            if (lSynced > 0)
               nReloadInfo = 7;
         }
         if (lRC & 2) {
            // files became unreadable and have been dropped
            if (lSynced == 0) {
               printf("[ WRITE : %s ]\n", glblFileSnap.getFileName());
               glblFileSnap.writeToFile();
               nReloadInfo = 7;
            }
            // else cluster write was done above
         }

         iBlink++;
         if (nReloadInfo > 0) {
            if (nReloadInfo == 7) {
               #ifdef SFK_WINPOPUP_SUPPORT
               showInfoPopup(0);
               #endif
            }
            nReloadInfo--;
            if (iBlink & 1) pszInfo = "* * * RELOAD CLUSTER NOW * * *";
            else            pszInfo = "                              ";
            printf("%s \r",pszInfo);
            fflush(stdout);
         } else {
            #ifdef _WIN32
            printf("%c auto-sync active, %d targets. (%u kb) %s \r",
               pszBlink[iBlink & 3],glblMemSnap.numberOfTargets(),
               sfkmem_bytes / 1024,
               pszInfo);
            #else
            printf("%c auto-sync active, %d targets. %s \r",
               pszBlink[iBlink & 3],glblMemSnap.numberOfTargets(),
               pszInfo);
            #endif
            fflush(stdout);
         }
      }

      bDone = 1;
   }

   if (!strncmp(pszCmd, "mapto=", strlen("mapto=")))
   {
      char *pszSnapFile = pszCmd+strlen("mapto=");
      glblFileSnap.setFileName(pszSnapFile);
      pszGlblOutFile = pszSnapFile; // to avoid inclusion of this in input

      // options, command supplied?
      int iDir = 2;
      bool bMix = 1;
      char *pszCmd = 0;

      while (iDir < argc) {
         if (!strcmp(argv[iDir], "-nomix")) {
            bMix = 0;
            iDir++;
         }
         else
         if (!strncmp(argv[iDir], "-cmd=", strlen("-cmd="))) {
            pszCmd = argv[iDir] + strlen("-cmd=");
            iDir++;
         }
         else
            break;
      }

      // determine current root dir
      #ifdef _WIN32
      _getcwd(szLineBuf,sizeof(szLineBuf)-10);
      #else
      getcwd(szLineBuf,sizeof(szLineBuf)-10);
      #endif
      char *psz = strrchr(szLineBuf, glblPathChar);
      if (!psz) {
         fprintf(stderr, "error: you can do this only from within a directory, not from root.\n");
         return 9;
      }
      psz++;
      pszGlblJamRoot = strdup(psz);
      glblFileSnap.setRootDir(pszGlblJamRoot);

      // collect dir and mask parms
      processDirParms(pszCmd, argc, argv, iDir, 0); // no autocomplete!

      long nReloadInfo = 0;

      // load the snapfile directly
      // printf("> load : %s\n", glblFileSnap.getFileName());
      long bDroppedAny = 0;
      if (glblFileSnap.readFromFile(bDroppedAny, 0))
         return 9;

      // go into compiler output parsing mode
      glblFileSnap.mapCompilerOutput(bMix, pszCmd);

      bDone = 1;
   }

   if (!strcmp(pszCmd, "stat"))
   {
      nGlblListMode = 1;

      int iDir = 2;
      while (iDir < argc) {
         if (!strncmp(argv[iDir], "-minsize=", strlen("-minsize="))) {
            char *pszMinSize = &argv[iDir][strlen("-minsize=")];
            nGlblListMinSize = atol(pszMinSize);
            iDir++;
         }
         else
            break;
      }

      if (iDir >= argc)
         setProcessCurrentDir();
      else
         processDirParms(pszCmd, argc, argv, iDir, 1);
      lRC = walkAllTrees(eFunc_FileStat, lFiles, lDirs, nBytes);

      long lMBytes = (long)(nBytes / 1000000);
      if (!bGlblQuiet) {
         if (nGlblNoFiles)
            printf("%lu non-regular files skipped.\n", nGlblNoFiles);
      }

      // printf("%5ld mb, %5ld files in total.\n", lMBytes, lFiles);

      bDone = 1;
   }

   if (!strcmp(pszCmd, "list"))
   {
      nGlblListMode = 2;
      int iDir = 2;
      while (iDir < argc) {
         if (!strcmp(argv[iDir], "-twinscan")) {
            nGlblListMode = 3;
            iDir++;
         }
         else
         if (!strcmp(argv[iDir], "-size")) {
            // do not just remember the flag, but also option sequence.
            nGlblListForm = ((nGlblListForm << 8) | 0x01);
            iDir++;
         }
         else
         if (!strncmp(argv[iDir], "-size=", strlen("-size="))) {
            // size format with digits specified
            nGlblListForm   = ((nGlblListForm << 8) | 0x01);
            char *psz1 = argv[iDir] + strlen("-size=");
            nGlblListDigits = atol(psz1);
            iDir++;
         }
         else
         if (!strcmp(argv[iDir], "-time")) {
            // do not just remember the flag, but also option sequence.
            nGlblListForm = ((nGlblListForm << 8) | 0x02);
            iDir++;
         }
         else
            break;
      }

      processDirParms(pszCmd, argc, argv, iDir, 1);
      lRC = walkAllTrees(eFunc_FileStat, lFiles, lDirs, nBytes);

      if (nGlblListMode == 3)
         glblTwinScan.analyze();
      glblTwinScan.shutdown();

      if (!bGlblQuiet) {
         if (nGlblNoFiles)
            printf("%lu non-regular files skipped.\n", nGlblNoFiles);
      }

      bDone = 1;
   }

   if (!strcmp(pszCmd, "grep"))
   {
      int iDir = 2;
      if (checkArgCnt(argc, 3)) return 9;
      if (strncmp(argv[iDir], "-", 1)) {
         // first argument supplied is a single pattern.
         glblGrepPat.addString(argv[iDir]);
         iDir++;
         if (iDir >= argc)
            setProcessCurrentDir();
      }
      if (iDir < argc)
         processDirParms(pszCmd, argc, argv, iDir, 1);
      lRC = walkAllTrees(eFunc_Grep, lFiles, lDirs, nBytes);
      bDone = 1;
   }

   if (!strcmp(pszCmd, "bin-to-src"))
   {
      if (checkArgCnt(argc, 4)) return 9;

      bool bPack = 0;
      int iDir = 2;
      while (iDir < argc) {
         if (!strcmp(argv[iDir], "-pack")) {
            iDir++;
            bPack = 1;
         }
         else
            break;
      }

      char *pszInFile  = argv[iDir+0];
      char *pszOutFile = argv[iDir+1];

      long  nFileSize = 0;
      uchar *pInFile = loadBinaryFile(pszInFile, nFileSize);
      if (!pInFile) { fprintf(stderr, "error: cannot read %s\n", pszInFile); return 9; }

      fGlblOut = fopen(pszOutFile, "w");
      if (!fGlblOut) {
         fprintf(stderr, "error: cannot write %s\n", pszOutFile);
         delete [] pInFile;
         return 9; 
      }

      lRC = execBin2Src(pInFile, nFileSize, bPack);

      fclose(fGlblOut);
      delete [] pInFile;

      bDone = 1;
   }

   if (!strcmp(pszCmd, "filter"))
   {
      // simple input stream filter
      // -+pat1 -+pat2 -!pat3

      if (checkArgCnt(argc, 3)) return 9;

      int iPat = 2;
      int nPat = argc - 2;

      bool bWithLineNumbers = 0;
      bool bCaseSensitive = 0;
      char *pszInFile = 0;

      while (iPat < argc) {
         if (!strcmp(argv[iPat], "-lnum")) {
            bWithLineNumbers = 1;
            iPat++;
            nPat--;
         }
         else
         if (!strcmp(argv[iPat], "-c")) {
            bCaseSensitive = 1;
            iPat++;
            nPat--;
         }
         else
         if (strncmp(argv[iPat], "-", 1)) {
            // non-option: take as input file and proceed
            pszInFile = argv[iPat];
            iPat++;
            nPat--;
            break;
         }
         else
            break; // any other option: proceed as well
      }

      FILE *fin = stdin;
      if (pszInFile)
         if (!(fin = fopen(pszInFile, "r")))
            { fprintf(stderr, "error: cannot read: %s\n", pszInFile); return 9; }

      long nMaxLineLen = sizeof(szLineBuf)-10;
      long nLine = 0;
      while (fgets(szLineBuf, nMaxLineLen, fin))
      {
         nLine++;
         removeCRLF(szLineBuf);
         bool bHasPositive = 0;
         bool bHasNegative = 0;
         bool bHavePosFilt = 0;
         bool bHaveNegFilt = 0;
         for (int i=0; i<nPat; i++) 
         {
            char *pszPat = argv[iPat+i];
            if (!strncmp(pszPat, "-+", 2)) {
               bHavePosFilt = 1;
               if (bCaseSensitive) {
                  if (strstr(szLineBuf, pszPat+2))
                     bHasPositive = 1;
               } else {
                  if (mystrstri(szLineBuf, pszPat+2))
                     bHasPositive = 1;
               }
            }
            else
            if (!strncmp(pszPat, "-ls+", 4)) {
               bHavePosFilt = 1;
               if (!strncmp(szLineBuf, pszPat+4, strlen(pszPat)-4))
                  bHasPositive = 1;
            }
            else
            if (!strncmp(pszPat, "++", 2)) {
               bHavePosFilt = 1;
               if (bCaseSensitive) {
                  if (strstr(szLineBuf, pszPat+2))
                     bHasPositive = 1;
                  else {
                     bHasNegative = 1;
                     break;
                  }
               } else {
                  if (mystrstri(szLineBuf, pszPat+2))
                     bHasPositive = 1;
                  else {
                     bHasNegative = 1;
                     break;
                  }
               }
            }
            else
            if (!strncmp(pszPat, "+ls+", 4)) {
               bHavePosFilt = 1;
               if (!strncmp(szLineBuf, pszPat+4, strlen(pszPat)-4))
                  bHasPositive = 1;
               else {
                  bHasNegative = 1;
                  break;
               }
            }
            else
            if (!strncmp(pszPat, SFK_FILT_NOT1, 2)) {
               bHaveNegFilt = 1;
               if (bCaseSensitive) {
                  if (strstr(szLineBuf, pszPat+2)) {
                     bHasNegative = 1;
                     break;
                  }
               } else {
                  if (mystrstri(szLineBuf, pszPat+2)) {
                     bHasNegative = 1;
                     break;
                  }
               }
            }
            else
            if (!strncmp(pszPat, SFK_FILT_NOT2, 4)) {
               bHaveNegFilt = 1;
               if (!strncmp(szLineBuf, pszPat+4, strlen(pszPat)-4)) {
                  bHasNegative = 1;
                  break;
               }
            }
            else
            if (!strcmp(pszPat, "-no-empty-lines")) {
               if (strlen(szLineBuf))
                  bHasPositive = 1;
               else {
                  bHasNegative = 1;
                  break;
               }
            }
         }

         bool bMatch = 0;

         // if ANY positive filter is given, matches MUST contain this.
         if (bHavePosFilt && bHasPositive && !bHasNegative)
            bMatch = 1;

         // if NO positive filter is given, ANYTHING matches,
         // except if there is a negative filter match.
         if (!bHavePosFilt && !bHasNegative)
            bMatch = 1;

         if (bMatch) {
            if (bWithLineNumbers)
               printf("%03u %s\n", nLine, szLineBuf);
            else
               printf("%s\n", szLineBuf);
            fflush(stdout);
         }
      }

      if (pszInFile)
         fclose(fin);

      bDone = 1;
   }

   if (!strcmp(pszCmd, "tee"))
   {
      if (checkArgCnt(argc, 3)) return 9;
      pszGlblOutFile = argv[2];

      fGlblOut = fopen(pszGlblOutFile, "w");
      if (!fGlblOut) { fprintf(stderr, "error: cannot write %s\n", pszGlblOutFile); return 9; }

      long nMaxLineLen = sizeof(szLineBuf)-10;
      while (fgets(szLineBuf, nMaxLineLen, stdin))
      {
         removeCRLF(szLineBuf);
         fprintf(stdout, "%s\n", szLineBuf);
         fflush(stdout);
         fprintf(fGlblOut, "%s\n", szLineBuf);
      }

      fclose(fGlblOut);

      bDone = 1;
   }

   if (!strcmp(pszCmd, "addhead"))
   {
      if (checkArgCnt(argc, 3)) return 9;

      int iDir=2;
      bool bblank=1;
      if (!strcmp(argv[iDir], "-noblank") && argc >= 4)
         { bblank=0; iDir++; }

      char *pszToAdd = argv[iDir];
      long nMaxLineLen = sizeof(szLineBuf)-10;
      while (fgets(szLineBuf, nMaxLineLen, stdin))
      {
         removeCRLF(szLineBuf);
         // left: collect all parameters to add as head
         szLineBuf2[0] = '\0';
         for (int i1=iDir; i1<argc; i1++) {
            strcat(szLineBuf2, argv[i1]);
            if (!bblank && (i1 == argc-1))
               continue;
            strcat(szLineBuf2, " "); 
         }
         // add middle
         strcat(szLineBuf2, szLineBuf);
         // no right side
         printf("%s\n", szLineBuf2);
      }
      fflush(stdout);
      bDone = 1;
   }

   if (!strcmp(pszCmd, "addtail"))
   {
      if (checkArgCnt(argc, 3)) return 9;

      int iDir=2;
      bool bblank=1;
      if (!strcmp(argv[iDir], "-noblank") && argc >= 4)
         { bblank=0; iDir++; }

      char *pszToAdd = argv[iDir];
      long nMaxLineLen = sizeof(szLineBuf)-10;
      while (fgets(szLineBuf, nMaxLineLen, stdin))
      {
         removeCRLF(szLineBuf);
         // right: collect all parameters
         for (int i1=iDir; i1<argc; i1++) {
            if (!bblank && (i1 == iDir))
               { }
            else
               strcat(szLineBuf, " "); 
            strcat(szLineBuf, argv[i1]);
         }
         printf("%s\n", szLineBuf);
      }
      fflush(stdout);
      bDone = 1;
   }

   if (!strcmp(pszCmd, "ziplist"))
   {
      // this internal function is merely to check if sfk is
      // (still) able of reading zip file local headers.
      if (checkArgCnt(argc, 3)) return 9;
      SFKMD5 md5;
      FileList oFiles;
      getZipMD5(argv[2], md5, oFiles, 1);
      long nFiles = oFiles.clNames.numberOfEntries();
      for (long i=0; i<nFiles; i++) {
         char *psz = oFiles.clNames.getEntry(i, __LINE__);
         printf("%s\n", psz);
      }
      bDone = 1;
   }

   if (!strcmp(pszCmd, "freezeto")) {
      printf(
         "sfk freezeto=targetdir [-quiet][-hidden][-verbose[=2]] -dir src1 -copy|zip\n"
         "\n"
         "    create self-verifying, self-unpacking archive tree, for dvd burning.\n"
         "    this copies or zips all or updated files from src to target,\n"
         "    USING THE EXTERNAL COMMANDS XCOPY from windows, ZIP and UNZIP\n"
         "    from the InfoZIP group. these tools must be available via the\n"
         "    command line PATH. if using -zip on trees, every directory's\n"
         "    content is packed into a single .zip file, for best compromise\n"
         "    between read speed, fail safety, and compression.\n"
         "\n"
         "    !dirname excludes directory \"dirname\".\n"
         "    quiet  : avoid listing every single zip or xcopy call.\n"
         "    hidden : also include system and hidden files.\n"
         "    verbose: show [=2 list] every zip or copy action.\n"
         "\n"
         "    examples:\n"
         "       sfk freezeto=d:\\freeze -dir mysrc !mysrc\\old\\ -zip -dir myprog -copy\n"
         "       sfk freezeto=d:\\freeze -from=freeze-script.sfk\n"
         "\n"
         "       this creates an archive tree d:\\freeze. files from mysrc are zipped,\n"
         "       except mysrc\\old, which is excluded. files from myprog are copied.\n"
         "       alternatively, supply all parms in a script and say -from=scriptname.\n"
         "       sfk will copy sfk" EXE_EXT " and unzip" EXE_EXT " into a dir d:\\freeze\\05-arc-tools,\n"
         "       and create several batch files for verification and later decompression.\n"
         "       before burning d:\\freeze onto dvd (or copying it to usb stick), test-run\n"
         "       the created batch 01-verify-arc.bat if it really behaves like expected.\n"
         "\n"
         "    For good performance, use freezeto only with TWO PHYSICALLY INDEPENDENT\n"
         "    HARD DISKS - e.g. from internal to external disk, or from network to local.\n"
         "    Do NOT freezeto DIRECTLY onto usb stick or dvd-rw media! Instead, 1. freeze\n"
         "    to an intermediate folder 2. then burn/copy this folder to the target media.\n"
         "\n"
         "    NOTE: as everything else, freezeto comes WITHOUT ANY WARRANTY!\n"
         "    -> Always have a manual check afterwards if your most important\n"
         "       files are really contained within the backup archives!\n"
         "    -> Test read your backup media on different drives and machines,\n"
         "       by running the auto-created script file 01-verify-arc.bat\n"
         "    -> Test restoring of your backups from time to time!\n"
         "       to do so, copy media onto hard disk, then 02-unpack-arc.bat,\n"
         "       \"del 01-arc-part.zip /S\", and finally run 03-verify-org.bat.\n"
         );

      return 0;
   }

   if (!strncmp(pszCmd, "freezeto=", strlen("freezeto=")))
   {
      if (argc < 3) { fprintf(stderr, "error: missing parms\n"); return 9; }

      char *pszDstRoot = pszCmd+strlen("freezeto=");
      if (!isDir(pszDstRoot))
         {  fprintf(stderr, "error: no such directory: %s\n", pszDstRoot); return 9; }

      // copy destination root, auto-add "\\" if missing
      pszGlblDstRoot = new char[strlen(pszDstRoot)+10];
      strcpy (pszGlblDstRoot, pszDstRoot);
      if (pszGlblDstRoot[strlen(pszGlblDstRoot)-1] != glblPathChar)
         strcat(pszGlblDstRoot, glblPathStr);

      int iDir = 2;
      // while (iDir < argc) {
      //    if (!strcmp(argv[iDir], "-quiet"))
      //       { iDir++; bGlblQuiet = 1; }
      //    else
      //       break;
      // }

      if (processDirParms(pszCmd, argc, argv, iDir, 1))
         return 9;

      // test if required tools are available, using freeze-log as tmpfile.
      bool bBail = 0;

      sprintf(szLineBuf2, "%s09-freeze-log.txt", pszGlblDstRoot);
      if (!isWriteable(szLineBuf2)) { fprintf(stderr, "error: cannot write to: %s\n", szLineBuf2); return 9; }

      long lToolRC = 0;

      #ifdef _WIN32
      lToolRC = checkXCopy(szLineBuf2, "[/D");
      if (lToolRC) { fprintf(stderr, "error: cannot run XCOPY command. please check your path. (rc %ld)\n", lToolRC); bBail=1; }
      #else
      pszGlblXCopyCmd = strdup("cp");
      #endif

      lToolRC = checkZipVersion(szLineBuf2);
      if (lToolRC) { fprintf(stderr, "error: cannot run ZIP command. please get InfoZIP 2.31 or higher.\n"); bBail=1; }

      lToolRC = checkUnzipVersion(szLineBuf2);
      if (lToolRC) { fprintf(stderr, "error: cannot run UNZIP command. please get Unzip 5.50 or higher.\n"); bBail=1; }

      char *pszSFKCmd = findPathLocation("sfk" EXE_EXT);
      if (!pszSFKCmd) { fprintf(stderr, "error: cannot find location of sfk" EXE_EXT " within PATH.\n"); bBail=1; }
      else
         pszGlblSFKCmd = strdup(pszSFKCmd);

      // cleanup tmp infos. szLineBuf2 string is further used in prep's.
      remove(szLineBuf2);

      if (bBail) return 9;

      // tell exactly which external tool commands we're using.
      printf("] using: %s, %s\n", pszGlblXCopyCmd, pszGlblSFKCmd);
      printf("] using: %s (%ld.%ld), %s (%ld.%ld)\n", pszGlblZipCmd, nGlblZipVersionHi, nGlblZipVersionLo, pszGlblUnzipCmd, nGlblUnzipVersionHi, nGlblUnzipVersionLo);

      char szContentsName[1024];
      char szMD5ArcName[1024];
      char szBatchName[1024];
      char szMD5OrgName[1024];
      char szTimesName[1024];
      char szToolsDir[1024];

      const char *pszVerArcBatch = "01-verify-arc.bat";
      const char *pszUnpArcBatch = "02-unpack-arc.bat";
      const char *pszVerOrgBatch = "03-verify-org.bat";
      const char *pszToolsDir    = "05-arc-tools";
      const char *pszContentFile = "06-content.txt";
      const char *pszMD5OrgFile  = "07-md5-org.txt";
      const char *pszMD5ArcFile  = "08-md5-arc.txt";
      #ifdef SFK_USE_DIR_TIMES
      const char *pszTimesFile   = "10-dir-times.txt";
      #endif

      sprintf(szContentsName, "%s%s", pszGlblDstRoot, pszContentFile);
      sprintf(szMD5ArcName  , "%s%s", pszGlblDstRoot, pszMD5ArcFile );
      sprintf(szMD5OrgName  , "%s%s", pszGlblDstRoot, pszMD5OrgFile );
      sprintf(szBatchName   , "%s%s", pszGlblDstRoot, pszUnpArcBatch);
      #ifdef SFK_USE_DIR_TIMES
      sprintf(szTimesName   , "%s%s", pszGlblDstRoot, pszTimesFile  );
      #endif
      sprintf(szToolsDir    , "%s%s", pszGlblDstRoot, pszToolsDir   );

      // ----- file preparations begin -----
      #ifdef SFK_USE_DIR_TIMES
      pszGlblDirTimes = loadFile(szTimesName, __LINE__);
      #endif

      #ifdef _WIN32
      int nRCmd = _mkdir(szToolsDir);
      #else
      int nRCmd =  mkdir(szToolsDir, S_IREAD | S_IWRITE | S_IEXEC);
      #endif
      if (!nRCmd) printf("] mkdir: %s\n", szToolsDir);
      // else we expect the dir already exists.

      // copy at least sfk.exe and unzip.exe to the tools dir:
      #ifdef _WIN32
      sprintf(szLineBuf, "copy %s %s /Y 1>%s 2>&1", pszGlblSFKCmd, szToolsDir, szLineBuf2);
      #else
      sprintf(szLineBuf, "cp %s %s 1>%s 2>&1", pszGlblSFKCmd, szToolsDir, szLineBuf2);
      #endif
      printf("] copy : %s -> %s\n", pszGlblSFKCmd, szToolsDir);
      nRCmd = system(szLineBuf);
      if (nRCmd) printf("warning: RC %d while copying sfk" EXE_EXT " to archive tree.\n", nRCmd);

      #ifdef _WIN32
      sprintf(szLineBuf, "copy %s %s /Y 1>%s 2>&1", pszGlblUnzipCmd, szToolsDir, szLineBuf2);
      #else
      sprintf(szLineBuf, "cp %s %s 1>%s 2>&1", pszGlblUnzipCmd, szToolsDir, szLineBuf2);
      #endif
      printf("] copy : %s -> %s\n", pszGlblUnzipCmd, szToolsDir);
      nRCmd = system(szLineBuf);
      if (nRCmd) printf("warning: RC %d while copying unzip" EXE_EXT " to archive tree.\n", nRCmd);

      #ifdef _WIN32
      if (!bGlblHiddenFiles)
         printf("] info : excluding hidden and system files. use \"dir /AHS /S /B\" to list them.\n");
      #endif

      // ----- file preparations end -----

      if (!(fGlblCont   = fopen(szContentsName, "w"))) { fprintf(stderr, "error: unable to write: %s\n", szContentsName); return 9; }
      if (!(fGlblMD5Arc = fopen(szMD5ArcName  , "w"))) { fprintf(stderr, "error: unable to write: %s\n", szMD5ArcName  ); return 9; }
      if (!(fGlblBatch  = fopen(szBatchName   , "w"))) { fprintf(stderr, "error: unable to write: %s\n", szBatchName   ); return 9; }
      if (!(fGlblMD5Org = fopen(szMD5OrgName  , "w"))) { fprintf(stderr, "error: unable to write: %s\n", szMD5OrgName  ); return 9; }
      #ifdef SFK_USE_DIR_TIMES
      if (!(fGlblTimes  = fopen(szTimesName   , "w"))) { fprintf(stderr, "error: unable to write: %s\n", szTimesName   ); return 9; }
      #endif

      lRC = walkAllTrees(eFunc_Mirror, lFiles, lDirs, nBytes);

      fclose(fGlblCont);
      fclose(fGlblMD5Arc);
      fclose(fGlblBatch );
      fclose(fGlblMD5Org);
      #ifdef SFK_USE_DIR_TIMES
      fclose(fGlblTimes );
      #endif

      sprintf(szBatchName, "%s%s", pszGlblDstRoot, pszVerArcBatch);
      if (!(fGlblBatch = fopen(szBatchName   , "w"))) { fprintf(stderr, "error: unable to write: %s\n", szBatchName   ); return 9; }
      fprintf(fGlblBatch, "%s%csfk md5check=%s %%1 %%2 %%3\n", pszToolsDir, glblPathChar, pszMD5ArcFile);
      fclose(fGlblBatch);

      sprintf(szBatchName, "%s%s", pszGlblDstRoot, pszVerOrgBatch);
      if (!(fGlblBatch = fopen(szBatchName   , "w"))) { fprintf(stderr, "error: unable to write: %s\n", szBatchName   ); return 9; }
      fprintf(fGlblBatch, "%s%csfk md5check=%s %%1 %%2 %%3\n", pszToolsDir, glblPathChar, pszMD5OrgFile);
      fclose(fGlblBatch);

      if (bGlblEscape)
         logError("error: user interrupt. archive tree is INCOMPLETE.");

      if (glblErrorLog.numberOfEntries() > 0) {
         long nErr = glblErrorLog.numberOfEntries();
         printf("=== %d errors occured: ===\n", nErr);
         for (long i=0; i<nErr; i++)
            printf("%s\n", glblErrorLog.getEntry(i, __LINE__));
      } else {
         printf("=== no errors registered. ===\n");
      }

      sprintf(szLineBuf2, "%s09-freeze-log.txt", pszGlblDstRoot);
      printf("=== content of logfile %s, without empty file warnings: ===\n", szLineBuf2);
      FILE *fin = fopen(szLineBuf2, "r");
      if (fin) {
         while (fgets(szLineBuf, sizeof(szLineBuf)-10, fin)) {
            removeCRLF(szLineBuf);
            if (!strstr(szLineBuf, "01-arc-part.zip not found or empty"))
               printf("%s\n", szLineBuf);
         }
         fclose(fin);
      }
      printf("=== end of logfile content. ===\n");

      printf("done: scanned %lu files in %lu sec (%lu kb/sec)\n",
         glblFileCount.value(), currentElapsedMSec()/1000, currentKBPerSec());
      printf("info: %lu files are frozen within %lu zip archives.\n", nGlblFzConArcFiles, nGlblFzConArchives);
      printf("info: %lu files have a copy in the target tree.\n", nGlblFzConCopFiles);

      if (glblErrorLog.numberOfEntries() > 0)
         printf("warn: there are %lu error messages. read error logs above.\n", glblErrorLog.numberOfEntries());

      printf("note: check your most important files now manually,\n");
      printf("note: by listing some archives within %s.\n", pszGlblDstRoot);

      if (nGlblFzMisArcFiles > 0)
         printf("warn: %lu files failed archiving (read error logs above).\n", nGlblFzMisArcFiles);

      if (nGlblFzMisCopFiles > 0)
         printf("warn: %lu files failed to copy (read error logs above).\n", nGlblFzMisCopFiles);

      // cleanup
      if (pszGlblXCopyCmd) delete [] pszGlblXCopyCmd;
      if (pszGlblZipCmd  ) delete [] pszGlblZipCmd  ;
      if (pszGlblUnzipCmd) delete [] pszGlblUnzipCmd;
      if (pszGlblSFKCmd  ) delete [] pszGlblSFKCmd  ;

      if (glblErrorLog.numberOfEntries() > 0)
         return 5;

      bDone = 1;
   }

   if (!strcmp(pszCmd, "patch")) 
   {
      lRC = patchMain(argc-1, argv, 1);
      bDone = 1;
   }

   if (!strcmp(pszCmd, "run"))
   {
      if (argc < 4) 
      {
         char c1 = glblRunChar;

         printf(
             "sfk run \"your command %cpfile [%cprfile] [...]\" [-quiet] [-sim] [...]\n"
             "\n"
             "    run a self-defined command on every file- or directory name.\n"
             "    within your command string, you may specify:\n"
             "\n", c1, c1
             );

         printf(
             "       %cquotfile    or %cqfile  - insert filename with path and \"\" quotes.\n"
             "       %cpurefile    or %cpfile  - insert filename with path and NO quotes.\n"
             "       %cquotrelfile or %cqrfile - insert relative filename without path.\n"
             "       %cquotbase    or %cqbase  - the relative base filename, without extension.\n"
             "       %cquotext     or %cqext   - filename extension. foo.bar.txt has extension .txt.\n"
             "       %cquotpath    or %cqpath  - the path (directory) without filename.\n",
             c1, c1, c1, c1, c1, c1, c1, c1, c1, c1, c1, c1
             );

         printf(
             "       also valid: %cpurerelfile, %cprfile, %cpurebase, %cpbase, %cpureext, %cpext,\n"
             "                   %cpurepath, %cppath.\n\n", c1, c1, c1, c1, c1, c1, c1, c1
             );

         printf(
             "    if you supply only path expressions, only directories will be processed.\n"
             "    option -ifiles allows processing of a filename  list from stdin.\n"
             "    option -idirs  allows processing of a directory list from stdin.\n"
             "    on stdin, '#' remark lines and empty lines are skipped.\n"
             "    note that \"sfk.exe <list.txt\" supports max. 4 KB for list.txt under windows.\n"
             "    note that \"type list.txt | sfk.exe\" supports unlimited stream length.\n"
             "\n"
             "    NOTE: always think a moment 1) if your filenames may contain BLANKS, and\n"
             "          2) if and where to use quoted or non-quoted (pure) expressions.\n"
             "          if you combine things, you may often have to use 'p'ure forms\n"
             "          embraced by slightly complicated \\\" quotes like this:\n"
             );

         printf(
             "             sfk run \"ren \\\"%cpbase.%cpext\\\" \\\"%cpbase-old.%cpext\\\"\" testfiles\n"
             "          in any way, have a -sim run first to simulate what may happen.\n"
             "\n"
             "       sfk run \"attrib -R %cqfile\" -quiet testfiles\\FooBank\\BarDriver\n"
             "       sfk run \"<img src=%cquotfile>\" -dir . -file .jpg -sim >index.html\n"
             "       sfk run \"xcopy %cqpath \\\"G:\\%cppath\\\" /S /H /I /R /K /Y /D\" -dir sources\n"
             "       type dirlist.txt | sfk run -idirs \"xcopy \\\"x:\\%cppath\\\" \\\"z:\\%cppath\\\" /I /D\"\n",
             c1, c1, c1, c1, c1, c1, c1, c1, c1, c1
             );
         return 0;
      }
      int iDir = 2;
      while (iDir < argc)
      {
         if (!strcmp(argv[iDir], "-ifiles")) {
            if (bGlblStdInDirs) { fprintf(stderr, "error: cannot use -ifiles and -idirs together.\n"); return 9; }
            bGlblStdInFiles = 1;
            iDir++;
         }
         else
         if (!strcmp(argv[iDir], "-idirs"))  {
            if (bGlblStdInFiles) { fprintf(stderr, "error: cannot use -ifiles and -idirs together.\n"); return 9; }
            if (anyFileInRunCmd(pszGlblRunCmd)) { fprintf(stderr, "error: -idirs only allowed with #ppath, not with file commands.\n"); return 9; }
            bGlblStdInDirs    = 1;
            bGlblWalkJustDirs = 1;
            iDir++;
         }
         else
         if (!strncmp(argv[iDir], "-", 1)) {
            break; // fall through to option processing
         }
         else
         if (!pszGlblRunCmd[0])
         {
            pszGlblRunCmd = argv[iDir];
            iDir++;
         }
         else
            break; // fall through to short dir processing
      }
      processDirParms(pszCmd, argc, argv, iDir, 1);
      if (bGlblStdInAny) { fprintf(stderr, "error: sfk run: -i not allowed, use -ifiles or -idirs.\n"); return 9; }
      if (bGlblStdInFiles || bGlblStdInDirs) {
         lRC = walkStdInListFlat(eFunc_Run, lFiles, nBytes);
      } else {
         if (!anyFileInRunCmd(pszGlblRunCmd))
            bGlblWalkJustDirs = true;
         lRC = walkAllTrees(eFunc_Run, lFiles, lDirs, nBytes);
      }
      bDone = 1;
   }

   #ifdef WITH_FN_INST
   if (!strcmp(pszCmd, "inst"))
   {
      if (argc < 4) {
      printf("usage:\n"
             "    sfk inst [-revoke] [-redo] [-keep-dates] mtkinc mtkmac -dir ...\n"
             "       mtkinc: path and name of mtktrace.hpp file\n"
             "       mtkmac: mtk block entry macro name, _mtkb_\n"
             "       revoke: undo all changes (copy backups back)\n"
             "       keep-dates: on revoke, also reactivate original file dates\n"
             "       redo  : redo all changes\n"
             "          sfk inst mtk/mtktrace.hpp _mtkb_ -dir . !save_ -file .cpp\n"
             "\n"
             "    read more about the sfk micro tracing kernel in the mtk/ dir.\n"
            );
            return 9;
      }

      int iDir = 2;
      while ((iDir < argc) && !strncmp(argv[iDir], "-", 1))
      {
         // inst-specific prefix options
         if (!strcmp(argv[iDir], "-revoke")) {
            bGlblInstRevoke = 1;
            iDir++; 
         }
         else
         if (!strcmp(argv[iDir], "-redo")) {
            bGlblInstRevoke = 1;
            bGlblInstRedo   = 1;
            iDir++; 
         }
         else
         if (!strcmp(argv[iDir], "-keep-dates")) {
            bGlblTouchOnRevoke = 0;
            iDir++; 
         }
         else
            break; // other option, fall through
      }
      if (!bGlblInstRevoke || bGlblInstRedo) {
         // no revoke: expect further parms
         if (iDir >= argc-1) { fprintf(stderr, "error  : inst: missing parms. supply include and macro.\n"); return 9; }
         pszGlblInstInc = argv[iDir++];
         pszGlblInstMac = argv[iDir++];
      }
      processDirParms(pszCmd, argc, argv, iDir, 1);

      lRC = walkAllTrees(eFunc_Inst, lFiles, lDirs, nBytes);

      bDone = 1;
   }
   #endif

   if (!strcmp(pszCmd, "strings")) 
   {
      // extract strings from single binary file
      if (checkArgCnt(argc, 3)) return 9;

      char *pszFile = argv[2];
      FILE *fin = fopen(pszFile, "rb");
      if (!fin) { fprintf(stderr, "error: cannot read: %s\n", pszFile); return 9; }
      long icol   = 0;
      long istate = 0;
      long iword  = 0;
      long ihi    = 0;
      bool bflush = 0;
      bool bisws  = 0;
      bool bwasws = 0;
      bool bisbin = 0;
      bool bishi  = 0;
      bool bispunct = 0;
      char c = 0;
      unsigned char uc = 0;
      while (1) 
      {
         long nRead = fread(szLineBuf, 1, sizeof(szLineBuf)-10, fin);
         if (nRead <= 0)
            break;
         for (long i=0; i<nRead; i++)
         {
            c  = szLineBuf[i];
            uc = (unsigned char)c;
            bflush = 0;

            if (c=='\r'||c=='\n')
               c = ' ';

            bisbin = ((uc >= 0x80) && (uc < 0xC0));
            bishi  =  (uc >= 0xC0);

            if (isprint(c) && !bisbin) {
               // printable char
               if (istate == 1) {
                  istate = 0;
                  // start collecting next word
                  iword = 0;
                  ihi   = 0;
               }
               // continue collecting current word,
               // reduce multi-whitespace sequences.
               bisws    = (c==' ' || c=='\t');
               bispunct = (c=='.' || c==',' || c==';');
               if (!(bisws && bwasws)) {
                  szLineBuf2[iword++] = c;
                  if (bishi)
                     ihi++;
               }
               // hard or soft word break?
               if (iword >= 80)
                  bflush = 1;
               else
               if ((iword >= 20) && (bisws || bispunct))
                  bflush = 1;
            } else {
               // non-printable (binary) char
               if (istate == 0)
                  bflush = 1;
               istate = 1;
               bwasws = 0;
               bisws  = 0;
               bispunct = 0;
            }
            // dump current word?
            if (bflush) {
               bflush = 0;
               if (iword > 2)
               {
                  if (bisws || bispunct)
                     printf("%.*s", (int)iword, szLineBuf2);
                  else
                     printf("%.*s ", (int)iword, szLineBuf2);
                  icol += (iword+1);
                  if (icol >= 80) {
                     fputc('\n', stdout);
                     icol = 0;
                  }
               }
               iword  = 0;
               ihi    = 0;
               bwasws = 0;
            }
            else
               bwasws = bisws;
         }
      }
      fclose(fin);
      bDone = 1;
   }

   if (!strcmp(pszCmd, "pathfind") || !strcmp(pszCmd, "where"))
   {
      // find out exact location of a command in the path
      if (checkArgCnt(argc, 3)) return 9;

      int iDir = 2;
      char *pszCmd = 0;

      while (iDir < argc) {
         if (!strcmp(argv[iDir], "-quiet"))
            bGlblQuiet = 1;
         else
         if (!strcmp(argv[iDir], "-debug"))
            bGlblDebug = 1;
         else
            pszCmd = argv[iDir];
         iDir++;
      }
      if (!pszCmd) { fprintf(stderr, "error: supply a command name to search within PATH.\n"); return 9; }

      char *pszAbs = findPathLocation(pszCmd);
      if (!pszAbs) {
         if (!bGlblQuiet) {
            printf("%s not found anywhere within PATH.\n", pszCmd);
            printf("... try also .exe, .bat, .cmd extensions.\n");
         }
         return 1;
      } else {
         if (!bGlblQuiet) {
            printf("%s\n", pszAbs);
         }
         return 0;
      }
   }

   if (!strcmp(pszCmd, "deblank"))
   {
      // sfk deblank -yes
      int iDir = 2;      
      processDirParms(pszCmd, argc, argv, iDir, 1);
      if (!bGlblYes) printf("] file rename preview:\n");
      lRC = walkAllTrees(eFunc_Deblank, lFiles, lDirs, nBytes);
      if (!bGlblYes) printf("] add -yes to execute.\n");
      bDone = 1;
   }

   if (!strcmp(pszCmd, "reflist"))
   {
      if (argc <= 2) {
      printf("sfk reflist [-abs] [-wide] -dir tdir -file .text -dir sdir -file .sext\n"
             "   find references to target files (tdir) within the source file set (sdir).\n"
             "   a list of target files is created. then all sources are loaded and checked\n"
             "   if the names of the targets appear within the source files. then each target\n"
             "   with it's reference count is listed, together with up to 10 (or 20) infos\n"
             "   about the sources. by default, this compares RELATIVE filenames WITHOUT path.\n"
             "   NOTE that sfk reflist DOES NOT GUARANTEE that files are (not) referenced.\n"
             "   This way of reference detection is just a FUZZY INDICATOR, so keep thinking\n"
             "   and make backups before you massively cleanup files without references!\n"
             "   -wide: show highly detailed infos for better analysis.\n"
             "   -abs : compare absolute filenames with path information.\n"
             "   -base: compare just basenames without path and extension.\n"
             "      sfk reflist -dir pic -file .png -dir movie -file .ppt\n"
            );
      return 0;
      }

      int iDir = 2;
      while (iDir < argc) {
         if (!strcmp(argv[iDir], "-abs")) {
            bGlblRefRelCmp = 0;
            iDir++; 
         }
         else
         if (!strcmp(argv[iDir], "-wide")) {
            bGlblRefWideInfo = 1;
            nGlblRefMaxSrc   = 20;
            iDir++; 
         }
         else
         if (!strcmp(argv[iDir], "-base")) {
            bGlblRefBaseCmp = 1;
            iDir++; 
         }
         else
            break;
      }
      processDirParms(pszCmd, argc, argv, iDir, 1);

      // check input: only two dir roots allowed
      if (glblFileSet.numberOfRootDirs() != 2) {
         fprintf(stderr, "error: need exactly 2 root dirs, for targets and sources.\n");
         return 9;
      }

      // tree layer 0: targets
      // tree layer 1: sources
      glblRefDst.addRow(__LINE__); // for dst file names
      glblRefDst.addRow(__LINE__); // for dst ref counts
      for (long i=0; i<nGlblRefMaxSrc; i++) {
         glblRefDst.addRow(__LINE__); // for source infos
      }
      long rlFiles=0, rlDirs=0;
      FileList oDirFiles;
      num  rlBytes = 0, nLocalMaxTime = 0, nTreeMaxTime  = 0;

      // collect list of targets
      nGlblFunc = eFunc_RefColDst;
      char *pszTree = glblFileSet.setCurrentRoot(0);
      if (bGlblDebug) printf("] dst: %s\n", pszTree);
      if (walkFiles(pszTree, 0, rlFiles, oDirFiles, rlDirs, rlBytes, nLocalMaxTime, nTreeMaxTime)) {
         bGlblError = 1;
         return 9;
      }
      if (!bGlblQuiet)
         printf("%.60s\r", pszGlblBlank);
      printf("%05u potential target files\n", glblRefDst.numberOfEntries(0));

      // process potential sources
      nGlblFunc = eFunc_RefProcSrc;
      pszTree = glblFileSet.setCurrentRoot(1);
      if (bGlblDebug) printf("] src: %s\n", pszTree);
      if (walkFiles(pszTree, 0, rlFiles, oDirFiles, rlDirs, rlBytes, nLocalMaxTime, nTreeMaxTime)) {
         bGlblError = 1;
         return 9;
      }
      if (!bGlblQuiet)
         printf("%.60s\r", pszGlblBlank);
      printf("%05u potential source files\n", nGlblRefSrcCnt);

      // dump reference list
      if (!bGlblEscape)
      for (long idst=0; idst<glblRefDst.numberOfEntries(0); idst++)
      {
         char *pszDst  = glblRefDst.getString(0, idst);
         long  nRefCnt = glblRefDst.getLong(1, idst, __LINE__);
         if (nRefCnt > 0) {
            printf("%05u %s       ", (unsigned long)nRefCnt, pszDst);
            if ((nRefCnt > 0) && bGlblRefWideInfo)
               printf("\n      +--- ");
            else
               printf("<- ");
            long nDump = nRefCnt;
            if (nDump > nGlblRefMaxSrc) nDump = nGlblRefMaxSrc;
            for (long i=0; i<nDump; i++) {
               char *pszSrc  = glblRefDst.getString(2+i, idst);
               printf("%s ", pszSrc);
               if (bGlblRefWideInfo && (i<nDump-1))
                  printf("\n      +--- ");
            }
            printf("\n");
         } else {
            printf("%05u %s\n", (unsigned long)nRefCnt, pszDst);
         }
      }
      // cleanup
      glblRefDst.reset();
      bDone = 1;
   }

   #ifdef WITH_TCP
   if (!strcmp(pszCmd, "ftpserv"))
   {
      if (argc >= 3 && !strncmp(argv[2], "-h", 2)) {
      printf(
         "sfk ftpserv [-h[elp]] [-port=nport] [-rw] [-maxsize=n]\n"
         "\n"
         "   creates an instant ftp server to enable easy file transfer.\n"
         "   * the CURRENT DIRECTORY is made accessible, without subdirs.\n"
         "   * any kind of directory traversal (.., / etc.) is blocked.\n"
         "   * just ONE CLIENT (browser etc.) can connect at a time.\n"
         "   * after 15 seconds of inactivity, the connection is closed.\n"
         "   port: use other port than default, e.g. -port=30199.\n"
         "   rw  : allow read+write access. default is readonly.\n"
         "   maxsize: increment size limit per file write to n mbytes.\n"
         "\n"
         "   NOTE: be aware that ANYONE may connect to your server.\n"
         "         with -rw specified, ANYONE may also write large files.\n"
         "         if this is a problem, do NOT use sfk ftpserv, but download\n"
         "         and install a full-scale ftp server like filezilla.\n"
         "\n"
         "   if you login to the server using a regular ftp client, but you cannot\n"
         "   transfer any files, it's usually a firewall vs. ftp protocol problem.\n"
         "   in this case, the sfk ftp client may help. type \"sfk ftp\" for info.\n"
         );
         return 0;
      }

      {
         int   iDir    =   2;
         ulong nPort   =  21;
         long  nState  =   0;
         bool  bRW     =   0;
         char *pszPath = ".";
         for (; iDir < argc; iDir++) {
            if (!strncmp(argv[iDir], "-port=", strlen("-port="))) {
               nPort  = atol(argv[iDir]+strlen("-port="));
               nState = 1;
            }
            else
            if (!strcmp(argv[iDir], "-rw")) {
               bRW    = 1;
               nState = 3;
            }
            else
            if (!strncmp(argv[iDir], "-maxsize=", strlen("-maxsize="))) {
               nGlblTCPMaxSizeMB = atol(argv[iDir]+strlen("-maxsize="));
               nState = 2;
            }
            else {
               pszPath = argv[iDir];
               break;
            }
         }
         if (!pszPath) { fprintf(stderr, "error: missing path.\n"); return 9; }
         // ftp server may act on current dir only.
         if (glblFileSet.beginLayer(false)) return 9;
         glblFileSet.addRootDir(pszPath, __LINE__, false);
         glblFileSet.autoCompleteFileMasks();
         glblFileSet.setBaseLayer();
         bGlblRecurseDirs = 0; // in case of list command
         ftpServ(nPort, bRW, pszPath);
         bDone = 1;
      }
   }

   if (!strcmp(pszCmd, "ftp"))
   {
      if (argc < 3) {
      printf(
         "sfk ftp host[:port] put|get filename\n"
         "\n"
         "   simple anonymous ftp client. if connected to sfk server,\n"
         "   this uses sfk/sft protocol, requiring fewer connections.\n"
         "\n"
         "      sfk ftp farpc put test.zip\n"
         "         send test.zip to farpc\n"
         "\n"
         "      sfk ftp 192.168.1.99:30199 get test.zip\n"
         "         receive test.zip from 192.168.1.99 port 30199\n"
         "\n"
         "      sfk ftp hostname\n"
         "         enter interactive mode, supporting commands:\n"
         "            dir, get filename, put filename.\n"
         );
         return 0;
      }

      {
         // client has no size limit, user should know what he's doing.
         nGlblTCPMaxSizeMB = 0;

         ulong nPort = 21;
         char *pszHost = strdup(argv[2]);
         char *psz = strchr(pszHost, ':');
         if (psz) {
            *psz++ = '\0';
            nPort = atol(psz);
         }
         if (argc >= 5) {
            char *pszCmd = argv[3];
            char *pszFileName = argv[4];
            sprintf(szLineBuf2, "%s %s", pszCmd, pszFileName);
            ftpClient(pszHost, nPort, szLineBuf2);
         } else {
            ftpClient(pszHost, nPort, 0);
         }
         delete [] pszHost;
         bDone = 1;
      }
   }
   #endif

   if (!bDone)
      fprintf(stderr, "error: unknown command: %s\n", pszCmd);

   // cleanup for all commands
   glblMemSnap.shutdown();
   glblFileSnap.shutdown();
   glblFileSet.reset();
   if (pGlblFileParms)
      delete pGlblFileParms;
   if (apGlblFileParms)
      delete [] apGlblFileParms;
   if (pszGlblJamRoot)
      delete [] pszGlblJamRoot;
   if (pszGlblDstRoot)
      delete [] pszGlblDstRoot;
   if (pszGlblJamPrefix)
      delete [] pszGlblJamPrefix;
   glblGrepPat.reset();
   if (pszGlblDirTimes)
      delete [] pszGlblDirTimes;

   #ifdef SFK_WINPOPUP_SUPPORT
   winCleanupGUI();
   #endif

   #ifdef _MSC_VER
   if (!bFatal && !bGlblError)
      listMemoryLeaks();
   #endif

   return lRC;
}
