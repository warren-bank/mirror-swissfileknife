/*
   swiss file knife base classes
*/

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

   long  put   (char *pkey, void *pvalue=0);
   // set a key, or put a value under a key.
   // the key is copied. pvalue is not copied.
   // if the key exists already, old pvalue is replaced.
   // rc 0:done >0:error.

   void  *get  (char *pkey, long *poutidx=0);
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

   long  remove(char *pkey);
   // remove entry with that key, if any.
   // rc: 0:done 1:no_such_key >=5:error.

   void *iget  (long nindex, char **ppkey=0);
   // walk through map entries, sorted by the key.
   // nindex must be 0 ... size()-1.
   // returns value, if any.
   // if key ptr is provided, it is set.

   long  put   (num nkey, void *pvalue=0);
   void *get   (num nkey);
   bool  isset (num nkey);
   long  remove(num nkey);
   void *iget  (long nindex, num *pkey);

   long  size  ( );
   // number of entries in KeyMap.

protected:
   void  wipe     ( );
   long  expand   (long n);
   int   bfind    (char *pkey, long &rindex);
   long  remove   (long nindex);

   long  nClArrayAlloc;
   long  nClArrayUsed;
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

   long  put      (char *pkey, char *pvalue);
   // value is COPIED and MANAGED by StringMap.
   // also accepts NULL values, if you want to use
   // only isset() instead of get.

   char  *get     (char *pkey, char *pszOptDefault=0);
   char  *iget    (long nindex, char **ppkey);
   long   remove  (char *pkey);

   long  put      (num nkey, char *pvalue);
   char *get      (num nkey);
   char *iget     (long nindex, num *pkey);
   long  remove   (num nkey);
};

// map of MANAGED strings with attributes.
class AttribStringMap : public StringMap {
public:
      AttribStringMap   ( );
     ~AttribStringMap   ( );

   long  put      (char *pkey, char *ptext, char *pattr);
   char  *get     (char *pkey, char **ppattr);
   char  *iget    (long nindex, char **ppkey, char **ppattr);

   long  put      (num nkey, char *ptext, char *pattr);
   char *get      (num nkey, char **ppattr);
   char *iget     (long nindex, num *pkey, char **ppattr);

private:
   char *mixdup   (char *ptext, char *pattr);
   long  demix    (char *pmixed, char **pptext, char **ppattr);
};

class LongTable {
public:
   LongTable            ( );
  ~LongTable            ( );
   long addEntry        (long nValue, long nAtPos=-1);
   long updateEntry     (long nValue, long nIndex);
   long numberOfEntries ( );
   long getEntry        (long iIndex, int nTraceLine);
   void resetEntries    ( );
private:
   long expand          (long nSoMuch);
   long nClArraySize;
   long nClArrayUsed;
   long *pClArray;
};

class StringTable {
friend class Array;
public:
   StringTable          ( );
  ~StringTable          ( );
   long addEntry        (char *psz, long nAtPos=-1, char **ppCopy=0);
   long removeEntry     (long nAtPos);
   long numberOfEntries ( );
   char *getEntry       (long iIndex, int nTraceLine);
   long  setEntry       (long iIndex, char *psz);
   void resetEntries    ( );
   bool isSet           (long iIndex);
   void dump            (long nIndent=0);
   long find            (char *psz); // out: index, or -1
private:
   long addEntryPrefixed(char *psz, char cPrefix);
   long setEntryPrefixed(long iIndex, char *psz, char cPrefix);
   long expand          (long nSoMuch);
   long nClArraySize;
   long nClArrayUsed;
   char **apClArray;
};

class NumTable {
public:
   NumTable            ( );
  ~NumTable            ( );
   long addEntry        (num nValue, long nAtPos=-1);
   long updateEntry     (num nValue, long nAtPos);
   long numberOfEntries ( );
   num  getEntry        (long iIndex, int nTraceLine);
   void resetEntries    ( );
private:
   long expand          (long nSoMuch);
   long nClArraySize;
   long nClArrayUsed;
   num  *pClArray;
};

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
   long addNull         (long lRow);
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

class FileSet {
public:
   FileSet  ( );
  ~FileSet  ( );
   long  beginLayer     (bool bWithEmptyCommand, int nTraceLine);
   long  addRootDir     (char *pszName, int nTraceLine, bool bNoCmdFillup);
   long  addDirMask     (char *pszName);
   long  addDirCommand  (long);
   long  addFileMask    (char *pszName);
   long  autoCompleteFileMasks   (int nWhat);
   void  setBaseLayer   ( );
   void  reset          ( );
   void  shutdown       ( );
   bool  hasRoot        (long iIndex);
   char* setCurrentRoot (long iIndex);
   char* getCurrentRoot ( );
   char* root           (bool braw=0); // like above, but returns "" if none, with braw: 0 if none
   long  numberOfRootDirs ( );
   Array &rootDirs      ( ) { return clRootDirs; }
   Array &dirMasks      ( ) { return clDirMasks; }
   Array &fileMasks     ( ) { return clFileMasks; }
   bool  anyRootAdded   ( );
   bool  anyFileMasks   ( );  // that are non-"*"
   char* firstFileMask  ( );  // of current root
   void  dump           ( );
   void  info           (void (*pout)(long nrectype, char *pline));
   long  getDirCommand  ( );  // of current root
   bool  isAnyExtensionMaskSet   ( );
   long  checkConsistency  ( );
private:
   long  ensureBase     (int nTraceLine);
   void  resetAddFlags  ( ); // per layer
   Array clRootDirs;    // row 0: names/str, row 1: command/int, row 2: layer/int
   Array clDirMasks;    // row == layer
   Array clFileMasks;   // row == layer
   long  nClCurDir;
   long  nClCurLayer;
   char  *pClLineBuf;
   long  bClGotAllMask;
   long  bClGotPosFile;
   long  bClGotNegFile;
};

class StringPipe
{
public:
      StringPipe  ( );
      char *read  (char **ppAttr=0);  // returns 0 on EOD
      void  resetPipe ( );
      bool  eod   ( );
      void  dump  (char *pszTitle);
      long  numberOfEntries ( ) { return clText.numberOfEntries(); }
      char *getEntry        (long nIndex, long nLine, char **pAttr=0);
      void  resetEntries    ( );
      long  addEntry        (char *psz, char *pAttr);
      long  setEntry        (long iIndex, char *psz, char *pAttr);
private:
   StringTable clText;
   StringTable clAttr;
   long nReadIndex;
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
   ulong rawmode; // for tracing
   ulong rawtype; // for tracing
   ulong rawnlnk; // link count
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

   long  status   ( );
   // 0:yet_unknown 1:ok 9:fails_to_read

   // Coi's must have an initial filename,
   // but it can be changed later by this:
   long  setName  (char *pszName, char *pszOptRootDir=0);
   // if rootdir is not given, the old one is kept.

   void  setIsDir    (bool bYesNo); // sets anydir status
   bool  isAnyDir    ( );
   bool  isTravelDir ( );
   bool  isDirLink   ( );

   long  setRef   (char *pszName);

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
   long  setExtStr   (char *psz);   // is copied
   char  *getExtStr  ( );           // null if none

   // check if coi is an existing file
   bool  existsFile  (bool bOrDir=0);

   // data I/O functions:
   bool   isFileOpen ( );
   long   open       (char *pmode); // like fopen, but RC 0 == OK
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

   // readLine alloc's another I/O buffer on demand:
   long   readLine   (char *pszOutBuf, long nOutBufLen);

   // directory and archive processing:
   long   openDir    ( );  // prep for nextEntry()
   Coi   *nextEntry  ( );  // result owned by CALLER.
   void   closeDir   ( );  // required after processing.
   bool   isDirOpen  ( );

   // reference counting:
   long  incref   (char *pTraceFrom); // increment refcnt
   long  decref   ( );  // decrement refcnt
   long  refcnt   ( );  // current no. of refs

   static bool bClDebug;

   bool   rawIsDir      ( );  // native filesystem dir

   long   getContent    (uchar **ppdata, num &rnSize);
   // ppdata is MANAGED BY COI! i.e. as soon as Coi
   // is deleted, returned data is deleted as well.
   // rc >0: unable to access file content.

   void   setContent    (uchar *pdata, num nsize, num ntime=0);
   // releases old content, if any.

   long   releaseContent( );
   // after getContent(), tell that data buffer can be freed.

   char  *lasterr       ( );

   #ifndef _WIN32
   // linux only:
   bool   haveNode      ( );
   num    getNode       ( );  // not always unique
   bool   haveFileID    ( );  // same as haveNode
   char  *getFileID     ( );  // built from node and device
   #endif

   // if status()==0, can call this:
   long  readStat       ( );

private:
   // nextEntry() does additional checks, this does none:
   Coi   *nextEntryRaw  ( );  // result owned by CALLER.

   // native file system
   long   rawOpenDir    ( );  // prep for nextEntry()
   Coi   *rawNextEntry  ( );  // result owned by CALLER.
   void   rawCloseDir   ( );  // required after processing.

   bool  debug    ( );

   // core data for every lightweight Coi:
   char  *pszClName;
   char  *pszClRoot;
   char  *pszClRef;
   char  *pszClExtStr;

   uchar nClStatus;
   ulong nClHave;
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

   // ON EXTENSIONS ABOVE, ADAPT COI::COPY!

   long  nClRefs;    // not to be coi::copied

   #ifndef _WIN32
public: // not yet defined
   // additional informal stuff
   ulong rawmode;  // for tracing
   ulong rawtype;  // for tracing
   ulong rawnlnk;  // link count
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

   long addEntry        (Coi &ocoi, long nAtPos=-1);
   // adds a COPY of the supplied coi.

   long removeEntry     (long nAtPos);
   long numberOfEntries ( );
   Coi  *getEntry       (long iIndex, int nTraceLine);
   long  setEntry       (long iIndex, Coi *pcoi);
   long addSorted       (Coi &ocoi, char cSortedBy, bool bUseCase);
   void resetEntries    ( );
   bool isSet           (long iIndex);
private:
   long expand          (long nSoMuch);
   long nClArraySize;
   long nClArrayUsed;
   Coi  **apClArray;
};

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
   ulong  ntold;         // warnings told about this file
   char   szlasterr[50]; // most recent error info
   bool   banyread;      // anything yet read after open()?

   // this buffer is for high-level Coi read functions:
   // -  readLine()
   // -  isBinaryFile()
   // it shall NOT be used by low-level read functions.
   struct CoiReadBuf {
      uchar  *data;     // alloc'ed on demand
      long    getsize;
      long    getindex;
      long    geteod;
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
};

class CoiAutoDelete {
public:
      CoiAutoDelete (Coi *pcoi, bool bDecRef)
         { pClCoi = pcoi; bClDecRef = bDecRef; }
     ~CoiAutoDelete ( ) {
         if (bClDecRef)
            pClCoi->decref();
         if (!pClCoi->refcnt())
            delete pClCoi; 
      }
private:
      Coi *pClCoi;
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
   void  setWidth        (long nColumns);
   void  setAddInfoWidth (long nColumns); // abs. columns, high prio
   void  setAddInfoHalve ( );  // fills half of line, with low prio
   void  setAddInfo      (const char *pszFormat, ...);
   void  setAction       (char *pszVerb, char *pszSubject, char *pszAddInfo=0, long nKeepFlags=0);
   void  setStatus       (char *pszVerb, char *pszSubject, char *pszAddInfo=0, long nKeepFlags=0);
   void  print           ( );  // print status now, keep line
   void  printLine       (long nFilter=0); // print final status, including newline
   void  cycle           ( );  // print status if enough time elapsed
   void  clear           ( );  // clear status, if it was printed
   long  print           (const char *pszFormat, ...);
   void  setProgress     (num nMax, num nCur, char *pszUnit);
   void  setStatProg     (char *pverb, char *psubj, num nMax, num nCur, char *pszUnit);

private:
   void  fixAddInfoWidth ( );
   void  dumpTermStatus  ( );
   void  clearTermStatus ( );  // if anything to clear
   long  nMaxChars;
   long  nMaxSubChars;
   long  nAddInfoCols;
   long  nDumped;
   num   nLastDumpTime;
   bool  bAddInfoPrio;
   long  nAddInfoReserve;
   long  nTurn;
   char  szVerb   [200];
   char  szSubject[200];
   char  szAddInfo[200];
   char  szTermBuf[400];
   char  szPrintBuf[400];
   char  szPerc   [20];
};

extern ProgressInfo info;

long isDir(char *pszName);
char *sfkLastError();

#define MAX_ABBUF_SIZE 100000
#define MAX_LINE_LEN 4096

extern long (*pGlblJamFileCallBack)(char *pszFilename, num &rLines, num &rBytes);
extern long (*pGlblJamLineCallBack)(char *pszLine, long nLineLen, bool bAddLF);
extern long (*pGlblJamStatCallBack)(char *pszInfo, ulong nFiles, ulong nLines, ulong nMBytes, ulong nSkipped, char *pszSkipInfo);

char *findPathLocation(char *pszCmd, bool bExcludeWorkDir=0);
extern long fileExists(char *pszFileName, bool bOrDir=0);

#define strcopy(dst,src) mystrcopy(dst,src,sizeof(dst)-10)
void  mystrcopy      (char *pszDst, char *pszSrc, long nMaxDst);
long  mystrstri      (char *pszHayStack, char *pszNeedle, long *lpAtPosition = 0);
char *mystrrstr      (char *psrc, char *ppat);
long  mystrncmp      (char *psz1, char *psz2, long nLen, bool bCase=0);
long  mystricmp      (char *psz1, char *psz2);
long  mystrnicmp     (char *psz1, char *psz2, long nLen);
bool  strBegins      (char *pszStr, char *pszPat);
bool  striBegins     (char *pszStr, char *pszPat);
bool  strEnds        (char *pszStr, char *pszPat);
void  removeCRLF     (char *pszBuf);
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
long  timeFromString (char *psz, num &nRetTime);
void  doSleep        (long nmsec);
uchar *loadBinaryFile(char *pszFile, num &rnFileSize);
bool  infoAllowed    ( );

class IOStatusPhase {
public:
		IOStatusPhase	(char *pinfo);
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

extern void myclosesocket(SOCKET hsock, bool bread=1, bool bwrite=1);
