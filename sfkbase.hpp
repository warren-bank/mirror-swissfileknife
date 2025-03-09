#ifndef _SFKBASE_HPP_
#define _SFKBASE_HPP_
/*
   swiss file knife base include
*/

// ========== core includes and operating system abstraction ==========

// enable LFS esp. on linux:
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64

#ifdef _WIN32
 #define WINFULL
#endif

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
  #define FD_SETSIZE 200   // must be set before windows.h
  #include <windows.h>
  #ifndef _MSC_VER
   #include <ws2tcpip.h>
  #endif
  #include <sys/timeb.h>
  #include <time.h>
  #include <process.h>
  #define getpid() _getpid()
  #include <errno.h>
  #include <direct.h>
  #include <signal.h>
  #include <io.h>
  #ifndef socklen_t
   #define socklen_t int
  #endif
  #ifndef SD_RECEIVE
   #define SD_RECEIVE 0x00
   #define SD_SEND    0x01
   #define SD_BOTH    0x02
  #endif
  #ifndef SHUT_RD
   #define  SHUT_RD   SD_RECEIVE
   #define	SHUT_WR   SD_SEND
   #define	SHUT_RDWR SD_BOTH
  #endif
  #define vsnprintf _vsnprintf
  #define snprintf  _snprintf
  // FILE_ATTRIBUTE_READONLY    0x00000001
  // FILE_ATTRIBUTE_HIDDEN      0x00000002
  // FILE_ATTRIBUTE_SYSTEM      0x00000004
  // FILE_ATTRIBUTE_DIRECTORY   0x00000010
  // FILE_ATTRIBUTE_ARCHIVE     0x00000020
  // FILE_ATTRIBUTE_NORMAL      0x00000080
  // FILE_ATTRIBUTE_TEMPORARY   0x00000100
  #define WINFILE_ATTRIB_MASK   0x0000001F
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
  #include <signal.h>
  #include <sys/statvfs.h>
  #include <utime.h>
  #include <pthread.h>
  #include <pwd.h>
  #include <ifaddrs.h>
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
  #define ioctlsocket ioctl
  #define WSAEWOULDBLOCK EWOULDBLOCK
#endif

// - - - - - basic types and tracing - - - - -

#ifdef MAC_OS_X
 #define fpos64_t  fpos_t
 #define fgetpos64 fgetpos
 #define fsetpos64 fsetpos
 #define statvfs64 statvfs
 #define stat64    stat
 #define __dev_t   dev_t
#endif // MAC_OS_X

#define uchar unsigned char
#define uint  unsigned int
#define ulong unsigned long
#define bool  unsigned char
#define cchar const char
#define str(x) (char*)x

#define mclear(x) memset(&x, 0, sizeof(x))

#ifdef CALLBACK_TRACING
 #define mtklog  cbtrace
 #define mtkerr  cbtrace
 #define mtkwarn cbtrace
 #define mtkdump
 #define _
 #define __
#else
 #ifdef WITH_TRACING
  #include "mtk/mtktrace.hpp"
  #ifdef MTKTRACE_CODE
   #include "mtk/mtktrace.cpp"
  #endif
  #define _  mtklog(("[sfk %d]",__LINE__));
  #define __ MTKBlock tmp983452(__FILE__,__LINE__,"");tmp983452.dummy();
 #else
  #define mtklog(x)
  #define mtkerr(x)
  #define mtkwarn(x)
  #define mtkdump
  #define _
  #define __
 #endif
#endif

extern const char  glblPathChar    ;
extern const char  glblWrongPChar  ;
extern const char *glblPathStr     ;
extern const char *glblAddWildCard ;
extern const char *glblDotSlash    ;
extern       char  glblNotChar     ;
extern       char  glblRunChar     ;
extern const char *glblLineEnd     ;

#ifdef _WIN32
 #define SFK_FILT_NOT1 "-!"
 #define SFK_FILT_NOT2 "-ls!"
 #define SFK_FILT_NOT3 "-le!"
 #define EXE_EXT ".exe"
 #define SFK_SETENV_CMD "set"
extern const char *glblWildStr     ;
extern const char  glblWildChar    ;
extern const char *glblWildInfoStr ;
#else
 #define SFK_FILT_NOT1 "-:"
 #define SFK_FILT_NOT2 "-ls:"
 #define SFK_FILT_NOT3 "-le:"
 #define EXE_EXT ""
 #define SFK_SETENV_CMD "export"
extern char  glblWildStr[10];       // "+";
extern char  glblWildChar;          // '+';
extern char  glblWildInfoStr[20];   // "+ or \\*";
#endif

// - - - - - 64 bit abstractions - - - - -

#ifdef WINFULL
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

extern char *numtostr(num n, int nDigits, char *pszBuf, int nRadix);
extern char *numtoa_blank(num n, int nDigits=12);
extern char *numtoa(num n, int nDigits, char *pszBuf);
extern char *numtohex(num n, int nDigits, char *pszBuf);
extern num   atonum(char *psz);
extern num   myatonum(char *psz);
extern mytime_t getSystemTime();

#ifndef SIG_UNBLOCK
   #define SIG_UNBLOCK 2
#endif

#ifndef SIGALRM
   #define SIGALRM 14
#endif

#ifndef sigset_t
   #define sigset_t int
#endif

#ifndef sigemptyset
   #define sigemptyset(sig)
#endif

#ifndef sigaddset
   #define sigaddset( set, sig)
#endif

#ifndef sigprocmask
   #define sigprocmask(a, b, c)
#endif

// ========== end core includes and operating system abstraction ==========

#ifndef SFKNOVFILE
 #define VFILEBASE
 #define VFILENET
#endif // SFKNOVFILE

int isDir(char *pszName);
cchar *sfkLastError();

#define MAX_ABBUF_SIZE 100000
#define MAX_LINE_LEN     4096

extern int (*pGlblJamCheckCallBack)(char *pszFilename);
extern int (*pGlblJamFileCallBack)(char *pszFilename, num &rLines, num &rBytes);
extern int (*pGlblJamLineCallBack)(char *pszLine, int nLineLen, bool bAddLF);
extern int (*pGlblJamStatCallBack)(char *pszInfo, uint nFiles, uint nLines, uint nMBytes, uint nSkipped, char *pszSkipInfo);
extern int bGlblPassThroughSnap;

char *findPathLocation(cchar *pszCmd, bool bExcludeWorkDir=0);
extern int fileExists(char *pszFileName, bool bOrDir=0);

#define strcopy(dst,src) mystrcopy(dst,src,sizeof(dst)-10)
void  mystrcopy      (char *pszDst, cchar *pszSrc, int nMaxDst);
int  mystrstri      (char *pszHayStack, cchar *pszNeedle, int *lpAtPosition = 0);
char *mystrrstr      (char *psrc, cchar *ppat);
char *mystrristr     (char *psrc, cchar *ppat);
int  mystrncmp      (char *psz1, cchar *psz2, int nLen, bool bCase=0);
int  mystricmp      (char *psz1, cchar *psz2);
int  mystrnicmp     (char *psz1, cchar *psz2, int nLen);
bool  strBegins      (char *pszStr, cchar *pszPat);
bool  striBegins     (char *pszStr, cchar *pszPat);
bool  strEnds        (char *pszStr, cchar *pszPat);
void  removeCRLF     (char *pszBuf);
bool  sfkisalpha     (uchar uc);
bool  sfkisalnum     (uchar uc);
bool  sfkisprint     (uchar uc);
void  myrtrim        (char *pszBuf);
void  skipToWhite    (char **pp);
void  skipWhite      (char **pp);

char *loadFile       (char *pszFile, bool bquiet=0);
num   getFileSize    (char *pszName);
num   getCurrentTime ( );
num   atonum         (char *psz);   // decimal only
num   myatonum       (char *psz);   // with 0x support
char *numtoa         (num n, int nDigits=1, char *pszBuf=0);
char *numtohex       (num n, int nDigits=1, char *pszBuf=0);
int  timeFromString (char *psz, num &nRetTime);
void  doSleep        (int nmsec);
uchar *loadBinaryFile(char *pszFile, num &rnFileSize);
bool  infoAllowed    ( );
struct hostent *sfkhostbyname(const char *pstr, bool bsilent=0);

class IOStatusPhase {
public:
		IOStatusPhase	(cchar *pinfo);
	  ~IOStatusPhase	( );
};

void	resetIOStatus	( );
num   countIOBytes	(num nbytes);
char	*getIOStatus	(num &nagemsec, num &nbytes, num &nmaxbytes);
// returns NULL if no status is set

class CharAutoDel {
public:
      CharAutoDel (char *p) { pClPtr = p; }
     ~CharAutoDel ( )       { if (pClPtr) delete [] pClPtr; }
private:
      char *pClPtr;
};

// max length of sfk (internal) filenames and URL's
#define SFK_MAX_PATH 512

#ifdef  MAX_PATH
 #undef MAX_PATH
#endif
#define MAX_PATH error_use_sfk_max_path

#ifdef  PATH_MAX
 #undef PATH_MAX
#endif
#define PATH_MAX error_use_sfk_max_path

#ifdef VFILEBASE
extern void  setDiskCacheActive(bool b);
extern bool  getDiskCacheActive( );
extern void  setDiskCachePath(char *p);
extern char *getDiskCachePath( );
#endif // VFILEBASE

extern void myclosesocket(SOCKET hsock, bool bread=1, bool bwrite=1);
bool userInterrupt   (bool bSilent=0, bool bWaitForRelease=0);

// some linux/mac sys\param.h define that:
#ifdef isset
 #undef isset
#endif

typedef unsigned uint32_t;

class SFKMD5
{
public:
    SFKMD5  ( );
   ~SFKMD5  ( );

   void   update  (uchar *pData, uint32_t nLen);
   uchar *digest  ( );

private:
   void   update     (uchar c);
   void   transform  ( );

   uint32_t nClCntHigh, nClCntLow;
   uchar    *pClBuf;
   uint32_t nClBufLen,nClBufCnt;
   uchar    *pClDig;
   uint32_t nClDigLen;
   uchar    aClBuf[100];
   uchar    aClDig[100];
   uint32_t alClSta[4];
   uint32_t alClBuf[16];
   bool     bClDigDone;
};

// map of managed keys and NOT MANAGED values.
class KeyMap {
public:
      KeyMap   ( );
     ~KeyMap   ( );

   void  reset ( );
   // removes all entries. keys are deleted.
   // values are not deleted, as their type is unknown.

   int  put   (char *pkey, void *pvalue=0);
   // set a key, or put a value under a key.
   // the key is copied. pvalue is not copied.
   // if the key exists already, old pvalue is replaced.
   // rc 0:done >0:error.

   void  *get  (char *pkey, int *poutidx=0);
   // if a value was stored then it is returned.
   // if not, null is returned, no matter if the key is set.
   // if poutidx is given, it returns the nearest comparison index,
   // even if no direct hit for the key was found. this index
   // however must be used with care, as it can be < 0 or >= size.

   bool  isset (char *pkey);
   // tell if the key was ever put (with or without value).
   // rc: 0:not_set 1:is_set.

   void  setcase(bool bYesNo);
   // default for key comparison is CASE SENSITIVE.
   // use setcase(0) to select CASE INSENSITIVE.

   void  setreverse(bool bYesNo);
   // toggle reverse sorting order.

   int  remove(char *pkey);
   // remove entry with that key, if any.
   // rc: 0:done 1:no_such_key >=5:error.

   void *iget  (int nindex, char **ppkey=0);
   // walk through map entries, sorted by the key.
   // nindex must be 0 ... size()-1.
   // returns value, if any.
   // if key ptr is provided, it is set.

   int  put   (num nkey, void *pvalue=0);
   void *get   (num nkey);
   bool  isset (num nkey);
   int  remove(num nkey);
   void *iget  (int nindex, num *pkey);

   int  size  ( );
   // number of entries in KeyMap.

protected:
   void  wipe     ( );
   int  expand   (int n);
   int   bfind    (char *pkey, int &rindex);
   int  remove   (int nindex);

   int  nClArrayAlloc;
   int  nClArrayUsed;
   char  **apClKey;
   void  **apClVal;

   bool  bClCase;
   bool  bClRev;
};

// map of MANAGED string values.
class StringMap : public KeyMap {
public:
      StringMap   ( );
     ~StringMap   ( );

   void  reset    ( );

   int  put      (char *pkey, char *pvalue);
   // value is COPIED and MANAGED by StringMap.
   // also accepts NULL values, if you want to use
   // only isset() instead of get.

   char  *get     (char *pkey, char *pszOptDefault=0);
   char  *iget    (int nindex, char **ppkey);
   int   remove  (char *pkey);

   int  put      (num nkey, char *pvalue);
   char *get      (num nkey);
   char *iget     (int nindex, num *pkey);
   int  remove   (num nkey);
};

// map of MANAGED strings with attributes.
class AttribStringMap : public StringMap {
public:
      AttribStringMap   ( );
     ~AttribStringMap   ( );

   int  put      (char *pkey, char *ptext, char *pattr);
   char  *get     (char *pkey, char **ppattr);
   char  *iget    (int nindex, char **ppkey, char **ppattr);

   int  put      (num nkey, char *ptext, char *pattr);
   char *get      (num nkey, char **ppattr);
   char *iget     (int nindex, num *pkey, char **ppattr);

private:
   char *mixdup   (char *ptext, char *pattr);
   int  demix    (char *pmixed, char **pptext, char **ppattr);
};

class LongTable {
public:
   LongTable            ( );
  ~LongTable            ( );
   int addEntry        (int nValue, int nAtPos=-1);
   int updateEntry     (int nValue, int nIndex);
   int numberOfEntries ( );
   int getEntry        (int iIndex, int nTraceLine);
   void resetEntries    ( );
private:
   int expand          (int nSoMuch);
   int nClArraySize;
   int nClArrayUsed;
   int *pClArray;
};

class StringTable {
friend class Array;
public:
   StringTable          ( );
  ~StringTable          ( );
   int addEntry        (char *psz, int nAtPos=-1, char **ppCopy=0);
   int removeEntry     (int nAtPos);
   int numberOfEntries ( );
   char *getEntry       (int iIndex, int nTraceLine);
   int  setEntry       (int iIndex, char *psz);
   void resetEntries    ( );
   bool isSet           (int iIndex);
   void dump            (int nIndent=0);
   int find            (char *psz); // out: index, or -1
private:
   int addEntryPrefixed(char *psz, char cPrefix);
   int setEntryPrefixed(int iIndex, char *psz, char cPrefix);
   int expand          (int nSoMuch);
   int nClArraySize;
   int nClArrayUsed;
   char **apClArray;
};

class NumTable {
public:
   NumTable            ( );
  ~NumTable            ( );
   int addEntry        (num nValue, int nAtPos=-1);
   int updateEntry     (num nValue, int nAtPos);
   int numberOfEntries ( );
   num  getEntry        (int iIndex, int nTraceLine);
   void resetEntries    ( );
private:
   int expand          (int nSoMuch);
   int nClArraySize;
   int nClArrayUsed;
   num  *pClArray;
};

class Array {
public:
   Array                (const char *pszID); // one-dimensional by default
  ~Array                ( );
   int addString       (char *psz);   // to current row (0 by default)
   int addString       (int lRow, char *psz);
   int setString       (int lRow, int iIndex, char *psz);
   char *getString      (int iIndex); // use isStringSet() before
   char *getString      (int lRow, int iIndex);
   int addLong         (int nValue, int nTraceLine); // to current row (0 by default)
   int addLong         (int lRow, int nValue, int nTraceLine);
   int getLong         (int iIndex); // use isLongSet() before
   int getLong         (int lRow, int iIndex, int nTraceLine);
   int setLong         (int lRow, int iIndex, int nValue, int nTraceLine);
   int numberOfEntries ( );           // in current row (0 by default)
   int numberOfEntries (int lRow);
   bool isLongSet       (int lRow, int iIndex); // index and type test
   bool isStringSet     (int iIndex); // index and type test
   bool isStringSet     (int lRow, int iIndex);
   int addRow          (int nTraceLine); // add empty row, set as current
   int setRow          (int iCurRow, int nTraceLine);// set current row
   bool hasRow          (int iRow);   // tell if row exists
   void reset           ( );           // removes all entries
   void dump            ( );
   int addNull         (int lRow);
private:
   bool isSet           (int iIndex); // in current row (0 by default)
   int ensureBase      ( );  // make sure at least one row exists
   int expand          (int nSoMuch);
   int nClRowsSize;
   int nClRowsUsed;
   StringTable **apClRows;
   int nClCurRow;
   const char *pszClID;
};

class FileSet {
public:
   FileSet  ( );
  ~FileSet  ( );
   int  beginLayer     (bool bWithEmptyCommand, int nTraceLine);
   int  addRootDir     (char *pszName, int nTraceLine, bool bNoCmdFillup);
   int  addDirMask     (char *pszName);
   int  addDirCommand  (int);
   int  addFileMask    (char *pszName);
   int  autoCompleteFileMasks   (int nWhat);
   void  setBaseLayer   ( );
   void  reset          ( );
   void  shutdown       ( );
   bool  hasRoot        (int iIndex);
   char* setCurrentRoot (int iIndex);
   bool  changeFirstRoot(char *pszNewRoot);
   char* getCurrentRoot ( );
   char* root           (bool braw=0); // like above, but returns "" if none, with braw: 0 if none
   int  numberOfRootDirs ( );
   Array &rootDirs      ( ) { return clRootDirs; }
   Array &dirMasks      ( ) { return clDirMasks; }
   Array &fileMasks     ( ) { return clFileMasks; }
   bool  anyRootAdded   ( );
   bool  anyFileMasks   ( );  // that are non-"*"
   char* firstFileMask  ( );  // of current root
   void  dump           ( );
   void  info           (void (*pout)(int nrectype, char *pline));
   int  getDirCommand  ( );  // of current root
   int  checkConsistency  ( );
private:
   int  ensureBase     (int nTraceLine);
   void  resetAddFlags  ( ); // per layer
   Array clRootDirs;    // row 0: names/str, row 1: command/int, row 2: layer/int
   Array clDirMasks;    // row == layer
   Array clFileMasks;   // row == layer
   int   nClCurDir;
   int   nClCurLayer;
   char  *pClLineBuf;
   int   bClGotAllMask;
   int   bClGotPosFile;
   int   bClGotNegFile;
};

class StringPipe
{
public:
      StringPipe  ( );
      char *read  (char **ppAttr=0);  // returns 0 on EOD
      void  resetPipe ( );
      bool  eod   ( );
      void  dump  (cchar *pszTitle);
      int  numberOfEntries ( ) { return clText.numberOfEntries(); }
      char *getEntry        (int nIndex, int nLine, char **pAttr=0);
      void  resetEntries    ( );
      int  addEntry        (char *psz, char *pAttr);
      int  setEntry        (int iIndex, char *psz, char *pAttr);
private:
   StringTable clText;
   StringTable clAttr;
   int nReadIndex;
};

#ifdef _WIN32
 #ifdef SFK_W64
  typedef __finddata64_t SFKFindData;
 #else
  typedef _finddata_t SFKFindData;
 #endif
#else
struct SFKFindData 
{
   char *name;
   int   attrib;
   num   time_write;
   num   time_create;
   num   size;
   bool  islink;
   uint rawmode; // for tracing
   uint rawtype; // for tracing
   uint rawnlnk; // link count
   num   ninode;     // under linux
   __dev_t ostdev;   // under linux
   bool  bhavenode;  // under linux
};
#endif

class CoiData;

// the caching object identifer (Coi) represents
// a file, a directory, or an url to a remote object.
class Coi
{
public:
    Coi  (char *pszName, char *pszRootDir);
   ~Coi  ( );

    // create deep copy, however containing only
    // the lightweight data like filename.
    Coi  *copy( );

   char  *name( );         // must be set
   char  *relName( );      // without any path portion
   char  *rootRelName( );  // returns full name if no root
   // in case of zip entries, may return subdir/thefile.txt
   char  *root(bool braw=0);  // "" if none, with braw: 0 if none
   char  *ref (bool braw=0);  // "" if none, with braw: 0 if none

   #ifdef VFILEBASE
   char  *orgName( );      // same as name() except on redirects
   #endif // VFILEBASE

   int  status   ( );
   // 0:yet_unknown 1:ok 9:fails_to_read

   // Coi's must have an initial filename,
   // but it can be changed later by this:
   int  setName  (char *pszName, char *pszOptRootDir=0);
   // if rootdir is not given, the old one is kept.

   void  setIsDir    (bool bYesNo); // sets anydir status
   bool  isAnyDir    ( );
   bool  isTravelDir ( );
   bool  isDirLink   ( );

   int  setRef   (char *pszName);

   bool  hasSize  ( );
   bool  hasTime  ( );

   num   getSize  ( );
   num   getTime  ( );

   void  setSize  (num nSize );
   void  setTime  (num nMTime, num nCTime = 0);

   bool  isWriteable ( );

   void  fillFrom (void *pfdat); // SFKFindData ptr

   // available after fillFrom() only:
   bool  isHidden ( );
   bool  isLink   ( );

   // extra string outside coi definition:
   int  setExtStr   (char *psz);   // is copied
   char  *getExtStr  ( );           // null if none

   // check if coi is an existing file
   bool  existsFile  (bool bOrDir=0);

   // data I/O functions:
   bool   isFileOpen ( );
   int   open       (cchar *pmode); // like fopen, but RC 0 == OK
   // supported pmodes: "rb", "r", "r+b", "wb"
   // with "r", reading stops as soon as binary is detected.
   size_t read       (void *pbuf, size_t nbufsize);
   size_t readRaw    (void *pbuf, size_t nbufsize);
   void   close      ( );

   int    seek       (num nOffset, int nOrigin);
   // rc0:ok >=0:failed to seek

   size_t write      (uchar *pbuf, size_t nbytes);
   // guarantees to write nbytes (incl. workaround
   // for windows network file writing bug).
   // rc: number of bytes written
   // in case of error, rc != nbytes

   // heuristic check by file header if it's binary.
   // status can also be set from external.
   void   setBinaryFile (bool bYesNo);
   bool   isBinaryFile  ( );
   uchar  isUTF16       ( ); // 0x00==none 0xFE==le 0xEF==be
   bool	 isSnapFile		( );

   // readLine alloc's another I/O buffer on demand:
   int   readLine   (char *pszOutBuf, int nOutBufLen);

   // directory and archive processing:
   int   openDir    ( );  // prep for nextEntry()
   Coi   *nextEntry  ( );  // result owned by CALLER.
   void   closeDir   ( );  // required after processing.
   bool   isDirOpen  ( );

   #ifndef VFILEZIP
   int  isZipSubEntry  ( )   { return 0; }
   bool   isTravelZip   (bool braw=0) { return 0; }
   void   setArc        (bool bIsArchive) { }
   bool   isKnownArc    ( )   { return 0; }
   #endif // NVFILEZIP

   #ifdef VFILEBASE

   int   rawLoadDir ( );
   Coi   *getElementByAbsName (char *pabs); // result is NOT locked

   bool  isNet    ( );  // ftp OR http
   bool  isFtp    ( );
   bool  isHttp   ( );

   // if it's zip, http or ftp, then it's virtual
   bool  isVirtual(bool bWithRootZips=0);

   num   getUsedBytes   ( );  // info for the cache

	int	 prefetch		(bool bLoadNonArcBinaries, num nFogSizeLimit, num nHardSizeLimit);

   // direct query of http header fields, if any given
   StringMap &headers     ( );            // creates on demand
   char      *header      (cchar *pname);  // returns NULL or the value

   static char *cacheName (char *pnamein, char *pbufin, int nbufsize, int *prDirPartLen=0);
   // nbufsize should be >= SFK_MAX_PATH

   #endif // VFILEBASE

   // reference counting:
   int  incref   (cchar *pTraceFrom); // increment refcnt
   int  decref   ( );  // decrement refcnt
   int  refcnt   ( );  // current no. of refs

   static bool bClDebug;

   bool   rawIsDir      ( );  // native filesystem dir

   int   getContent    (uchar **ppdata, num &rnSize);
   // ppdata is MANAGED BY COI! i.e. as soon as Coi
   // is deleted, returned data is deleted as well.
   // rc >0: unable to access file content.

   void   setContent    (uchar *pdata, num nsize, num ntime=0);
   // releases old content, if any.

   int   releaseContent( );
   // after getContent(), tell that data buffer can be freed.

   cchar  *lasterr      ( );

   #ifndef _WIN32
   // linux only:
   bool   haveNode      ( );
   num    getNode       ( );  // not always unique
   bool   haveFileID    ( );  // same as haveNode
   char  *getFileID     ( );  // built from node and device
   #endif

   // if status()==0, can call this:
   int  readStat       ( );

private:
   // nextEntry() does additional checks, this does none:
   Coi   *nextEntryRaw  ( );  // result owned by CALLER.

   // native file system
   int   rawOpenDir    ( );  // prep for nextEntry()
   Coi   *rawNextEntry  ( );  // result owned by CALLER.
   void   rawCloseDir   ( );  // required after processing.

   #ifdef VFILEBASE

   int   provideInput  (int nTraceLine, bool bsilent=0);
   int   loadOwnFileRaw(num nmaxsize, uchar **ppout, num &rsize);

   // ftp folders and files
   bool   rawIsFtpDir    ( );
   //     rawLoadDir     ( )  // is generic
   Coi   *rawNextFtpEntry( );
   void   rawCloseFtpDir ( ); 
   int   rawLoadFtpDir  ( ); // load remote dir listing

   int   rawOpenFtpSubFile   (cchar *pmode);
   size_t rawReadFtpSubFile   (void *pbufin, size_t nBufSize);
   void   rawCloseFtpSubFile  ( );

   // http pages and files
   bool   rawIsHttpDir    ( );
   //     rawLoadDir     ( )  // is generic
   Coi   *rawNextHttpEntry( );
   void   rawCloseHttpDir ( ); 
   int   rawLoadHttpDir  ( );

   int   rawOpenHttpSubFile  (cchar *pmode);
   size_t rawReadHttpSubFile  (void *pbufin, size_t nBufSize);
   void   rawCloseHttpSubFile ( );

   #endif // VFILEBASE

   bool  debug    ( );

   // core data for every lightweight Coi:
   char  *pszClName;
   char  *pszClRoot;
   char  *pszClRef;
   char  *pszClExtStr;

   uchar nClStatus;
   uint nClHave;
   num   nClSize;
   num   nClMTime;   // modification time
   num   nClCTime;   // creation time, or <= 0
   bool  bClRead;
   bool  bClWrite;
   bool  bClDir;     // any dir, e.g. by name
   bool  bClFSDir;   // verified native filesystem dir
   bool  bClHidden;
   bool  bClLink;
   bool  bClBinary;
   bool  bClArc;
   uchar nClUCS;     // 0:none 0xFE:LE 0xEF:BE
   bool	bClSnap;		// sfk snapfile

   // ON EXTENSIONS ABOVE, ADAPT COI::COPY!

   int  nClRefs;    // not to be coi::copied

   #ifndef _WIN32
public: // not yet defined
   // additional informal stuff
   uint rawmode;  // for tracing
   uint rawtype;  // for tracing
   uint rawnlnk;  // link count
   num   nClINode;   // under linux
   __dev_t oClStDev; // under linux
   // file id is made from:
   //   16 bytes stdev (expanded as string)
   //   16 bytes inode (expanded as string)
   //   zero terminator
   char  szClFileID[40];
   #endif

public: // not really
   // heavyweight Coi's use this as well:
   CoiData *pdata;
   CoiData  &data    ( );
   bool   hasData    ( );  // has a CoiData object

   #ifdef VFILEBASE
   bool   bClInCache;
	bool	 isCached	( );
   bool   hasContent ( );  // has CoiData AND cached data
   #endif // VFILEBASE

   // adapt ctr and copy() on extensions!
};

#define COI_HAVE_SIZE   (1UL<<0)
#define COI_HAVE_TIME   (1UL<<1)
#define COI_HAVE_READ   (1UL<<2)
#define COI_HAVE_WRITE  (1UL<<3)
#define COI_HAVE_DIR    (1UL<<4)
#define COI_HAVE_HIDDEN (1UL<<5)
#define COI_HAVE_LINK   (1UL<<6)
#define COI_HAVE_BINARY (1UL<<7)
#define COI_HAVE_NODE   (1UL<<8)
#define COI_HAVE_ARC    (1UL<<9)

class CoiTable {
public:
   CoiTable             ( );
  ~CoiTable             ( );

   int addEntry        (Coi &ocoi, int nAtPos=-1);
   // adds a COPY of the supplied coi.

   int removeEntry     (int nAtPos);
   int numberOfEntries ( );
   Coi  *getEntry       (int iIndex, int nTraceLine);
   int  setEntry       (int iIndex, Coi *pcoi);
   int addSorted       (Coi &ocoi, char cSortedBy, bool bUseCase);
   void resetEntries    ( );
   bool isSet           (int iIndex);
private:
   int expand          (int nSoMuch);
   int nClArraySize;
   int nClArrayUsed;
   Coi  **apClArray;
};

#ifdef VFILEBASE
class ZipReader;
class HTTPClient;
class FTPClient;
#endif // VFILEBASE

// data for directory and archive processing:
// must be declared before ~coi otherwise data dtr is not called?
class CoiData {
public:
    CoiData  ( );
   ~CoiData  ( );

   // directory traversal data
   bool   bdiropen;
   char   *prelsubname;  // filename within directory

   // generic I/O support
   bool   bfileopen;     // between open() ... close() of files
   char   szmode[10];    // i/o mode: "rb","r+b","wb"
   bool   bwrite;        // (also) writing to file
   bool   bstoprdbin;    // stop read if binary detected
   num    ntotalread;    // bytes read since open()
   uint  ntold;         // warnings told about this file
   char   szlasterr[50]; // most recent error info
   #ifdef VFILEBASE
   bool   bloaddirdone;  // don't repeat dir loading
   bool   bstopread;
   #endif // VFILEBASE
   bool   banyread;      // anything yet read after open()?

   // this buffer is for high-level Coi read functions:
   // -  readLine()
   // -  isBinaryFile()
   // it shall NOT be used by low-level read functions.
   struct CoiReadBuf {
      uchar  *data;     // alloc'ed on demand
      int    getsize;
      int    getindex;
      int    geteod;
      num     getpos;
   } rbuf;  // overlapping read cache buffer

   // this buffer can optionally cache the whole input
   struct CoiSrcBuf {
      uchar  *data;  // NULL if not yet cached
      num     size;
      num     index; // current read index
      num     time;  // src mtime
   } src;

   // native filesystem I/O
   char  *pdirpat;
   #ifdef _WIN32
   bool   bdir1stdone;
   intptr_t otrav;
   #else
   DIR     *ptrav;
   #endif
   FILE    *pfile;   // managed by Coi, not by CoiData

   #ifdef VFILEBASE
   // INTERNAL list of subfiles (zip, net).
   // elements shall NOT be passed directly
   // to callers, but only copies of them.
   CoiTable *pelements;
   CoiTable &elements ( );

   int nNextElemEntry;

   num  nPreCacheFileCnt;
   num  nPreCacheFileBytes;

   // ftp support
   FTPClient *pClFtp;
   // when set, it is allocated by the coi,
   // and must be released after use.
   int  getFtp(char *purl);
   // on rc0, can use pClFtp.
   int  releaseFtp( );
   // does NOT close connection, but clears pClFtp.

   // http support
   HTTPClient *pClHttp;
   // when set, it is allocated by the coi,
   // and must be released after use.
   int  getHttp(char *purl);
   // on rc0, can use pClHttp.
   int  releaseHttp( );
   // clears pClHttp, but if connection is
   // in keep-alive, does NOT close it yet.
   StringMap &headers( );
   // http headers, so far only non-redundant entries
   // like content-type, but not multiple set-cookies.
   StringMap *pClHeaders;

   // if the coi was http redirected, then
   bool  bRedirected;   // this is true
   char  *pClOrgName;   // this is the first coi name
   #endif // VFILEBASE
};

class CoiAutoDelete {
public:
      CoiAutoDelete (Coi *pcoi, bool bDecRef)
         { pClCoi = pcoi; bClDecRef = bDecRef; }
     ~CoiAutoDelete ( ) {
         if (!pClCoi)
            return;
         if (bClDecRef)
            pClCoi->decref();
         if (!pClCoi->refcnt())
            delete pClCoi; 
      }
private:
      Coi *pClCoi;      // can be NULL
      bool bClDecRef;   // on dtr, do a single decref
};

enum eProgressInfoKeepFlags {
   eKeepProg   = (1<<0),
   eKeepAdd    = (1<<1),
   eNoCycle    = (1<<2),
   eSlowCycle  = (1<<4),
};

class ProgressInfo
{
public:
   ProgressInfo          ( );
   void  setWidth        (int nColumns);
   void  setAddInfoWidth (int nColumns); // abs. columns, high prio
   void  setAddInfoHalve ( );  // fills half of line, with low prio
   void  setAddInfo      (const char *pszFormat, ...);
   void  setAction       (cchar *pszVerb, cchar *pszSubject, cchar *pszAddInfo=0, int nKeepFlags=0);
   void  setStatus       (cchar *pszVerb, cchar *pszSubject, cchar *pszAddInfo=0, int nKeepFlags=0);
   void  print           ( );  // print status now, keep line
   void  printLine       (int nFilter=0); // print final status, including newline
   void  cycle           ( );  // print status if enough time elapsed
   void  clear           ( );  // clear status, if it was printed
   int  print           (const char *pszFormat, ...);
   void  setProgress     (num nMax, num nCur, cchar *pszUnit);
   void  setStatProg     (cchar *pverb, cchar *psubj, num nMax, num nCur, cchar *pszUnit);

private:
   void  fixAddInfoWidth ( );
   void  dumpTermStatus  ( );
   void  clearTermStatus ( );  // if anything to clear
   int  nMaxChars;
   int  nMaxSubChars;
   int  nAddInfoCols;
   int  nDumped;
   num   nLastDumpTime;
   bool  bAddInfoPrio;
   int  nAddInfoReserve;
   int  nTurn;
   char  szVerb   [200];
   char  szSubject[200];
   char  szAddInfo[200];
   char  szTermBuf[400];
   char  szPrintBuf[400];
   char  szPerc   [20];
};

extern ProgressInfo info;

// simple double linked list

class ListEntry
{
public:
    ListEntry   ( );
   ~ListEntry   ( );

   ListEntry *next      ( ) { return pClNext; }
   ListEntry *previous  ( ) { return pClPrevious; }

   ListEntry *pClNext;
   ListEntry *pClPrevious;

   // user payload
   void  *data;
};

class List
{
public:
   List  ( );
  ~List  ( );

   ListEntry *first  ( ) { return pClFirst; }
   ListEntry *last   ( ) { return pClLast; }

   void add          (ListEntry *p);
   void addAsFirst   (ListEntry *p);
   void addAfter     (ListEntry *after, ListEntry *toadd);
   void remove       (ListEntry *entry);
   void reset        ( );

private:
   ListEntry *pClFirst;
   ListEntry *pClLast;
};

#ifdef SFK_PROFILING // windows only

class StaticPerformancePoint;

#define MAX_PERF_POINTS 100

class StaticPerformanceStats
{
public:
      StaticPerformanceStats  ( );

   void  addPoint       (StaticPerformancePoint *pPoint);
   int   numberOfPoints ( );
   StaticPerformancePoint *getPoint(int iIndex);

StaticPerformancePoint
   *apClPoints [MAX_PERF_POINTS+10],
   *pClCurrentPoint;
int
    iClPoints;
};

extern StaticPerformanceStats glblPerfStats;

class StaticPerformancePoint
{
public:
      StaticPerformancePoint  (const char *pszID, const char *pszFile, int iTraceLine);

      inline void  blockEntry ( );
      inline void  blockExit  (int iElapsedTicks);

const char *pszClID;
const char *pszClFile;
int   iClTraceLine;
num   iClHits;
num   iClTotalTime;
num   iClSubTimes;
};

class DynamicPerformancePoint
{
public:
       DynamicPerformancePoint (StaticPerformancePoint *pStaticPoint);
      ~DynamicPerformancePoint ( );

StaticPerformancePoint
      *pClStaticPoint,
      *pClStaticParent;
num
       nClEntryTickCount;
};

#define _p2(id,file,line)  \
   static StaticPerformancePoint oStatPoint##line(id,file,line); \
   DynamicPerformancePoint oDynaPoint##line(&oStatPoint##line);

#define _p(id) _p2(id,__FILE__,__LINE__)

extern void logProfile();

inline num getPerfCnt()
{
   LARGE_INTEGER val1;
   QueryPerformanceCounter(&val1);
   return val1.QuadPart;
}

inline num getPerfFreq()
{
   LARGE_INTEGER val1;
   QueryPerformanceFrequency(&val1);
   return val1.QuadPart;
}

#else

#define _p(id)

extern void logProfile();

#endif

#endif // _SFKBASE_HPP_

