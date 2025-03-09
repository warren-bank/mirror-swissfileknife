/*
   swiss file knife base classes
*/

class StringTable {
friend class Array;
public:
   StringTable          ( );
  ~StringTable          ( );
   long addEntry        (char *psz);
   long numberOfEntries ( );
   char *getEntry       (long iIndex, int nTraceLine);
   long  setEntry       (long iIndex, char *psz);
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
   void  shutdown       ( );
   bool  hasRoot        (long iIndex);
   char* setCurrentRoot (long iIndex);
   char* getCurrentRoot ( );
   long  numberOfRootDirs ( );
   Array &rootDirs      ( ) { return clRootDirs; }
   Array &dirMasks      ( ) { return clDirMasks; }
   Array &fileMasks     ( ) { return clFileMasks; }
   bool  anythingAdded  ( );
   void  dump           ( );
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

long isDir(char *pszName);
char *sfkLastError();

#define MAX_ABBUF_SIZE 100000
#define MAX_LINE_LEN 4096

bool sfkisalnum(uchar uc);
bool sfkisprint(uchar uc);

extern long (*pGlblJamLineCallBack)(char *pszLine, long nLineLen, bool bAddLF);
extern long (*pGlblJamStatCallBack)(char *pszInfo, ulong nFiles, ulong nLines, ulong nMBytes, ulong nSkipped, char *pszSkipInfo);
