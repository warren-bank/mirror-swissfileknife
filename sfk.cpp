/*
   swiss file knife

   known issues:
   -  syncto: no empty target files supported yet
   -  option handling is not very flexible yet,
      often it's unclear where to specify an option.
   -  patch, inst: option -keep-dates only works
      with Win32 yet (uses xcopy command).
   -  mirrorto: zip called sometimes without any need
      to update the archive, due to file time check.
   -  whole internal time handling not yet 64 bit based.

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
#else
  #include <unistd.h>
  #include <dirent.h>
  // #include <ndir.h>
  // #include <sys/ndir.h>
  // #include <sys/dir.h>
  // #define dirent direct
#endif

#define uchar unsigned char
#define ulong unsigned long
#define bool  unsigned char

#define WITH_FN_INST

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
#else
static const char  glblPathChar    = '/';
static const char  glblWrongPChar  = '\\';
static const char *glblPathStr     = "/";
static const char *glblAddWildCard = "";
static const char *glblDotSlash    = "./";
#endif

// ========== 64 bit abstraction layer begin ==========

#ifdef _WIN32
 typedef __int64 num;
#else
 typedef long long num;
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

// ========== 64 bit abstraction layer end ============

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
};

enum eConvTargetFormats {
   eConvFormat_LF     = 1,
   eConvFormat_CRLF   = 2
};

long bGlblDebug = 0;
long nGlblFunc  = 0;
long nGlblFiles = 0;
long nGlblLines = 0;
long nGlblTabSize  = 0;
long nGlblTabsDone = 0;
long nGlblTabFiles = 0;
long bGlblScanTabs = 0;
char *pszGlblOutFile = 0;  // if set, some funcs will take care not to read this file
FILE *fGlblOut    = 0; // general use
FILE *fGlblCont   = 0; // mirrorto: content list
FILE *fGlblMD5Arc = 0;
FILE *fGlblMD5Org = 0;
FILE *fGlblBatch  = 0;
FILE *fGlblTimes  = 0;
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
char *pszBlank =
   "                                                "
   "                                                ";
num  nGlblStartTime = 0;
long nGlblListMinSize = 0; // in mbytes
long nGlblListMode = 0; // 1==stat 2==list
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
char *pszGlblSnapFileStamp    = ":snapfile sfk,1.0,prefix=:";
char *pszGlblClusterFileStamp = ":cluster sfk,1.0,prefix=:";
bool  bGlblRefRelCmp    = 1;
bool  bGlblRefBaseCmp   = 0;
bool  bGlblRefWideInfo  = 0;
long  nGlblRefMaxSrc    = 10;
bool  bGlblStdInAny     = 0;  // all cmd except run: take list from stdin
bool  bGlblStdInFiles   = 0;  // run only: take filename list from stdin
bool  bGlblStdInDirs    = 0;  // run only: take directory list from stdin
long  nGlblMD5Skip      = 0;

#define _ { printf("[%d]\n",__LINE__); fflush(stdout); }

long execDetab       (char *pszFile);
long execDirMirror   (char *pszName, long  lLevel, num &ntime1, num &ntime2);
long execFileMirror  (char *pszFile, num &ntime1, num &ntime2);
long execDirXCopy    (char *pszName, long  lLevel, num &ntime1, num &ntime2);
long execFileXCopy   (char *pszFile, num &ntime1, num &ntime2);
long execDirFreeze   (char *pszName, long  lLevel, num &ntime1, num &ntime2);
long execFileFreeze  (char *pszFile, num &ntime1, num &ntime2);
long getFileMD5      (char *pszFile, SFKMD5 &md5, bool bSilent=0);
long execFormConv    (char *pszFile);
long walkFiles       (char *pszIn  , long lLevel, long &lFiles, long &lDirs, num &lBytes, num &ntime1, num &ntime2);
long execSingleFile  (char *pszFile, long lLevel, long &lFiles, long &lDirs, num &lBytes, num &ntime1, num &ntime2);
long execSingleDir   (char *pszName, long lLevel, long &lFiles, long &lDirs, num &lBytes, num &ntime1, num &ntime2);
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
   printf("] %.*sstringtable %p, %d entries:\n",nIndent,pszBlank,this,nClArrayUsed);
   for (long i=0; i<nClArrayUsed; i++) {
      printf("]   %.*s%s\n", nIndent,pszBlank,apClArray[i]);
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
   ultoa(uValue, szBuf, 16);
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
   ultoa(uValue, szBuf, 16);
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
   ultoa(uValue, szBuf, 16);
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

   // a "command" is "-copy" or "-zip", used only with mirrorto.
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
   if (!strcmp(psz1, "-debug"))  { bGlblDebug = 1; return true; }
   if (!strcmp(psz1, "-quiet"))  { bGlblQuiet = 1; return true; }
   if (!strcmp(psz1, "-sim"))    { bGlblSim = 1; return true; }
   if (!strcmp(psz1, "-norec"))  { bGlblRecurseDirs = 0; return true; }
   if (!strcmp(psz1, "-i"))      { bGlblStdInAny = 1; return true; }
   return false;
}

long processDirParms(char *pszCmd, int argc, char *argv[], int iDir, bool bAutoCompleteMasks)
{
   long checkMask(char *pszMask);
   char *loadFile(char *pszFile, int nLine);

   if (iDir < argc)
   {
      char *pszFirstParm = argv[iDir];

      if (!strcmp(pszFirstParm, "-debug")) {
         bGlblDebug   = 1;
         printf("[debug activated]\n");
         pszFirstParm = (iDir < argc-1) ? argv[++iDir] : (char*)"";
      }

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

      // fetch prefix options, if any
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

            if (psz1[0] == '!') {
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
               if (!strncmp(pszCmd, "mirrorto=", strlen("mirrorto=")))
                  { fprintf(stderr, "error: no -file masks supported with mirrorto command.\n"); return 9; }
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

// just for sfk run: process text list with file- OR dirnames
long walkStdInListFlat(long nFunc, long &rlFiles, num &rlBytes)
{
   if (bGlblError)
      return 9;

   nGlblFunc = nFunc;

   long lDirs = 0;
   num nLocalMaxTime = 0, nTreeMaxTime  = 0;

   while (fgets(szLineBuf, sizeof(szLineBuf)-10, stdin))
   {
      removeCRLF(szLineBuf);

      // skip remark and empty lines
      if (szLineBuf[0] == '#') continue;
      if (!strlen(szLineBuf))  continue;

      if (bGlblStdInFiles)
      {
         char *pszSub = strdup(szLineBuf);
         long lRC = execSingleFile(pszSub, 0, rlFiles, lDirs, rlBytes,
                                   nLocalMaxTime, nTreeMaxTime);
         delete [] pszSub;
         rlFiles++;
         if (lRC >= 9)
            return lRC;
      }

      if (bGlblStdInDirs)
      {
         char *pszSub = strdup(szLineBuf);
         long rlDirs = 0;
         num  nLocalBytes = 0;
         long lRC = execSingleDir(pszSub, 0, rlFiles, rlDirs, nLocalBytes,
                                  nLocalMaxTime, nTreeMaxTime);
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
      num  nLocalBytes = 0, ntime1 = 0, ntime2 = 0;

      char *pszSub = strdup(szLineBuf);
      if (bGlblDebug) printf("] siw: %s\n", pszSub);
      long lRC = walkFiles(pszSub, 0, rlFiles, lDirs, nLocalBytes, ntime1, ntime2);
      if (!lRC && isDir(pszSub))
      {
         // if this succeeded, treat the tree as another dir.
         if (bGlblDebug) printf("] sid: %s\n", pszSub);
         if (execSingleDir(pszSub, 0, rlFiles, lDirs, nLocalBytes, ntime1, ntime2))
         {
            bGlblError = 1;
            delete [] pszSub;
            return 9;
         }
      }
      rlBytes += nLocalBytes;
      delete [] pszSub;

      if (lRC >= 9)
         return lRC;
   }

   return 0;
}

long walkAllTrees(long nFunc, long &rlFiles, long &rlDirs, num &rlBytes) 
{
   if (bGlblError)
      return 9;

   if (bGlblStdInAny)
      return walkStdInListDeep(nFunc, rlFiles, rlBytes);

   nGlblFunc = nFunc;

   num nLocalMaxTime = 0, nTreeMaxTime  = 0;

   for (long nDir=0; glblFileSet.hasRoot(nDir); nDir++)
   {
      // process one of the trees given at commandline.
      char *pszTree = glblFileSet.setCurrentRoot(nDir);
      if (bGlblDebug) printf("] wtr: %s\n", pszTree);
      if (walkFiles(pszTree, 0, rlFiles, rlDirs, rlBytes, nLocalMaxTime, nTreeMaxTime)) {
         bGlblError = 1;
         return 9;
      } else {
         // if this succeeded, treat the tree as another dir.
         if (bGlblDebug) printf("] esr: %s\n", pszTree);
         if (execSingleDir(pszTree, 0, rlFiles, rlDirs, rlBytes, nLocalMaxTime, nTreeMaxTime)) {
            bGlblError = 1;
            return 9;
         }
      }
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

num getFileTime(char *pszName)
{
   struct _stat buf;
   if (_stat(pszName, &buf))
      return 0;
   // return time of last change
   return (num)buf.st_mtime; // atime only on NTFS
}

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
   struct _stat buf;
   if (_stat(pszName, &buf)) {
      return -1;
   }
   rbIsDirectory = (buf.st_mode & _S_IFDIR ) ? 1 : 0;
   #ifdef _MSC_VER
   rbCanRead     = (buf.st_mode & _S_IREAD ) ? 1 : 0;
   rbCanWrite    = (buf.st_mode & _S_IWRITE) ? 1 : 0;
   #endif
   rlFileTime    =  buf.st_mtime;
   rlFileSize    =  buf.st_size;
   return 0;
}

num getFileSize(char *pszName) 
{
   long bIsDir    = 0;
   long bCanRead  = 1;
   long bCanWrite = 1;
   num  lFileTime = 0;
   num  nFileSize = 0;
   if (getFileStat(pszName, bIsDir, bCanRead, bCanWrite, lFileTime, nFileSize))
      return -1;
   return nFileSize;
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
   return 0;
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
   static bool bTold = 0;
   if (getKeyPress() == VK_ESCAPE) {
      if (!bTold) {
         bTold = 1;
         printf("[aborted by user]                   \n");
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
   pszGlblSnapFileStamp,
   ":snapfile ", ":cluster ", ":create ", ":done ",
   ":file: ", ":skip-begin ", ":skip-end ",

   // these keys may not be as relevant to clusters.
   // nevertheless, by default, files with them are also skipped.
   ":patch ", ":info ", ":root ", 
   ":file ", ":from ", ":to ",
   ":mkdir ", ":select-replace ", ":set ",
   ":# ", ":edit ", ":READ ", 

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

char *loadFile(char *pszFile, int nLine) {
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
   SnapShot ( );
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
   void  resetTargets      ( );
   void  resetNotes        ( );
   TextFile **apClTargets;
   long  nClMaxTargets;
   long  nClTargets;
   char  *pClFileName;
   char  *pClRootName;
   long  nClNamePadding;
   long  lClLastSavedBuild;
   char  *apLastSync[MAX_SYNC_INFO];
   char  *apNotes[MAX_NOTES_LINES+10]; // used only in FileSnap
   long  nClNotes;
};

SnapShot glblMemSnap;
SnapShot glblFileSnap;

SnapShot::SnapShot() {
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
   resetTargets();
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
      fprintf(stderr, "error: target number differs (%d,%d)\n", oMaster.nClTargets, nClTargets);
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
   resetTargets();

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

void SnapShot::resetTargets() {
   if (apClTargets) {
      for (long i=0; i<nClTargets; i++)
         delete apClTargets[i];
      delete [] apClTargets;
      apClTargets   = 0;
      nClMaxTargets = 0;
      nClTargets    = 0;
   }
}

long SnapShot::copyTargetsFrom(SnapShot &oSrc) {
   resetTargets();
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
   fprintf(fout, ":notes-begin\n");
   for (long i0=0; i0<nClNotes; i0++) {
      fprintf(fout, "%s\n", apNotes[i0]);
   }
   fprintf(fout, ":notes-end\n\n");

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
   fclose(fout);
   return 0;
}

long SnapShot::readFromFile(long &rbDroppedAny, long bForceBuildMatch) 
{
   resetTargets();
   resetNotes();

   if (!pClFileName) { fprintf(stderr, "error: missing filename, or root\n"); return 9; }

   // load snapfile in one block
   char *pszRaw = loadFile(pClFileName, __LINE__);
   if (!pszRaw) return 9;

   // pass 1: parse control block, build target index
   char *psz1 = pszRaw;

   long nIndexSize = 0;
   char **apIndex  = 0;
   long nIndexUsed = 0;

   rbDroppedAny = 0;
   bool bWithinNotes = 0;

   while (psz1)
   {
      // if (bGlblDebug) printf("] \"%.10s\"\n", psz1);

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
         if (!strncmp(szLineBuf, ":cluster ", strlen(":cluster "))) { 
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
            // if (bGlblDebug) printf("added index entry\n");
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
            printf("[ DROP : %s ] %.20s\n", pszTargetName, pszBlank);
            // at mem snapshot as well
            // NOTE: if user forgot to reload snapfile, target
            //       was already dropped before at mem-snap.
            if (glblMemSnap.hasTarget(pszTargetName))
               glblMemSnap.dropTarget(pszTargetName);
            else
               printf("[ WARN : reload the cluster! ] %.20s\n", pszTargetName, pszBlank);
            rbDroppedAny = 1;
         }

         // continue in snapfile
         psz1 = pszContinue;
      } else {
         // if (strlen(psz1) > 10) {
         //    fprintf(stderr, "error: unexpected content in %s:\n", pClFileName);
         //    fprintf(stderr, "\"%.100s\"\n", psz1);
         //    return 9;
         // }
         psz1 = 0;
      }
   }

   // pass 3: add all files of index not yet in snapfile
   for (long i5=0; i5<nIndexUsed; i5++) {
      if (strlen(&apIndex[i5][lPreFix])) {
         char *pszTargetName = &apIndex[i5][lPreFix];
         if (fileExists(pszTargetName)) {
            // create and load to file-snap
            TextFile *pTarget = new TextFile(pszTargetName);
            if (pTarget->loadFromFile()) {
               // loading failed, e.g. due to keywords contained
               printf("[ SKIP : %s ] %.20s\n", pszTargetName, pszBlank);
               delete pTarget;
            } else {
               // loading ok: add to file snapshot (this)
               printf("[ ADD  : %s ] %.20s\n", pszTargetName, pszBlank);
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

      if (pszMask[0] == '!') {
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
      if (pszMask[0] == '!') {
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

long execFileStat(char *pszFileName, long lLevel, long &lFiles, long &lDirs, num &lBytes) 
{
   long bIsDir    = 0;
   long bCanRead  = 1;
   long bCanWrite = 1;
   num  lFileTime = 0;
   num  nFileSize = 0;
   getFileStat(pszFileName, bIsDir, bCanRead, bCanWrite, lFileTime, nFileSize);

   if (!strncmp(pszFileName, glblDotSlash, 2))
      pszFileName += 2;

   switch (nGlblListMode)
   {
      case 1:
         if (!bGlblQuiet && (nGlblListMinSize > 0))
         {
            long lMBytes = (long)(nFileSize / 1000000);
            if (lMBytes >= nGlblListMinSize)
            {
               int nIndent = (int)lLevel;
               if (nIndent > strlen(pszBlank)) nIndent = strlen(pszBlank);
               if (nIndent > 10) nIndent = 10;
                 
               printf("%4d mb,              %.*s%s\n", lMBytes, nIndent, pszBlank, pszFileName);
            }
         }
         break;

      case 2:
         if (!bGlblQuiet)
            printf("%s\n", pszFileName);
         break;

      case 3:
         glblTwinScan.addFile(pszFileName);
         break;
   }

   lBytes += nFileSize;
   return 0;
}

#ifdef WITH_FN_INST
long execInst(char *pszFileName, long lLevel, long &lFiles, long &lDirs, num &lBytes) 
{
   extern int sfkInstrument(char *pszFile, char *pszInc, char *pszMac, bool bRevoke, bool bRedo, bool bTouchOnRevoke);

   // source code automatic instrumentation, highly experimental
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
   "$purepath"    , "$ppath"  , "$quotpath"     , "$qpath"  ,

   "$purefile"    , "$pfile"  , "$quotfile"     , "$qfile"  ,
   "$pure_file"   , "$p_file" , "$quot_file"    , "$q_file" ,
   "$purerelfile" , "$prfile" , "$quotrelfile"  , "$qrfile" ,
   "$purebase"    , "$pbase"  , "$quotbase"     , "$qbase"  ,
   "$pureext"     , "$pext"   , "$quotext"      , "$qext"   ,
};

// tell if a supplied user command references single files.
// if not, it will be applied on directories only.
bool anyFileInRunCmd(char *pszCmd) 
{
   for (ulong i=4; i<(sizeof(apRunTokens)/sizeof(apRunTokens[0])); i++)
      if (strstr(pszCmd, apRunTokens[i]))
         return true;
   return false;
}

long onRunExpression(char *psz1, long &lExpLength, bool &bquot)
{
   ulong nPtrs = (sizeof(apRunTokens)/sizeof(apRunTokens[0]));
   ulong nRows = nPtrs/4;
   for (ulong irow=0; irow<nRows; irow++)
   {
      for (ulong icol=0; icol<4; icol++)
      {
         const char *psz2 = apRunTokens[irow*4+icol];
         if (!strncmp(psz1, psz2, strlen(psz2)))
         {
            lExpLength = strlen(psz2);
            bquot = (icol >= 2) ? true : false;
            if (bGlblDebug) printf("orp %u %u - %s\n", lExpLength, bquot, psz2);
            return irow;
         }
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
   char *psz1 = strchr(szLineBuf, '$');
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
      psz1 = strchr(psz1, '$');
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

long execDirStat(char *pszDir, long lLevel, long lFiles, long lDirs, num lBytes)
{
   if (nGlblListMode != 1)
      return 0;

   int nIndent = (int)lLevel;
   if (nIndent > strlen(pszBlank)) nIndent = strlen(pszBlank);
   if (nIndent > 10) nIndent = 10;

   if (!strncmp(pszDir, glblDotSlash, 2))
      pszDir += 2;

   unsigned long lMBytes = (unsigned long)lBytes / 1000000UL;

   if (bGlblQuiet) {
      if (lLevel == 0)
         printf("%4d mb %s\n", lMBytes, pszDir);
   } else {
      if (lMBytes >= nGlblListMinSize)
         printf("%4d mb, %5d files, %.*s%s\n", lMBytes, lFiles, nIndent, pszBlank, pszDir);
   }

   return 0;
}

long walkFiles(char *pszIn, long lLevel, long &lFiles, long &lDirs, num &lBytes, num &nLocalMaxTime, num &nTreeMaxTime)
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

   #ifdef _WIN32 // --------- Windows directory walking code ----------

   WIN32_FIND_DATA myfdat;
   HANDLE myfdh = FindFirstFile(pszPattern, &myfdat);
   if (myfdh == INVALID_HANDLE_VALUE) {
      printf("%s not found\n", pszPattern);
      delete [] pszPattern;
      delete [] pszBasePath;
      return 1; 
   }

   do
   {

   #else // ----------- unix directory walking code -------------

   // printf("TRY: %s\n", pszPattern);

   struct SfkFindData {
      char *cFileName;
      int   dwFileAttributes; 
      char  szNameBuf[200];
   } myfdat;

   DIR *myfdh = opendir(pszPattern);

   if (!myfdh) {
      printf("%s not found\n", pszPattern);
      delete [] pszPattern;
      delete [] pszBasePath;
      return 1; 
   }

   while (myfdat.cFileName = readDirEntry(myfdh))
   {
      // printf("READ: %s\n", myfdat.cFileName);
   
   #endif // _WIN32
   
      char *pszFile = myfdat.cFileName;

      if (   !strcmp(pszFile, ".")
          || !strcmp(pszFile, ".."))
         continue;

      long lSize2  = strlen(myfdat.cFileName);
      char *pszSub = new char[lSize1+lSize2+10];
      sprintf(pszSub, "%s%c%s", pszBasePath, glblPathChar, myfdat.cFileName);

   #ifndef _WIN32

      // is current object a file or dir?
      struct stat hStat1;
      if (stat(pszSub, &hStat1)) {
         printf("NOSTAT: %s\n", pszSub);
         return 0;
      }
      #ifdef S_IFLNK
      if ((hStat1.st_mode & S_IFREG) == S_IFREG || (hStat1.st_mode & S_IFLNK) == S_IFLNK)
      #else
      if ((hStat1.st_mode & S_IFREG) == S_IFREG)
      #endif
         myfdat.dwFileAttributes = 0x00; // file
      else
         myfdat.dwFileAttributes = 0x10; // dir

   #endif

      if (myfdat.dwFileAttributes & 0x10)
      {
         // subdirectory

         long nDirFiles = 0;
         long nDirDirs  = 0;
         num  nDirBytes = 0, nDirLocalMaxTime = 0, nDirTreeMaxTime = 0;

         // temporary attach '\\' char for exact path matching
         strcat(pszSub, glblPathStr);
         bool bMatch = matchesDirMask(pszSub);
         pszSub[strlen(pszSub)-1] = '\0';

         long lRC = 0;

         if (bGlblRecurseDirs && bMatch)
         {
            lRC = walkFiles(pszSub, lLevel+1, nDirFiles, nDirDirs, nDirBytes,
                            nDirLocalMaxTime, nDirTreeMaxTime);

            if (bGlblDebug) printf("] esd: %d %s\n", lLevel, pszSub);
            if (execSingleDir(pszSub, lLevel+1, nDirFiles, nDirDirs, nDirBytes,
                              nDirLocalMaxTime, nDirTreeMaxTime))
            {
               delete [] pszSub; // falling past delete below
               break;
            }
            // NOTE: local maxtime is NOT promoted, it was used w/in execSingleDir.
            //       we only promote the tree max time:
            if (nDirTreeMaxTime > nTreeMaxTime)
                nTreeMaxTime = nDirTreeMaxTime;
         }

         lFiles += nDirFiles;
         lDirs  += nDirDirs ;
         lBytes += nDirBytes;

         if (lRC)
         {
            #ifdef _WIN32
            FindClose(myfdh);
            #else
            closedir(myfdh);
            #endif
            delete [] pszSub;
            return lRC;
         }
      }
      else
      {
         // normal file: check mask against file name WITHOUT path
         if (!bGlblWalkJustDirs && matchesFileMask(myfdat.cFileName))
         {
            if (lRC = execSingleFile(pszSub, lLevel+1, lFiles, lDirs, lBytes,
                                     nLocalMaxTime, nTreeMaxTime))
            {
               delete [] pszSub; // falling past delete below
               break;
            }
         }

         // NOTE: maxtimes are promoted to next-higher level
         lFiles++;
      }

      delete [] pszSub;

      if (userInterrupt())
         break;
   }
   #ifdef _WIN32
   while (FindNextFile(myfdh, &myfdat));
   FindClose(myfdh);
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
   if (nListSize==0) nListSize=1;
   ulong nOldPerc = 0;
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
      // fix filename ending
      if (psz = strchr(pszFile, '\r')) *psz = 0;
      if (psz = strchr(pszFile, '\n')) *psz = 0;
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
   while (fgets((char*)abBuf, nMaxLineLen, fin)) // yes, exact len
   {
      nGlblLines++;
      nLocalLines++;

      if (strlen((char*)abBuf) == nMaxLineLen)
         fprintf(stderr, "warning: max line length %d reached, splitting. file %s, line %d\n", nMaxLineLen, pszFile, nLocalLines);

      if (   (!strncmp((char*)abBuf, pszGlblSnapFileStamp, strlen(pszGlblSnapFileStamp)))
          || (!strncmp((char*)abBuf, pszGlblClusterFileStamp, strlen(pszGlblClusterFileStamp)))
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

      fputs((char*)abBuf, fGlblOut);
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

long execSingleFile(char *pszFile, long lLevel, long &lFiles, long &lDirs, num &lBytes, num &ntime1, num &ntime2)
{
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
      case eFunc_FileStat: return execFileStat(pszFile, lLevel, lFiles, lDirs, lBytes);  break;
      case eFunc_Grep    : return execGrep(pszFile);       break;
      case eFunc_Mirror  : return execFileMirror(pszFile, ntime1, ntime2); break;
      case eFunc_Run     : return execRunFile(pszFile, lLevel, lFiles, lDirs, lBytes);  break;
      case eFunc_FormConv: return execFormConv(pszFile);   break;
      #ifdef WITH_FN_INST
      case eFunc_Inst    : return execInst(pszFile, lLevel, lFiles, lDirs, lBytes);  break;
      #endif
      case eFunc_RefColDst : return execRefColDst(pszFile);  break;
      case eFunc_RefProcSrc: return execRefProcSrc(pszFile); break;
      default: break;
   }
   return 0;
}

long execDirFreeze(char *pszName, long lLevel, num &nLocalMaxTime, num &nTreeMaxTime)
{
   // in: pszGlblSrcRoot, DstRoot, pszName
   if (!strncmp(pszName, glblDotSlash, 2))
      pszName += 2;

   // create all needed target directories
   sprintf(szLineBuf, "%s%s", pszGlblDstRoot, pszName);
   char *psz1 = szLineBuf;
   char *psz2 = strchr(psz1, glblPathChar);
   while (psz2) {
      strncpy((char*)abBuf, psz1, psz2-psz1);
      abBuf[psz2-psz1] = 0;
      char *pszDir = (char*)abBuf;
      if (!isDir(pszDir)) {
         if (bGlblDebug) printf("mkdir: %s\n", pszDir);
         #ifdef _WIN32
         if (_mkdir(pszDir))
         #else
         if (mkdir(pszDir, S_IREAD | S_IWRITE))
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
      if (mkdir(pszDir, S_IREAD | S_IWRITE))
      #endif
      {
         fprintf(stderr, "error: cannot create dir: %s\n", pszDir);
         return 9;
      }
   }

   // check file time maxima: any need to call zip?
   sprintf(szLineBuf, "%s%s\\01-arc.zip", pszGlblDstRoot, pszName);
   num nZipTime = getFileTime(szLineBuf);

   // a maxtime of 0 means there are no local files.
   // a ziptime of 0 means there is no zip file.
   // if there is no local file, or there is a newer zip file:
   if ((nLocalMaxTime == 0) || ((nZipTime != 0) && (nLocalMaxTime <= nZipTime))) {
      // printf("%c scan for changes %u     \r", pszGlblTurn[(nGlblTurnCnt++)&3], nGlblTurnCnt);
      // fflush(stdout);
   } else {
      // else create or update zip archive
      sprintf(szLineBuf, 
              "zip -u -D \"%s%s\\01-arc.zip\" \"%s\\*\" "
              "2>%s09-freeze-log.txt",
              pszGlblDstRoot, pszName, pszName,
              pszGlblDstRoot
             );
      printf("zip: %s          \n", pszName);
      // printf("zip: %s lmax %u zip %u\n", pszName, nLocalMaxTime, nZipTime);
      long lRC = 0;
      if (lRC = system(szLineBuf)) {
         // zip RC 12: nothing to do - no files w/in dir, ignore
         if (lRC == 12)
            return 0;
         // all other RC's: real error, like cannot access file
         logError("error: RC %d while running command:\n   %s", lRC, szLineBuf);
         // continue, list errors when finishing
         return 0;
      }
   }

   // recreate target archive name
   sprintf(szLineBuf, "%s%s\\01-arc.zip", pszGlblDstRoot, pszName);

   // 1. write single file entry:
   // not done here, but in execFileFreeze.

   // 2. write md5 of archive zip:
   SFKMD5 md5;
   if (getFileMD5(szLineBuf, md5, 1)) {
      // this happens whenever there are no files w/in dir.
      // logError("error: no archive created:\n   %s", szLineBuf);
      return 0;
   }
   unsigned char *pmd5 = md5.digest();
   for (int i=0; i<16; i++)
      fprintf(fGlblMD5Arc,"%02x",pmd5[i]);
   fprintf(fGlblMD5Arc," *%s\\01-arc.zip\n",pszName); // md5sum similar

   // 3. extend unpack batch:
   fprintf(fGlblBatch, "02-tools\\unzip \"%s\\01-arc.zip\"\n", pszName);

   // remember dir maxtime in dir-times.txt
   char szTimeBuf[100];
   fprintf(fGlblTimes, "%s %s\\01-arc.zip\n", numtoa(nLocalMaxTime,15,szTimeBuf), pszName);
   fflush(fGlblTimes);

   return 0;
}

long execFileMirror(char *pszName, num &ntime1, num &ntime2)
{
   if (glblFileSet.getDirCommand() == eCmd_CopyDir)
      return execFileXCopy(pszName, ntime1, ntime2);
   else
   if (glblFileSet.getDirCommand() == eCmd_FreezeDir)
      return execFileFreeze(pszName, ntime1, ntime2);
   fprintf(stderr, "error: no command supplied for directory\n");
   return 9;
}

long execDirMirror(char *pszName, long lLevel, num &ntime1, num &ntime2)
{
   if (glblFileSet.getDirCommand() == eCmd_CopyDir)
      return execDirXCopy(pszName, lLevel, ntime1, ntime2);
   else
   if (glblFileSet.getDirCommand() == eCmd_FreezeDir)
      return execDirFreeze(pszName, lLevel, ntime1, ntime2);
   fprintf(stderr, "error: no command supplied for directory\n");
   return 9;
}

long execFileFreeze(char *pszName, num &nLocalMaxTime, num &nTreeMaxTime)
{
   // doesn't actually freeze the single file, but

   // writes the file's md5 hash
   SFKMD5 md5;
   if (getFileMD5(pszName, md5)) {
      logError("error: failed to read:\n   %s", pszName);
      return 0;
   }
   unsigned char *pmd5 = md5.digest();
   for (int i=0; i<16; i++)
      fprintf(fGlblMD5Org,"%02x",pmd5[i]);
   fprintf(fGlblMD5Org," *%s\n",pszName); // md5sum similar

   // adds file to global inventory
   fprintf(fGlblCont, "%s\n", pszName);

   // update maxtimes
   num nFileTime = getFileTime(pszName);
   if (nFileTime > nLocalMaxTime)
       nLocalMaxTime = nFileTime;
   if (nFileTime > nTreeMaxTime)
       nTreeMaxTime = nFileTime;

   // show some stats
   if (glblFileCount.count()) {
      printf("[%u scan %s]     \r", glblFileCount.value(), glblFileSet.getCurrentRoot()); fflush(stdout);
   }

   return 0;
}

long execDirXCopy(char *pszName, long lLevel, num &nLocalMaxTime, num &nTreeMaxTime)
{
   // copy current dir or whole subtree?
   bool bAnyMasksGiven = glblFileSet.dirMasks().isStringSet(0,0);
   char *pszMode = "";
   num  nMaxTime = nLocalMaxTime;
   if (!bAnyMasksGiven) {
      // running in whole-tree mode: only toplevel acts
      if (lLevel > 0)
         return 0;
      // tell xcopy to copy whole subtree:
      pszMode  = " /S";
      nMaxTime = nTreeMaxTime;
   }

   // in: pszGlblSrcRoot, DstRoot, pszName
   if (!strncmp(pszName, glblDotSlash, 2))
      pszName += 2;

   // this uses szLineBuf:
   num nOldDirTime = getOldDirTime(pszName);

   // do we have to call xcopy at all?
   if ((nMaxTime == 0) || ((nOldDirTime != 0) && (nMaxTime <= nOldDirTime))) {
      // printf("%c scan for changes %u    \r", pszGlblTurn[(nGlblTurnCnt++)&3], nGlblTurnCnt);
      // fflush(stdout);
      // printf("xcopy: %s - no change\n", pszName); // , nOldDirTime, nMaxTime);
   } else {
      // xcopy a dir, excluding subdirectories (NO /S option)
      sprintf(szLineBuf, "xcopy \"%s\" \"%s%s\" /H /I /R /K /Y /D%s",
              pszName, pszGlblDstRoot, pszName, pszMode
             );
      printf("xcopy: %s%s        \n", pszName, pszMode);
      if (system(szLineBuf)) {
         logError("error: failed to fully execute command:\n   %s", szLineBuf);
         // continue anyway, list errors when finishing.
      }
   }

   // remember dir maxtime in dir-times.txt
   char szTimeBuf[100];
   fprintf(fGlblTimes, "%s %s\n", numtoa(nLocalMaxTime,15,szTimeBuf), pszName);
   fflush(fGlblTimes);

   return 0;
}

long execFileXCopy(char *pszName, num &nLocalMaxTime, num &nTreeMaxTime)
{
   // this doesn't actually copy, but

   // writes a global directory entry
   fprintf(fGlblCont, "%s\n", pszName);

   // writes the file's md5 hash
   SFKMD5 md5;
   if (getFileMD5(pszName, md5)) {
      logError("error: failed to read:\n   %s", pszName);
      return 0;
   }
   unsigned char *pmd5 = md5.digest();
   for (int i=0; i<16; i++)
      fprintf(fGlblMD5Org,"%02x",pmd5[i]);
   fprintf(fGlblMD5Org," *%s\n",pszName); // md5sum similar

   // update max times
   long nFileTime = getFileTime(pszName);
   if (nFileTime > nLocalMaxTime) {
      nLocalMaxTime = nFileTime;
      // printf("upd lmt %s %u\n",pszName,nLocalMaxTime);
   }
   if (nFileTime > nTreeMaxTime) {
      nTreeMaxTime = nFileTime;
      // printf("upd tmt %s %u\n",pszName,nLocalMaxTime);
   }

   // show some stats
   if (glblFileCount.count()) {
      printf("[%u scan %s]      \r", glblFileCount.value(), glblFileSet.getCurrentRoot()); fflush(stdout);
   }

   return 0;
}

long execSingleDir(char *pszName, long lLevel, long &lFiles, long &lDirs, num &lBytes, num &ntime1, num &ntime2)
{
   // skip initial dot slash which might be returned by dir scanning
   if (!strncmp(pszName, glblDotSlash, 2))
      pszName += 2;

   switch (nGlblFunc) 
   {
      case eFunc_FileStat: return execDirStat(pszName, lLevel, lFiles, lDirs, lBytes); break;
      case eFunc_Mirror  : return execDirMirror(pszName, lLevel, ntime1, ntime2); break;
      case eFunc_Run     :
         if (bGlblWalkJustDirs)
            return execRunDir(pszName, lLevel, lFiles, lDirs, lBytes);
         break;
      default: break;
   }
   return 0;
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
      printf("based on the free sfk 1.0.6 by stahlworks art & technology.\n");
      printf("Distributed for free under the GNU General Public License,\n"
             "without any warranty. Use with care and on your own risk.\n");
      printf("\n"
             "usage:\n"
             "   sfk snapto=outfile [-pure] [-prefix=str] [-norec] dir mask [mask2] [...]\n"
             "       collect many files into one large text file.\n"
             "       pure: don't insert filenames. prefix: insert str before every block.\n"
             "       norec: do not recurse into subdirs. stat: show time stats at end.\n"
             "          sfk snapto=all-src.cpp -prefix=!$! . .cpp .hpp\n"
             "          sfk snapto=all-src.cpp -prefix=!$! -dir src1 src2 -file .cpp .hpp .xml\n"
             "   sfk syncto=dbfile dir mask [!mask2]\n"
             "       gather files into one large editable clusterfile.\n"
             "       changes in the cluster are written back to sourcefiles automatically.\n"
             "       changes in the source files are synced into the cluster automatically.\n"
             "          sfk syncto=cluster.cpp . .cpp .hpp\n"
             "   sfk syncto=dbfile [-up]\n"
             "       reuse an existing clusterfile.\n"
             "       on start, by default, cluster diffs are written to the sourcefiles.\n"
             "       on start, with -up, sourcefile diffs are written into the cluster.\n"
             "          sfk syncto=cluster.cpp\n"
             "   sfk syncto=dbfile -from=myconfig.sfk\n"
             "       read command line parameters from config file, e.g.\n"
             "          -dir\n"
             "             foosys\\bar1\\include\n"
             "             foosys\\bar1\\source\n"
             "             !save_patch\n"
             "          -file\n"
             "             .cpp .hpp\n"
             "   mapping of compiler error output line numbers:\n"
             "       make yoursys.mak >err.txt 2>&1\n"
             "       sfk mapto=yourcluster.cpp [-nomix] [-cmd=...] <err.txt\n"
             "          nomix: list only the mapped output lines. cmd: on first mapped line,\n"
             "          call supplied command, with clustername and line as parameters.\n"
             "\n"
             "   sfk patch [...]\n"
             "       dynamic source file patching. type \"sfk patch\" for help.\n"
             "\n"
             "   sfk detab=tabsize dir mask [!mask2]\n"
             "       replace tabs by spaces within file(s).\n"
             "          sfk detab=3 . .cpp .hpp\n"
             "          sfk detab=3 singleFileName.txt\n"
             "   sfk scantab dir mask [!mask2]\n"
             "       check if files contain tabs.\n"
             "          sfk scantab -dir src1 src2 -file .cpp .hpp\n"
             "   sfk lf-to-crlf or crlf-to-lf\n"
             "       just like detab, for CRLF conversion\n"
             "   sfk text-join-lines infile outfile\n"
             "       for text with lines split by email reformatting.\n"
             "   sfk stat [-minsize=mb] [dir] [-i]\n"
             "       show directory size statistics in mbytes.\n"
             "       minsize: list only dirs and files >= minsize mbytes.\n"
             "       -i: read list of files and directories from stdin.\n"
             "          sfk stat -minsize=10m\n"
             "          type dirlist.txt | sfk stat -quiet -i\n"
             "   sfk list [-twinscan] dir [mask]\n"
             "       plain listing of files within dir.\n"
             "       twinscan: list only identical files, if any.\n"
             "          sfk list -dir src1 -file .cpp -dir src2 -file .hpp\n"
             "   sfk grep [-pat] pattern [pattern2] [-dir] [dir1] [mask1] ...\n"
             "       very simple, case-insensitive pattern search for text files.\n"
             "       only lines containing all patterns are listed.\n"
             "          sfk grep mytext . .hpp\n"
             "          sfk grep -pat text1 text2 -dir src1 src2 -file .cpp .hpp\n"
             "   sfk bin-to-src [-pack] infile outfile\n"
             "       create sourcefile containing a binary object\n"
             "          sfk bin-to-src myimg.dat imgsrc.cpp\n"
             "   sfk filter <input >output [-lnum] [-c] -+orpat [++andpat] [-!nopat] [...]\n"
             "       filter lines from standard input, case-insensitive.\n"
             "       -+   this pattern MAY be  part of a result line\n"
             "       -+   this pattern MAY be start of a result line\n"
             "       ++   this pattern MUST be  part of a result line\n"
             "       +ls+ this pattern MUST be start of a result line\n"
             "       -!   this pattern must NOT be  part of a result line\n"
             "       -ls! this pattern must NOT be start of a result line\n"
             "       -no-empty-lines removes all empty lines from stream\n"
             "       -lnum preceed all result lines by input line number\n"
             "       -c   compare case-sensitive (not default)\n"
             "          anyprog | sfk filter -+mypat -!otherpat\n"
             "   sfk addhead <in >out [-noblank] string1 string2 ...\n"
             "   sfk addtail <in >out [-noblank] string1 string2 ...\n"
             "       add string(s) at start or end of lines.\n"
             "       with noblank specified, does not add blank char.\n"
             "\n"
             "   sfk md5gento=outfile dir mask [mask2] [!mask3] [...]\n"
             "   sfk md5gento=outfile -dir dir1 dir2 -file mask1 mask2 !mask3 [...]\n"
             "       create list of md5 checksums over all files.\n"
             "          sfk md5gento=md5.dat . *\n"
             "   sfk md5check=infile [-skip=n] [-skip n]\n"
             "       verify list of md5 checksums. to speed up verifys by spot checking,\n"
             "       specify -skip=n: after every checked file, n files will be skipped.\n"
             "          sfk md5check=md5.dat\n"
             "\n"
             "   sfk run \"your command $file [$relfile] [...]\" [-quiet] [-sim]\n"
             "       run self-defined command on files or directories.\n"
             "       type \"sfk run\" for help.\n"

             #ifdef _WIN32 // uses Window-specific XCopy command
             "\n"
             "   sfk mirrorto=targetdir -dir src1 src2 !src1\\ignore\\ -copy|zip\n"
             "       copy or freeze all or updated files from src to target,\n"
             "       using the external commands xcopy and zip.\n"
             "       !dirname excludes directory \"dirname\".\n"
             "          sfk mirrorto=d:\\mirrorc -dir pic dev doc !pic\\tmp\\ -zip\n"
             #endif

             #ifdef WITH_FN_INST
             "\n"
             "   sfk inst [...]\n"
             "       instrument c++ source code with calls to micro tracing kernel.\n"
             "       type \"sfk inst\" for help.\n"
             #endif

             "\n"
             "   sfk reflist [-abs] [-wide] -dir tdir -file .text -dir sdir -file .sext\n"
             "       find file references and dependencies. \"sfk reflist\" for help.\n"
             "\n"
             "   general option -norec avoids recursion into subdirectories.\n"
             "   general option -quiet shows less output on some commands.\n"
            );
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

   if (!strncmp(pszCmd, "snapto=", strlen("snapto="))) 
   {
      pszGlblOutFile = pszCmd+strlen("snapto=");
      fGlblOut = fopen(pszGlblOutFile, "w");
      if (!fGlblOut) { fprintf(stderr, "error: cannot write %s\n", pszGlblOutFile); return 9; }

      int iDir = 2;
      if ((iDir < argc) && !strncmp(argv[iDir],"-prefix=",strlen("-prefix="))) {
         pszGlblJamPrefix = strdup(argv[iDir]+strlen("-prefix="));
         iDir++;
      }
      if ((iDir < argc) && !strcmp(argv[iDir],"-pure")) {
         bGlblJamPure = 1;
         iDir++;
      }
      bool bstat = 0;
      if ((iDir < argc) && !strcmp(argv[iDir],"-stat")) {
         bstat = 1;
         iDir++;
      }

      // collect dir and mask parms
      processDirParms(pszCmd, argc, argv, iDir, 1);

      // write global file header
      fprintf(fGlblOut, "%s\n\n", pszGlblSnapFileStamp);
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

   if (!strncmp(pszCmd, "binpatch", strlen("binpatch"))) 
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
      if ((iDir < argc) && !strcmp(argv[iDir], "-up")) {
         iDir++;
         bInitialUpSync = 1;
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
            printf("> done : %d targets (re)written\n", nSynced);
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

      // options supplied?
      int iDir = 2;
      bool bMix = 1;
      if ((iDir < argc) && !strcmp(argv[iDir], "-nomix")) {
         bMix = 0;
         iDir++;
      }

      // command supplied?
      char *pszCmd = 0;
      if ((iDir < argc) && !strncmp(argv[iDir], "-cmd=", strlen("-cmd="))) {
         pszCmd = argv[iDir] + strlen("-cmd=");
         iDir++;
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

   if (!strncmp(pszCmd, "stat", strlen("stat"))) 
   {
      nGlblListMode = 1;

      int iDir = 2;
      if ((iDir < argc) && !strncmp(argv[iDir], "-minsize=", strlen("-minsize="))) {
         char *pszMinSize = &argv[iDir][strlen("-minsize=")];
         nGlblListMinSize = atol(pszMinSize);
         iDir++;
      }
      if (iDir >= argc)
         setProcessCurrentDir();
      else
         processDirParms(pszCmd, argc, argv, iDir, 1);
      lRC = walkAllTrees(eFunc_FileStat, lFiles, lDirs, nBytes);
      long lMBytes = (long)(nBytes / 1000000);
      printf("%4d mb, % 5d files in total.\n", lMBytes, lFiles);
      bDone = 1;
   }

   if (!strncmp(pszCmd, "list", strlen("list")))
   {
      nGlblListMode = 2;
      int iDir = 2;
      if ((iDir < argc) && !strncmp(argv[iDir], "-twinscan", strlen("-twinscan"))) {
         nGlblListMode = 3;
         iDir++;
      }
      processDirParms(pszCmd, argc, argv, iDir, 1);
      lRC = walkAllTrees(eFunc_FileStat, lFiles, lDirs, nBytes);
      if (nGlblListMode == 3)
         glblTwinScan.analyze();
      glblTwinScan.shutdown();
      bDone = 1;
   }

   if (!strncmp(pszCmd, "grep", strlen("grep"))) 
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

   if (!strncmp(pszCmd, "bin-to-src", strlen("bin-to-src"))) 
   {
      if (checkArgCnt(argc, 4)) return 9;

      bool bPack = 0;
      int iDir = 2;
      if (!strcmp(argv[iDir], "-pack")) {
         iDir++;
         bPack = 1;
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

   if (!strncmp(pszCmd, "filter", strlen("filter"))) 
   {
      // very simple input stream filter
      // -+pat1 -+pat2 -!pat3

      if (checkArgCnt(argc, 3)) return 9;

      int iPat = 2;
      int nPat = argc - 2;

      bool bWithLineNumbers = 0;
      if (!strcmp(argv[iPat], "-lnum")) {
         bWithLineNumbers = 1;
         iPat++;
         nPat--;
      }

      bool bCaseSensitive = 0;
      if (!strcmp(argv[iPat], "-c")) {
         bCaseSensitive = 1;
         iPat++;
         nPat--;
      }

      long nMaxLineLen = sizeof(szLineBuf)-10;
      long nLine = 0;
      while (fgets(szLineBuf, nMaxLineLen, stdin))
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
            if (!strncmp(pszPat, "-!", 2)) {
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
            if (!strncmp(pszPat, "-ls!", 4)) {
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

      bDone = 1;
   }

   if (!strncmp(pszCmd, "tee", strlen("tee"))) 
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

   if (!strncmp(pszCmd, "addhead", strlen("addhead")))
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

   if (!strncmp(pszCmd, "addtail", strlen("addtail")))
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

   if (!strncmp(pszCmd, "mirrorto=", strlen("mirrorto=")))
   {
      if (argc < 3) { fprintf(stderr, "error: missing parms\n"); return 9; }

      char *pszDstRoot = pszCmd+strlen("mirrorto=");
      if (!isDir(pszDstRoot))
         {  fprintf(stderr, "error: no such directory: %s\n", pszDstRoot); return 9; }

      // copy destination root, auto-add "\\" if missing
      pszGlblDstRoot = new char[strlen(pszDstRoot)+10];
      strcpy (pszGlblDstRoot, pszDstRoot);
      if (pszGlblDstRoot[strlen(pszGlblDstRoot)-1] != glblPathChar)
         strcat(pszGlblDstRoot, glblPathStr);

      int iDir = 2;
      if (processDirParms(pszCmd, argc, argv, iDir, 1))
         return 9;

      char szContentsName[1024];
      char szMD5ArcName[1024];
      char szBatchName[1024];
      char szMD5OrgName[1024];
      char szTimesName[1024];

      const char *pszContentFile = "01-content.txt";
      const char *pszToolsDir    = "02-tools";
      const char *pszMD5ArcFile  = "03-md5-arc.txt";
      const char *pszVerArcBatch = "04-verify-arc.bat";
      const char *pszUnpArcBatch = "05-unpack-arc.bat";
      const char *pszMD5OrgFile  = "06-md5-org.txt";
      const char *pszVerOrgBatch = "07-verify-org.bat";
      const char *pszTimesFile   = "08-dir-times.txt";

      sprintf(szContentsName, "%s%s", pszGlblDstRoot, pszContentFile);
      sprintf(szMD5ArcName  , "%s%s", pszGlblDstRoot, pszMD5ArcFile );
      sprintf(szMD5OrgName  , "%s%s", pszGlblDstRoot, pszMD5OrgFile );
      sprintf(szTimesName   , "%s%s", pszGlblDstRoot, pszTimesFile  );
      sprintf(szBatchName   , "%s%s", pszGlblDstRoot, pszUnpArcBatch);

      // preparations: is there any dir-times already?
      pszGlblDirTimes = loadFile(szTimesName, __LINE__);

      if (!(fGlblCont   = fopen(szContentsName, "w"))) { fprintf(stderr, "error: unable to write: %s\n", szContentsName); return 9; }
      if (!(fGlblMD5Arc = fopen(szMD5ArcName  , "w"))) { fprintf(stderr, "error: unable to write: %s\n", szMD5ArcName  ); return 9; }
      if (!(fGlblBatch  = fopen(szBatchName   , "w"))) { fprintf(stderr, "error: unable to write: %s\n", szBatchName   ); return 9; }
      if (!(fGlblMD5Org = fopen(szMD5OrgName  , "w"))) { fprintf(stderr, "error: unable to write: %s\n", szMD5OrgName  ); return 9; }
      if (!(fGlblTimes  = fopen(szTimesName   , "w"))) { fprintf(stderr, "error: unable to write: %s\n", szTimesName   ); return 9; }

      lRC = walkAllTrees(eFunc_Mirror, lFiles, lDirs, nBytes);

      fclose(fGlblCont);
      fclose(fGlblMD5Arc);
      fclose(fGlblBatch );
      fclose(fGlblMD5Org);
      fclose(fGlblTimes );

      sprintf(szBatchName, "%s%s", pszGlblDstRoot, pszVerArcBatch);
      if (!(fGlblBatch = fopen(szBatchName   , "w"))) { fprintf(stderr, "error: unable to write: %s\n", szBatchName   ); return 9; }
      fprintf(fGlblBatch, "%s\\sfk md5check=%s %%1 %%2 %%3\n", pszToolsDir, pszMD5ArcFile);
      fclose(fGlblBatch);

      sprintf(szBatchName, "%s%s", pszGlblDstRoot, pszVerOrgBatch);
      if (!(fGlblBatch = fopen(szBatchName   , "w"))) { fprintf(stderr, "error: unable to write: %s\n", szBatchName   ); return 9; }
      fprintf(fGlblBatch, "%s\\sfk md5check=%s %%1 %%2 %%3\n", pszToolsDir, pszMD5OrgFile);
      fclose(fGlblBatch);

      if (glblErrorLog.numberOfEntries() > 0) {
         long nErr = glblErrorLog.numberOfEntries();
         printf("===== %d errors occured: =====\n", nErr);
         for (long i=0; i<nErr; i++)
            printf("%s\n", glblErrorLog.getEntry(i, __LINE__));
         printf("see also %s09-freeze-log.txt\n",pszGlblDstRoot);
         return 5;
      } else {
         printf("all done. check also %s09-freeze-log.txt\n",pszGlblDstRoot);
      }

      bDone = 1;
   }

   if (!strcmp(pszCmd, "patch")) 
   {
      lRC = patchMain(argc-1, argv, 1);
      bDone = 1;
   }

   if (!strcmp(pszCmd, "run"))
   {
      if (argc < 4) {
         printf(
             "sfk run \"your command $file [$relfile] [...]\" [-quiet] [-sim] [...]\n"
             "\n"
             "    run a self-defined command on every file- or directory name.\n"
             "    within your command string, you may specify:\n"
             "\n"
             "       $quotfile    or $qfile  - insert filename with path and \"\" quotes.\n"
             "       $purefile    or $pfile  - insert filename with path and NO quotes.\n"
             "       $quotrelfile or $qrfile - insert relative filename without path.\n"
             "       $quotbase    or $qbase  - the relative base filename, without extension.\n"
             "       $quotext     or $qext   - filename extension. foo.bar.txt has extension .txt.\n"
             "       $quotpath    or $qpath  - the path (directory) without filename.\n"
             "       also valid: $purerelfile, $prfile, $purebase, $pbase, $pureext, $pext,\n"
             "                   $purepath, $ppath.\n"
             "\n"
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
             "             sfk run \"ren \\\"$pbase.$pext\\\" \\\"$pbase-old.$pext\\\"\" testfiles\n"
             "          in any way, have a -sim run first to simulate what may happen.\n"
             "\n"
             "       sfk run \"attrib -R $qfile\" -quiet testfiles\\FooBank\\BarDriver\n"
             "       sfk run \"<img src=$quotfile>\" -dir . -file .jpg -sim >index.html\n"
             "       sfk run \"xcopy $qpath \\\"G:\\$ppath\\\" /S /H /I /R /K /Y /D\" -dir sources\n"
             "       type dirlist.txt | sfk run -idirs \"xcopy \\\"x:\\$ppath\\\" \\\"z:\\$ppath\\\" /I /D\"\n"
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
            if (anyFileInRunCmd(pszGlblRunCmd)) { fprintf(stderr, "error: -idirs only allowed with $path[p], not with file commands.\n"); return 9; }
            bGlblStdInDirs    = 1;
            bGlblWalkJustDirs = 1;
            iDir++;
         }
         else
         if (!strncmp(argv[iDir], "-", 1)) {
            break; // fall through
         }
         else {
            pszGlblRunCmd = argv[iDir];
            iDir++;
         }
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

   if (!strncmp(pszCmd, "strings", strlen("strings"))) 
   {
      // extract strings from single binary file
      if (checkArgCnt(argc, 3)) return 9;

      char *pszFile = argv[2];
      FILE *fin = fopen(pszFile, "rb");
      if (!fin) { fprintf(stderr, "error: cannot read: %s\n", pszFile); return 9; }
      long icol   = 0;
      long istate = 0;
      long iword  = 0;
      bool bflush = 0;
      while (1) 
      {
         long nRead = fread(szLineBuf, 1, sizeof(szLineBuf)-10, fin);
         if (nRead <= 0)
            break;
         for (long i=0; i<nRead; i++)
         {
            char c = szLineBuf[i];
            bflush = 0;
            if (isprint(c)) {
               // printable char
               if (istate == 1) {
                  istate = 0;
                  // start collecting next word
                  iword = 0;
               }
               // continue collecting current word
               szLineBuf2[iword++] = c;
               // check for word buffer overflow
               if (iword >= 80) {
                  bflush = 1;
               }
            } else {
               // non-printable (binary) char
               if (istate == 0) {
                  bflush = 1;
               }
            }
            // dump current word?
            if (bflush) {
               bflush = 0;
               if (iword > 2) {
                  printf("%.*s ", (int)iword, szLineBuf2);
                  icol += (iword+1);
                  if (icol >= 80) {
                     fputc('\n', stdout);
                     icol = 0;
                  }
                  iword = 0;
               }
            }
         }
      }
      fclose(fin);
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
      num  rlBytes = 0, nLocalMaxTime = 0, nTreeMaxTime  = 0;

      // collect list of targets
      nGlblFunc = eFunc_RefColDst;
      char *pszTree = glblFileSet.setCurrentRoot(0);
      if (bGlblDebug) printf("] dst: %s\n", pszTree);
      if (walkFiles(pszTree, 0, rlFiles, rlDirs, rlBytes, nLocalMaxTime, nTreeMaxTime)) {
         bGlblError = 1;
         return 9;
      }
      if (!bGlblQuiet)
         printf("%.60s\r", pszBlank);
      printf("%05u potential target files\n", glblRefDst.numberOfEntries(0));

      // process potential sources
      nGlblFunc = eFunc_RefProcSrc;
      pszTree = glblFileSet.setCurrentRoot(1);
      if (bGlblDebug) printf("] src: %s\n", pszTree);
      if (walkFiles(pszTree, 0, rlFiles, rlDirs, rlBytes, nLocalMaxTime, nTreeMaxTime)) {
         bGlblError = 1;
         return 9;
      }
      if (!bGlblQuiet)
         printf("%.60s\r", pszBlank);
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
