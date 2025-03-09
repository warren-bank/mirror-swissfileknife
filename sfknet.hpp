
#ifdef USE_DCACHE

// area one, ordered by namesum
struct DMetaEntry {
   num   sumhi;
   num   sumlo;
   num   size;
   num   time;
};

// area two, ordered by fifo
struct DFifoEntry {
   num   sumhi;
   num   sumlo;
};

#endif // USE_DCACHE

// map of managed keys and MANAGED Cois.
// entries can also be read in a fifo way.
class CoiMap : public KeyMap {
public:
      CoiMap   ( );
     ~CoiMap   ( );

   void  reset (bool bWithDiskCache=0);
   // expects: all coi entries have zero references.
   // if not, errors are dumped.
   // deletes all coi entries.

   int  put   (char *pkey, Coi *pTransferCoi, const char *pTraceFrom, int nmode=0);
   // enter the Coi under that key.
   // the Coi is NOT COPIED.
   // Coi ownership is TRANSFERED to the CoiMap.
   // the Coi may be DELETED anytime if it's
   // reference counter reaches zero.

   Coi *get    (char *pkey);
   // CALLER MUST RELEASE (decref) after use!
   // if a value was stored then it is returned.
   // the Coi is NOT COPIED.
   // if not, null is returned, no matter if the key is set.

   int  remove(char *pkey);
   // remove entry with that key, if any.
   // associated Coi must have zero refs.
   // associated Coi is deleted.
   // rc: 0:done 1:no_such_key >=5:error.

   num   byteSize(bool bCalcNow=0);
   // approximate added bytesize of all objects.
   // calcnow forces immediate, expensive calculation.

   num   bytesMax       ( );  // peak memory use
   num   filesDropped   ( );  // files dropped during processing

   int  tellByteSizeChange(Coi *pcoi, num nOldSize, num nNewSize);
   // if pcoi is in the cache, this method quickly
   // adapts the overall byte count.

protected:
   int  putDiskCache(char *pkey, Coi *pcoi, const char *pTraceFrom);
   int   bfindDMeta(num nsumlo,num nsumhi,int &rindex);

   List  clFifo;
   num   nClByteSize;   // approximately
   num   nClBytesMax;   // peak memory use
   num   nClDropped;    // files dropped during processing

   #ifdef USE_DCACHE
   int        nClDAlloc;
   int        nClDUsed;
   DMetaEntry *apClDMeta;
   DFifoEntry *apClDFifo;
   #endif // USE_DCACHE
};

class TCPCore;
class Coi;
class CoiTable;

// optional class: derive own objects from this
// and register with TCPCon's to associate own
// data with connections.
class TCPConData
{
public:
            TCPConData  ( );
   virtual ~TCPConData  ( ) = 0;
};

class TCPCon
{
public:
      TCPCon   (SOCKET hsock, TCPCore *pcorein, int nTraceLine=0);
     ~TCPCon   ( );

   // ===== data I/O functions =====
   void  setBlocking (bool bYesNo);

   int  read     (uchar *pblock, uint nlen);
   // read raw block of bytes. this will block
   // until nlen bytes are read, or connection is closed.
   // returns <= 0 on connection close, else nlen.
   
   int  send     (uchar *pBlock, uint nLen);

   char *readLine (char *poptbuf=0, uint noptmaxbuf=0);
   // read()'s char by char until CRLF is reached.
   // returns supplied buffer, or NULL on EOD/close.
   // if no buffer is supplied, provides internal buffer.
   // returned lines are stripped from CRLF.
   // SHARE: may share buffer with putf.

   // raw text I/O, sends all CRLF as supplied by caller:
   int  puts  (char *pline);
   int  putf  (cchar *pmask, ...);
   // SHARE: may share buffer with readLine.

   // ftp and http protocol support
   int  setProtocol (char *psz);   // "ftp", "http"
   int  readReply   (int nMinRC, int nMaxRC);
   // reads a reply like "200 OK", "HTTP/1.1 200 OK",
   // checks RC if it's within range, returns 0 if so.

// private:
   TCPCore &core  ( );
   void  rawClose ( );
   char *buffer   (int &rbufsize);

   TCPCore     *pClCore;
   SOCKET       clSock;
   TCPConData  *pClUserData;  // optional
   char        *pClIOBuf;     // alloc'ed on demand
   static int  nClIOBufSize; // if it is alloc'ed

   int   nClTraceLine;
   num   nClStartTime;

friend class TCPCore;
};

// basic tcp i/o support.
class TCPCore
{
public:
      TCPCore  (char *pszID="");
     ~TCPCore  ( );

   char  *getID   ( );

   // windows only:
   static int sysInit  ( );
   // run TCPCore::sysInit() before any tcp transfer

   static void sysCleanup  ( );
   // run this last before exiting the application

   void  shutdown ( );
   // close everything, delete all user objects
   // associated with connections (if any).
   // windows only: if bSysCleanup is set, runs also
   // WSACleanup() to cleanup the windows TCP stack.

   int makeServerSocket (int nport, TCPCon **ppout);
   // returned object is managed by TPCCore.
   // returned server socket is added to readfds set.

   int accept (TCPCon *pServerCon, TCPCon **ppout);
   // returned object is managed by TPCCore.
   // returned client socket is added to core's readfds set.

   int connect(char *phostorip, int nport, TCPCon **ppout);
   // returned object is managed by TPCCore.
   // client socket is added to readfds set.

   int  close    (TCPCon *pcon);
   // closes connection, deletes associated objects, if any.
   // DELETES supplied pcon.

   int  lastError   ( );
   // system errno of last operation. depends on call if it's set.

   // for a centralized read and processing thread:
   int  selectInput (TCPCon **pNextActiveCon, TCPCon *pToQuery=0);
   // from all sockets pending input, returns the first one
   // that received an accept/connect or read event.
   // if pToQuery is given, returns that pointer and rc0
   // if exactly that connection has input pending.
   // rc0=OK_active_socket_set rc1=nothing_to_do >=5:fatal error

   // optional: register a socket with a user-supplied object
   // derived from TCPConData.
   // -  the object must be created by the user through "new".
   // -  it is deleted by SFKTCP on close() or shutdown().
   int  registerData(TCPCon *pcon, TCPConData *pUserData);

   void  setVerbose  (bool bYesNo);
   // dump sent/received control lines to terminal

   bool  verbose     ( );

   // reference counting:
   int  incref   ( );  // increment refcnt
   int  decref   ( );  // decrement refcnt
   int  refcnt   ( );  // current no. of refs

   // close all open connections:
   void  closeConnections  ( );

   // set basic proxy infos, may be used by derived classes only.
   static int    setProxy (char *phost, int nport);

protected:
   int  checksys ( );
   void  wipe     ( );
   int  addCon   (TCPCon *pcon);   // add to fdset
   int  remCon   (TCPCon *pcon);   // remove from fdset

   char   *pClID;
   int    nClCon;     // must be < FD_SETSIZE

   // TODO: check sizes of these. maybe make them dynamic ptrs,
   //       to assure a tcp core still suits on a stack.
   TCPCon *aClCon[FD_SETSIZE+2];
   fd_set  clReadSet; // all sockets pending accept and read
   fd_set  clSetCopy;
   bool    bClVerbose;
   int    nClRefs;

   static  char szClProxyHost[100];
   static  int  nClProxyPort;
   static  bool  bClProxySet;

   static  bool bSysInitDone;
};

// class to auto close a connection,
// AND to automatically clear it's pointer.
class ConAutoClose {
public:
      ConAutoClose   (TCPCore *pcore, TCPCon **pcon, int nTraceLine);
     ~ConAutoClose   ( );
private:
      int     nClTraceLine;
      TCPCore  *pClCore;
      TCPCon  **ppClCon;
};

class CharAutoDelete {
public:
      CharAutoDelete (char **pdata) { ppClPtr = pdata; }
     ~CharAutoDelete ( ) { if (*ppClPtr) delete [] *ppClPtr; }
private:
      char **ppClPtr;
};

// TODO: rearrange logic?
// -  one ftpclient instance per host
// -  this ftpclient must remember the ftp://hostname
// -  then list() returns absolute names based on this
// UNCLEAR: when to logout on such a client
class FTPClient : public TCPCore
{
public:
      FTPClient(char *pszID);
     ~FTPClient( );

   int   login (char *phostorip, int nport=21);
   // anonymous ftp only, so far.

   int   loginOnDemand(char *phostorip, int nport);
   // does nothing if already logged in

   void   logout( );

   int   openFile   (char *pfilename, cchar *pmode);
   // begin download of a file with a relative path,
   // e.g. subdir/thefile.txt

   int   readFile   (uchar *pbuf, int nbufsize);
   // read block from currently open file

   void   closeFile  ( );
   // close currently open fle

   int   list (char *pdir, CoiTable **pptable, char *pRootURL=0);
   // NOTE: if pptable is the address of a NULL pointer,
   //       a CoiTable will be created and returned.
   // NOTE: if pptable points to an existing CoiTable,
   //       file and dir entries will be filled therein.
   // returned CoiTable is always managed by caller.
   // RootURL: if supplied, every filename is prefixed by that,
   //          e.g. "ftp://thehost/thedir/"

   int  setPassive  (TCPCon **ppout);

   char  *line    ( );  // line i/o buffer
   int   lineMax ( );  // max size of that buffer

private:
   int  readLine (TCPCon *pcon);
   int  readLong (TCPCon *pcon, uint &rOut, cchar *pszInfo);
   int  sendLong (TCPCon *pcon, uint nOut, cchar *pszInfo);
   int  readNum  (TCPCon *pcon, num &rOut, cchar *pszInfo);
   int  readNum  (uchar *pbuf, int &roff, num &rOut, cchar *pszInfo);
   int  sendNum  (TCPCon *pcon, num nOut, cchar *pszInfo);

   void  wipe     ( );
   int  splitURL (char *purl);
   char *curhost  ( );  // from spliturl
   char *curpath  ( );  // from spliturl
   int  curport  ( );  // from spliturl
   int  sendLine (char *pline);
   char *readLine ( );
   int  readReply(char **ppline);

   TCPCon   *pClCtlCon;        // control connection
   TCPCon   *pClDatCon;        // data connection, if separate
   char      aClURLBuf[300];   // current hostname, path
   char     szClLineBuf[1000]; // for file i/o
   int      nClPort;          // default is 21
   char     *pClCurPath;       // within URLBuf

   bool      bClLoggedIn;
   bool      bClSFT;
   int      nClSFTVer;
   char     *pszClHost;

   struct   CurrentIOFile {
      char   *name;
      bool    blockmode;   // else bulk
      int    block;       // read block number
      num     time;
      num     size;
      num     remain;
      uchar   abmd5[20];
      SFKMD5 *md5;
   }  file;

   // reply string to PASV. if set, we're in passive mode,
   // receiving one or more downloads.
   char     *pClPassive;

friend class Coi;
};

class HTTPClient : public TCPCore
{
public:
      HTTPClient(char *pszID);
     ~HTTPClient( );

   int   open (char *purl, cchar *pmode, Coi *pcoi);
   // begin download from an url.
   // NOTE: pcoi's name might be changed on redirect.

   int   read (uchar *pbuf, int nbufsize);
   // read block from currently open url stream.
   // chunked transfer is managed transparently.

   void   resetCache( );
   // use between access to two url's.

   void   close( );
   // close currently open url.

   int   getFileHead   (char *purl, Coi *pout, cchar *pinfo="");
   // read header infos like content-type and size.
   // the provided Coi is filled.

   int   list (char *purl, CoiTable **pptable);
   // NOTE: if pptable is the address of a NULL pointer,
   //       a CoiTable will be created and returned.
   // NOTE: if pptable points to an existing CoiTable,
   //       file and dir entries will be filled therein.
   // returned CoiTable is always managed by caller.

   int  loadFile (char *purl, num nMaxSize, uchar **ppout, num &routlen);
   // returned pout is managed by caller.
   // returned pout is zero terminated.

   int  sendReq  (cchar *pcmd, char *pfile, char *phost, int nport);
   // requires an open connection

   int  connectHttp    (char *phostorip, int nport, TCPCon **ppout);
   // returned object is managed by TPCCore.
   // same as connect() but with optional proxy handling.

private:
   void  wipe     ( );

   int  readraw  (uchar *pbuf, int nbufsize);
   // read block without any chunk handling.

   int  splitURL (char *purl);
   char *curhost  ( );  // from spliturl
   char *curpath  ( );  // from spliturl

   int  joinURL  (char *pabs, char *pref);
   char *curjoin  ( );  // from joinurl

   int  splitHeader (char *phead);
   char *curname  ( );  // of header entry
   char *curval   ( );  // of header entry

   int  sendLine (char *pline);
   char *readLine ( );

   int  rawReadHeaders (int &rwebrc, char *purlinfo, Coi *pOptCoi=0); // , StringMap *pheads=0);
   // returns sfk rc, 0 (OK) ... 9 (fatal error)
   // returns also webrc, e.g. 200 (OK) or 302 (redirect)

   int      nClPort;

   TCPCon   *pClCurCon; // current i/o connection

   char      aClURLBuf[300];  // current hostname, path
   char     *pClCurPath;      // within URLBuf

   char      aClHeadBuf[200]; // current header line
   char     *pClCurVal;       // within HeadBuf

   char      aClJoinBuf[300]; // join: recombined url

   // Header-Free Servers: first reply line cache
   char     *pszClLineCache;  // only for first line

   // Transfer-Encoding: chunked support
   bool      bClChunked;
   uchar    *pClCache;        // alloc'ed on demand
   int      nClCacheAlloc;
   int      nClCacheUsed;    // remaining unread bytes

   // is http server blocking head requests?
   bool      bClFirstReq;
   bool      bClNoHead;

   // if set, no connections are possible:
   bool      bClNoCon;

friend class Coi;
};

class ConCache : public KeyMap
{
public:
      ConCache   ( );
     ~ConCache   ( );

      // creates clients on demand. returned clients
      // - are still MANAGED BY THE CACHE
      // - must be released after use
      HTTPClient  *allocHttpClient(char *pBaseURL);
      FTPClient   *allocFtpClient (char *pBaseURL);
      // returns NULL if no locked con is available

      // must be called after temporary use,
      // e.g. download of a single web file:
      int  releaseClient (HTTPClient *p);
      int  releaseClient (FTPClient  *p);

      void  closeConnections  ( );
      void  reset ( );

private:
      void  closeAndDelete(void *praw);
      // caller MUST remove() manually after that!

      int  forceCacheLimit( );

      // to remember fifo sequence:
      // fifo list with managed key strings
      num       nAddCnt;
      StringMap clFifo;
};

// implemented in sfknet.cpp, cleanup in sfk.cpp:
extern ConCache glblConCache;
extern CoiMap glblVCache;
extern KeyMap glblCircleMap;


