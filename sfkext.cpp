/*
   1.5.4
   -  fix: missing connection close in Coi::rawCloseHttpSubFile

   1.5.1
   -  add: find  : support for zipfile reading.
   -  add: filter: support for zipfile reading.
   -  add: list  : listing of zip's embedded in zip's.
*/

#define USE_DCACHE
#define USE_SFT
#define REDUCE_CACHE_FILE_NAMES

#include "sfkbase.hpp"

// just close on a socket is not enough.
// myclosesocket also does the shutdown().
#define closesocket myclosesocket

#ifdef _WIN32
 #ifdef SFK_MEMTRACE
  #define  MEMDEB_JUST_DECLARE
  #include "memdeb.cpp"
 #endif
#endif

#include "sfkext.hpp"

#ifndef SO_REUSEPORT
 #define SO_REUSEPORT 15
#endif

extern unsigned char abBuf[MAX_ABBUF_SIZE+100];

int perr(const char *pszFormat, ...);
int pwarn(const char *pszFormat, ...);
int pinf(const char *pszFormat, ...);
int printx(const char *pszFormat, ...);
int esys(const char *pszContext, const char *pszFormat, ...);
int myfseek(FILE *f, num nOffset, int nOrigin);
bool endsWithExt(char *pname, char *pszextin);

bool endsWithArcExt(char *pname, int iTraceFrom);
extern cchar *arcExtList[];
void tellMemLimitInfo();
int quietMode();
char *getHTTPUserAgent();
int createOutDirTree(char *pszOutFile);
size_t myfwrite(uchar *pBuf, size_t nBytes, FILE *fout, num nMaxInfo=0, num nCur=0, SFKMD5 *pmd5=0);
int getTwoDigitHex(char *psz);
int sfkmemcmp2(uchar *psrc1, uchar *psrc2, num nlen, bool bGlobalCase, uchar *pFlags);
void myfgets_init    ( );
int myfgets         (char *pszOutBuf, int nOutBufLen, FILE *fin, bool *rpBinary=0, char *pAttrBuf=0);
extern char *ipAsString(struct sockaddr_in *pAddr, char *pszBuffer, int iBufferSize, uint uiFlags=0);

extern bool bGlblEscape;
extern num  nGlblMemLimit;

extern char szLineBuf[MAX_LINE_LEN+10];
extern char szLineBuf2[MAX_LINE_LEN+10];
extern char szLineBuf3[MAX_LINE_LEN+10];
extern char szAttrBuf[MAX_LINE_LEN+10];
extern char szAttrBuf2[MAX_LINE_LEN+10];
extern char szAttrBuf3[MAX_LINE_LEN+10];

#ifndef USE_SFK_BASE
UDPIO sfkNetIO;
UDPIO sfkNetSrc;
#endif // USE_SFK_BASE

#ifdef VFILEBASE

// vfile processing caches
CoiMap glblVCache;      // downloaded zip's cache
ConCache glblConCache;  // ftp and http connection cache

#define COI_CAPABILITY_NET (1UL<<0)
uint nGlblCoiConfig = 0xFFFFUL;

class DiskCacheConfig {
public:
   DiskCacheConfig ( );

   char *getPath   ( );
   int  setPath   (char *ppath);

   bool  getActive ( );
   void  setActive (bool byesno);

   char szPath[SFK_MAX_PATH+10];
   bool bactive;
};

DiskCacheConfig::DiskCacheConfig() 
{
   memset(this, 0, sizeof(*this));
   // is active by default,
   // but TEMP will be accessed on demand.
   bactive = 1;
}

// guarantees non-void pointer
char *DiskCacheConfig::getPath( ) 
{
   if (szPath[0]) return szPath;

   char *ptmp = getenv("TEMP");

   if (!ptmp)
         ptmp = getenv("TMP");

   if (!ptmp) {
      bactive = 0;
      static bool btold1 = 0;
      if (!btold1) {
         btold1 = 1;
         if (infoAllowed())
            pinf("cannot cache to disk: no TEMP variable found.\n"); 
      }
      return szPath; // i.e. return empty
   }

   // set: "c:\temp\00-sfk-cache\"
   snprintf(szPath, sizeof(szPath)-10, "%s%c00-sfk-cache", ptmp, glblPathChar);

   /*
   static bool bFirstCall = 1;
   if (bFirstCall) {
      bFirstCall = 0;
      if (infoAllowed())
         pinf("using cache %s\n", szPath);
   }
   */

   return szPath;
}

int DiskCacheConfig::setPath(char *ppath)
{
   strcopy(szPath, ppath);

   char szSubName[100];
   sprintf(szSubName, "%c00-sfk-cache", glblPathChar);
   int nsublen = strlen(szSubName);

   // always force "00-sfk-cache" as subdir:
   int nlen = strlen(szPath);
   if (nlen >= nsublen && !strcmp(szPath+nlen-nsublen, szSubName))
      return 0;

   if (nlen > 0 && szPath[nlen-1] == '/' ) szPath[nlen-1] = '\0';
   if (nlen > 0 && szPath[nlen-1] == '\\') szPath[nlen-1] = '\0';

   nlen = strlen(szPath);
   int nrem = (sizeof(szPath)-10)-nlen;
   if (nrem < nsublen)
      return 9+perr("cache path too long: %s", ppath); 

   strcat(szPath, szSubName);

   return 0;
}

void DiskCacheConfig::setActive(bool byesno) { bactive = byesno; }
bool DiskCacheConfig::getActive( ) { return bactive; }

DiskCacheConfig glblDiskCache;

// optional query callback on first download
bool (*pGlblDiskCacheAskSave)() = 0;

void  setDiskCacheActive(bool b) { glblDiskCache.setActive(b); }
bool  getDiskCacheActive( )      { return glblDiskCache.getActive(); }
void  setDiskCachePath(char *p)  { glblDiskCache.setPath(p); }
// guarantees non-void pointer: on error, path is "".
char *getDiskCachePath( )        { return glblDiskCache.getPath(); }

CoiMap::CoiMap( ) 
{
   #ifdef USE_DCACHE_EXT
   nClDAlloc   = 0;
   nClDUsed    = 0;
   apClDMeta   = 0;
   apClDFifo   = 0;
   #endif // USE_DCACHE_EXT

   nClByteSize = 0;
   nClBytesMax = 0;
   nClDropped  = 0;

   reset();
}

CoiMap::~CoiMap( ) {
   reset();
}

void CoiMap::reset(bool bWithDiskCache)
{
   // delete all clFifo entries
   ListEntry *pcur  = clFifo.first();
   ListEntry *pnext = 0;
   for (;pcur; pcur=pnext)
   {
      Coi *pcoi = (Coi *)pcur->data;
      if (pcoi->refcnt() > 1) {
         if (!bGlblEscape) {
            pinf("cannot drop cache entry with %d refs: %s (%p)\n", pcoi->refcnt(), pcoi->name(), pcoi);
         }
      } else {
         nClByteSize -= pcoi->getUsedBytes();
         pcoi->decref();
         delete pcoi;
      }
      pnext = pcur->next();
      delete pcur;
   }
   // is now ready for flat reset
   clFifo.reset();

   extern int (*pGlblSFKStatusCallBack)(int nMsgType, char *pmsg);

   KeyMap::reset();
   if (nClByteSize != 0) {
      // TODO: so far, the bytesize is just an approximate value.
      nClByteSize = 0;
   } 

   #ifdef USE_DCACHE_EXT
   if (bWithDiskCache)
   {
      mtklog(("dcache.release alloc=%d used=%d",nClDAlloc,nClDUsed));
      if (apClDMeta) { delete [] apClDMeta; apClDMeta = 0; }
      if (apClDFifo) { delete [] apClDFifo; apClDFifo = 0; }
      nClDAlloc = 0;
      nClDUsed  = 0;
   }
   #endif // USE_DCACHE_EXT
}

int CoiMap::tellByteSizeChange(Coi *pcoi, num nOldSize, num nNewSize)
{
   if (!pcoi || !pcoi->bClInCache) return 9;

   nClByteSize += (nNewSize - nOldSize);

   if (nClByteSize > nClBytesMax)
       nClBytesMax = nClByteSize;

   return 0;
}

num CoiMap::byteSize(bool bCalcFromList)
{
   if (!bCalcFromList)
      return nClByteSize;

   // expensive recalc:
   int nelem  = size();
   num  nbytes = 0;
   for (int i=0; i<nelem; i++) {
      Coi *pcoi = (Coi*)iget(i);
      if (!pcoi) { perr("int. #108281302"); break; }
      if (pcoi->hasData())
         nbytes += pcoi->getUsedBytes();
   }
   return nbytes;
}

num CoiMap::bytesMax( )     { return nClBytesMax; }
num CoiMap::filesDropped( ) { return nClDropped; }

int CoiMap::put(char *pkey, Coi *pcoi, const char *pTraceFrom, int nmode)
{__
   if (!pkey) return 9+perr("int. #208290634");
   if (!pcoi) return 9+perr("int. #208290635");

   bool bSkipDiskCache = (nmode & 1) ? 1 : 0;

   mtklog(("cache: put %s from=%s coiname=%s", pkey, pTraceFrom, pcoi->name()));

	if (pcoi->bClInCache)
		return 1+pwarn("cache-put twice, ignoring: %s", pkey);

   num nAddSize = pcoi->getUsedBytes();

   // drop cache entries due to memlimit?
   while (nClByteSize + nAddSize > nGlblMemLimit)
   {
      // drop first entry with refcnt == 1.
      // scan first 1000 cache entries, then bail out.
      // list is CHANGED below, therefore have always to re-run from start.
      ListEntry *plx = clFifo.first();
      int nbail = 1000;
      for (; plx; plx = plx->next()) {
         Coi *pxcoi = (Coi*)plx->data;
         if (pxcoi && pxcoi->refcnt() <= 1) break;
         if (nbail-- <= 0) break;
      }

      if (!plx) {
         bool btold = 0;
         if (!btold) { btold = 1;
            pwarn("cache overflow (%d), try to increase -memlimit (current=%d).\n", (int)size(), (int)(nGlblMemLimit/1000000));
         }
      }
      else 
      {
         Coi *pxcoi = (Coi*)plx->data;
         if (!pxcoi) break;
   
         char *pxkey = pxcoi->name();
         // int nkeylen = strlen(pxkey);
   
         if (!quietMode()) 
         {
            char szAddInfo[100];
            snprintf(szAddInfo, sizeof(szAddInfo)-10, "%u files %u mb", size(), (uint)(nClByteSize/1000000UL));
            info.setAddInfoWidth(strlen(szAddInfo));
            info.setStatus("free", pxkey, szAddInfo, eKeepAdd);
            // pinf("cache-drop: %d %d %.50s\n", ndropmb, ncachmb, pxinfo);
         }
   
         mtklog(("cache-drop: %s", pxkey));
   
         // successful remove will DELETE pxcoi AND pxkey!
         if (remove(pxkey))
            { perr("int. #258282159 on cache drop"); break; }

         nClDropped++;
      }
   }

   // overwriting an existing cache entry?
   if (KeyMap::isset(pkey)) {
      // CoiMap::remove takes care of references
      mtklog(("cache-drop: %s (existing)", pkey));
      remove(pkey);
      nClDropped++;
   }

   int nrc = KeyMap::put(pkey, pcoi);
   if (!nrc) {
      // base put ok, add also in fifo list
      ListEntry *pentry = new ListEntry();
      pentry->data = pcoi;
      clFifo.add(pentry);
   }

   // INCREMENT THE REFCNT.
   pcoi->incref("cput");

   pcoi->bClInCache = 1;

   nClByteSize += pcoi->getUsedBytes();

   if (nClByteSize > nClBytesMax)
       nClBytesMax = nClByteSize;

   #ifdef USE_DCACHE
   // cache only net files
   if (   !bSkipDiskCache && pcoi->isNet()
       && pcoi->isKnownArc() && !pcoi->isZipSubEntry()
       && pcoi->hasContent()
      )
   {
      // no matter if active or not, if this is set
      if (pGlblDiskCacheAskSave)
      {
         // then ask back on first download
         bool nrc = pGlblDiskCacheAskSave();
         glblDiskCache.setActive(nrc);
         pGlblDiskCacheAskSave = 0;
      }

      if (glblDiskCache.getActive()) {
         putDiskCache(pkey, pcoi, pTraceFrom);
      } else {
         mtklog(("cache-put : no disk caching (active=%d cont=%d)", glblDiskCache.bactive, pcoi->hasContent()));
      }
   }
   else
   {
      mtklog(("cache-put: skip disk, bskip=%d isnet=%d isarc=%d issub=%d cont=%d",
         bSkipDiskCache, pcoi->isNet(), pcoi->isKnownArc(),
         pcoi->isZipSubEntry(), pcoi->hasContent()
         ));
   }
   #endif // USE_DCACHE

   return nrc;
}

int md5FromString(char *psz, num &rsumhi, num &rsumlo)
{
   SFKMD5 md5;
   md5.update((uchar*)psz, strlen(psz));
   uchar *pdig = md5.digest();
   num nlo=0, nhi=0;
   memcpy(&nhi, pdig+0, 8);
   memcpy(&nlo, pdig+8, 8);
   rsumhi = nhi;
   rsumlo = nlo;
   return 0;
}

#ifdef USE_DCACHE
char *Coi::cacheName(char *pnamein, char *pbuf, int nmaxbuf, int *pDirPartLen)
{__
   char *prel = pnamein;

   // only cache net files to disk.
   cchar *pprot = "";
   if (strBegins(prel, "http://"))
      { pprot="http"; prel += strlen("http://"); }
   else
   if (strBegins(prel, "ftp://"))
      { pprot="ftp"; prel += strlen("ftp://"); }
   else {
      // local file: do NOT cache to disk
      return 0;
   }

   //  in: http://foo.com/sub/dir/get.php?a=1&b=5&sess=c9352ff7
   // out: c:\temp\00-sfk-cache\foo.com\subdirget.phpa1b5sessc9352ff7
   char *pCacheDir = glblDiskCache.getPath();
   if (!pCacheDir) return 0; // error was told

   if (nmaxbuf < 100) {
      static bool btold2 = 0;
      if (!btold2) { btold2=1; pwarn("cannot cache to disk: name buffer too small.\n"); }
      return 0;
   }

   nmaxbuf -= 10; // safety

   // add: "c:\temp\00-sfk-cache" and "\"
   mystrcopy(pbuf, pCacheDir, nmaxbuf);
   strcat(pbuf, glblPathStr);

   // also add protocol to allow rebuild of url
   strcat(pbuf, pprot);
   strcat(pbuf, glblPathStr);

   char *pdst = pbuf + strlen(pbuf);
   int  nrem = nmaxbuf - strlen(pbuf);

   if (pDirPartLen) *pDirPartLen = strlen(pbuf);

   // prel: foo.com/sub/dir/get.php?a=1&b=5&sess=c9352ff7
   char *prel2 = strchr(prel, '/');
   if (!prel2) return 0; // wrong name format
   int ndomlen = prel2 - prel;
   prel2++;

   // prel2: sub/dir/get.php?a=1&b=5&sess=c9352ff7
   // add: "foo.com\"
   if (ndomlen >= nrem) {
      pwarn("cannot cache to disk, name too long: %s\n", pnamein); 
      return 0;
   }
   memcpy(pdst, prel, ndomlen);
   pdst += ndomlen; nrem -= ndomlen;
   *pdst++ = glblPathChar; nrem--;
   *pdst = '\0';

   // convert and add rest of url.
   // be very careful on name reductions,
   // to avoid mixup with archive sub entries
   // when retrieving stuff from the cache later.

   // http: $-_.+!*'(),
   char szBuf[10];
   char *psrc = prel2;
   while (*psrc && (nrem > 0)) 
   {
      char c = *psrc++;
      if (isalnum(c))
         { *pdst++ = c; nrem--; continue; }

      #ifdef REDUCE_CACHE_FILE_NAMES
      // if finding ".zip?parm=..." then stop here.
      // but make sure ".zip\\" or ".zip//" is never stripped,
      // to avoid mixup with sub entries later.
      if (c == '.') 
      {
         // scan for the longest .tar.gz etc. extension.
         // arcExtList is sorted by reverse length.
         for (int i=0; arcExtList[i]; i++) 
         {
            if (!mystrnicmp(psrc-1, arcExtList[i], strlen(arcExtList[i]))) 
            {
               // an extension match is found, but how does it continue?
               int nextlen = strlen(arcExtList[i]);
               if (nextlen < 1) break; // safety
               char *pcont  = psrc - 1 + nextlen;

               // never strip sub archive identifiers
               if (!strncmp(pcont, "\\\\", 2)) continue;
               if (!strncmp(pcont, "//", 2))   continue;

               memcpy(pdst, arcExtList[i], nextlen);
               pdst[nextlen] = '\0';

               // EARLY EXIT: archive extension found
               mtklog(("cache: built aext name: %s", pbuf));
               return pbuf;
            }
         }
      }
      #endif

      switch (c) 
      {
         case '-': case '+': case '.': case '_':
            *pdst++ = c; nrem--; continue;

         // case '_':
         //    *pdst++ = '-'; nrem--; continue;

         // case '/':
         // case '\\':
         //    *pdst++ = '_'; nrem--; continue;
      }
      sprintf(szBuf, "%%%02X", (unsigned)c);
      *pdst++ = szBuf[0];
      *pdst++ = szBuf[1];
      *pdst++ = szBuf[2];
      nrem -= 3;
   }
   *pdst = '\0';

   if (nrem <= 0) {
      pwarn("cannot cache to disk, name too long: %s\n", pnamein); 
      return 0;
   }
   
   return pbuf;
}

// make sure coi has cached data before calling this
int CoiMap::putDiskCache(char *pkey, Coi *pcoi, const char *pTraceFrom)
{__
   char szCacheName[SFK_MAX_PATH+10];

   uchar *pdata = pcoi->data().src.data;
   num    nsize = pcoi->data().src.size;

   if (!pdata) return 9+perr("int. 35291439");

   // the coi may have been redirected. in that case,
   // the dstname is different from the srcname.
   char *psrcname = pkey;
   char *pdstname = pcoi->name();

   int  nCacheDirLen = 0;
   char *pszCacheName = Coi::cacheName(pdstname, szCacheName, SFK_MAX_PATH, &nCacheDirLen);
   if (!pszCacheName) return 0;

   if (createOutDirTree(pszCacheName))
      return 5+pwarn("cannot create caching dir for: %s", pszCacheName);

   if (infoAllowed())
      pinf("saving %s\n", pszCacheName);

   FILE *fout = fopen(pszCacheName, "wb");
   if (!fout)
      return 5+pwarn("cannot write cache file: %s", pszCacheName);

   mtklog(("cache: disk: write pdata=%p size=%d", pdata, (int)nsize));

   if (myfwrite(pdata, nsize, fout) != nsize) {
      fclose(fout);
      return 5+pwarn("failed to write cache file, probably disk full: %s", pszCacheName);
   }

   fclose(fout);

   // if the coi was redirected, also store the source info.
   if (strcmp(psrcname, pdstname))
   {
      pszCacheName = Coi::cacheName(psrcname, szCacheName, SFK_MAX_PATH, &nCacheDirLen);
      if (!pszCacheName) return 0;
   
      if (createOutDirTree(pszCacheName))
         return 5+pwarn("cannot create caching dir for: %s", pszCacheName);
   
      if (infoAllowed())
         pinf("saving %s\n", pszCacheName);
   
      FILE *fout = fopen(pszCacheName, "wb");
      if (!fout)
         return 5+pwarn("cannot write cache meta file: %s", pszCacheName);

      fprintf(fout, "[sfk-cache-redirect]\n%s\n", pdstname);
   
      fclose(fout);
   }

   mtklog(("cache: put on disk: %s (%s)", pkey, pszCacheName));

   return 0;
}
#endif // USE_DCACHE

#ifdef USE_DCACHE_EXT
// rc =0:found_and_index_set
// rc <0:insert_before_index
// rc >0:insert_after_index
int CoiMap::bfindDMeta(num nsumlo,num nsumhi,int &rindex)
{__
   // binary search for key, or insert position
   uint nbot=0,ndist=0,nhalf=0,imid=0;
   uint ntop=nClArrayUsed; // exclusive

   num   ntmphi=0,ntmplo=0;

   int    ncmp=-1;   // if empty, insert before index 0

   while (1)
   {
      if (nbot > ntop) // shouldn't happen
         { perr("int. 187281850"); ncmp=-1; break; }

      ndist = ntop - nbot;
      // mtklog(("dist %d bot %d top %d",ndist,nbot,ntop));
      if (ndist == 0) break; // nothing left
      nhalf = ndist >> 1;
      imid  = nbot + nhalf;

      ntmphi= apClDMeta[imid].sumhi;
      ntmplo= apClDMeta[imid].sumlo;

      // 128 bit comparison
      if (nsumhi < ntmphi) ncmp = -1;
      else
      if (nsumhi > ntmphi) ncmp =  1;
      else
      if (nsumlo < ntmplo) ncmp = -1;
      else
      if (nsumlo > ntmplo) ncmp =  1;
      else
         ncmp = 0;

      if (ncmp < 0) {
         // select lower half, if any
         // mtklog((" take lower %xh %xh %d",nval,ntmp,imid));
         if (ntop == imid) break; // safety
         ntop = imid;
      }
      else
      if (ncmp > 0) {
         // select upper half, if any
         // mtklog((" take upper %xh %xh %d",nval,ntmp,imid));
         if (nbot == imid+1) break; // required
         nbot = imid+1;
      } else {
         // straight match
         mtklog(("%d = indexof(%xh) used=%u",imid,(uint)nsumhi,nClDAlloc));
         break; // found
      }
   }

   rindex = imid;
   return ncmp;
}

int CoiMap::putDiskCache(char *pkey, Coi *pcoi)
{__
   int nfree = nClDAlloc - nClDUsed;
   if (nfree < 10) {
      // expand arrays: alloc new
      int nAllocNew = nClDAlloc + (nClDAlloc ? nClDAlloc : 100);
      DMetaEntry *pdnew = new DMetaEntry[nAllocNew+10];
      DFifoEntry *pfnew = new DFifoEntry[nAllocNew+10];
      if (nClDUsed) {
         memcpy(pdnew, apClDMeta, sizeof(DMetaEntry)*nClDUsed);
         memcpy(pfnew, apClDFifo, sizeof(DFifoEntry)*nClDUsed);
      }
      // then free old and swap
      delete [] apClDMeta;
      delete [] apClDFifo;
      apClDMeta = pdnew;
      apClDFifo = pfnew;
      nClDAlloc = nAllocNew;
      nfree = nClDAlloc - nClDUsed;
   }
   // put basic infos into (mem) cached meta data
   num nsumlo=0,nsumhi=0;
   md5FromString(pkey,nsumlo,nsumhi);

   // find insert position
   int nindex = 0;
   int nrc = bfindDMeta(nsumlo,nsumhi,nindex);
   if (!nrc) return 1;  // already in cache

   if (nrc < 0) {
      // insert before index
   } else {
      // insert after index
      nindex++;
   }
   int ntomove = nClDUsed - nindex;
   if (ntomove > 0)
      memmove(&apClDMeta[nindex+1], &apClDMeta[nindex+0],
         sizeof(DMetaEntry) * ntomove);
   // now, set at index
   DMetaEntry *pdmet = &apClDMeta[nindex];
   pdmet->sumhi = nsumhi;
   pdmet->sumlo = nsumlo;
   pdmet->size  = pcoi->getSize();
   pdmet->time  = pcoi->getTime();
   // append key entry to fifo
   DFifoEntry *pdfif = &apClDFifo[nClDUsed];
   pdfif->sumhi = nsumhi;
   pdfif->sumlo = nsumlo;
   // finally, count new entry
   nClDUsed++;
   mtklog("dcache.insert at %d of %d: sumlo=%s key=%s", nindex, nClDUsed,
      numtohex(nsumlo), pkey);
   return 0;
}
#endif // USE_DCACHE_EXT

// caller MUST release object after use!
Coi *CoiMap::get(char *pkey) 
{__
   Coi *pcoi = (Coi*)KeyMap::get(pkey);
   if (pcoi) {
      pcoi->incref("cget");
      mtklog(("cache: %p = cache.get( %s )",pcoi,pkey));
      return pcoi;
   }

   #ifdef USE_DCACHE

   // disk cache is yet used only for net files
   if (!strBegins(pkey, "http://") && !strBegins(pkey, "ftp://"))
      return 0;

   // not yet in mem cache, but maybe in disk cache?
   if (!glblDiskCache.bactive) return 0;

   mtklog(("cache: cache.get( %s ) begin",pkey));

   char szCachePathBuf[SFK_MAX_PATH+10];
   char szCacheReDirBuf[SFK_MAX_PATH+10];
   char *pszDiskCacheFile = Coi::cacheName(pkey, szCachePathBuf, SFK_MAX_PATH, 0);

   if (!pszDiskCacheFile) {
      mtklog(("cache: no disk name: %s",pkey));
      return 0;
   }

   num nFileSize = getFileSize(pszDiskCacheFile);
   if (nFileSize < 0) {
      return 0;
   }

   // reject cache files beyond the memory limit
   if (nFileSize > nGlblMemLimit) {
      pinf("cache file too large, cannot load: %s\n", pszDiskCacheFile);
      return 0;
   }

   if (infoAllowed())
      pinf("using cache file %s\n", pszDiskCacheFile);

   // exists in disk cache: create a memory coi in mem cache.
   uchar *pdata = loadBinaryFile(pszDiskCacheFile, nFileSize);
   if (!pdata) {
      pinf("failed to load cache file: %s\n", pszDiskCacheFile);
      return 0;
   }

   mtklog(("cache: loaded, size=%d: %s",(int)nFileSize,pszDiskCacheFile));

   if (strBegins((char*)pdata, "[sfk-cache-redirect]"))
   {
      // have to load another file with actual content
      char *pdstname = (char*)pdata + strlen("[sfk-cache-redirect]");
      while (*pdstname && (*pdstname != '\n')) pdstname++;
      if (*pdstname) pdstname++;
      char *psz2 = pdstname;
      while (*psz2 && *psz2 != '\r' && *psz2 != '\n') psz2++;
      *psz2 = '\0';

      strcopy(szCacheReDirBuf, pdstname);
      pdstname = szCacheReDirBuf;

      // target name isolated, clear temporary:
      delete [] pdata; pdata = 0;
      // ptr will be reused immediately

      // now holding the virtual filename in pdstname (http://...)
      // build cache name from that.
      pszDiskCacheFile = Coi::cacheName(pdstname, szCachePathBuf, SFK_MAX_PATH, 0);
      if (!pszDiskCacheFile) {
         mtklog(("cache: no cache name for: %s", pdstname));
         return 0;
      }

      pinf("using cache file %s\n", pszDiskCacheFile);

      pdata = loadBinaryFile(pszDiskCacheFile, nFileSize);
      if (!pdata) {
         pinf("failed to load cache file: %s\n", pszDiskCacheFile);
         return 0;
      }

      mtklog(("cache: loaded redir, size=%d: %s",(int)nFileSize,pszDiskCacheFile));
   }

   // the cacheName function must make sure that keys for subentries
   // never return a cache file name pointing to the overall .zip.
   // i.e. if the name is reduced, reduction shall not strip sub infos,
   // otherwise we retrieve wrong data (and coi names) here.
   pcoi = new Coi(pszDiskCacheFile, 0);

   pcoi->setContent(pdata, nFileSize);

   mtklog(("cache: put entry: key=%s", pkey));
   mtklog(("cache: put entry: coi=%s", pcoi->name()));

   put(pkey, pcoi, "dcache", 1); // skipping disk cache write
   // TODO: what to do if memCachePut fails after diskCacheLoad?

   // put() sets the refcnt to 1.
   // but as get() was originally called,
   // we must return refcnt==2:
   pcoi->incref("cget2");

   #endif // USE_DCACHE

   return pcoi;
}

int CoiMap::remove(char *pkey) 
{
   // get coi, need it for fifo search
   // do NOT use CoiMap::get() as it increments the ref!
   Coi *pcoi = (Coi*)KeyMap::get(pkey);
   if (!pcoi) return 1; // not found

   // remove map entry
   int nrc = KeyMap::remove(pkey);

   // remove from fifo list as well
   ListEntry *plx = clFifo.first();
   for (;plx; plx=plx->next())
      if (plx->data == pcoi)
         break;

   // no list entry should NOT happen
   if (plx) {
      clFifo.remove(plx);
      delete plx;
      plx = 0; 
   } else {
      perr("int. 187282050");
   }

   // finally, delete the managed coi
   if (pcoi->refcnt() > 1) {
      if (!bGlblEscape) {
         pinf("cannot drop cache entry with %d refs: %s", pcoi->refcnt(), pcoi->name());
      }
   }
   else 
   {
      nClByteSize -= pcoi->getUsedBytes();

      pcoi->bClInCache = 0;

      pcoi->decref();

      delete pcoi;
   }

   return nrc;
}

bool TCPCore::bSysInitDone = 0;

ConAutoClose::ConAutoClose(TCPCore *pcore, TCPCon **ppcon, int nTraceLine) 
{
   nClTraceLine = nTraceLine;
   pClCore  =  pcore;
   ppClCon  = ppcon;
}

ConAutoClose::~ConAutoClose( ) {
   TCPCon *pcon = *ppClCon;
   if (pcon) {
      // closes con AND deletes the object
      pClCore->close(pcon);
      *ppClCon = 0;
   } else {
      perr("unexpected NULL ptr on ConAutoClose, line %d", nClTraceLine);
   }
}

ConCache::ConCache( ) {
}

ConCache::~ConCache( ) {
   reset();
}

void ConCache::closeConnections( ) 
{__
   // close all connections, but keep
   // the http/ftp clients available.
   void *praw=0;
   for (int i=0; i<size(); i++) {
      if ((praw = iget(i))) {
         TCPCore *pcore = (TCPCore *)praw;
         pcore->closeConnections();
      }
   }
}

void ConCache::reset( ) 
{__
   mtklog(("concache-reset %d", size()));

   // does also closeConnections
   void *praw=0;
   for (int i=0; i<size(); i++) 
      if ((praw = iget(i)))
         closeAndDelete(praw);

   // remove all invalid entries from keymap:
   KeyMap::reset();
   clFifo.reset();
}

void ConCache::closeAndDelete(void *praw)
{__
   // identify type of praw:
   TCPCore *pcore = (TCPCore *)praw;

   char *pszID = pcore->getID();

   if (pcore->refcnt() > 1) {
      perr("connection in use, cannot close: %s %p %d", pszID, pcore, pcore->refcnt());
      return;
   }

   if (strBegins(pszID, "http://")) {
      mtklog(("close-and-delete %p %s", praw, pszID));
      HTTPClient *pcl = (HTTPClient*)praw;
      pcl->close();  // in case curcon is active
      pcl->closeConnections();
      delete pcl;
   }
   else
   if (strBegins(pszID, "ftp://")) {
      mtklog(("close-and-delete %p %s", praw, pszID));
      FTPClient *pcl = (FTPClient*)praw;
      pcl->logout(); // in case we're still logged in
      pcl->closeConnections(); // if any remaining
      delete pcl;
   }
   else
      perr("wrong cache entry type: %s %p", pcore->getID(), pcore);
   // stored value pointer is now INVALID,
   // but we're cleanup up all below
}

HTTPClient *ConCache::allocHttpClient(char *pBaseURL) 
{__
   if (!strBegins(pBaseURL, "http://")) return 0;

   void *praw = get(pBaseURL);

   // use existing client?
   if (praw) {
      mtklog(("concache-reuse %s %p", pBaseURL, praw));
      HTTPClient *pres = (HTTPClient *)praw;
      if (pres->refcnt() > 1) {
         perr("too many locks on connection: %s (%d)", pBaseURL, pres->refcnt());
         return 0;
      }
      pres->incref();
      return pres;
   }

   mtklog(("concache-alloc %s", pBaseURL));

   // create new, on demand:
   forceCacheLimit();

   HTTPClient *pres = new HTTPClient(pBaseURL);
   if (put(pBaseURL, pres)) return 0;

   // also remember sequence of adding:
   clFifo.put(nAddCnt++, pBaseURL);

   // as it is managed by concache, ref is always >= 1
   pres->incref();

   return pres;
}

int ConCache::releaseClient(HTTPClient *pcln)
{__
   mtklog(("concache-release %p http", pcln));
   if (pcln->refcnt() != 2) { // should NOT happen
      perr("unexpected refcnt on client: %s %d", pcln->getID(), pcln->refcnt());
      pcln->decref();
      return 9;
   }
   pcln->decref();
   return 0;
}

FTPClient *ConCache::allocFtpClient(char *pBaseURL) 
{
   if (!strBegins(pBaseURL, "ftp://")) return 0;

   mtklog(("concache-alloc %s", pBaseURL));

   void *praw = get(pBaseURL);

   // use existing client?
   if (praw) {
      FTPClient *pres = (FTPClient *)praw;
      if (pres->refcnt() > 1) {
         perr("too many locks on connection: %s (%d)", pBaseURL, pres->refcnt());
         return 0;
      }
      pres->incref();
      return pres;
   }

   // create new, on demand:
   forceCacheLimit();

   FTPClient *pres = new FTPClient(pBaseURL);
   if (put(pBaseURL, pres)) return 0;
   mtklog(("concache-put %s done", pBaseURL));

   // also remember sequence of adding:
   clFifo.put(nAddCnt++, pBaseURL);

   // as it is managed by concache, ref is always >= 1
   pres->incref();

   return pres;
}

int ConCache::releaseClient(FTPClient *pcln)
{__
   mtklog(("concache-release %p ftp", pcln));
   if (pcln->refcnt() != 2) { // should NOT happen
      perr("unexpected refcnt on client: %s %d", pcln->getID(), pcln->refcnt());
      pcln->decref();
      return 9;
   }
   pcln->decref();
   return 0;
}

int ConCache::forceCacheLimit()
{__
   if (size() < 10) {
      mtklog(("concache-drop not required, size=%d", size()));
      return 0;
   }

   // return 0;

   // max. of 10 connections is kept in cache,
   // so drop the oldest one to keep some space:
   num nAddKey = 0;
   char *pBaseURL = (char*)clFifo.iget(0, &nAddKey);
   if (!pBaseURL) return 9+perr("int. #267281138");
   mtklog(("concache-iget first %s", pBaseURL));

   // fifo only stores keys, get matching tcpcore:
   TCPCore *pcon = (TCPCore *)get(pBaseURL);
   if (!pcon) {
      mtklog(("concache-get %s failed", pBaseURL));
      return 9+perr("int. #267281139");
   }

   // for now, assume the oldest one is unlocked
   if (pcon->refcnt() > 1) {
      perr("connection cache overflow, %d %d", size(), pcon->refcnt());
      return 9;
   }

   // can drop the oldest connection
   mtklog(("concache-drop oldest connection %p %s", pcon, pBaseURL));

   // delete connection
   closeAndDelete(pcon);

   // must remove manually
   if (remove(pBaseURL))
      perr("int. #267281208");

   mtklog(("concache-remove done %p remain %d", pcon, size()));

   // also remove from fifo, deletes pBaseURL
   clFifo.remove(nAddKey);

   return 0;
}

int TCPCon::nClIOBufSize = 4096;

TCPCon::TCPCon(SOCKET hsock, TCPCore *pcorein, int nTraceIn) 
{__
   mtklog(("  tcpcon ctr %p line %d",this,nTraceIn));
   memset(this, 0, sizeof(*this));
   pClCore = pcorein;
   clSock  = hsock;
   nClTraceLine = nTraceIn;
   nClStartTime = getCurrentTime();
}

TCPCon::~TCPCon( ) 
{__
   mtklog(("  tcpcon dtr %p used %d msec",this,(int)(getCurrentTime() - nClStartTime)));

   if (clSock != INVALID_SOCKET) {
      // close was forgotten. as tcpcon's are managed by the core,
      // this is probably never reached:
      perr("missing close on tcpcon %p hsock %xh line %d", this, (uint)clSock, nClTraceLine);
      rawClose();
   }

   if (pClUserData) delete pClUserData;
   if (pClIOBuf   ) delete [] pClIOBuf;

   memset(this, 0, sizeof(*this));
   clSock = INVALID_SOCKET;
}

void TCPCon::rawClose() 
{__
   if (clSock != INVALID_SOCKET) {
      mtklog(("con %p close socket %xh",this,(uint)clSock));
      myclosesocket(clSock);
   }
   clSock = INVALID_SOCKET;
}

static void localSetBlocking(SOCKET hSock, bool bYesNo)
{
   #ifdef _WIN32
   unsigned long ulParm = bYesNo ? 0 : 1;
   ioctlsocket(hSock, FIONBIO, &ulParm);
   #else
   if (bYesNo)
      fcntl(hSock, F_SETFL, (fcntl(hSock,F_GETFL) & ~O_NONBLOCK));
   else
      fcntl(hSock, F_SETFL, (fcntl(hSock,F_GETFL) | O_NONBLOCK));
   #endif
}

void  TCPCon::setBlocking (bool bYesNo) 
{
   // unsigned long ulParm = bYesNo ? 0 : 1;
   // ioctlsocket(clSock, FIONBIO, &ulParm);
   localSetBlocking(clSock, bYesNo); // 169
}

int  TCPCon::read(uchar *pBlock, uint nLen) 
{__
	// if a foreground thread detects the escape key:
	if (bGlblEscape)
		return 0; // NOT -1, size_t problem

	IOStatusPhase ophase("read tcp");

   int nRemain = nLen;
   int nCursor = 0;
   while (nRemain > 0) {
      int nRead = recv(clSock, (char*)pBlock+nCursor, nRemain, 0);
      // mtklog(("< %d = recv() max %d", nRead, nRemain));
      // mtklog(("    recv'd %d", nRead));
      if (nRead <= 0)
         break;
		countIOBytes(nRead);
      nRemain -= nRead;
      nCursor += nRead;
   }

   return nCursor;
}

int TCPCon::send(uchar *pBlock, uint nLen) 
{__
   return ::send(clSock, (char*)pBlock, nLen, 0);
}

char *TCPCon::buffer(int &rbufsize)
{
   if (!pClIOBuf) {
      pClIOBuf = new char[nClIOBufSize+100];
      memset(pClIOBuf, 0, nClIOBufSize+100);
   }
   rbufsize = nClIOBufSize;
   return pClIOBuf;
}

char *TCPCon::readLine(char *poptbuf, uint noptmaxbuf) 
{__
   // use which buffer?
   char *pBuf    = poptbuf;
   int  nBufMax = noptmaxbuf;

   // alloc internal buffer on demand:
   if (!pBuf) pBuf = buffer(nBufMax);

	IOStatusPhase ophase("read tcp");

   int nCursor = 0;
   int nRemain = nBufMax;
   pBuf[0] = '\0';

   // switching from readLine mode to readBinary is tricky,
   // therefore read char by char to exactly get the point
   // of CRLF, from which on we may switch to binary.
   while (nRemain > 10) 
   {
      // recv blocks until at least 1 byte is available.
      // mtklog(("   rline.recv begin sock=%xh",(uint)clSock));
      int nRead = recv(clSock, pBuf+nCursor, 1, 0);
      // mtklog(("   recv'd %d \"%c\"", nRead, pBuf[nCursor]));
      if (nRead <= 0)
         return 0;
		countIOBytes(nRead);
      nCursor += nRead;
      nRemain -= nRead;
      pBuf[nCursor] = '\0';
      if (nCursor >= 1 && pBuf[nCursor-1]=='\n')
         break;
   }

   // always return lines without CRLF.
   removeCRLF(pBuf);
   if (core().verbose())
      printf("< %s\n", pBuf);
   else {
      mtklog(("< %.200s", pBuf));
   }

   return pBuf;
}

int TCPCon::puts(char *pline) 
{__
   int nBufMax = 0;
   char *pBuf = buffer(nBufMax);
   nBufMax -= 10; // tolerance

   int nlen = strlen(pline);
   if (nlen > nBufMax) {
      perr("string overflow on send: %.200s", pline); 
      return 9;
   }

   strncpy(pBuf, pline, nBufMax);
   pBuf[nBufMax] = '\0';

   // if no CRLF is found at end of string, append it
   nlen = strlen(pBuf);
   if (nlen < 2 || strncmp(pBuf+nlen-2, "\r\n", 2))
      strcat(pBuf, "\r\n");

	IOStatusPhase ophase("send tcp");

   // finally, send it:
   int nsent = send((uchar*)pBuf, strlen(pBuf));
   if (nsent != (int)strlen(pBuf)) return 9;

   mtklog(("> %s", pBuf));

   return 0;
}

int  TCPCon::putf(cchar *pmask, ...) 
{__
   int nBufMax = 0;
   char *pBuf = buffer(nBufMax);
   nBufMax -= 10; // tolerance

   va_list argList;
   va_start(argList, pmask);
   ::vsnprintf(pBuf, nBufMax, pmask, argList);
   pBuf[nBufMax] = '\0';

   // if no CRLF is found at end of string, append it
   int nlen = strlen(pBuf);
   if (nlen < 2 || strncmp(pBuf+nlen-2, "\r\n", 2))
      strcat(pBuf, "\r\n");

	IOStatusPhase ophase("send tcp");

   // finally, send it:
   int nsent = send((uchar*)pBuf, strlen(pBuf));
   if (nsent != (int)strlen(pBuf)) return 9;

   mtklog(("> %s", pBuf));

   return 0;
}

int  TCPCon::setProtocol(char *psz) {
   return 9;
}

int  TCPCon::readReply(int nMinRC, int nMaxRC) {
   return 9;
}

TCPCore &TCPCon::core( ) {
   return *pClCore; 
}

bool TCPCore::verbose( ) {
   return bClVerbose;
}

void TCPCore::wipe() 
{__
   // safe replacement for memset(this)
   // in case that virtualization is used.
   // does NOT free any memory objects!
   pClID    = 0;
   nClCon   = 0;
   mclear(aClCon);
   mclear(clReadSet);
   mclear(clSetCopy);
   bClVerbose = 0;
   nClRefs  = 0;
}

TCPCore::TCPCore(char *pszID) 
{__
   mtklog(("tcpcore ctr %p",this));
   if (sizeof(*this) > 10000)
      perr("tcpcore stack sizing problem");
   wipe();
   pClID = strdup(pszID);
}

TCPCore::~TCPCore( ) 
{__
   mtklog(("tcpcore dtr %p",this));
   if (nClCon > 0)
      perr("missing shutdown() on tcpcore %p",this);
   shutdown();
   delete [] pClID;
   wipe();
}

char *TCPCore::getID( ) {
   return pClID;
}

char TCPCore::szClProxyHost[100];
int TCPCore::nClProxyPort = -1;
bool TCPCore::bClProxySet  =  0;

// this may also be called BEFORE sysInit().
int TCPCore::setProxy(char *phost, int nport) 
{__
   strcopy(szClProxyHost, phost);
   nClProxyPort = nport;
   bClProxySet = 1;
   return 0;
}

int TCPCore::incref( )  { return ++nClRefs; }
int TCPCore::decref( )  { return --nClRefs; }
int TCPCore::refcnt( )  { return nClRefs;   }

int TCPCore::lastError( ) 
{
   #ifdef _WIN32
   return WSAGetLastError();
   #else
   return errno;
   #endif
}

int TCPCore::sysInit( ) 
{__
   #ifdef _WIN32
   if (!bSysInitDone) {
      // done once per process
      WORD wVersionRequested = MAKEWORD(1,1);
      WSADATA wsaData;
      if (WSAStartup(wVersionRequested, &wsaData)!=0)
         return 9+perr("WSAStartup failed");
   }
   #endif

   // it is possible that the proxy was set
   // by a call before sysInit().
   if (!bClProxySet) {
      mclear(szClProxyHost);
      nClProxyPort = 0;
      char *pproxy = getenv("SFK_PROXY");
      if (pproxy) {
         char szBuf[200];
         static bool bFirst = 1;
         strcopy(szBuf, pproxy);
         char *psz1 = szBuf;
         while (*psz1 && *psz1 != ':') psz1++;
         if (*psz1) *psz1++ = '\0';
         int nport = atol(psz1);
         setProxy(szBuf, nport);
         if (bFirst) {
            bFirst = 0;
            printf("using proxy %s port %d\n", szBuf, nport);
         }
      }
   }

   bSysInitDone = 1;

   return 0;
}

void TCPCore::sysCleanup( ) 
{__
   #ifdef _WIN32
   if (bSysInitDone)
      WSACleanup();
   #endif
   bSysInitDone = 0;
}

void TCPCore::closeConnections( ) 
{__
   mtklog(("tcpcore closecon %p ncon=%d", this, nClCon));
   while (nClCon > 0) {
      TCPCon *pcon = aClCon[0];
      close(pcon);
   }
   memset(aClCon, 0, sizeof(aClCon));
}

void TCPCore::shutdown( ) 
{__
   mtklog(("tcpcore shutdown %p", this));
   for (int i=0; i<nClCon; i++) {
      TCPCon *pcon = aClCon[i];
      delete pcon;
   }
   nClCon = 0;
   memset(aClCon, 0, sizeof(aClCon));
}

int TCPCore::makeServerSocket (int nportin, TCPCon **ppout)
{__
   if (checksys()) return 9;

   if (nClCon >= FD_SETSIZE-2)
      return 9+perr("too many sockets, cannot create server socket\n");

   uint nPort = (uint)nportin;

   struct sockaddr_in ServerAdr;
   socklen_t nSockAdrSize = sizeof(sockaddr_in);

   ServerAdr.sin_family      = AF_INET;
   ServerAdr.sin_addr.s_addr = htonl(INADDR_ANY);
   ServerAdr.sin_port        = htons((unsigned short)nPort);

   SOCKET hServSock = socket(AF_INET, SOCK_STREAM, 0);
   if (hServSock == INVALID_SOCKET)
      return 9+perr("cannot create socket on port %u", nPort);

   if (bind(hServSock, (struct sockaddr *)&ServerAdr, sizeof(sockaddr_in)) == SOCKET_ERROR)
      return 9+perr("cannot bind socket on port %u", nPort);

   int nerr = getsockname(hServSock, (struct sockaddr *)&ServerAdr, &nSockAdrSize);
   if (nerr == SOCKET_ERROR)
      return 9+perr("getsockname failed, %d\n", lastError());

   /*
   bool bTell = (nPort == 0);
   nPort    = (uint)ntohs(ServerAdr.sin_port);
   nNewPort = nPort;
   if (bTell)
      printf("< local port %u (%u, %u)\n", nPort, (nPort>>8), nPort&0xFF);
   */

   // make later accepts non-blocking:
   // unsigned long ulParm = 1;
   // ioctlsocket(hServSock, FIONBIO, &ulParm);
   localSetBlocking(hServSock, 0); // 169

   if (listen(hServSock, 4) == SOCKET_ERROR)
      return 9+perr("cannot listen on port %u", nPort);

   // finally, create and remember the TCPCon
   TCPCon *pcon = new TCPCon(hServSock, this, __LINE__);
   // unblock was done above
   addCon(pcon);
   *ppout = pcon;

   return 0;
}

int TCPCore::addCon(TCPCon *pcon) 
{__
   if (nClCon >= FD_SETSIZE-2)
      return 9+perr("internal: too many connections");

   // add to fdset
   FD_SET(pcon->clSock, &clReadSet);

   // and to pointer table
   aClCon[nClCon++] = pcon;

   return 0;
}

int TCPCore::remCon(TCPCon *pcon) 
{__
   // remove from fdset
   FD_CLR(pcon->clSock, &clReadSet);

   // search in pointer table
   int nAtPos=0;
   for (nAtPos=0; nAtPos<nClCon; nAtPos++)
      if (aClCon[nAtPos] == pcon)
         break;

   if (nAtPos >= nClCon)
      return 9+perr("internal: cannot remove connection %p",this);

   // remove from pointer table
   for (int i=nAtPos; i<nClCon-1; i++)
      aClCon[i] = aClCon[i+1];
   aClCon[nClCon-1] = 0; // just in case

   nClCon--;

   return 0;
}

int TCPCore::accept(TCPCon *pServerCon, TCPCon **ppout) 
{__
   if (checksys()) return 9;

   if (nClCon >= FD_SETSIZE-2)
      return 9+perr("too many sockets, cannot accept more");

	IOStatusPhase ophase("tcp accept");

   struct sockaddr_in ClientAdr;
   socklen_t nSoLen = sizeof(sockaddr_in);
   SOCKET hClient = ::accept(pServerCon->clSock, (struct sockaddr *)&ClientAdr, &nSoLen);
   if (hClient == INVALID_SOCKET)
      return 9+perr("accept failed (%d)", lastError());

   TCPCon *pcon = new TCPCon(hClient, this, __LINE__);
   pcon->setBlocking(0);
   addCon(pcon);
   *ppout = pcon;

   return 0;
}

int TCPCore::checksys( ) {
   if (bSysInitDone) return 0;
   return 9+perr("internal: missing tcp sysinit");
}

int TCPCore::connect(char *pszHost, int nportin, TCPCon **ppout)
{__
   if (checksys()) return 9;

   if (nClCon >= FD_SETSIZE-2)
      return 9+perr("too many sockets, cannot connect more");

	IOStatusPhase ophase("connect tcp");

   uint nPort = nportin;

   struct hostent *pTarget;
   struct sockaddr_in sock;
   SOCKET hSock = socket(AF_INET, SOCK_STREAM, 0);
   if (hSock == INVALID_SOCKET)
      return 9+perr("cannot create socket");

   if ((pTarget = sfkhostbyname(pszHost)) == NULL)
      return 9+perr("cannot get host\n");

   memcpy(&sock.sin_addr.s_addr, pTarget->h_addr, pTarget->h_length);
   sock.sin_family = AF_INET;
   sock.sin_port = htons((unsigned short)nPort);

   if ((::connect(hSock, (struct sockaddr *)&sock, sizeof(sock))) == -1) {
      int nerr = lastError();
      // TODO: myclosesocket(hSock) required?
      return 9+perr("cannot connect to %s:%u, rc %d\n", pszHost, nPort, nerr);
   }

   TCPCon *pcon = new TCPCon(hSock, this, __LINE__);
   // TODO: pcon->setBlocking(0) required?
   addCon(pcon);
   *ppout = pcon;

   return 0;
}

int TCPCore::close(TCPCon *pcon) 
{__
   remCon(pcon);
   mtklog(("tcpcore %p close socket %xh",this,(uint)pcon->clSock));
   myclosesocket(pcon->clSock);
   pcon->clSock = INVALID_SOCKET;
   delete pcon;
   return 0;
}

int  TCPCore::selectInput (TCPCon **ppNextActiveCon, TCPCon *pToQuery) 
{__
   mtklog(("tcp-core select on %d connections",nClCon));

	IOStatusPhase ophase("tcp select");

   // fd_set is modified on select, therefore:
   memcpy(&clSetCopy, &clReadSet, sizeof(fd_set));
   int nrc = select(nClCon,
      &clSetCopy,
      NULL, 
      NULL, 
      NULL  // blocking (no timeout)
      );
   if (nrc <= 0)
      return 9+perr("select() failed, %d",lastError());
   // identify connection with input
   mtklog(("tcp-core select rc %d",nrc));
   for (int i=0; i<nClCon; i++)
   {
      TCPCon *pcon = aClCon[i];
      if (FD_ISSET(pcon->clSock, &clSetCopy)) {
         if (pToQuery && (pcon != pToQuery)) {
            mtklog(("tcp-core: con is set: %p - not the query, skipping", pcon));
            continue;
         }
         mtklog(("tcp-core: con is set: %p - returning", pcon));
         *ppNextActiveCon = pcon;
         return 0;
      }
      mtklog(("tcp-core: con not set: %p", pcon));
   }
   return 1; // nothing to do
}

int  TCPCore::registerData(TCPCon *pcon, TCPConData *pUserData) {
   return 9;
}

void  TCPCore::setVerbose  (bool bYesNo) {
   bClVerbose = bYesNo;
}

// - - - http client - - -

HTTPClient::HTTPClient(char *pszID)
 : TCPCore(pszID)
{__
   wipe();
}

HTTPClient::~HTTPClient( ) 
{__
   if (pClCache) {
      // if (nClCacheUsed)
      //    pwarn("http: %d unused bytes remaining in cache, %p\n",nClCacheUsed,this);
      delete [] pClCache;
   }
   if (pszClLineCache) {
      delete [] pszClLineCache;
   }
   if (pClCurCon)
      perr("http: unclosed connection, %p", this);
   wipe();
}

void HTTPClient::resetCache() 
{__
   nClCacheUsed = 0;
}

void HTTPClient::wipe( ) 
{__
   pClCurCon  = 0;

   mclear(aClURLBuf);
   pClCurPath = 0;

   mclear(aClHeadBuf);
   pClCurVal  = 0;

   mclear(aClJoinBuf);

   nClPort    = 80;

   bClChunked   = 0;
   pClCache     = 0;
   nClCacheAlloc= 0;
   nClCacheUsed = 0;

   bClFirstReq  = 1;
   bClNoHead    = 0;
   bClNoCon     = 0;

   pszClLineCache = 0;
}

char *HTTPClient::curname( ) {
   return aClHeadBuf; 
}

char *HTTPClient::curval( ) {
   return pClCurVal ? pClCurVal : (char*)"";
}

// "http://thehost.com/dir1/" + "../rel.txt"
// -> "http://thehost.com/rel.txt"
int HTTPClient::joinURL(char *pabs, char *pref)
{__
   // 1) "image.png"
   // 1) "subdir/the.txt"
   // 2) "/topdir/the.txt"
   // 3) "../../reldir/"
   // 4) "http://other.com/..."
   strcopy(aClJoinBuf, pabs);
   char *pmax = aClJoinBuf + sizeof(aClJoinBuf) - 10;

   bool bNameIsRoot = 0;
   char *psz3 = aClJoinBuf;
   if (strBegins(psz3, "http://")) {
      psz3 += 7;
      while (*psz3 && *psz3 != '/') psz3++;
      if (!*psz3 || !*(psz3+1))
         bNameIsRoot = 1;
   }

   mtklog(("href: \"%s\" buf \"%s\"", pref, aClJoinBuf));

   // find last valid slash in current dir
   char *psla = strrchr(aClJoinBuf, '/');
   if (!psla) return 9; // shouldn't happen

   // if not about to step up
   if (strncmp(pref, "../", 3))
      psla++; // past slash

   // 4) external ref?
   if (   strBegins(pref, "http://")
       || strBegins(pref, "https://")
      )
   {
      return 9;
   }
   else
   // 2) "/topdir/the.txt" ?
   if (*pref == '/') {
      // step src
      pref++;
      // set dst to host root
      psla  = aClJoinBuf;
      if (strncmp(psla, "http://", 7)) return 9;
      psla += 7;
      while (*psla && *psla != '/') psla++;
      if (!*psla) return 9;
      psla++; // behind http://thehost.com/
   }
   else
   // step slashes up as long as "../"  
   while (!strncmp(pref, "../", 3)) {
      // step src
      pref += 3;
      // step dst
      while (psla > aClJoinBuf && *(psla-1) != '/')
         psla--;
   }

   // now join psla and pref
   *psla = '\0';
   int nreflen = strlen(pref);
   if (psla + nreflen > pmax)
      return 9+perr("url overflow: %.100s",pref);
   memcpy(psla, pref, nreflen);
   psla[nreflen] = '\0';

   return 0;
}

char *HTTPClient::curjoin( ) {
   return aClJoinBuf;
}

int HTTPClient::splitURL(char *purl)
{__
   // isolate hostname from url
   aClURLBuf[0] = '\0';

   // http://thehost.com/thedir/thefile.txt
   if (strncmp(purl, "http://", 7)) return 9;
   char *psz1 = purl + 7;

   // copy from thehost.com
   strcopy(aClURLBuf, psz1);
   char *pdst = aClURLBuf;

   // then null the first "/" or ":"
   char *psla = pdst;
   while (*psla && (*psla != '/' && *psla != ':')) psla++;
   if (*psla == ':') {
      // isolate embedded port number
      *psla++ = '\0';
      char *pport = psla;
      while (*psla && *psla != '/') psla++;
      if (*psla) *psla++ = '\0';
      nClPort = atol(pport);
   } else {
      // no embedded port number
      if (*psla) *psla++ = '\0';
   }

   // printf("host \"%s\" port %d path \"%s\"\n",aClURLBuf,nClPort,psla);

   // anything after "/" is the relative path
   pClCurPath = psla;

   return 0;
}

char *HTTPClient::curhost( ) {
   return aClURLBuf;
}

char *HTTPClient::curpath( ) {
   return pClCurPath ? pClCurPath : (char*)"";
}

int HTTPClient::splitHeader(char *phead)
{__
   // isolate hostname from url
   aClHeadBuf[0] = '\0';
   pClCurVal = 0;

   // content-type: text/html
   strcopy(aClHeadBuf, phead);

   char *psz1 = aClHeadBuf;
   while (*psz1 && *psz1 != ':') psz1++;
   if (!*psz1) return 9; // wrong header format

   *psz1++ = '\0';
   while (*psz1 && *psz1 == ' ') psz1++;
   pClCurVal = psz1;

   // make header name all lowercase
   for (psz1 = aClHeadBuf; *psz1; psz1++)
      *psz1 = tolower(*psz1);

   return 0;
}

char *HTTPClient::readLine( ) {
   if (!pClCurCon)
      { perr("int. #175291 missing connection"); return 0; }
   return pClCurCon->readLine();
}

// returns RC
int HTTPClient::sendLine(char *pline) {
   if (!pClCurCon)
      { perr("int. #175292 missing connection"); return 9; }
   return pClCurCon->puts(pline);
}

int HTTPClient::list(char *purl, CoiTable **pptable)
{
   return 9;
}

int HTTPClient::getFileHead(char *purl, Coi *pcoi, cchar *pinfo)
{__
   mtklog(("getFileHead %s %s", purl, pinfo));

   if (bClNoCon) {
      mtklog(("http-getfilehead: nocon set, blocked: %s", purl));
      return 9; // error issued before
   }

   if (splitURL(purl))
      return 9+perr("http: wrong url format: %s",purl);

   char *phost = curhost();
   char *pfile = curpath();

   int nport = nClPort;
   if (connectHttp(phost, nport, &pClCurCon)) {
      bClNoCon = 1;
      return 9+perr("http connect failed: %s:%d",phost,nport);
   }

   ConAutoClose ocls(this, &pClCurCon, __LINE__);

   // header field container
   StringMap *pheads = &pcoi->headers(); // gets filled below
   if (!pheads) return 9+perr("out of memory, cannot read head");

   int nbail = 0;

   do
   {
      // send request to return headers.
      // TODO: on first request, check suspicious HEAD replies
      //       like "image/gif" from a root directory.
      int nrc = 0;
      bool bHeadDone = 0;
      if (bClNoHead)
         nrc = sendReq("GET",pfile,phost,nport);
      else {
         nrc = sendReq("HEAD",pfile,phost,nport);
         bHeadDone = 1;
      }
      if (nrc) return nrc;
   
      // read headers on reply stream.
      int nwebrc = 0;
      pheads->reset();
      if (rawReadHeaders(nwebrc, purl, pcoi))
         return 9;
      // this also (re)sets the chunked mode

      // suspicious reply from root directory?
      char *pcont = pheads->get(str("content-type"));
      mtklog(("fr %d hd %d len %d cont %p %s",
         bClFirstReq, bHeadDone, strlen(pfile),
         pcont, pcont ? pcont : "<null>"));
      if (   bClFirstReq && bHeadDone
          && !strlen(pfile)
          && pcont && !strstr(pcont, "text")
         )
      {
         // no text data from root dir: disable HEAD requests
         bClNoHead = 1;
         // then repeat the request
         close();
         if (connectHttp(phost, nport, &pClCurCon))
            return 9+perr("http reconnect failed: %s:%d",phost,nport);
         continue;
      }
 
      if (nwebrc == 200)
         bClFirstReq = 0;

      // http redirect?
      if (nwebrc < 300 || nwebrc >= 400)
         break; // hopefully got the target
      
      // "HTTP/1.1 3xx" redirection
      // "Location: main.html"
      char *ptarg = pheads->get(str("location"));
      if (!ptarg) return 9+perr("rc %d but no Location on %s", nwebrc, curpath());
      mtklog(("redir to %s", ptarg));

      // CHANGE FILENAME OF COI according to redirection:
      char *pabsnew = 0;

      if (strBegins(ptarg, "http:")) {
         // absolute url supplied: use directly
         pabsnew = ptarg; // will be copied
      } else {
         // relative url supplied: join with old url
         if (joinURL(purl, ptarg))
            return 9+perr("redirect failed: %s / %s",purl,ptarg);
         pabsnew = curjoin();
      }

      // now holding "http://thehost.com/newfile.txt"
      pcoi->setName(pabsnew); // is copied
      purl = pcoi->name();   // take copy
      pinf("redirected to %s\n", purl);

		// evaluate new url for archive extensions
		if (endsWithArcExt(purl, 7)) {
			mtklog(("hth: archive detected by redirect: %s", purl));
			pcoi->setArc(1);
		}

      // block dup reads
      glblCircleMap.put(purl, 0); // if already set, does nothing

      // have to split this again
      if (splitURL(purl))
         return 9+perr("http: wrong url format: %s",purl);
   
      // take redirected target
      phost = curhost();
      pfile = curpath();

      // repeat the request with the new target.
      // TODO: close and re-open connection in case of GET
      if (!bHeadDone) {
         close();
         if (connectHttp(phost, nport, &pClCurCon))
           return 9+perr("http reconnect failed: %s:%d",phost,nport);
      }
   }
   while (nbail++ < 5); // until no longer redirected, or bail out

   // connection is auto closed
   return 0;
}

char *nextNonBlank(char *psz) {
   while (*psz && *psz != ' ') psz++;
   while (*psz && *psz == ' ') psz++;
   return *psz ? psz : 0;
}

int HTTPClient::rawReadHeaders
 (
   int  &rwebrc,
   char    *purl,    // only informal
   Coi     *pcoi     // optional
 )
{__
   if (!pClCurCon)
      { perr("int. #175293 missing connection"); return 9; }

   StringMap *pheads = &pcoi->headers();
   if (!pheads) return 9+perr("out of memory, cannot read headers");

   char *pline  = 0;
   bool  bfirst = 1;
   int  nwebrc = 0;
   bool  btext  = 0;

   // always reset chunked flag until we know better
   bClChunked = 0;

   mtklog(("http: begin reading of headers"));

   while ((pline = readLine()))
   {
      if (!strlen(pline)) break; // end of header

      mtklog(("< %s (from %s)", pline, purl));

      if (bfirst) 
      {
         bfirst = 0;

         // header-free server?
         char *pprobe = pline;
         while (*pprobe==' ' || *pprobe=='\t') pprobe++;
         if (*pprobe == '<') 
         {
            // cache first line of content
            if (pszClLineCache) delete [] pszClLineCache;
            pszClLineCache = new char[strlen(pline)+10];
            memcpy(pszClLineCache, pline, strlen(pline)+1);
            return 0; // "done headers"
         }

         // HTTP/1.1 200 OK
         char *psz1 = pline;
         while (*psz1 && *psz1 != ' ') psz1++;
         while (*psz1 && *psz1 == ' ') psz1++;
         nwebrc = atol(psz1);
         rwebrc = nwebrc;

			#ifdef USE_404
			// recoverable rc?
			if (nwebrc == 404) {
				// continue to load the error page,
				// but remember rc for additional info.
            pheads->put("webrc", "404");
				continue;
			}
			#endif // USE_404

         // fatal error?
         if (nwebrc >= 400) {
            perr("%.40s: %s",pline,purl);
            return 9;
         }

         continue;
      }

      if (splitHeader(pline)) {
         perr("wrong http header on object: %s", purl);
         pinf("header line is: %s\n",pline);
         return 9; 
      }

      mtklog(("   head \"%s\" val \"%s\"",curname(),curval()));

      // if map is supplied, store some headers there
      if (pheads) {
         // if (nwebrc >= 300 && nwebrc < 400 && !strcmp(curname(), "location"))
         if (   !strcmp(curname(), "location")
             || !strcmp(curname(), "content-type")
            )
            pheads->put(curname(), curval());
      }

      // interpret some headers directly
      // Transfer-Encoding: chunked
      if (   !mystricmp(curname(), "transfer-encoding")
          && !mystricmp(curval(), "chunked")) {
         bClChunked = 1;
         mtklog(("chunked read of reply"));
      }

      // if no coi is given, ignore header contents
      if (!pcoi)
         continue;

      // .gz  application/x-gzip
      // .zip application/x-zip-compressed
      // .bz2 application/x-bzip2
      // .tar application/x-tar
      // .tgz application/x-compressed
		// however, some servers say only "application/octet-stream"
		// for all archive files. in this case, the redirect file name
		// must be evaluated.

      // content-type: text/html, application/zip etc.
      if (!mystricmp(curname(), "content-type")) {
         if (strBegins(curval(), "text/html")) {
            pcoi->setBinaryFile(0); // NOT binary
            btext = 1;
         } 
         else
         if (  (strBegins(curval(), "application/") && strstr(curval(), "zip"))
             || strBegins(curval(), "application/x-tar")
             || strBegins(curval(), "application/x-compressed")
            )
         {
            pcoi->setBinaryFile(1); // is binary
            pcoi->setArc(1);        // but also an archive
         } 
         else {
            pcoi->setBinaryFile(1); // is any binary
         }
         continue;
      }

      // content-length: 20000
      if (!mystricmp(curname(), "content-length")) {
         pheads->put(curname(), curval());
         pcoi->setSize(atonum(curval()));
         // for live stat display, if any
         void setIOStatMaxBytes(num n);
         setIOStatMaxBytes(atonum(curval()));
         continue;
      }

      // "Date" or
      // "Last-Modified" val "Tue, 01 Jan 2008 01:01:25 GMT"
      if (   !mystricmp(curname(), "date")
          || !mystricmp(curname(), "last-modified")
         ) 
      {
         pheads->put(curname(), curval());
         char *pstr = nextNonBlank(curval()); // skip "Tue, "
         num ntime  = 0;
         // force format "01 Jan 2008 01:01:25 GMT"
         //               012345678901234567890123
         int nlen  = strlen(pstr);
         if (nlen < 20) continue;
         if (pstr[14] != ':' || pstr[17] != ':') continue;
         if (!timeFromString(pstr, ntime))
            pcoi->setTime(ntime);
         continue;
      }

      // TODO: process further headers
   }

   // connection is auto closed.
   return 0;
}

int HTTPClient::connectHttp(char *phostorip, int nport, TCPCon **ppout)
{__
   if (nClProxyPort < 0)
      return 9+perr("internal, wrong http initial state\n");

   if (szClProxyHost[0])
      return connect(szClProxyHost, nClProxyPort, ppout);
   else
      return connect(phostorip, nport, ppout);
}

int HTTPClient::open(char *purl, cchar *pmode, Coi *pcoi)
{__
   if (bClNoCon) {
      mtklog(("http::open: nocon set, blocked: %s", purl));
      return 9; // error issued before
   }

   if (splitURL(purl))
      return 9+perr("http: wrong url format: %s",purl);

   char *phost = curhost();
   char *pfile = curpath();

   int nport = nClPort;
   if (connectHttp(phost, nport, &pClCurCon)) {
      bClNoCon = 1;
      return 9+perr("http connect failed: %s:%d",phost,nport);
   }

   // header field container
   StringMap *pheads = &pcoi->headers();
   if (!pheads) return 9+perr("out of memory, cannot read meta data");

   int nbail = 0;

   do
   {
_     // send request to return headers.
      int nrc = sendReq("GET",pfile,phost,nport);
      if (nrc) return nrc;
_
      // read headers on reply stream.
      int nwebrc = 0;
      pheads->reset();
      if (rawReadHeaders(nwebrc, purl, pcoi))
         return 9;
      // this also (re)sets the chunked mode
_
      // http redirect?
      if (nwebrc < 300 || nwebrc >= 400)
         break; // hopefully got the target
_
      // "HTTP/1.1 3xx" redirection
      // "Location: main.html"
      char *ptarg = pheads->get(str("location"));
      if (!ptarg) return 9+perr("rc %d but no Location on %s", nwebrc, curpath());
      mtklog(("redir to %s", ptarg));
_
      // CHANGE FILENAME OF COI according to redirection:
      char *pabsnew = 0;
_
      if (strBegins(ptarg, "http:")) {
         // absolute url supplied: use directly
         pabsnew = ptarg; // will be copied
      } else {
         // relative url supplied: join with old url
         if (joinURL(purl, ptarg))
            return 9+perr("redirect failed: %s / %s",purl,ptarg);
         pabsnew = curjoin();
      }

      // now holding "http://thehost.com/newfile.txt"
      pcoi->setName(pabsnew); // is copied
      purl = pcoi->name();   // take copy
      pinf("redirected to %s\n", purl);

		// evaluate new url for archive extensions
		if (endsWithArcExt(purl, 8)) {
			mtklog(("hto: archive detected by redirect: %s", purl));
			pcoi->setArc(1);
		}

      // block dup reads
      glblCircleMap.put(purl, 0); // if already set, does nothing

      // have to split this again
      if (splitURL(purl))
         return 9+perr("http: wrong url format: %s",purl);

      // take redirected target
      phost = curhost();
      pfile = curpath();

      // repeat the request with the new target.
      close();
      if (connectHttp(phost, nport, &pClCurCon))
        return 9+perr("http reconnect failed: %s:%d",phost,nport);
   }
   while (nbail++ < 5); // until no longer redirected, or bail out

   // connection is auto closed
   return 0;
}

// returns no. of bytes or 0
int HTTPClient::readraw(uchar *pbuf, int nbufsize) 
{__
   if (!pClCurCon)
      { perr("int. #175294 missing connection"); return 0; }
   return pClCurCon->read(pbuf, nbufsize);
}

// TODO: reorg error rc handling
int HTTPClient::read(uchar *pbuf, int nbufmax) 
{__
   mtklog(("http::read max=%d chunked=%d incache=%d", nbufmax, bClChunked, nClCacheUsed));

   if (!pClCurCon)
      { perr("int. #175295 missing connection"); return 0; }

   if (pszClLineCache) 
   {
      int nlen = strlen(pszClLineCache);
      int nrem = 0;
      if (nlen > nbufmax)
         { nrem=nlen-nbufmax; nlen=nbufmax; }
      mtklog(("http::read use cacheline len=%d remain=%d",nlen,nrem));
      memcpy(pbuf, pszClLineCache, nlen);
      if (nrem) {
         memmove(pszClLineCache, pszClLineCache+nlen, nrem+1); // WITH term
      } else {
         delete [] pszClLineCache;
         pszClLineCache = 0;
      }
      return nlen;
   }

   if (!bClChunked)
      return readraw(pbuf, nbufmax);

   if (nClCacheUsed <= 0)
   {
      // cache is empty, read next chunk:
   
      // 1. read control line with size of next chunk
      char *pline = pClCurCon->readLine();
      mtklog(("http next chunk: control line \"%s\"", pline ? pline : "<null>"));
      if (!pline)
         return 0+perr("http read error, no chunk header");

      int nchunksize = strtol(pline, 0, 0x10);
      mtklog(("http next chunk %d bytes \"%s\"", nchunksize, pline));
      if (!nchunksize)
         return 0; // EOD flagged by "0" chunk size
   
      // 2. force IOCache to provide enough space
      //    for the whole chunk. note that there may
      //    still be bytes left over from last read.
      int nCacheRemain = nClCacheAlloc - nClCacheUsed;
      if (nchunksize > nCacheRemain) 
      {
         // expand cache
         int nMissing  = nchunksize - nCacheRemain;
         int nAllocNew = nClCacheAlloc + nMissing + 100;
         uchar *pnew = new uchar[nAllocNew+10];
         memcpy(pnew, pClCache, nClCacheUsed);
         // swap new and old
         delete [] pClCache;
         pClCache = pnew;
         nClCacheAlloc = nAllocNew;
         mtklog(("http cache resized to %d", nClCacheAlloc));
         // nClCacheUsed stays unchanged
         nCacheRemain = nClCacheAlloc - nClCacheUsed;
         if (nchunksize > nCacheRemain)
            return 0+perr("int. 167281949");
      }
   
      // 3. read exactly the chunk, adding it to cache
      uchar *pdst = pClCache + nClCacheUsed;
      int nread  = pClCurCon->read(pdst, nchunksize);
      mtklog(("http read %d bytes", nread));
      if (nread < nchunksize)
         return 0+perr("http: incomplete chunk read, %d %d",nread,nchunksize);

      // every chunk is followed by CRLF
      pClCurCon->readLine();

      // fall through to copy from cache
      nClCacheUsed += nread;
   }

   // independent from chunk sizes,
   // return nbufmax OR LESS bytes.
   if (nClCacheUsed <= 0)
      return 0+perr("int. 167281950");

   int ntocopy = nbufmax;
   if (nClCacheUsed < ntocopy)
        ntocopy = nClCacheUsed;

   memcpy(pbuf, pClCache, ntocopy);

   // shrink cache contents
   uchar *psrc = pClCache + ntocopy;
   int   nrem = nClCacheUsed - ntocopy;
   memmove(pClCache, psrc, nrem);
   nClCacheUsed -= ntocopy;

   mtklog(("http cache remain %d alloc %d", nClCacheUsed, nClCacheAlloc));

   return ntocopy;
}

void HTTPClient::close( ) 
{__
   mtklog(("http::close: pcon=%p cacheAlloc=%d cacheUsed=%d", pClCurCon, nClCacheAlloc, nClCacheUsed));

   if (pClCurCon) {
      TCPCore::close(pClCurCon);
      pClCurCon = 0;
   }

   nClCacheUsed = 0;
}

// - - - ftp client - - -

FTPClient::FTPClient(char *pszID)
 : TCPCore(pszID)
{__
   mtklog(("ftpclient ctr %p", this));
   wipe();
}

FTPClient::~FTPClient( ) 
{__
   mtklog(("ftpclient dtr %p", this));
   if (pClCtlCon)
      perr("ftp: unclosed control connection, %p",this);
   if (pClPassive) delete [] pClPassive;
   if (pszClHost ) delete [] pszClHost;
   if (file.name ) delete [] file.name;
   if (file.md5  ) delete file.md5;
   wipe();
}

char  *FTPClient::line( )    { return szClLineBuf; }
int   FTPClient::lineMax( ) { return sizeof(szClLineBuf)-100; }

void FTPClient::wipe( ) 
{__
   pClCtlCon   =  0;
   pClPassive  =  0;
   mclear(aClURLBuf);
   mclear(szClLineBuf);
   pClCurPath  =  0;
   bClLoggedIn =  0;
   pszClHost   =  0;
   nClPort     = 21;
   bClSFT      =  0;
   nClSFTVer   =  0;
   mclear(file);
}

char *FTPClient::curhost( ) { 
   return aClURLBuf;
}

char *FTPClient::curpath( ) { 
   return pClCurPath ? pClCurPath : (char*)"";
}

int FTPClient::curport( ) { return nClPort; }

int FTPClient::splitURL(char *purl)
{__
   // isolate hostname from url
   aClURLBuf[0] = '\0';

   // ftp://thehost.com/thedir/thefile.txt
   if (strncmp(purl, "ftp://", 6)) return 9;
   char *psz1 = purl + 6;

   // copy from thehost.com
   strcopy(aClURLBuf, psz1);
   char *pdst = aClURLBuf;

   // then null the first "/" or ":"
   char *psla = pdst;
   while (*psla && (*psla != '/' && *psla != ':')) psla++;
   if (*psla == ':') {
      // isolate embedded port number
      *psla++ = '\0';
      char *pport = psla;
      while (*psla && *psla != '/') psla++;
      if (*psla) *psla++ = '\0';
      nClPort = atol(pport);
   } else {
      // no embedded port number
      if (*psla) *psla++ = '\0';
   }

   // printf("host \"%s\" port %d path \"%s\"\n",aClURLBuf,nClPort,psla);

   // anything after "/" is the relative path
   pClCurPath = psla;

   return 0;
}

char *FTPClient::readLine( ) {
   return pClCtlCon->readLine();
}

int FTPClient::sendLine(char *pline) {
   return pClCtlCon->puts(pline);
}

// guarantees to set pline, even if empty
int FTPClient::readReply(char **ppline)
{__
   char *pline = str("");
   int ncode = 0;
   while ((pline = readLine())) {
      // e.g. "230-some text" or "230 login done"?
      int nlen = strlen(pline);
      ncode = atol(pline);
      if (nlen < 4) break;
      if (pline[3] != '-') break;
      // else continue reading the reply
      mtklog(("[reply %d continued]",ncode));
   }
   *ppline = pline ? pline : (char*)"";
   return pline ? ncode : 0;
}

int FTPClient::login(char *phostorip, int nport)
{__
   mtklog(("ftp-login %s", phostorip));

   if (pszClHost)  { delete [] pszClHost; pszClHost = 0; }

   if (connect(phostorip, nport, &pClCtlCon))
      return 9+perr("ftp login failed: %s:%d",phostorip,nport);

   char *pline = 0;
   int  ncode = 0;
_
   // expect "220 " or "220-"
   if (!(ncode = readReply(&pline)))
      return 9+perr("ftp: wrong server hello: %s", pline);
_
   // detect sfk ftp server with SFT
   char *psz1 = strstr(pline, ". sft ");
   if (psz1) {
      int nSFTVer = atol(psz1+6);
      if (nSFTVer >= 100) {
         bClSFT = 1;
         printf("> server speaks sft %d. mget, mput enabled.\n", nSFTVer);
         nClSFTVer = nSFTVer;
      } else {
         printf("> unexpected sft info \"%s\"\n", psz1);
      }
   }
_
   if (sendLine(str("USER anonymous"))) return 9;
   if (!readLine()) return 9; // 331
   if (sendLine(str("PASS sft102@"))) return 9;
_
   // expect "230 login done"
   if (!(ncode = readReply(&pline)))
      return 9+perr("ftp: wrong login reply: %s", pline);
_
   if (sendLine(str("TYPE I"))) return 9;
   if (!readLine()) return 9; // 200 OK
_
   bClLoggedIn = 1;
   pszClHost   = strdup(phostorip);

   mtklog(("ftp-login done %s", phostorip));

   return 0;
}

int FTPClient::loginOnDemand(char *phostorip, int nport)
{__
   // TODO: check maybe by dummy command if line is still valid
   if (bClLoggedIn) {
      mtklog(("ftp-login-reuse: %s", phostorip));
      return 0;
      // logout();
   }
   return login(phostorip, nport);
}

void FTPClient::logout( )
{__
   if (bClLoggedIn) {
      mtklog(("ftp-logout: %s", pszClHost ? pszClHost:""));
      if (pClCtlCon) close(pClCtlCon);
      pClCtlCon = 0;
      bClLoggedIn = 0;
   }
}

int FTPClient::setPassive(TCPCon **ppout)
{__
   if (pClPassive) { delete [] pClPassive; pClPassive=0; }

   if (!pClPassive) {
      // first call to setPassive:
      if (sendLine(str("PASV"))) return 10;
      char *pTmp = 0;
      if (!(pTmp = readLine())) return 11;
      pClPassive = strdup(pTmp);
   }

   // (re)use reply string to PASV
   char *pBuf = pClPassive;

   // 227 Entering Passive Mode (127,0,0,1,117,246)
   char *psz = strchr(pBuf, '(');
   if (!psz) return 12+perr("set passive failed: \"%s\"",pBuf);
   psz++;
   uchar n[6];
   for (int i=0; i<6; i++) {
      n[i] = (uchar)atol(psz);
      psz = strchr(psz+1, ',');
      if (psz) psz++; else break;
   }
   char szIP[50];
   sprintf(szIP, "%d.%d.%d.%d",n[0],n[1],n[2],n[3]);
   uint nPort = (((uint)n[4])<<8)|((uint)n[5]);

   // if (connectSocket(szIP, nPort, SoAdr, hData, "pasv data")) return 9;
   SOCKET hSock = socket(AF_INET, SOCK_STREAM, 0);
   if (hSock == INVALID_SOCKET)
      return 9+perr("set passive failed: cannot create socket");

   struct hostent *pTarget = 0;
   if ((pTarget = sfkhostbyname(szIP)) == NULL)
      return 9+perr("set passive failed: cannot get host %s", szIP);

   struct sockaddr_in ClntAdr;
   memcpy(&ClntAdr.sin_addr.s_addr, pTarget->h_addr, pTarget->h_length);
   ClntAdr.sin_family = AF_INET;
   ClntAdr.sin_port = htons((unsigned short)nPort);

	IOStatusPhase ophase("connect ftp");

   if ((::connect(hSock, (struct sockaddr *)&ClntAdr, sizeof(struct sockaddr_in))) == -1)
      return 9+perr("set passive failed: cannot establish connection to %s", szIP);

   if (verbose())
      printf("< connected to %s:%u\n", szIP, nPort);

   // turn this into a tcpcore managed connection
   TCPCon *pcon = new TCPCon(hSock, this, __LINE__);
   // TODO: pcon->setBlocking(0) required?
   addCon(pcon);
   *ppout = pcon;

   return 0;
}

// list remote files of a directory
int FTPClient::list(char *pdir, CoiTable **ppout, char *pRootURL)
{__
   if (!pdir) return 9;

   if (!*pdir)    pdir = str(".");
   if (!pRootURL) pRootURL = str(""); // safety

   #ifdef DEEP_FTP
   // printf("ftp list begin: dir=%s root=%s\n",pdir,pRootURL);
   #endif

   int nrc = 0;

   TCPCon *pdat = 0;

   if (bClSFT) {
      // if (nClSFTVer >= 105) may use SLSB
      if (pClCtlCon->putf("SLST %s\r\n", pdir)) return 9;
      pdat = pClCtlCon;
   } else {
      // create a "passive" data connection
      if ((nrc = setPassive(&pdat)))
         return 9+perr("set passive failed, %d", nrc);
      pdat->setBlocking(0);
   
      // still send commands on control connection
      if (pClCtlCon->putf("LIST %s\r\n", pdir)) return 9;
   }

   char *pres = readLine(); // expect "150 Listing"
   if (!pres)
      return 9+perr("ftp list failed: %s", pdir);
   if (!strBegins(pres, "150 "))
      return 9+perr("ftp error: %s", pres);
 
   CoiTable  *ptab = *ppout;
   if (!ptab) ptab = new CoiTable();

   bool bGotDone = 0;

   // not at all a refname, just a buffer:
   char szTmpBuf1[300]; mclear(szTmpBuf1);
   char szTmpBuf2[500]; mclear(szTmpBuf2);

   // receive list of filenames on data connection
   while (1)
   {
      char *pline = 0;

      if (bClSFT) {
         if (!(pline = readLine()))
            return 9+perr("ftp list error: %s", pdir);
         if (!strncmp(pline, "226 ", 4)) {
            bGotDone = 1;
            break; // ok all done
         }
      } else {
         // any further lines on the data connection?
         TCPCon *prec = 0;
         int nrc = selectInput(&prec, pdat);
   
         if (nrc > 0) {
            // no: check control connection
            nrc = selectInput(&prec, pClCtlCon);
            if (nrc >= 5)
               return 9+perr("ftp list failed: %d", nrc);
            if (nrc == 1) // no status available
               { doSleep(500); continue; }
            if (!(pline = readLine()))
               return 9+perr("ftp list error: %s", pdir);
            mtklog(("from ctl: %s",pline));
            if (!strncmp(pline, "226 ", 4)) {
               bGotDone = 1;
               break; // ok all done
               // TODO: can really stop here w/o further data read?
            }
            pinf("%s", pline);
            continue;
         }
   
         // no: fall through to data read
         mtklog(("read from dcon %p", pdat));
         if (!(pline = pdat->readLine())) {
            mtklog(("eod from dcon"));
            break;
         }
      }

      // === extract meta data from pline ===

      // -rw-rw-rw-t 1 ftp ftp        30353 Sep 08 13:47   readme.txt
      //    0        1  2   3          4    5   6   7      8
      // drw-rw-rw-d 1 ftp ftp            0 20061231235959 mydir
      // -rw-rw-rw-b 1 ftp ftp        30353 20061231235959 test.dat
      //    0        1  2   3          4    5              6

      // since SFT 105, t/b are no longer provided.

      char *apcol[20];
      memset(apcol, 0, sizeof(apcol));

      char *pszFileRaw = pline;

      // parse raw line, copy from ftpClient()
      int ncol = 0;
      strcopy(szTmpBuf1, pszFileRaw);
      char *psz1 = szTmpBuf1;
      while (ncol < 15) {
         // store column start
         apcol[ncol++] = psz1;
         // find end of column
         skipToWhite(&psz1);
         if (!*psz1) break;
         *psz1++ = '\0';
         // find start of next column
         skipWhite(&psz1);
         if (!*psz1) break;
      }

      bool bIsDir = 0;
      bool bHaveBinInfo = 0;
      bool bIsBinary    = 0;
      char *pattr = apcol[0];
      if (pattr) {
         // 12345678901
         // drw-rw-rw-d
         // -rw-rw-rw-b : binary
         // -rw-rw-rw-t : text
         int nlen = strlen(pattr);
         if (pattr[0] == 'd') bIsDir = 1;
         if (nlen >= 11 && pattr[10] == 'b')
            { bHaveBinInfo=1; bIsBinary=1; }
         if (nlen >= 11 && pattr[10] == 't')
            { bHaveBinInfo=1; bIsBinary=0; }
      }

      char *pszMonTS = apcol[5]; // month or timestamp
      bool bHaveFlatTime = 0;
      if (pszMonTS && isdigit(*pszMonTS))
           bHaveFlatTime = 1;

      int iName = 8;
      if (bHaveFlatTime) {
         if (ncol < 7)
            {  pwarn("wrong format: %s - skipping\n", pszFileRaw); continue; }
         iName = 6;
      }
      else
      if (ncol < 9)
         {  pwarn("wrong format: %s - skipping\n", pszFileRaw); continue; }

      // always take filename from original buffer,
      // blanks may have been replaced by zeros in RefNameBuf.
      char *pszFile = pszFileRaw + (apcol[iName]-szTmpBuf1);
      char *pszSize = apcol[4];

      num nFileTime = 0;

      // if (bGlblFTPSetAttribs)
      {
         if (bHaveFlatTime) {
            timeFromString(pszMonTS, nFileTime);
         } else {
            if (apcol[5] && apcol[6] && apcol[7]) {
               sprintf(szTmpBuf2, "%s %s %s",apcol[5],apcol[6],apcol[7]);
               timeFromString(szTmpBuf2, nFileTime);
            }
         }
      }

      num nFileSize = atonum(pszSize);
      if (!bIsDir && (nFileSize <= 0))
      {
         printf("] skip, size=%s: %s\n", pszSize, pszFile);
         continue;
      }

      // === end meta data extraction ===

      // prefix every entry by its source url
      cchar *pInsPath = "";
      cchar *pszTrail = "";

      int nlen = strlen(pRootURL);
      if (nlen > 0 && pRootURL[nlen-1] != '/')
            pInsPath = "/";
      int nRootUseLen = strlen(pRootURL);

      #ifdef DEEP_FTP
      // does the root contain pdir redundantly?
      int nSubDirLen = strlen(pdir);
      if (    nSubDirLen < nRootUseLen
          && !strcmp(pRootURL+nRootUseLen-nSubDirLen, pdir)
         )
      {
         // printf("root contains pdir\n");
         nRootUseLen -= nSubDirLen;
         if (nRootUseLen > 0 && pRootURL[nRootUseLen-1] != '/')
            pInsPath = "/";
         else
            pInsPath = "";
      } else {
         // printf("root does not contain dir \"%s\" \"%s\" \"%s\" %d %d\n",pRootURL+nRootUseLen-nSubDirLen,pdir,pRootURL,nRootUseLen,nSubDirLen);
      }
      if (bIsDir) pszTrail = "/"; // TODO: check if there's a slash already
      #endif

      snprintf(szTmpBuf2, sizeof(szTmpBuf2), "%.*s%s%s%s", nRootUseLen,pRootURL,pInsPath,pszFile,pszTrail);

      // force whole url to net slashes
      void setNetSlashes(char *pdst);
      setNetSlashes(szTmpBuf2);

      Coi ocoi(szTmpBuf2, pRootURL);

      SFKFindData myfdat;
      memset(&myfdat, 0, sizeof(myfdat));
      myfdat.size = nFileSize;
      myfdat.time_write = nFileTime;
      if (bIsDir) {
         // printf("copy dir : %s\n",ocoi.name());
         myfdat.attrib |= 0x10;
      } else {
         // printf("copy file: %s\n",ocoi.name());
      }

      ocoi.fillFrom(&myfdat);

      if (bHaveBinInfo) {
         ocoi.setBinaryFile(bIsBinary);
         mtklog(("ftp list: %s : %s",bIsBinary?"isbin":"istxt",ocoi.name()));
      }

      #ifdef DEEP_FTP
      // printf("ftp list: add entry: %s  (dir=%d)\n", ocoi.name(), bIsDir);
      #endif

      ptab->addEntry(ocoi);
   }

   if (pdat != pClCtlCon) {
      // ftp: close separate data connection
      close(pdat);
   }

   // expect 226 Closing
   if (!bGotDone)
      if (!readLine())
         return 9;

   // result is owned by caller
   *ppout = ptab;

   mtklog(("ftp list end: %s return %d entries",pdir,ptab->numberOfEntries()));

   return 0;
}

int FTPClient::readLine(TCPCon *pcon)
{__
   char *pszLineBuf = line();
   int  nMaxLineBuf = lineMax();

   if (!pcon->readLine(pszLineBuf, nMaxLineBuf))
      return 9;

   // sft101: optional skip records to enforce socket flushing
   if (strBegins(pszLineBuf, "SKIP "))
   {
      // read intermediate skip record
      uint nLen = (uint)atol(pszLineBuf+5);
      if ((int)nLen > nMaxLineBuf) return 9;
      if (nLen) {
         if (pcon->read((uchar*)pszLineBuf, nLen) < 0)
            return 9;
      }

      // now read the actual record
      if (!pcon->readLine(pszLineBuf, nMaxLineBuf))
         return 9;
   }

   mtklog(("ftp: < %s", pszLineBuf));

   return 0;
}

int FTPClient::readLong(TCPCon *pcon, uint &rOut, cchar *pszInfo)
{__
   uchar szBuf[100];
   if (pcon->read(szBuf, 4) != 4) return 9+perr("failed to read %s\n", pszInfo);
   uint nLen =   (((uint)szBuf[3])<<24)
                | (((uint)szBuf[2])<<16)
                | (((uint)szBuf[1])<< 8)
                | (((uint)szBuf[0])<< 0);
   rOut = nLen;
   return 0;
}

int FTPClient::sendLong(TCPCon *pcon, uint nOut, cchar *pszInfo)
{__
   uchar szBuf[100];
   szBuf[3] = ((uchar)(nOut >> 24));
   szBuf[2] = ((uchar)(nOut >> 16));
   szBuf[1] = ((uchar)(nOut >>  8));
   szBuf[0] = ((uchar)(nOut >>  0));
   int nSent = pcon->send(szBuf, 4);
   if (nSent != 4) return 9+perr("failed to send %s\n", pszInfo);
   return 0;
}

int FTPClient::readNum(TCPCon *pcon, num &rOut, cchar *pszInfo)
{__
   uchar szBuf[100];
   if (pcon->read(szBuf, 8) != 8) return 9;
   num nOut = 0;
   for (int i=0; i<8; i++) {
      nOut <<= 8;
      nOut |= (uint)szBuf[i];
   }
   rOut = nOut;
   return 0;
}

int FTPClient::readNum(uchar *pbuf, int &roff, num &rOut, cchar *pszInfo)
{__
   num nOut = 0;
   for (int i=0; i<8; i++) {
      nOut <<= 8;
      nOut |= (uint)pbuf[roff+i];
   }
   roff += 8;
   rOut = nOut;
   return 0;
}

int FTPClient::sendNum(TCPCon *pcon, num nOut, cchar *pszInfo)
{__
   uchar szBuf[100];
   // this may fail with num's >= 2 up 63.
   for (int i=7; i>=0; i--) {
      szBuf[i] = (uchar)(nOut & 0xFF);
      nOut >>= 8;
   }
   int nSent = pcon->send(szBuf, 8);
   if (nSent != 8) return 9+perr("failed to send %s\n", pszInfo);
   return 0;
}

extern int nGlblTCPMaxSizeMB;
extern char *localPath(char *pAbsFile);
extern int fastMode();

bool bGlblBulkFTPIO = 0;

void setBulkFTPIO(bool bYesNo) { bGlblBulkFTPIO = bYesNo; }

// open remote file for download
// RC:  9 == general error
//     10 == fatal communication error, connection invalid
int FTPClient::openFile(char *pfilename, cchar *pmode)
{__
   mtklog(("ftp: openFile: %s %s",pfilename,pmode));

   if (strcmp(pmode, "rb"))
      return 9+perr("cannot open ftp file, only read supported: %s", pfilename);

   TCPCon *pcon = pClCtlCon;

   #ifdef USE_SFT
   if (bClSFT)
   {
      // so far used only by dview:
      bool bbulk = bGlblBulkFTPIO;

      // request block mode transfer
      if (bbulk) {
         if (pcon->putf("SGET %s", pfilename)) return 10;
      } else {
         if (pcon->putf("SOPEN %s", pfilename)) return 10;
      }
      if (readLine(pcon)) return 10; // 200 OK
      if (!strBegins(line(), "200")) return 9;

      // read SFT file header meta infos
      uchar abHead[512+10];
   
      uint nMetaSize = 0;
      if (readLong(pcon, nMetaSize, "metalen")) return 9;
      if (nMetaSize > sizeof(abHead)-10)
         return 9+perr("unsupported SFT protocol version\n");
      if (pcon->read(abHead, nMetaSize) != (int)nMetaSize) return 9;
   
      // take from header what we need
      int ioff = 0;
   
      // meta 1: 8 bytes filesize
      num nLen = 0, nTime = 0, nFlags = 0;
      char szLocalTime[20]; mclear(szLocalTime);
      if (readNum(abHead, ioff, nLen, "size")) return 9;
   
      if (nGlblTCPMaxSizeMB)
         if (nLen > nGlblTCPMaxSizeMB * 1000000)
            return 9+perr("illegal length received, %s\n", numtoa(nLen));
   
      if (nClSFTVer >= 102) {
         if (readNum(abHead, ioff, nTime , "time" )) return 9;
         if (readNum(abHead, ioff, nFlags, "flags")) return 9;
      }

      if (nClSFTVer < 102) {
         // meta 2: md5 before content
         memcpy(file.abmd5, abHead+ioff, 16);
         ioff += 16;
      }

      if (file.name) delete [] file.name;
      file.name   = strdup(pfilename);

      if (file.md5)  delete file.md5;
      file.md5    = new SFKMD5();

      file.time   = nTime;
      file.size   = nLen;
      file.remain = nLen;

      file.blockmode = bbulk ? 0 : 1;
      file.block     = 0;

      // we do NOT really write a file here!
   }
   else
   #endif // USE_SFT
   {
      if (setPassive(&pClDatCon)) return 10;
      if (pClCtlCon->putf("RETR %s", pfilename)) return 10;
   
      // TODO: check actual RC if file exists
      if (!pClCtlCon->readLine()) return 10;
      // download started on data connection
   
      if (verbose()) {
         printf("< %s   \r", pfilename);
         fflush(stdout);
      }
   }

   return 0;
}

// read block of a file
int FTPClient::readFile(uchar *pbuf, int nmaxlen)
{__
   #ifdef USE_SFT
   if (bClSFT)
   {
      TCPCon *pcon = pClCtlCon;

      int nmaxin = nmaxlen; (void)nmaxin;

      if (nmaxlen > file.remain) nmaxlen = file.remain;

      if (nmaxlen <= 0) {
         mtklog(("ftp: readFile: max=%d return=%d", nmaxin, nmaxlen));
         return nmaxlen;
      }

      file.block++;

      if (nClSFTVer >= 102 && file.blockmode) 
      {
         num nreqlen = nmaxlen;

         // optim: on the first block, request only maxlen.
         // from the second block, transfer all in one.
         // NOTE: there can be no upper limit for this
         //       all-in-one size, as otherwise we have
         //       to remember subblock lengths which can
         //       become complicated.

         extern int fastMode();

         // with fastmode, get all in one from first block.
         int nbiglatch = fastMode() ? 1 : 2;

         if (file.block == nbiglatch) {
            // request bulk transfer, no further SREAD.
            nreqlen = file.remain;
         }

         if (file.block <= nbiglatch) {
            mtklog(("ftp: request %d remain=%s", nreqlen, numtoa(file.remain)));
            if (pcon->putf("SREAD %s\n", numtoa(nreqlen))) {
               perr("failed to send block request");
               return 0; // NOT -1, size_t problem
            }
         }
      }

      int nread = pClCtlCon->read(pbuf, nmaxlen);

      mtklog(("ftp: read %d", nread));

      if (nread > 0) 
      {
         if (file.md5) file.md5->update(pbuf, nread);
         file.remain -= nread;
         // reached end of content flank?
         if (file.remain == 0 && nClSFTVer >= 102) 
         {
            if (file.blockmode) {
               // request md5_post
               mtklog(("ftp: req sum"));
               if (pcon->putf("SSUM\n")) {
                  perr("failed to request checksum");
                  return 0; // NOT -1, size_t problem
               }
            }
            mtklog(("ftp: wait for sum"));
            if (pClCtlCon->read(file.abmd5, 16) != 16) {
               perr("failed to receive checksum");
               return 0; // NOT -1, size_t problem
            }
         }
      }
      mtklog(("ftp: readFile: max=%d return=%d", nmaxin, nread));
      // SKIP record will follow after content, we ignore that
      return nread;
   }
   else
   #endif // USE_SFT
   {
      if (!pClDatCon) {
         perr("ftp: cannot read, no download open");
         return 0;
      }
      return pClDatCon->read(pbuf, nmaxlen);
   }
}

void FTPClient::closeFile( )
{__
   char szBuf[300];

   #ifdef USE_SFT
   if (bClSFT)
   {
      // sender will wait now until we confirm successful transfer.
      TCPCon *pcon = pClCtlCon;

      mtklog(("ftp: closeFile %s remain=%d", file.name ? file.name : "", (int)file.remain));
   
      if (nClSFTVer >= 102) {
         if (file.blockmode) {
            // send close
            mtklog(("ftp: req close"));
            if (pcon->putf("SCLOSE\n")) {
               perr("failed to send close");
               return;
            }
         } else {
            // bulk SFT cannot handle premature file close
            if (file.remain > 0) {
               pwarn("bulk transfer but %d remaining for: %s\n", (int)file.remain, file.name ? file.name : "");
            }
            while (file.remain > 0) {
               int nread = readFile((uchar*)line(), lineMax());
               if (nread <= 0) break;
            }
         }
         // expect and receive SKIP record of any length
         mclear(szBuf);
         recv(pcon->clSock, (char*)szBuf, sizeof(szBuf)-10, 0);
         mtklog(("ftp: got past close: %s",szBuf));
         // ack is sent below
      } else {
         // old SFT cannot handle premature file close
         while (file.remain > 0) {
            int nread = readFile((uchar*)line(), lineMax());
            if (nread <= 0) break;
         }
      }

      if (!file.remain && file.md5) 
      {
         // check and clear md5
         uchar abdig[20];
         memcpy(abdig, file.md5->digest(), 16);

         delete file.md5;
         file.md5 = 0;

         if (memcmp(abdig, file.abmd5, 16)) {
            pcon->send((uchar*)"EE\n\n", 4);
            perr("md5 mismatch - transfered file corrupted (size=%d)\n", (int)file.size);
            return ;
         }
      }

      // send short confirmation, so peer can safely close socket.
      int nSent = pcon->send((uchar*)"OK\n\n", 4);
      if (nSent != 4) {
         perr("failed to send reply, %d\n", nSent);
         return;
      }
   }
   else
   #endif // USE_SFT
   {
      if (!pClDatCon) {
         perr("ftp: cannot close, no download open");
         return;
      }
      close(pClDatCon);
      pClDatCon = 0;

      // expect reply on control connection
      char *pline = readLine();
      if (pline && !strBegins(pline, "226 "))
         perr("ftp: unexpected reply after download: %s", pline);
   }
}

// returns RC
int HTTPClient::sendReq
 (
   cchar *pcmd,
   char *pfile,
   char *phost,
   int  nport
 )
{
   if (!pClCurCon)
      { perr("int. #175296 missing connection"); return 9; }

   char szPort[20];
   szPort[0] = '\0';
   if (nport != 80)
      sprintf(szPort, ":%u", nport);

   if (szClProxyHost[0])
   return pClCurCon->putf(
      "%s http://%s%s/%s HTTP/1.1\r\n"
      "Host: %s%s\r\n"
      "User-Agent: %s\r\n"
      "Accept: text/html;q=0.9,*/*;q=0.8\r\n"
      "Accept-Language: en-us,en;q=0.8\r\n"
      "Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7\r\n"
      "Proxy-Connection: close\r\n"
      "\r\n"
      , pcmd, phost, szPort, pfile
      , phost, szPort
      , getHTTPUserAgent()
      );
   else
   return pClCurCon->putf(
      "%s /%s HTTP/1.1\r\n"
      "Host: %s%s\r\n"
      "User-Agent: %s\r\n"
      "Accept: text/html;q=0.9,*/*;q=0.8\r\n"
      "Accept-Language: en-us,en;q=0.8\r\n"
      "Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7\r\n"
      "Connection: close\r\n"
      "\r\n"
      , pcmd, pfile, phost, szPort
      , getHTTPUserAgent()
      );
}

int HTTPClient::loadFile(char *purl, num nMaxSize, uchar **ppout, num &routlen)
{__
   // load the whole http text document
   if (splitURL(purl))
      return 9+perr("http: wrong url format: %s",purl);

   num nStart = getCurrentTime(); (void)nStart;

   char *phost = curhost();
   char *pfile = curpath();

   int nport = nClPort;
   if (connectHttp(phost, nport, &pClCurCon))
      return 9+perr("http connect failed: %s:%d",phost,nport);

   ConAutoClose ocls(this, &pClCurCon, __LINE__);

   if (sendReq("GET",pfile,phost,nport))
      return 9;

   // read headers, hoping(!) for a size info
   Coi ocoi(purl, 0); // tmp
   int nwebrc = 0;
   if (rawReadHeaders(nwebrc, purl, &ocoi)) return 9;
   // this also (re)sets the chunked mode

   num nAllocSize = 50000; // initial

   // http may or may not return a content-length
   num nTargetSize = 0;
   if (ocoi.hasSize()) {
      nTargetSize = ocoi.getSize();
      if (nTargetSize > nMaxSize)
         return 9+perr("too large, cannot load: %s", purl);
      nAllocSize = nTargetSize + 10; // safety
      mtklog(("read target size %d",(int)nTargetSize));
   }

   // alloc (initial) memory
   uchar *pdata = new uchar[nAllocSize+10];
   num nUsed = 0;

   mtklog(("read-start alloc %d used %d",(int)nAllocSize,(int)nUsed));

   // load content blockwise.
   // if the size was unknown, have to read til eod.
   while (1) 
   {
      num nRemain = (nTargetSize ? nTargetSize : nAllocSize) - nUsed;

      if (bClChunked) 
      {
         // read next chunk size first
         char *pline = pClCurCon->readLine();
         if (!pline) { delete [] pdata; return 9; }
         // conflict: bufsize might be smaller than this
         int nchunksize = strtol(pline, 0, 0x10);
         mtklog(("read-chunk clen %d \"%s\"", nchunksize, pline));
         if (!nchunksize) {
            if (strcmp(pline, "0"))
               perr("unexpected last-chunk end: \"%s\"", pline);
            break; // EOD flagged by "0" chunk size
         }
         if (nchunksize > nRemain) 
         {
            // resize buffer to fit whole chunk into
            num nAllocNew = nAllocSize * 2 + nchunksize + 10;
            uchar *pnew = new uchar[nAllocNew+10];
            memcpy(pnew, pdata, nUsed);
            delete [] pdata;
            // swap new and old
            pdata = pnew;
            nAllocSize = nAllocNew;
            mtklog(("read-chunk alloc %d used %d",(int)nAllocSize,(int)nUsed));
         }
         // make sure exactly the chunk is read
         nRemain = nchunksize;
      }
      else 
      if (nRemain < 20000)
      {
         // read stream of unknown size
         num nAllocNew = nAllocSize * 2;
         uchar *pnew = new uchar[nAllocNew+10];
         memcpy(pnew, pdata, nUsed);
         delete [] pdata;
   
         // swap new and old
         pdata = pnew;
         nAllocSize = nAllocNew;
         mtklog(("read-cont  alloc %d used %d",(int)nAllocSize,(int)nUsed));
      }

      num nRead = readraw(pdata+nUsed, nRemain);
      if (nRead <= 0)
         break; // EOD

      if (bClChunked)
         // every chunk is followed by CRLF
         pClCurCon->readLine();

      mtklog(("%d (%x) = readraw(max %d)",(int)nRead,(int)nRead,(int)nRemain));
      // mtkdump("raw ",pdata+nUsed,nRead);

      nUsed += nRead;
      if (nTargetSize > 0 && nUsed >= nTargetSize) {
         if (nUsed > nTargetSize) {
            delete [] pdata;
            return 9+perr("download has unexpected size: %s %d/%d", purl, (int)nUsed, (int)nTargetSize);
         }
         // all bytes read, stop immediately
         break;
      }
   }

   if (nTargetSize > 0 && nUsed < nTargetSize) {
      delete [] pdata;
      return 9+perr("download incomplete: %s %d/%d", purl, (int)nUsed, (int)nTargetSize);
   }

   // how much is left empty in read cache?
   num nRemain = nAllocSize - nUsed;
   if (nRemain > 1000) 
   {
      // adapt to the size really used
      num nAllocNew = nUsed + 10;
      uchar *pnew = new uchar[nAllocNew];
      memcpy(pnew, pdata, nUsed);
      delete [] pdata;
      // swap new and old
      pdata = pnew;
      nAllocSize = nAllocNew;
   }

   // result is managed by caller.
   pdata[nUsed] = '\0'; // is guaranteed
   *ppout  = pdata;
   routlen = (size_t)nUsed;

   mtklog(("%s downloaded in %d msec", purl, (int)(getCurrentTime() - nStart)));

   return 0;
}

int CoiData::getFtp(char *purl)
{__
   TCPCore::sysInit();

   if (pClFtp) return 0; // reuse

   int nprelen = strlen("ftp://");
   if ((int)strlen(purl) <= nprelen) return 9;

   // isolate base url with the hostname
   char szBase[200];
   strcopy(szBase, purl);
   char *psz = szBase + nprelen;
   psz = strchr(psz, '/');
   if (!psz) return 9;
   *psz = '\0';

   // now try to alloc the relevant client
   if (!(pClFtp = glblConCache.allocFtpClient(szBase))) {
      perr("unable to ftp to: %s", szBase);
      return 9;
   }

   return 0; // ok, use pClFtp
}

int CoiData::getHttp(char *purl)
{__
   TCPCore::sysInit();

   if (pClHttp) return 0; // reuse

   int nprelen = strlen("http://");
   if ((int)strlen(purl) <= nprelen) return 9;

   // isolate base url with the hostname
   char szBase[200];
   strcopy(szBase, purl);
   char *psz = szBase + nprelen;
   psz = strchr(psz, '/');

   if (psz)
      *psz = '\0';
   // else it is http://justhost

   // now try to alloc the relevant client
   if (!(pClHttp = glblConCache.allocHttpClient(szBase))) {
      perr("unable to http to: %s", szBase);
      return 9;
   }

   mtklog(("coidata.http.alloc done %p", pClHttp));

   return 0; // ok, use pClHttp
}

int CoiData::releaseFtp( ) 
{__
   if (!pClFtp)
      return 9+perr("releaseFtp without ptr, %p", this);
   pClFtp->decref();
   pClFtp = 0;
   return 0;
}

int CoiData::releaseHttp( ) 
{__
   if (!pClHttp)
      return 9+perr("releasehttp without ptr, %p", this);
   pClHttp->resetCache();
   pClHttp->decref();
   pClHttp = 0;
   return 0;
}

CoiTable &CoiData::elements( )
{
   if (!pelements) pelements = new CoiTable();
   return *pelements;
}

StringMap &CoiData::headers( )
{
   if (!pClHeaders) pClHeaders = new StringMap();
   return *pClHeaders;
}

StringMap &Coi::headers( ) {
   return data().headers();
}

char *Coi::header(cchar *pname) {
   if (!hasData()) return 0;
   if (!data().pClHeaders) return 0;
   return data().headers().get((char*)pname);
}

bool sameDomain(char *psz1in, char *psz2in, int &rdomlen)
{
   char *psz1 = psz1in;
   char *psz2 = psz2in;

   // http://sub.do.main.thehost.com/whatever1/
   // http://www.thehost.com/whatever2/
   if (!strBegins(psz1, "http://")) return 0;
   if (!strBegins(psz2, "http://")) return 0;
   psz1 += 7;
   psz2 += 7;

   // isolate main domains
   if (!(psz1 = strchr(psz1, '/'))) return 0;
   if (!(psz2 = strchr(psz2, '/'))) return 0;
   char *psla1 = psz1+1; // including slash
   char *psla2 = psz2+1; // including slash

   int nlen1 = psla1 - psz1in;
   int nlen2 = psla2 - psz2in;
   if (nlen1 != nlen2) return 0;

   bool brc = strncmp(psz1in, psz2in, nlen1) ? 0 : 1;

   if (brc) rdomlen = nlen1;

   return brc;

/*
   // step 2 dots back, or until /
   int ndots = 2;
   for (; psz1 > psz1in; psz1--) {
      if (*psz1 == '.' && !(--ndots)) break;
      if (*psz1 == '/') break;
   }
   char *pdom1 = psz1+1;   
   int  nlen1 = psla1 - pdom1;

   // same on psz2
   ndots = 2;
   for (; psz2 > psz2in; psz2--) {
      if (*psz2 == '.' && !(--ndots)) break;
      if (*psz2 == '/') break;
   }
   char *pdom2 = psz2+1;   
   int  nlen2 = psla2 - pdom2;

   if (nlen1 != nlen2) return 0;

   return strncmp(pdom1, pdom2, nlen1) ? 0 : 1;
*/
}

// list of non-traversable binary extensions
cchar *apBinExtList[] = {
   "jpg","gif","png",
   0 // EOD
};

int Coi::rawLoadDir( )
{__
   if (data().bloaddirdone) {
      mtklog(("coi::loaddir: done, %d entries", data().elements().numberOfEntries()));
      return 1; // already done
   }
   data().bloaddirdone = 1;

   #ifdef SFKDEEPZIP
   if (cs.probefiles)
      probeFile();
   #endif // SFKDEEPZIP

   if (isHttp())  return rawLoadHttpDir();
   if (isFtp())   return rawLoadFtpDir();

   return 9; // n/a with fsdirs
}

int Coi::rawLoadHttpDir( ) 
{__
   if (!isHttp()) return 9;

   mtklog(("rawOpenHttpDir %s", name()));

   if (data().getHttp(name())) return 9;
   HTTPClient *phttp = data().pClHttp;

   // use supplied client, release after dir download
   uchar *ppageu = 0;
   num    npageu = 0;
   if (phttp->loadFile(name(), 2000000, &ppageu, npageu)) {
      data().releaseHttp();
      return 9+perr("cannot open dir: %s", name());
   }
   // returned ppage is managed by us.

   char *ppage = (char*)ppageu;
   CharAutoDelete odel(&ppage);

   mtklog((" ... %d bytes page",(int)strlen(ppage)));

   // extract links from html page.
   char *psz1 = ppage;
   char *pszc = 0;
   for (; psz1 && *psz1; psz1 = pszc)
   {
      // find next "<a href" or "<img src"
      char *psz2 = psz1;
      char ceref = '"';
      for (; *psz2; psz2++) {
         // <a target="_blank" href=" ...
         if (!mystrnicmp(psz2, "<a ", 3)) {
            int npos = 0;
            if (   mystrstrip(psz2, " href=\"", &npos)
                || mystrstrip(psz2, " href='", &npos)
               )
            {
               ceref = psz2[npos+6];
               psz2 += npos+7;
               break; 
            }
            if (mystrstrip(psz2, " href=", &npos))
            {
               ceref = ' ';
               psz2 += npos+6;
               break; 
            }
         }
         if (!mystrnicmp(psz2, "<img ", 5)) {
            int npos = 0;
            if (   mystrstrip(psz2, " src=\"", &npos)
                || mystrstrip(psz2, " src='", &npos)
               )
            {
               ceref = psz2[npos+5];
               psz2 += npos+6;
               break; 
            }
         }
      }
      if (!*psz2) break;

      char *pref = psz2;
      psz2 = strchr(pref, ceref);
      if (!psz2) break;

      *psz2++ = '\0';

      mtklog(("rawref \"%.90s\"", pref));

      // set continue point past ref
      pszc = psz2;

      // process single ref
      if (strBegins(pref, "javascript:")) continue;
      // skip javascript url construction code
      if (strchr(pref, '\'')) continue;
      if (strchr(pref, ' ')) continue;
      // truncate '#' in the url, as they're for the browser only
      char *psz6 = strchr(pref, '#');
      if (psz6) *psz6 = '\0';

      // are we "http://thehost.com/" root?
      bool bNameIsRoot = 0;
      if (strBegins(name(), "http://")) {
         char *psz3 = name()+7;
         while (*psz3 && *psz3 != '/') psz3++;
         if (!*psz3 || !*(psz3+1))
            bNameIsRoot = 1;
      }

      bool bextref = 0;

      // for now, block
      // - stepping up ".."
      // - jumping to the root "/" except if name() is root
      // - external refs
      if (strBegins(pref, "../"))       continue;

      // this is relevant for circular deps only
      // if (*pref == '/' && !bNameIsRoot) continue;

      if (strBegins(pref, "https://"))  continue;
      if (strBegins(pref, "http://")) {
         // external or internal ref?
         int ndomlen = 0;
         bool brc = sameDomain(name(), pref, ndomlen);
         mtklog(("%d = sameDomain(%s, %s) %d",brc,name(),pref,ndomlen));
         if (!brc)
            bextref = 1; // continue; // ext
         else
            pref += (ndomlen - 1); // make it internal, keep first "/"
      } else {
         // block non-http protocols
         char *psz5 = pref;
         while (*psz5 && *psz5 != ':' && *psz5 != '/') psz5++;
         if (*psz5 == ':') continue; // e.g. mailto:
      }

      char *pabsref = pref;

      bool csExtDomRef();
      if (bextref && !csExtDomRef())
         continue;   // skip external domain refs

      if (!bextref) {
         // internal ref: create joined url
         if (phttp->joinURL(name(), pref)) {
            mtklog(("   cannot join ref"));
            continue;
         }
         pabsref = phttp->curjoin();
      }

      // TODO: how to handle dynamic pages?
      // for now, strip ALL dynamic parms
      // char *pquote = strchr(szBuf, '?');
      // if (pquote) *pquote = '\0';

      // find out if already collected
      if (glblCircleMap.isset(pabsref))
         continue; // is a dup

      // no dup: remember in circle map
      glblCircleMap.put(pabsref, 0);

      // quick identify binaries by extension
      int iext=0;
      for (; apBinExtList[iext]; iext++)
         if (mystrstri(pabsref, apBinExtList[iext]))
            break;

   /*
      if (apBinExtList[iext]) {
         // is binary by extension, add simple file entry
         Coi ocoi(pabsref, name());
         data().elements().addEntry(ocoi); // is copied
         mtklog(("   added binfile \"%s\" offs %d", pabsref, (int)(psz1-ppage)));
         continue;
      }
   */

      SFKFindData myfdat;
      memset(&myfdat, 0, sizeof(myfdat));
      myfdat.size       = 0;
      myfdat.time_write = 0;

      Coi ocoi(pabsref, name());
      ocoi.fillFrom(&myfdat); // avoid later stat's
      data().elements().addEntry(ocoi); // is copied

   /* 
      // now identify the content type
      if (phttp->getFileHead(pabsref, &ocoi, "opendir"))
         continue; // cannot get header

      if (ocoi.isBinaryFile()) {
         // is binary (image), add simple file entry
         data().elements().addEntry(ocoi); // is copied
         mtklog(("   added binfile \"%s\" offs %d", pabsref, (int)(psz1-ppage)));
      }
      else 
      {
         // is text/html, therefore

         // 1. create a directory entry
         ocoi.setIsDir(1);
         data().elements().addEntry(ocoi); // is copied
         mtklog(("   added dir  \"%s\" offs %d", pabsref, (int)(psz1-ppage)));

         // 2. AND create a file entry with the same name
         ocoi.setIsDir(0);
         data().elements().addEntry(ocoi); // is copied
         mtklog(("   added file \"%s\" offs %d", pabsref, (int)(psz1-ppage)));
      }
   */

      // continue searching
   }

   // start from entry 0
   data().nNextElemEntry = 0;

   // ppage is auto deleted.
   data().releaseHttp();

   return 0;
}

// caller MUST RELEASE COI after use!
Coi *Coi::rawNextHttpEntry( ) 
{__
   if (!data().bdiropen) {
      perr("http nextEntry() called without openDir()");
      mtklog(("http nextEntry without openDir coi %p", this));
      return 0;
   }

   mtklog(("rawNextHttpEntry %s", name()));

   Coi *psubsrc = 0;

   do
   {
      // something left to return?
      if (data().nNextElemEntry >= data().elements().numberOfEntries())
         return 0; // end of list

      // return a COPY from the internal list entries.
      psubsrc = data().elements().getEntry(data().nNextElemEntry, __LINE__);
      data().nNextElemEntry++;
   }
   while (0); // (psubsrc->isTravelDir()); // for now, SKIP dir entries of ftp

   // if there is a global cache entry machting this sub,
   Coi *pcached = glblVCache.get(psubsrc->name());

   mtklog(("rawNextHttpEntry %p = cache.get(%s)", pcached, psubsrc->name()));

   // caller MUST RELEASE COI after use!

   if (pcached) 
   {
      bool bhasptr = pcached->data().src.data ? 1 : 0; (void)bhasptr;
      mtklog(("coi::nexthttpentry cache hit %d %d %s", (int)bhasptr, (int)pcached->data().src.size, pcached->name()));
      return pcached;
   }
   else 
   {
      Coi *psubdst = psubsrc->copy();
      psubdst->incref("nht");
      mtklog(("coi::nexthttpentry cache miss, returning copy of %s", psubdst->name()));
      return psubdst;
   }
}

void Coi::rawCloseHttpDir ( ) 
{__
   // see remarks in rawCloseZipDir
   mtklog(("rawCloseHttpDir %s coi %p", name(), this));
   data().bdiropen = 0;
}

// caller MUST RELEASE COI after use!
Coi *Coi::rawNextFtpEntry( ) 
{
   if (!data().bdiropen) {
      perr("ftp nextEntry() called without openDir()");
      return 0;
   }

   Coi *psubsrc = 0;

   #ifndef DEEP_FTP
   do
   #endif
   {
      // something left to return?
      if (data().nNextElemEntry >= data().elements().numberOfEntries())
         return 0; // end of list

      // return a COPY from the internal list entries.
      psubsrc = data().elements().getEntry(data().nNextElemEntry, __LINE__);
      data().nNextElemEntry++;
   }
   #ifndef DEEP_FTP
   while (psubsrc->isTravelDir()); // for now, SKIP dir entries of ftp
   #endif

   // if there is a global cache entry machting this sub,
   Coi *pcached = glblVCache.get(psubsrc->name());

   // caller MUST RELEASE COI after use!

   if (pcached) 
   {
      bool bhasptr = pcached->data().src.data ? 1 : 0; (void)bhasptr;
      mtklog(("coi::nextftpentry cache hit %d %d %s", (int)bhasptr, (int)pcached->data().src.size, pcached->name()));
      return pcached;
   }
   else 
   {
      Coi *psubdst = psubsrc->copy();
      psubdst->incref("nft");
      mtklog(("coi::nextftpentry cache miss, returning copy of %s", psubdst->name()));
      return psubdst;
   }
}

void Coi::rawCloseFtpDir( ) {
   // see remarks in rawCloseZipDir
   data().bdiropen = 0;
}

int Coi::rawOpenFtpSubFile(cchar *pmode) 
{
   if (!isFtp()) return 9;
   if (strcmp(pmode, "rb")) return 9;

   // get client for that base url
   if (data().getFtp(name())) return 9;

   // use supplied client, release after dir download
   FTPClient *pftp = data().pClFtp;

   // isolate hostname from url
   if (pftp->splitURL(name())) {
      perr("ftp: wrong url format: %s", name());
      return 9;
   }
   // nRelIndex is now the relative path start index.
   char *phost    = pftp->curhost();
   char *prelfile = pftp->curpath();
   int  nport    = pftp->curport();

   mtklog((" ftp open host \"%s\" file \"%s\"", phost, prelfile));

   int nrc = 0;

   for (int itry=0; itry<2; itry++)
   {
      if (pftp->loginOnDemand(phost, nport))
         return 9;

      nrc = pftp->openFile(prelfile, pmode);
      // RC  9 == general error, e.g. file not available
      // RC 10 == communication failed, connection invalid

      if (nrc < 10)
         break;

      // loginOnDemand thought the line is still valid, but it isn't.
      pinf("ftp session expired, retrying\n");
   
      // devalidate session, relogin and retry
      pftp->logout();
   }
   
   if (nrc) {
      if (nrc > 9) {
         pinf("ftp communication failed (connection closed)\n");
         pftp->logout();
      }
      data().releaseFtp();  // failed
   }
   else
      data().bfileopen = 1;

   // general releaseFtp is done after file download.

   return nrc;
}

int Coi::rawOpenHttpSubFile(cchar *pmode)
{__
   if (!isHttp()) return 9;
   if (strcmp(pmode, "rb")) return 9;

   if (data().getHttp(name())) return 9;
   HTTPClient *phttp = data().pClHttp;

   mtklog(("http-open %s",name()));

   int nrc = phttp->open(name(), pmode, this);
   // may redirect and change current coi's name!

   if (!nrc) data().bfileopen = 1;

   return nrc;
}

extern char *getxlinfo();
extern num   getxllim();

size_t Coi::rawReadFtpSubFile(void *pbufin, size_t nBufSize) 
{
   if (!data().pClFtp || !data().bfileopen) {
      perr("ftp not open, cannot read: %s (%d)", name(), data().bfileopen);
      return 0;
   }

   int nread = data().pClFtp->readFile((uchar*)pbufin, (int)nBufSize);
   mtklog((" ftp read %s %u done %d", name(), (uint)nBufSize, nread));

   return nread;
}

size_t Coi::rawReadHttpSubFile(void *pbufin, size_t nBufSize) 
{__
   if (!data().pClHttp || !data().bfileopen) {
      perr("http not open, cannot read: %s (%d)", name(), data().bfileopen);
      return 0;
   }

   int nread = data().pClHttp->read((uchar*)pbufin, (int)nBufSize);
   mtklog(("http-read %s done=%d max=%d", name(), nread, (int)nBufSize));

   return nread;
}

void Coi::rawCloseFtpSubFile( ) 
{
   mtklog(("ftp-close %s", name()));

   if (!data().pClFtp) { perr("ftp not open, cannot close: %s", name()); return; }

   data().pClFtp->closeFile();

   // release connection ptr, without closing it:
   data().releaseFtp();

   data().bfileopen = 0;
}

void Coi::rawCloseHttpSubFile( ) 
{__
   mtklog(("http-close %s", name()));

   if (!data().pClHttp) {
      perr("http not open, cannot close: %s", name());
      return; 
   }

   // so far, coi http core can NOT reuse the same connection
   // for multiple commands, so we MUST close the socket.
   // (otherwise GET without full read must be avoided)
   data().pClHttp->close();

   // release connection ptr, without closing it:
   data().releaseHttp();

   data().bfileopen = 0;
}

int Coi::prefetch(bool bLoadNonArcBinaries, num nFogSizeLimit, num nHardSizeLimit)
{
	// this is only for virtual network files
	if (!isNet()) return 1;	// ignore

   if (open("rb")) return 9;

   // NO RETURN WITHOUT CLOSE BEGIN

	// -	after coi::open, binary and arc status is known.
	bool bIsKnownBinary = ((nClHave & COI_HAVE_BINARY) && bClBinary) ? 1 : 0;
   bool bIsKnownArc    = ((nClHave & COI_HAVE_ARC   ) && bClArc   ) ? 1 : 0;

   if (bIsKnownBinary && !bLoadNonArcBinaries && !bIsKnownArc) 
   {
      cchar *pctype = header("content-type"); // or null
      if (!pctype) pctype = "n/a";
      close();
      return 2;   // skip download and caching as it's not text, and no archive
   }

   num    nalloc = 10000;
   uchar *pdata  = new uchar[nalloc+100];
   num    nused  = 0;

   while (1)
   {
      num nrem  = nalloc - nused;
      if (nrem < nalloc / 4) {
         // expand buffer
         num nalloc2 = nalloc * 2;
         uchar *ptmp = new uchar[nalloc2+100];
         memcpy(ptmp, pdata, nused);
         // swap old and new
         delete [] pdata;
         pdata  = ptmp;
         nalloc = nalloc2;
         nrem   = nalloc - nused;
      }
      num nread = read(pdata+nused, nrem);
      if (nread <= 0) break;
      // else continue til EOD
      nused += nread;
   }

   // how much is left empty in read cache?
   num nrem = nalloc - nused;
   if (nrem > 1000)
   {
      // adapt to the size really used
      num nalloc2 = nused + 10;
      uchar *ptmp = new uchar[nalloc2];
      memcpy(ptmp, pdata, nused);
      // swap old and new
      delete [] pdata;
      pdata  = ptmp;
      nalloc = nalloc2;
   }

   // close asap, caller may change our data().src
   close();

   // NO RETURN WITHOUT CLOSE END

   // result is managed by caller.
   pdata[nused] = '\0'; // is guaranteed

   // will delete old data, if any:
   setContent(pdata, nused);

	return 0;
}

int Coi::loadOwnFileRaw(num nmaxsize, uchar **ppout, num &rsize)
{
   // no provideInput() here, EXCEPT:
   // - zipSubEntries may call provideInput thru open() on parent

   // load file without any size info
   if (open("rb")) return 9;
   
   num    nalloc = 10000;
   uchar *pdata  = new uchar[nalloc+100];
   num    nused  = 0;

   while (1) 
   {
      num nrem  = nalloc - nused;
      if (nrem < nalloc / 4) {
         // expand buffer
         num nalloc2 = nalloc * 2;
         uchar *ptmp = new uchar[nalloc2+100];
         memcpy(ptmp, pdata, nused);
         // swap old and new
         delete [] pdata;
         pdata  = ptmp;
         nalloc = nalloc2;
         nrem   = nalloc - nused;
      }
      num nread = read(pdata+nused, nrem);
      if (nread <= 0) break;
      // else continue til EOD
      nused += nread;
   }

   // how much is left empty in read cache?
   num nrem = nalloc - nused;
   if (nrem > 1000) 
   {
      // adapt to the size really used
      num nalloc2 = nused + 10;
      uchar *ptmp = new uchar[nalloc2];
      memcpy(ptmp, pdata, nused);
      // swap old and new
      delete [] pdata;
      pdata  = ptmp;
      nalloc = nalloc2;
   }

   // close asap, caller may change our data().src
   close();

   // result is managed by caller.
   pdata[nused] = '\0'; // is guaranteed
   *ppout = pdata;
   rsize  = nused;

   return 0;
}

// rc  0: ok, really loaded
// rc >0: not loaded, e.g. because file is too large
int Coi::provideInput(int nTraceLine, bool bsilent)
{__
   if (isNet() && isTravelZip(110,1))
      {_ } // accept, need to cache whole file
   else
   if (isZipSubEntry())
      {_ } // accept, need to cache sub entry via parent
   else {
      mtklog(("coi::provideInput not needed: %s net=%d tz=%d", name(),isNet(),isTravelZip(110,1)));
_     return 0; // nothing to do
   }

   // is input already loaded?
   if (data().src.data)
      return 0; // nothing to do

   mtklog(("coi::provideInput from %d", nTraceLine));

   // strictly need to check the size here:
   num nSize = getSize();
   if (nSize > nGlblMemLimit) {
      snprintf(data().szlasterr, sizeof(data().szlasterr)-4,
         " - content too large (%u mb)", (uint)(nClSize/1000000UL));
      tellMemLimitInfo(); // just once
      return 1; // block loading
   }

   int nrc = 0;

   mtklog(("loadownraw, unknown size: %s", name()));
   uchar *pdata = 0;
   num    nsize = 0;
   if ((nrc = loadOwnFileRaw(100 * 1000000, &pdata, nsize))) {
      mtklog(("loadownraw failed, %d", nrc));
      return 9;   // failed
   }

   mtklog(("cload ok, %d bytes: %s", (int)nsize, name()));

   setContent(pdata, nsize);

   return 0;
}

Coi *Coi::getElementByAbsName(char *pabsname)
{__
   if (!data().elements().numberOfEntries())
      if (rawLoadDir() >= 5)
         return 0;

   // check absname, if it matches ourselves at all
   if (!striBegins(pabsname, name())) {
      // should NOT happen
      pwarn("cannot get element, name mismatch: %s / %s", pabsname, name());
      return 0;
   }

   Coi *psub = 0;
   for (int i=0; i<data().elements().numberOfEntries(); i++)
   {
      psub = data().elements().getEntry(i, __LINE__);
      if (!strcmp(psub->name(), pabsname)) break;
   }

   return psub;
}

bool Coi::isFtp() {
   #ifdef VFILENET
	if (nGlblCoiConfig & COI_CAPABILITY_NET)
	   return strBegins(name(), "ftp://");
   #endif // VFILENET
	return 0;
}

bool Coi::isHttp() {
   #ifdef VFILENET
	if (nGlblCoiConfig & COI_CAPABILITY_NET)
	   return strBegins(name(), "http://");
   #endif // VFILENET
	return 0;
}

bool Coi::isNet() {
   return isHttp() || isFtp();
}

bool Coi::isVirtual(bool bWithRootZips)
{
   if (isNet() || isZipSubEntry())     return 1;
   if (bWithRootZips && isTravelZip(109)) return 1;
   return 0;
}

bool Coi::rawIsFtpDir() 
{
   if (!isFtp()) return 0;

   // try to detect by name
   char *pnam = name();
   int nlen  = strlen(pnam);
   if (nlen > 0 && pnam[nlen-1] == '/')
      return 1;

   // but also use the coi flag
   return bClDir;
}

// TODO: rework error rc handling?
bool Coi::rawIsHttpDir( ) 
{__
   #ifndef SFKSKIP01

   if (!isHttp()) return 0;

   extern bool httpTravel();
   if (!httpTravel()) return 0;

   // name() must point to a html page
   if (data().getHttp(name())) return 0;

   HTTPClient *phttp = data().pClHttp;
   if (phttp->getFileHead(name(), this, "isdir")) {
      perr("cannot get headers: %s", name());
      return 0;
   }
   // -> sets isBinary(), getSize()

   // if it's text, assume content with links
   if (isBinaryFile()) return 0;

   // accept only text below 1 MB
   if (getSize() > 1000000) return 0;

   // a text document of reasonable size
   mtklog(("http-istext %s root %s", name(), pszClRoot ? pszClRoot:"none"));

   // without any root, consider as a dir
   // if (!pszClRoot) return 1;

   // if the root matches the own url, then as well
   // if (!strcmp(pszClRoot, name())) return 1;

   // direct query of text document, without context:
   // always consider as a searchable dir.
   return 1;

   #endif // SFKSKIP01
}

int Coi::rawLoadFtpDir( ) 
{
   if (!isFtp()) return 9;

   // get client for that base url
   if (data().getFtp(name())) return 9;

   // use supplied client, release after dir download
   FTPClient *pftp = data().pClFtp;
   if (pftp->splitURL(name())) {
      perr("ftp: wrong url format: %s", name());
      data().releaseFtp();
      return 9;
   }
   // nRelIndex is now the relative path start index.
   char *phost = pftp->curhost();
   char *ppath = pftp->curpath();
   int  nport = pftp->curport();

   mtklog((" ftp open host \"%s\" path \"%s\"", phost, ppath));

   if (pftp->loginOnDemand(phost, nport)) {
      data().releaseFtp();
      return 9+perr("ftp login failed: %s", phost);
   }

   // let the ftp client fill our elements list.
   // it must prefix any name by our name.
   CoiTable *plist = &data().elements();
   plist->resetEntries(); // if any
   if (pftp->list(ppath, &plist, name())) {
      data().releaseFtp();
      return 9+perr("ftp list failed: %s", phost);
   }

   // list downloaded: do NOT close connection
   //   pftp->logout();
   // as it may be used for subsequent downloads

   // start from zip entry 0
   data().nNextElemEntry = 0;

   // current I/O job done: release the client,
   // meaning the refcnt is dec'ed, but NO logout.
   data().releaseFtp();

   return 0;
}

num Coi::getUsedBytes()
{
   num nsize = sizeof(*this);

   if (pszClName)   nsize += strlen(pszClName)+1;
   if (pszClRoot)   nsize += strlen(pszClRoot)+1;
   if (pszClRef)    nsize += strlen(pszClRef)+1;
   if (pszClExtStr) nsize += strlen(pszClExtStr)+1;

   if (hasData()) {
      nsize += sizeof(CoiData);
      CoiData *p = pdata;
      nsize += p->src.size;
      if (p->prelsubname)  nsize += strlen(p->prelsubname)+1;
      if (p->pdirpat    )  nsize += strlen(p->pdirpat)+1;
   }

   // NOT considered, as they are volatile and may have
   // different values on cache put and remove:
   //    rbuf.data, pzip

   return nsize;
}

#endif // VFILEBASE

// ----------------------------------------------------------------

int netErrno()
{
   #ifdef _WIN32
   return WSAGetLastError();
   #else
   return errno;
   #endif
}

char *netErrStr(int ncode)
{
   if (ncode < 0)
      ncode = netErrno();

   static char szErrBuf[200];
 
   const char *perr = "";

   #ifdef _WIN32
   switch (ncode)
   {
      case WSAEWOULDBLOCK      : perr="WSAEWOULDBLOCK"; break;
      case WSAEINPROGRESS      : perr="WSAEINPROGRESS"; break;
      case WSAEALREADY         : perr="WSAEALREADY"; break;
      case WSAENOTSOCK         : perr="WSAENOTSOCK"; break;
      case WSAEDESTADDRREQ     : perr="WSAEDESTADDRREQ"; break;
      case WSAEMSGSIZE         : perr="WSAEMSGSIZE"; break;
      case WSAEPROTOTYPE       : perr="WSAEPROTOTYPE"; break;
      case WSAENOPROTOOPT      : perr="WSAENOPROTOOPT"; break;
      case WSAEPROTONOSUPPORT  : perr="WSAEPROTONOSUPPORT"; break;
      case WSAESOCKTNOSUPPORT  : perr="WSAESOCKTNOSUPPORT"; break;
      case WSAEOPNOTSUPP       : perr="WSAEOPNOTSUPP"; break;
      case WSAEPFNOSUPPORT     : perr="WSAEPFNOSUPPORT"; break;
      case WSAEAFNOSUPPORT     : perr="WSAEAFNOSUPPORT"; break;
      case WSAEADDRINUSE       : perr="WSAEADDRINUSE"; break;
      case WSAEADDRNOTAVAIL    : perr="WSAEADDRNOTAVAIL"; break;
      case WSAENETDOWN         : perr="WSAENETDOWN"; break;
      case WSAENETUNREACH      : perr="WSAENETUNREACH"; break;
      case WSAENETRESET        : perr="WSAENETRESET"; break;
      case WSAECONNABORTED     : perr="WSAECONNABORTED"; break;
      case WSAECONNRESET       : perr="WSAECONNRESET"; break;
      case WSAENOBUFS          : perr="WSAENOBUFS"; break;
      case WSAEISCONN          : perr="WSAEISCONN"; break;
      case WSAENOTCONN         : perr="WSAENOTCONN"; break;
      case WSAESHUTDOWN        : perr="WSAESHUTDOWN"; break;
      case WSAETOOMANYREFS     : perr="WSAETOOMANYREFS"; break;
      case WSAETIMEDOUT        : perr="WSAETIMEDOUT"; break;
      case WSAECONNREFUSED     : perr="WSAECONNREFUSED"; break;
      case WSAELOOP            : perr="WSAELOOP"; break;
      case WSAENAMETOOLONG     : perr="WSAENAMETOOLONG"; break;
      case WSAEHOSTDOWN        : perr="WSAEHOSTDOWN"; break;
      case WSAEHOSTUNREACH     : perr="WSAEHOSTUNREACH"; break;
      case WSAENOTEMPTY        : perr="WSAENOTEMPTY"; break;
      case WSAEUSERS           : perr="WSAEUSERS"; break;
      case WSAEDQUOT           : perr="WSAEDQUOT"; break;
      case WSAESTALE           : perr="WSAESTALE"; break;
      case WSAEREMOTE          : perr="WSAEREMOTE"; break;
   }
   #else
   switch (ncode)
   {
      case EWOULDBLOCK         : perr="EWOULDBLOCK"; break;
      case EINPROGRESS         : perr="EINPROGRESS"; break;
      case EALREADY            : perr="EALREADY"; break;
      case ENOTSOCK            : perr="ENOTSOCK"; break;
      case EDESTADDRREQ        : perr="EDESTADDRREQ"; break;
      case EMSGSIZE            : perr="EMSGSIZE"; break;
      case EPROTOTYPE          : perr="EPROTOTYPE"; break;
      case ENOPROTOOPT         : perr="ENOPROTOOPT"; break;
      case EPROTONOSUPPORT     : perr="EPROTONOSUPPORT"; break;
      case ESOCKTNOSUPPORT     : perr="ESOCKTNOSUPPORT"; break;
      case EOPNOTSUPP          : perr="EOPNOTSUPP"; break;
      case EPFNOSUPPORT        : perr="EPFNOSUPPORT"; break;
      case EAFNOSUPPORT        : perr="EAFNOSUPPORT"; break;
      case EADDRINUSE          : perr="EADDRINUSE"; break;
      case EADDRNOTAVAIL       : perr="EADDRNOTAVAIL"; break;
      case ENETDOWN            : perr="ENETDOWN"; break;
      case ENETUNREACH         : perr="ENETUNREACH"; break;
      case ENETRESET           : perr="ENETRESET"; break;
      case ECONNABORTED        : perr="ECONNABORTED"; break;
      case ECONNRESET          : perr="ECONNRESET"; break;
      case ENOBUFS             : perr="ENOBUFS"; break;
      case EISCONN             : perr="EISCONN"; break;
      case ENOTCONN            : perr="ENOTCONN"; break;
      case ESHUTDOWN           : perr="ESHUTDOWN"; break;
      case ETOOMANYREFS        : perr="ETOOMANYREFS"; break;
      case ETIMEDOUT           : perr="ETIMEDOUT"; break;
      case ECONNREFUSED        : perr="ECONNREFUSED"; break;
      case ELOOP               : perr="ELOOP"; break;
      case ENAMETOOLONG        : perr="ENAMETOOLONG"; break;
      case EHOSTDOWN           : perr="EHOSTDOWN"; break;
      case EHOSTUNREACH        : perr="EHOSTUNREACH"; break;
      case ENOTEMPTY           : perr="ENOTEMPTY"; break;
      case EUSERS              : perr="EUSERS"; break;
      case EDQUOT              : perr="EDQUOT"; break;
      case ESTALE              : perr="ESTALE"; break;
      case EREMOTE             : perr="EREMOTE"; break;
   }
   #endif

   #ifdef _WIN32
   if (strBegins((char*)perr, "WSA")) perr += 3;
   #endif

   if (strlen(perr))
      snprintf(szErrBuf, sizeof(szErrBuf)-10, "status=%d (%s)", ncode, perr);
   else
      snprintf(szErrBuf, sizeof(szErrBuf)-10, "status=%d", ncode);

   return szErrBuf;
}

// ----------------------------------------------------------------

UDPIO::UDPIO( )
{
   rawInit();
}

void UDPIO::rawInit( )
{
   memset(this, 0, sizeof(*this));

   fdClSocket = INVALID_SOCKET;
   mclear(szClDescription);
   mclear(clTargetAddr);
   mclear(clRawInAddr);
   mclear(clInBufInAddr);
   iClOwnReceivePort = 0;
   iClTargetSendPort = 0;
   bClMulticast = 0;
   bClVerbose = false;
   iClPackageSize = 1000;
   bClRawText = 0;
   iClReqNum = 1;
   iClTimeout = 1000;
   cClCurrentInColor = ' ';
   cClTellColor = '\0';
   iClUsingAltPortInsteadOf = -1;
   iClNonDuplexSendDelay = 10;
}

UDPIO::~UDPIO( )
{
   if (fdClSocket != INVALID_SOCKET)
      closesocket(fdClSocket);
}

bool UDPIO::isOpen( )
{
   return (fdClSocket != INVALID_SOCKET) ? 1 : 0;
}

int UDPIO::closeAll( )
{
   if (fdClSocket != INVALID_SOCKET)
   {
      closesocket(fdClSocket);
   }

   rawInit();
 
   return 0;
}

bool UDPIO::isMulticast( )
{
   return bClMulticast;
}

// RC  9 : cannot create socket
//    10 : cannot bind socket
//    11 : cannot multicast
int UDPIO::initSendReceive
 (
   const char *pszDescription,
   int iOwnReceivePort,
   int iTargetSendPort,
   char *pszTargetAddress,
   uint uiFlags
 )
{
   strcopy(szClDescription, pszDescription);

   iClOwnReceivePort = iOwnReceivePort;
   iClTargetSendPort = iTargetSendPort;
   iClUsingAltPortInsteadOf = -1;

   // printf("UDP: initSendReceive ownport=%d targport=%d addr=%s\n",
   //   iOwnReceivePort, iTargetSendPort, pszTargetAddress);

   bClMulticast = (uiFlags & 1) ? 1 : 0;
   bool bReuse  = (uiFlags & 2) ? 1 : 0;
   bool bRetry  = (uiFlags & 4) ? 1 : 0;

   if (pszTargetAddress && strchr(pszTargetAddress, '.'))
   {
      // check for multicast addresses "224.x" to "239.x"
      int iFirstPart = atoi(pszTargetAddress);
      if (iFirstPart >= 224 && iFirstPart <= 239)
         bClMulticast = true;
   }

   int fdTmp = socket(AF_INET, SOCK_DGRAM, 0);

   if (fdTmp < 0)
   {
      perr("Error creating socket for %s. errno=%u %s\n",
         szClDescription, netErrno(), netErrStr());
      return 9;
   }

   if (bReuse)
   {
      int nOnVal = 1;
      setsockopt(fdTmp, SOL_SOCKET, SO_REUSEADDR, (const char *)&nOnVal, sizeof(nOnVal));
   }

   if (iOwnReceivePort >= 0)
   {
      int iMaxTry = bRetry ? 5 : 1;

      for (int itry=0; itry<iMaxTry; itry++)
      {
         // on port conflict, retry 10 ports higher
         iClOwnReceivePort = iOwnReceivePort + itry * 10;
   
         // own address for input
         struct sockaddr_in saOwnAddr;
         memset((char *)&saOwnAddr, 0,sizeof(saOwnAddr));

         saOwnAddr.sin_family      = AF_INET;
         saOwnAddr.sin_port        = htons(iClOwnReceivePort);
         saOwnAddr.sin_addr.s_addr = INADDR_ANY;
      
         if (bind(fdTmp, (struct sockaddr *)&saOwnAddr, sizeof(saOwnAddr)) >= 0) {
            if (itry > 0)
               iClUsingAltPortInsteadOf = iOwnReceivePort;
            break; // OK
         }

         if (itry < iMaxTry-1) {
            pwarn("Cannot listen on port %d, retrying on %d.", iClOwnReceivePort, iClOwnReceivePort+10);
            // and reloop
         } else {
            perr("Cannot listen on port %d, rc=%d.", iClOwnReceivePort, netErrno());
            perr("Missing access rights, or port used by other program.\n");
            return 10;
         }
      }
   }

   if (isMulticast())
   {
      struct ip_mreq mreq;
      memset(&mreq, 0, sizeof(mreq));
      mreq.imr_interface.s_addr = htonl(INADDR_ANY);

      #if defined(MAC_OS_X) || defined(SOLARIS)
        #define SOL_IP IPPROTO_IP
      #endif

      #ifdef _WIN32
 
      char name[512];
      PHOSTENT hostinfo;
      mclear(name);
      mclear(hostinfo);

      if (gethostname(name, sizeof(name)))
         return 11+perr("gethostname failed\n");

      if (!(hostinfo=gethostbyname(name)))
         return 11+perr("get ownhost failed (%s) (2)\n", name);

      struct in_addr *pin_addr = (struct in_addr *)*hostinfo->h_addr_list;
      mreq.imr_interface.s_addr = pin_addr->s_addr;
      mreq.imr_multiaddr.s_addr = inet_addr(pszTargetAddress);

      // force IP_ADD_MEMBERSHIP of ws2tcpip.h
      #define MY_IP_ADD_MEMBERSHIP 12

      if (setsockopt(fdTmp, IPPROTO_IP, MY_IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof(mreq)) != 0 )
         return 11+perr("Join multicast failed. Errno=%u (%s)",  netErrno(), netErrStr());

      // in case of error 10042 see
      //    http://support.microsoft.com/kb/257460
      // wrong winsocket header, runtime linkage etc.
 
      #else

      if (inet_aton(pszTargetAddress, &mreq.imr_multiaddr) == 0)
      {
         perr("Join multicast failed: bad address %s\n", pszTargetAddress);
         return 11; // cannot multicast
      }

      if (setsockopt(fdTmp, SOL_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) != 0 )
      {
         perr("No default-route to support multicast.");
         perr("Try 'route add -net 224.000 netmask 240.000 eth0'");
         return 11;
      }

      #endif
 
      // printf("mcast: fd=%d outport=%d inport=%d group=%s active.\n",
      //   fdTmp, iClTargetSendPort, iClOwnReceivePort, pszTargetAddress);

      // do not call gethostbyname, but set addr manually
      memset((char *)&clTargetAddr, 0,sizeof(clTargetAddr));

      clTargetAddr.sin_family      = AF_INET;
      clTargetAddr.sin_addr.s_addr = inet_addr(pszTargetAddress);
      clTargetAddr.sin_port        = htons((int)iClTargetSendPort);
   }
   else if (pszTargetAddress)
   {
      // printf("udp: fd=%d outport=%d inport=%d active.\n",
      //   fdTmp, iClTargetSendPort, iClOwnReceivePort);

      if (setTarget(pszTargetAddress, iClTargetSendPort))
         return 12;
   }
 
   fdClSocket = fdTmp;

   return 0;
}

int UDPIO::setTarget(char *pszTargetAddress, int iTargetPort)
{
   memset((char *)&clTargetAddr, 0,sizeof(clTargetAddr));

   struct hostent *pTarget;
   if ((pTarget = sfkhostbyname(pszTargetAddress)) == NULL)
      return 11+perr("cannot get host %s, rc=%d\n", pszTargetAddress, netErrno());

   clTargetAddr.sin_family = AF_INET;
   memcpy(&clTargetAddr.sin_addr.s_addr, pTarget->h_addr, pTarget->h_length);
   clTargetAddr.sin_port   = htons((int)iTargetPort);

   return 0;
}

// RC  0 : OK
//     9 : send failed, no socket
//    10 : send failed, transmission error
//    11 : send failed, wrong parameters
int UDPIO::sendData(uchar *pData, int iDataSize)
{
   if (fdClSocket == INVALID_SOCKET)
      return 9+perr("send: invalid UDP socket\n");

   if (sendto(fdClSocket, (char *)pData, iDataSize, 0,
          (struct sockaddr *)&clTargetAddr, sizeof(clTargetAddr)) != iDataSize)
   {
      perr("send: failed errno=%d %s\n", netErrno(), netErrStr());
      return 12;
   }

   return 0;
}

// RC >  0 : number of bytes received
// RC <= 0 : error
int UDPIO::receiveData(
   uchar *pBuffer, int iBufferSize,
   struct sockaddr_in *pAddrIncoming,
   int iSizeOfAddrIncoming
 )
{
   if (fdClSocket == INVALID_SOCKET)
      return -1;

   int iRead = 0;

   if (isDataAvailable(0, 0))
   {
      struct sockaddr addrIncoming;
      memset(&addrIncoming, 0, sizeof(addrIncoming));
   
      socklen_t clilen = sizeof(addrIncoming);
   
      // receive with 10 bytes buffer tolerance.
      // result is guaranteed to be zero terminated.
      iRead = recvfrom(fdClSocket, (char*)pBuffer, iBufferSize-10, 0, &addrIncoming, &clilen);
   
      if (pAddrIncoming != 0 && clilen == iSizeOfAddrIncoming)
         memcpy(pAddrIncoming, &addrIncoming, iSizeOfAddrIncoming);
   }

   // always guarantee zero termination.
   if (iRead >= 0)
      pBuffer[iRead] = '\0';

   return iRead;
}

bool UDPIO::isDataAvailable(int iSec, int iMSec)
{
   if (fdClSocket == INVALID_SOCKET)
      return false;

   struct timeval tv;
   fd_set fdvar;

   tv.tv_sec  = iSec;
   tv.tv_usec = iMSec ? iMSec * 1000 : 100;

   FD_ZERO(&fdvar);
   FD_SET(fdClSocket, &fdvar);

   if (select(fdClSocket+1, &fdvar, 0, 0, &tv) > 0)
      return 1;

   return 0;
}

// SYNC also: sendDuplexReply
int UDPIO::addHeader( )
{
   iClRecentReqNum = iClReqNum;

   char szStartColor[20];
   szStartColor[0] = '\0';
   if (cClTellColor) 
   {
      sprintf(szStartColor, ",sc%c", cClTellColor);
      cClTellColor = '\0';
   }

   sprintf(aClRawOutBuf, ":sfktxt:v100,req%d%s,rt0%s%s,fl\n", 
      iClReqNum++,
      bClDuplex ? ",copy" : "",
      bClColor ? ",cs1" : "",
      szStartColor
      );

   // remember index of retry number
   char *psz = strstr(aClRawOutBuf, ",rt0");
   if (psz)
      iClRetryOff = (psz - aClRawOutBuf) + 3;
   
   // remember index of split line indicator
   iClSLIOff = strlen(aClRawOutBuf) - 3;

   for (int i=0; i<iClCommand; i++) 
   {
      strcat(aClRawOutBuf, ":");
      strcat(aClRawOutBuf, aClCommand[i]);
      strcat(aClRawOutBuf, "\n");
   }
   strcat(aClRawOutBuf, "\n");
   
   iClCommand = 0;

   iClHeadSize = strlen(aClRawOutBuf);
   iClOutIndex = iClHeadSize;
   return 0;
}

bool UDPIO::hasCachedOutput( )
{
   return iClOutIndex > iClHeadSize ? 1 : 0;
}

int UDPIO::flushSend(bool bTellAboutSplitLine)
{
   // set optional split line indicator in header
   if (   !bClRawText && bTellAboutSplitLine 
       && iClSLIOff > 0 && iClSLIOff < iClHeadSize
      )
      aClRawOutBuf[iClSLIOff] = 's'; // ,fl -> ,sl

   int irc = 0;
   int itry = 0;
   
   // printf("-----------------------------\n");

   for (; itry<10; itry++)
   {
      if (itry >= 5) {
         perr("line broken, failed to send package %d times.\n", itry);
         irc = 9;
         break;
      }   
      
      if (iClRetryOff > 0 && iClRetryOff < iClHeadSize) {
         aClRawOutBuf[iClRetryOff] = itry + '0';
      }

      if ((irc = sendData((uchar*)aClRawOutBuf, iClOutIndex)))
         break;
         
      // printf("SENT: %s\n", aClRawOutBuf);

      if (bClDuplex)
      {
         if (!isDataAvailable(0, iClTimeout))
            continue; // retry send

         aClRawInBuf2[0] = '\0';

         int i = receiveData((uchar*)aClRawInBuf2, sizeof(aClRawInBuf2)-100);
         if (i <= 0)
            continue; // retry send

         // printf("RECV: \"%s\" (%d)\n", aClRawInBuf2, i);

         // :sfktxt:v100,rep123,ok\n\n
         char *psz = strstr(aClRawInBuf2, ",rep");
         if (!psz) {
            perr("wrong -duplex reply received: %.30s", aClRawInBuf2);
            irc = 9;
            break;
         }

         int iReqNum = iClRecentReqNum;
         int iRepNum = atoi(psz+4);
         if (iRepNum < iReqNum) 
         {
            // happens when sending retry requests which are answered 
            // AFTER we already sent the next original request.
            pwarn("duplex: old reply record (%d/%d), lines may be duplicated\n", iReqNum, iRepNum);
            irc = 5;
            break;
         }
         else
         if (iRepNum != iReqNum)
         {
            // should not happen.
            pwarn("duplex: wrong reply record (%d/%d), lines may be invalid\n", iReqNum, iRepNum);
            irc = 5;
            break;
         }

         // package was fully confirmed
         break;
      }
      else
      {
         if (iClNonDuplexSendDelay)
            doSleep(iClNonDuplexSendDelay);
         break;
      }
   }

   iClHeadSize = 0;
   iClOutIndex = 0;
   iClCommand  = 0;
   iClSLIOff   = 0;

   return irc;
}

int UDPIO::addCommand(char *pszCmd)
{
   int iMaxCommands = sizeof(aClCommand)/sizeof(aClCommand[0]);
   if (iClCommand < 0 || iClCommand >= iMaxCommands-2)
      return 5;
      
   int iMaxCopy = sizeof(aClCommand[0])-2;
   strncpy(aClCommand[iClCommand], pszCmd, iMaxCopy);
   aClCommand[iClCommand][iMaxCopy-2] = '\0';
   iClCommand++;
   
   return 0;
}

char UDPIO::sfkToNetColor(char c)
{
   switch (c)
   {
      // keep these as is
      case 'r': case 'g': case 'b': case 'y': case 'c': case 'm':
      case 'R': case 'G': case 'B': case 'Y': case 'C': case 'M':
         return c;

      // white is sfk-internally coded as v to separate from
      // logical 'w'arning color.
      case 'v': return 'w';
      case 'V': return 'W';

      // default color ' ' is marked as 'd'
      case ' ': return 'd';
   }
   
   // all other cases are sfk logical colors
   extern int sfkMapAttrToColor(char cAttr);
   int nSFKIntColor = sfkMapAttrToColor(c);
   
   // bit   0:bright 1:red 2:green 3:blue
   // value     1       2      4      8
   switch (nSFKIntColor)
   {
      case  0: case 1: return 'd';
      case  2: return 'r';
      case  3: return 'R';
      case  4: return 'g';
      case  5: return 'G';
      case  6: return 'y';
      case  7: return 'Y';
      case  8: return 'b';
      case  9: return 'B';
      case 10: return 'm';
      case 11: return 'M';
      case 12: return 'c';
      case 13: return 'C';
      case 14: return 'w';
      case 15: return 'W';
   }

   // all other cases
   return 'd';
}

int UDPIO::checkTellCurrentColor(char cSFKAttrib)
{
   if (iClOutIndex > 0)
      return 5; // something was cached already

   cClTellColor = sfkToNetColor(cSFKAttrib);
   
   return 0;
}

int UDPIO::addOrSendText(char *pszInText, char *pszInAttr)
{
   if (bClRawText)
   {
      bClColor = 0;
      return addOrSendText(pszInText, strlen(pszInText), 0);
   }

   int iPhrase = 0;
   int iSrcCur = 0;
   int iAttCur = 0;
   
   // next addHeader should tell about color coding
   bClColor = 1;

   // if nothing was cached or sent yet, tell current color
   char cFirstColor = pszInAttr[0] ? pszInAttr[0] : ' ';
   checkTellCurrentColor(cFirstColor);

   // force setting of current color on every line start
   char aOld = '\0';

   char szCmd[30];

   memset(szCmd, 0, sizeof(szCmd));
   szCmd[0] = (char)0x1F;

   int irc = 0;

   while (1)
   {
      char c = pszInText[iSrcCur];
      char a = pszInAttr[iAttCur];

      if (!c)
         break;

      // attribs may end sooner, e.g. on LF of text
      if (a)
         iAttCur++;
      else
         a = aOld;

      if (a != aOld) 
      {
         // color change: flush recent phrase, if any
         if (iSrcCur > iPhrase)
            if ((irc = addOrSendText(pszInText+iPhrase, iSrcCur-iPhrase, 0)))
               return irc;
         iPhrase = iSrcCur;

         // send \x1F and color code
         szCmd[1] = sfkToNetColor(a);
         if ((irc = addOrSendText(szCmd, 2, 1)))
            return irc;
         
         // switch current color
         aOld = a;
      }
      
      if (((uchar)c) == 0x1FU)
      {
         // input contains non control \x1F: flush phrase
         if (iSrcCur > iPhrase)
            if ((irc = addOrSendText(pszInText+iPhrase, iSrcCur-iPhrase, 0)))
               return irc;
         iPhrase = iSrcCur;

         // send escaped \x1F\x1F
         szCmd[1] = sfkToNetColor(a);
         if ((irc = addOrSendText(szCmd, 2, 1)))
            return irc;
      }
      
      iSrcCur++;
   }

   // flush remainder:
   if (iSrcCur > iPhrase)
      if ((irc = addOrSendText(pszInText+iPhrase, iSrcCur-iPhrase, 0)))
         return irc;

   return 0;
}

int UDPIO::addOrSendText(char *pszPhrase, int iPhraseLen, bool bNoWrap)
{
   int irc = 0;

   if (!iClOutIndex && !bClRawText)
      addHeader();

   // package size with tolerance for EOL etc.
   int iNettoPackSize = iClPackageSize - 4;

   int iSrcLenRaw = iPhraseLen;
   int iCopyLen   = iSrcLenRaw;

   char *pszSrcCur= pszPhrase;
   int iSrcRemain = iSrcLenRaw;

   // color control sequences should never be split
   if (bNoWrap != 0 && iClOutIndex + iCopyLen > iNettoPackSize)
   {
      // therefore don't try to wrap, but flush immediately
      if ((irc = flushSend(0)))
         return irc;

      if (!bClRawText)
         addHeader();

      // iClOutIndex was reset, is now after new header.
   }

   while (   iCopyLen > 0
          && iClOutIndex + iCopyLen > iNettoPackSize
         )
   {
      // phrase does not fit completely.
      // try a soft wrap based on CR or LF.
      // we do NOT consider control sequences here.
      int iWrap = 0;
      for (int i=0; iClOutIndex+i<iNettoPackSize;)
      {
         if (!strncmp(pszSrcCur+i, "\r\n", 2))
            { iWrap=i+2; i+=2; } // after EOL
         else
         if (pszSrcCur[i]=='\r' || pszSrcCur[i]=='\n')
            { iWrap=i+1; i++; }  // after EOL
         else
            i++;
      }

      bool bSplitLine = 0;

      if (iWrap > 0) {
         iCopyLen = iWrap; // until wrap point
         // printf("... soft wrap %d\n", iWrap);
      }
      else
      if (iClOutIndex - iClHeadSize > 0) {
         // found no soft wrap point, but there is cached text
         iCopyLen = 0;     // flush recent as is
         // printf("... flush recent %d\n", iClOutIndex - iClHeadSize);
      }
      else {               // hard wrap
         iCopyLen = iNettoPackSize - iClOutIndex;
         bSplitLine = 1;
         // printf("... hard wrap %d %d %d\n", iCopyLen, iNettoPackSize, iClOutIndex);
      }
      
      // printf("addOrSend: %.*s (%d)\n", (int)iCopyLen, pszSrcCur, iCopyLen);
         
      if (iCopyLen > 0)
         memcpy(aClRawOutBuf+iClOutIndex, pszSrcCur, iCopyLen);
      iClOutIndex += iCopyLen;

      if ((irc = flushSend(bSplitLine)))
         return irc;

      if (!bClRawText)
         addHeader();

      pszSrcCur  += iCopyLen;
      iSrcRemain -= iCopyLen;
      iCopyLen    = iSrcRemain;
   }
   
   if (iCopyLen > 0)
   {
      // add (last part of) phrase to cache
      memcpy(aClRawOutBuf+iClOutIndex, pszSrcCur, iCopyLen);
      iClOutIndex += iCopyLen;
   }
   
   return irc;
}

int UDPIO::storeHeader(char *pszRaw, int iHeadLen)
{
   /*
      :sfktxt:v100,req1,copy,scd\n
      :clear\n
      :setpos,10,10\n
      :status,step 1\n
   */

   if (iHeadLen > sizeof(aClHeaderBuf)-100)
       iHeadLen = sizeof(aClHeaderBuf)-100;
   memcpy(aClHeaderBuf, pszRaw, iHeadLen);
   aClHeaderBuf[iHeadLen] = '\0';

   char *pszLine = aClHeaderBuf;

   // parse headline
   while (*pszLine)
   {
      if (*pszLine=='\n')
         { pszLine++; break; }
      if (*pszLine==',') {
         pszLine++;
         if (!strncmp(pszLine, "req", 3))
            iClInReqNum = atoi(pszLine+3);
         if (!strncmp(pszLine, "rt", 2))
            iClInRetryNum = atoi(pszLine+2);
         if (!strncmp(pszLine, "copy", 4))
            bClCopyRequest = 1; // duplex
         if (!strncmp(pszLine, "cs1", 3))
            bClDecodeColor = 1;
         if (bClDecodeColor && !strncmp(pszLine, "sc", 2))
            cClCurrentInColor = pszLine[6]; // start color
         continue;
      }
      pszLine++;
   }
   
   // parse commands.
   while (*pszLine)
   {
      // valid command lines must end with LF
      char *pszNext = strchr(pszLine, '\n');
      if (!pszNext)
         break; // possibly truncated header
      *pszNext++ = '\0';

      if (!strcmp(pszLine, ":clear"))
         bClCmdClear = 1;

      // to next line
      pszLine = pszNext;
   }

   return 0;
}

// IN : aClRawInBuf1 with mixed text
// OUT: aClRawInBuf2 with plain text
//      aClRawInAttr2 with color attributes
int UDPIO::decodeColorText(int iFromOffset)
{
   aClRawInBuf2[0]  = '\0';
   aClRawInAttr2[0] = '\0';

   int iSrcCur = iFromOffset;

   int iDstCur = 0;
   int iDstMax = sizeof(aClRawInBuf2) - 100;
   
   int iAttCur = 0;
   int iAttMax = sizeof(aClRawInAttr2) - 100;

   char a = cClCurrentInColor;

   while (iDstCur < iDstMax)
   {
      char c = aClRawInBuf1[iSrcCur];

      if (!c)
         break;

      if (bClDecodeColor!=0 && ((uchar)c) == 0x1FU)
      {
         // control sequence. get next char.
         iSrcCur++;
         char c2 = aClRawInBuf1[iSrcCur];

         if (!c2)
         {
            // invalid sequence at end of text.
            // treat as single control char.
            aClRawInBuf2[iDstCur++] = c;
            if (iAttCur < iAttMax)
             aClRawInAttr2[iAttCur++] = a;
            pwarn("invalid control sequence in color text\n");
            break;
         }
         
         if (((uchar)c2) == 0x1FU)
         {
            // escaped \x1F. replace by single char.
            aClRawInBuf2[iDstCur++] = c;
            if (iAttCur < iAttMax)
             aClRawInAttr2[iAttCur++] = a;

            iSrcCur++;
            continue;
         }
         
         // change current color
         a = c2;
         
         // also remember for package spanning text
         cClCurrentInColor = a;
         
         iSrcCur++;
         continue;
      }
      
      // copy through plain text char
      aClRawInBuf2[iDstCur++] = c;
      if (iAttCur < iAttMax)
       aClRawInAttr2[iAttCur++] = a;

      iSrcCur++;
   }

   // MUST zero terminate.
   aClRawInBuf2[iDstCur] = '\0';
   aClRawInAttr2[iAttCur] = '\0';

   return 0;
}

int UDPIO::getClientIndex(struct sockaddr_in *pAddr, bool *pFound)
{
   int iEmpty  = -1;
   int nOldest =  0;
   int iOldest =  0;

   struct UCPClientState *pcln = 0;

   for (int i=0; i<UDPIO_MAX_CLIENTS; i++)
   {
      pcln = &aClClients[i];

      // empty slot?
      if (!pcln->ntime) {
         if (iEmpty < 0)
            iEmpty = i;
         continue;
      }
      // oldest slot?
      if (nOldest==0 || pcln->ntime<nOldest) {
         nOldest = pcln->ntime;
         iOldest = i;
      }
      // matching?
      if (   pAddr->sin_addr.s_addr == pcln->addr.sin_addr.s_addr
          && pAddr->sin_port == pcln->addr.sin_port
         )
      {
         *pFound = 1;
         return i;
      }
   }

   int iReuse = (iEmpty >= 0) ? iEmpty : iOldest;

   pcln = &aClClients[iReuse];

   memset(pcln, 0, sizeof(aClClients[iReuse]));
   pcln->ntime = getCurrentTime();
   pcln->reqnum = 0; // filled in later
   memcpy(&pcln->addr, pAddr, sizeof(struct sockaddr_in));
   pcln->color = 'd';

   return iReuse;
}

void dumpdata(const char *pszTitle, char *psz) 
{
   printf("%s : \"", pszTitle);
   while (*psz) {
      switch (*psz) {
         case 0x1F: printf("{COL}"); break;
         case '\r': printf("{CR}"); break;
         case '\n': printf("{LF}"); break;
         default  : putchar(*psz);
      }
      psz++;
   }
   printf("\"\n");
}

bool UDPIO::hasCachedInput( )
{
   return iClRawInputCached > 0 ? 1 : 0;
}

int UDPIO::receiveText( )
{
   // to detect mixed input from different clients
   int iRecentClient = iClClient;
   bool bChangedClient = 0;

   if (!iClRawInputCached)
   {
      // cache is empty, get next network packet
      mclear(clRawInAddr);
      int clilen = sizeof(clRawInAddr);
      bool bSkipDecode = 0;
      
      // reinit per packet
      aClRawInBuf1[0]   = '\0';
      aClRawInBuf2[0]   = '\0';
      aClRawInAttr2[0]  = '\0';
      bClForceNextInput = 0;

      // taken from header, if any
      iClInReqNum = 0;
      iClInRetryNum = 0;
      bClCopyRequest = 0;
      cClCurrentInColor = 'd';
      bClCmdClear = 0;
      bClContinuedStream = 0;
      bClDecodeColor = 0;

      int iRawRead = receiveData((uchar*)aClRawInBuf1, sizeof(aClRawInBuf1)-100,
                                 &clRawInAddr, clilen);
      if (iRawRead <= 0)
         return 0;

      // data is zero terminated.
      // dumpdata("RECV", aClRawInBuf1); fflush(stdout);

      // set current client index by input address
      bool bClientRefound = 0;
      iClClient = getClientIndex(&clRawInAddr, &bClientRefound);

      // detect mixed client input
      if (iRecentClient != iClClient)
         bChangedClient = 1;
      
      bClRawText = 1;

      // parse header, if any
      if (!strncmp(aClRawInBuf1, ":sfktxt:", 8))
      {
         char *pszPastHeader = strstr(aClRawInBuf1, "\n\n");
         if (pszPastHeader) 
         {
            // parse header
            pszPastHeader += 2; // keep the LFs
            int iHeadLen = pszPastHeader - aClRawInBuf1;
            storeHeader(aClRawInBuf1, iHeadLen);
            // sets reqnum, copyreq, currentcolor, cmdclear

            // is there a continued stream with a client?
            if (bClientRefound) {
               // normal transfer: reqnum increments by one
               if (iClInReqNum == aClClients[iClClient].reqnum + 1)
                  bClContinuedStream = 1;
               // retry transfer: trynum increments by one
               if (   iClInReqNum == aClClients[iClClient].reqnum
                   && iClInRetryNum == aClClients[iClClient].copytry + 1)
               {
                  // the client sent us same record TWICE because
                  // it received the first reply with a huge delay.
                  // repair this: the stream is still intact
                  bClContinuedStream = 1;
                  // we already replied for that reqnum, therefore
                  aClClients[iClClient].copytry++;
                  bClCopyRequest = 0;
                  // and the dup text must be dropped.
                  // because we skip colorDecode:
                  aClRawInBuf2[0] = '\0';
                  // leads to iToCopy == 0 below.
                  bSkipDecode = 1;
               }
            }
            aClClients[iClClient].reqnum = iClInReqNum;

            // then reuse recent color. redundant to storeHeader.
            if (bClContinuedStream && bClDecodeColor)
               cClCurrentInColor = aClClients[iClClient].color;

            // reply duplex request immediately
            if (bClCopyRequest) {
               bClCopyRequest = 0;
               sendDuplexReply(&clRawInAddr);
            }

            // apply color decoding. will just copy thru if bClDecodeColor==0
            if (!bSkipDecode) {
               int iText = pszPastHeader - aClRawInBuf1;
                decodeColorText(iText);
                // from InBuf1 to InBuf2
               aClClients[iClClient].color = cClCurrentInColor;
            }

            // cached in RawInBuf2, RawInAttr2
            bClRawText = 0;
            
            // printf("PART: \"%s\" clf=%d cstrm=%d\n", aClRawInBuf2, bClientRefound, bClContinuedStream);
         }
         // else fall thru as raw text
      }

      if (bClRawText)
      {
         // take plain UDP text as is
         memcpy(aClRawInBuf2, aClRawInBuf1, iRawRead);
         aClRawInBuf2[iRawRead] = '\0';
         memset(aClRawInAttr2, 'd', iRawRead);
         aClRawInAttr2[iRawRead] = '\0';
      }
   }

   // new or cached text input is now in RawInBuf2, RawInAttr2.
   int iToCopy = strlen(aClRawInBuf2);
   // can be NULL if input was just a color control sequence!
   // in this case, only state information like reqnum is cached.

   // when should we print collected chars to terminal?
   // - if the sfktxt client changed
   bool bShouldFlush = bChangedClient;
   // - OR if same client has no continued stream
   if (!bClContinuedStream)
        bShouldFlush = 1;
   // but never on raw text input, unless option LFOnRaw is set
   if (bClRawText && !bClAppendLFOnRaw) 
      bShouldFlush = 0;

   // if there a partial line stored in output, we MUST feed the line
   // -  if the client changed
   // -  if the same client provides a mismatched request number
   if (iClInBufUsed > 0 && bShouldFlush)
   {
      // then the output line MUST be flushed now.
      // we keep current input in aClRawInBuf2.
      // all state like iClClient stays as is.
      iClRawInputCached = iToCopy; // can be NULL!
      bClForceNextInput = 1;
      // printf("back: caller must flush %d (1)\n", iClInBufUsed);
      return 1;
   }

   // is enough space in rejoin buffer for further text?
   if (iClInBufUsed + iToCopy > MAX_LINE_LEN)
   {
      // not enough space in inbuf for additional text.
      // caller must flush input. this flag signals
      // that no end of line char should be searched.
      iClRawInputCached = iToCopy; // can be NULL!
      bClForceNextInput = 1;
      // printf("back: caller must flush %d (2)\n", iClRawInputCached);
      return 1;
   }
   
   // add next line part to line rejoin buffer
   memcpy(aClInBuf+iClInBufUsed, aClRawInBuf2, iToCopy);
   memcpy(aClInAtt+iClInBufUsed, aClRawInAttr2, iToCopy);
   
   // on add of first part also copy sender info
   if (!iClInBufUsed)
      memcpy(&clInBufInAddr, &clRawInAddr, sizeof(clRawInAddr));

   // no cache remains   
   iClRawInputCached = 0;

   iClInBufUsed += iToCopy;
   
   aClInBuf[iClInBufUsed] = '\0';
   aClInAtt[iClInBufUsed] = '\0';

   // caller must consume new input lines as soon as
   // getNextInput() returns a complete line.
   
   // printf("join: \"%s\"\n", aClInBuf);
   
   return 0;
}

int UDPIO::sendDuplexReply(struct sockaddr_in *pAddr)
{
   if (fdClSocket == INVALID_SOCKET)
      return 9;

   char szReply[200];
   
   snprintf(szReply, sizeof(szReply)-10,
      ":sfktxt:v100,rep%d,rt%d,ok\n\n",
      iClInReqNum, iClInRetryNum
      );

   int iSendSize = strlen(szReply);

   if (sendto(fdClSocket, szReply, iSendSize, 0,
          (struct sockaddr *)pAddr, sizeof(struct sockaddr_in)) != iSendSize
      )
      return 10;

   aClClients[iClClient].copyreq = iClInReqNum;
   aClClients[iClClient].copytry = iClInRetryNum;

   return 0;
}

char *UDPIO::getNextCommand( )
{
   if (bClCmdClear) {
      bClCmdClear = 0;
      return (char*)"clear";
   }
   return 0;
}

char *UDPIO::getNextInput(char **ppAttr, struct sockaddr_in *pSenderAddr, bool bDontCache)
{
   if (iClInBufUsed <= 0)
      return 0;

   bool bAllowJoin = 1;
   if (bClRawText && bClAppendLFOnRaw)
        bAllowJoin = 0;

   // when receiving raw text, take input as is.
   // if input buffer overflows, also take as is.
   if (!bDontCache && !bClForceNextInput && bAllowJoin)
   {
      // wait for next CR or LF
      char cLast = aClInBuf[iClInBufUsed-1];
      if (cLast!='\r' && cLast!='\n')
         return 0;
   }
   
   bClForceNextInput = 0;

   iClInBufUsed = 0;

   if (ppAttr)
      *ppAttr = aClInAtt;

   if (pSenderAddr)
      memcpy(pSenderAddr, &clInBufInAddr, sizeof(struct sockaddr_in));

   // dumpdata("nxin", aClInBuf);

   return aClInBuf;
}

int UDPIO::getTextSendDelay( )
{
   if (bClDuplex)
      return 0;

   return iClNonDuplexSendDelay;
}

#ifndef USE_SFK_BASE

// ------------- sfk patch - Text File Patching support -----------------

#define SFKPATCH_MAX_CMDLINES    10000 // max lines per :file ... :done block
#define SFKPATCH_MAX_CACHELINES  10000 // max lines per :from ... :to pattern
#define SFKPATCH_MAX_NUMCMD        500 // max number of :from commands per patchfile
#define SFKPATCH_MAX_OUTLINES   500000 // max lines per target file

class SFKPatch
{
public:
      SFKPatch ( );
     ~SFKPatch ( );
 
static SFKPatch
   *current    ( );

   int processCmdPatch(char *psz);
   int processCmdInfo (char *psz);
   void log(int nLevel, const char *pFormat, ... );
   int detabLine(char *pBuf, char *pTmp, int nBufSize, int nTabSize);
   int processCmdRoot (char *pszIn);
   int patchMainInt(int argc, char *argv[], int offs);
   char *skipspace(char *psz);
   int processPatchFile(char *pszPatchFileName);
   int processCmdFile(char *pszIn);
   int processCreateFile(char *pszIn);
   int processCreateDir(char *pszIn);
   void shrinkLine(char *psz, char *pszOut);
   int compareLines(char *psz1, char *psz2);
   int processFileUntilDone(char *pszTargFileName);

static SFKPatch
   *pClCurrent;

FILE *fpatch;
char *pszRoot;
char szCmdBuf[MAX_LINE_LEN];
int  bGlblRevoke;
int  bGlblBackup;
int  bGlblSimulate;
int  bGlblTouchOnRevoke;
int  nCmdFileLineEndings;
int  nGlblLine;
int  bGlblIgnoreWhiteSpace;
int  bGlblAlwaysSimulate;
int  bGlblVerbose;
int  bGlblQuickSum;
int  bGlblUnixOutput;
int  nGlblPatchedFiles;
int  nGlblRevokedFiles;
char *pszGlblRelWorkDir;
int  bGlblAnySelRep;
int  bGlblCheckSelRep;
int  bGlblStats;
int  nGlblDetabOutput;
int  bGlblVerify;
int  bGlblNoPID ;
int  bGlblIgnoreRoot;
char **apOut;

// select-replace table over all targets
#define MAX_GLOBAL_CHANGES 50
char *apGlobalChange[MAX_GLOBAL_CHANGES][3];
int anGlobalChange[MAX_GLOBAL_CHANGES];
int iGlobalChange;

// select-replace table local to current target
#define MAX_LOCAL_CHANGES 50
char *apLocalChange[MAX_LOCAL_CHANGES][3];
int anLocalChange[MAX_GLOBAL_CHANGES];
int iLocalChange;

char *aPatch[SFKPATCH_MAX_CMDLINES];
char *aBuf[SFKPATCH_MAX_CACHELINES];
int   aifrom[SFKPATCH_MAX_NUMCMD];
int   aifromlen[SFKPATCH_MAX_NUMCMD];
int   aito[SFKPATCH_MAX_NUMCMD];
int   aitolen[SFKPATCH_MAX_NUMCMD];

char szLine1[MAX_LINE_LEN];
char szLine2[MAX_LINE_LEN];
};

class PatchMemCover {
public:
   PatchMemCover  ( ) {
      bdead = 0;
      SFKPatch::current()->apOut = new char*[SFKPATCH_MAX_OUTLINES+10];
      if (!SFKPatch::current()->apOut)
         bdead = 1;
   }
   ~PatchMemCover ( ) {
      if (SFKPatch::current()->apOut) {
         delete [] SFKPatch::current()->apOut;
         SFKPatch::current()->apOut = 0;
      }
   }
   int bdead;
};

SFKPatch *SFKPatch :: pClCurrent = 0;

SFKPatch :: SFKPatch( )
{
   memset(this, 0, sizeof(*this));
   bGlblTouchOnRevoke = 1;
   nCmdFileLineEndings = -1;
   bGlblIgnoreWhiteSpace = 1;
}

SFKPatch *SFKPatch :: current( )
{
   if (!pClCurrent)
      pClCurrent = new SFKPatch();

   return pClCurrent;
}

int patchMain(int argc, char *argv[], int offs)
{
   return SFKPatch::current()->patchMainInt(argc, argv, offs);
}

int SFKPatch :: processCmdPatch(char *psz) { return 0; }
int SFKPatch :: processCmdInfo (char *psz) { return 0; }

// 0 == error, >0 == normal msg, >= 5 do not tell if in QuickSum mode
void SFKPatch :: log(int nLevel, const char *pFormat, ... )
{
   va_list argList;
   va_start(argList, pFormat);
   if (nLevel == 0) {
      vfprintf(stderr, pFormat, argList);
      fflush(stderr);
   } else {
      if (!(bGlblQuickSum && nLevel >= 5)) {
         vprintf(pFormat, argList);
         fflush(stdout);
      }
   }
   va_end(argList);
}

int SFKPatch :: detabLine(char *pBuf, char *pTmp, int nBufSize, int nTabSize)
{
   strcpy(pTmp, pBuf);
   int iout=0, nInsert=0;
   for (int icol=0; pTmp[icol] && iout < (nBufSize-nTabSize-2); icol++) {
      char c1 = pTmp[icol];
      if (c1 == '\t') {
         nInsert = nTabSize - (iout % nTabSize);
         for (int i2=0; i2<nInsert; i2++)
            pBuf[iout++] = ' ';
      } else {
         pBuf[iout++] = c1;
      }
   }
   pBuf[iout] = '\0';
   if (iout >= (nBufSize-nTabSize-2)) {
      log(0, "error  : detab: line buffer overflow. max line len supported is %d\n",(nBufSize-nTabSize-2));
      return 9;
   }
   return 0;
}

int SFKPatch :: processCmdRoot (char *pszIn) {
   strcpy(szCmdBuf, pszIn);
   char *psz = 0;
   if ((psz = strchr(szCmdBuf, '\r')) != 0) *psz = 0;
   if ((psz = strchr(szCmdBuf, '\n')) != 0) *psz = 0;
   pszRoot = strdup(szCmdBuf);
   if (!bGlblIgnoreRoot && strcmp(pszRoot, pszGlblRelWorkDir)) {
      log(0, "error  : you are not in the %s directory.\n",pszRoot);
      return 1+4;
   }
   return 0;
}

int SFKPatch :: patchMainInt(int argc, char *argv[], int offs)
{
   PatchMemCover mem;
   if (mem.bdead) {
      log(0, "error: out of memory in patchMain\n");
      return 9;
   }

   if (!strcmp(argv[1+offs], "-example"))
   {
      printx(
         "#patchfile example, containing all supported patchfile commands:\n\n"
         ":patch \"enable FooBar testing\"\n"
         ":info makes some stuff public, for direct access by test funcs\n"
         "\n"
         ":root foosrc\n"
         "\n"
         ":file include\\Foobar.hpp\n"
         ":from \n"
         "private:\n"
         "    bool                  isAvailable               (int nResource);\n"
         "    void                  openBar                   (int nMode);\n"
         ":to\n"
         "public: // [patch-id]\n"
         "    bool                  isAvailable               (int nResource);\n"
         "    void                  openBar                   (int nMode);\n"
         ":from \n"
         "    // returns the application type, 0x00 == not set.\n"
         "    UInt16                getAppType                ( );\n"
         ":to\n"
         "    // returns the application type, 0x00 == not set.\n"
         "    UInt16                getAppType                ( );\n"
         "    UInt32                getAppTypeInternal        ( );\n"
         ":done\n"
         "\n"
         ":## this is a remark, allowed only outside :file blocks.\n"
         ":## the above syntax is sufficient for most cases; but now follow some\n"
         ":## more commands for global replace, file and dir creation, etc.\n"
         ":## select-replace has 3 parms, and applies changes (parms 2+3) only\n"
         ":## in lines containing the search term (parm 1).\n"
         "\n"
         ":file include\\Another.cpp\n"
         ":select-replace /MY_TRACE(/\\n\"/\"/\n"
         ":select-replace _printf(\"spam: _printf(_while(0) printf(_\n"
         ":set only-lf-output\n"
         ":from \n"
         "    bool                  existsFile                (char *psz);\n"
         ":to\n"
         "    // [patch-id]\n"
         "    int                   existsFile                (char *psz);\n"
         ":done\n"
         "\n"
         ":mkdir sources\n"
         ":create sources\\MyOwnFix.hpp\n"
         "// this file is generated by sfk patch.\n"
         "##define OTHER_SYMBOL MY_OWN_SYMBOL\n"
         ":done\n"
         "\n"
         ":skip-begin\n"
         "this is outcommented stuff. the skip-end is optional.\n"
         ":skip-end\n"
         );
      return -1;
   }

   if (!strcmp(argv[1+offs], "-template") || !strcmp(argv[1+offs], "-tpl"))
   {
      printf(
         ":patch \"thepatch\"\n"
         "\n"
         ":root theproject\n"
         "\n"
         ":file include\\file1.hpp\n"
         ":from \n"
         ":to\n"
         "    // [patch-id]\n"
         ":from \n"
         ":to\n"
         ":done\n"
         "\n"
         ":file sources\\file1.cpp\n"
         ":from \n"
         ":to\n"
         "    // [patch-id]\n"
         ":done\n"
         "\n"
         );
      return -1;
   }

   szCmdBuf[0] = '\0';
   #ifdef _WIN32
   if (_getcwd(szCmdBuf,sizeof(szCmdBuf)-10)) { }
   #else
   if (getcwd(szCmdBuf,sizeof(szCmdBuf)-10)) { }
   #endif
   char *psz = strrchr(szCmdBuf, glblPathChar);
   if (!psz) {
      log(0, "error: cannot identify working dir. make sure you are in the correct directory.\n");
      return 9;
   }
   psz++;
   pszGlblRelWorkDir = strdup(psz);
 
   char *pszPatchFileName = 0;
   char *pszPatchFileBackup = 0;
   int bWantRevoke = 0;
   int bWantRedo   = 0;
   int bJustSim    = 0;

   for (int iarg=1; iarg<argc; iarg++) {
      if (!strcmp(argv[iarg+offs], "-revoke")) {
         bWantRevoke = true;
      }
      else
      if (!strcmp(argv[iarg+offs], "-redo")) {
         bWantRevoke = true;
         bWantRedo   = true;
      }
      else
      if (!strcmp(argv[iarg+offs], "-keep-dates")) {
         bGlblTouchOnRevoke = 0;
      }
      else
      if (!strcmp(argv[iarg+offs], "-exact-match")) {
         bGlblIgnoreWhiteSpace = 0;
      }
      else
      if (!strcmp(argv[iarg+offs], "-sim")) {
         bJustSim = 1;
      }
      else
      if (!strcmp(argv[iarg+offs], "-verify")) {
         bJustSim    = 1;
         bGlblVerify = 1;
      }
      else
      if (!strcmp(argv[iarg+offs], "-qs")) {
         bGlblQuickSum = 1;
      }
      else
      if (!strcmp(argv[iarg+offs], "-verbose")) {
         bGlblVerbose = 1;
      }
      else
      if (!strcmp(argv[iarg+offs], "-stat")) {
         bGlblStats = 1;
      }
      else
      if (!strcmp(argv[iarg+offs], "-nopid")) {
         bGlblNoPID = 1;
      }
      else
      if (!strcmp(argv[iarg+offs], "-anyroot")) {
         bGlblIgnoreRoot = 1;
      }
      else
      if (!strncmp(argv[iarg+offs], "-", 1)) {
         printf("unknown option: %s\nuse with no parameters to get help.\n",argv[iarg+offs]);
         return 9;
      }
      else {
         pszPatchFileName = argv[iarg+offs];
         char *psz = strrchr(pszPatchFileName, glblPathChar);
         if (!psz) psz = strrchr(pszPatchFileName,':');
         if (!psz) { psz = pszPatchFileName; } else psz++;
         sprintf(szCmdBuf,"save_patch%c%s", glblPathChar, psz);
         pszPatchFileBackup = strdup(szCmdBuf);
      }
   }

   // pass -1: check logic
   if (bJustSim && bWantRevoke) {
      log(0, "error  : cannot simulate and revoke together.\n");
      return 9;
   }

   // pass 0: init stats
   for (int i1=0; i1<MAX_GLOBAL_CHANGES; i1++)
      anGlobalChange[i1] = 0;
   for (int i2=0; i2<MAX_LOCAL_CHANGES; i2++)
      anLocalChange[i2]  = 0;

   // pass 1: revoke: always, unconditional.
   int iRC = 0;
   if (bWantRevoke) {
      bGlblRevoke = 1;
      if (fileExists(pszPatchFileBackup)) {
         log(5, "* revoking changes from: %s\n", pszPatchFileBackup);
         iRC = processPatchFile(pszPatchFileBackup);
      } else {
         log(5, "* revoking changes from: %s\n", pszPatchFileName);
         iRC = processPatchFile(pszPatchFileName);
      }
      bGlblRevoke = 0;
      if (!iRC && !bWantRedo) {
         if (bGlblQuickSum) {
            printf("* patch revoked: %s - %d target files\n",pszPatchFileName,nGlblRevokedFiles);
         } else {
            if (bGlblTouchOnRevoke)
               printf("* all changes revoked. the target files got the current time stamp,\n"
                      "* to ease recompile. you may use -keepdates to change this behaviour.\n");
            else
               printf("* all changes revoked, including original timestamps.\n");
         }
      }
      if (!iRC) {
         // remove patchfile backup
         if (fileExists(pszPatchFileBackup))
            if (remove(pszPatchFileBackup)) {
               log(0, "error  : cannot remove stale backup: %s\n",pszPatchFileBackup);
               return 1;
            }
      }
      if (!iRC && !bWantRedo)
         return 0;
   }

   // pass 2: pre-scan if all patches may be applied
   bGlblSimulate = 1;
   if (!iRC) {
      log(5, "* checking target file compliancies\n");
      iRC = processPatchFile(pszPatchFileName);
   }
   if (bJustSim) {
      if (!iRC) {
         if (bGlblQuickSum)
          printf("* patch %s: %s\n", bGlblVerify ? "intact" : "valid", pszPatchFileName);
         else
          printf("* all checked. the patch is %s.\n", bGlblVerify ? "still intact" : "valid and may be applied.");
         return 0;
      } else {
         if (bGlblQuickSum)
            log(0, "patch  : %s\n",pszPatchFileName);
         printf("* there were errors. the patch cannot be applied.\n");
         printf("* however, if an older patch is active, you may still -revoke it.\n");
         return 1;
      }
   }
   bGlblSimulate = 0;

   // pass 3: create backups
   if (!iRC && !bGlblNoPID) {
      log(5, "* creating backups\n");
      bGlblBackup = 1;
      iRC = processPatchFile(pszPatchFileName);
      bGlblBackup = 0;
   }

   // pass 3: apply patches
   if (!iRC) {
      log(5, "* applying patches%s\n", bGlblNoPID?" permanently":"");
      bGlblCheckSelRep = 1;
      iRC = processPatchFile(pszPatchFileName);
      if (!iRC) {
         if (bWantRedo) {
            if (bGlblQuickSum)
               printf("* all changes re-applied: %s - %d target files\n",pszPatchFileName,nGlblPatchedFiles);
            else
               printf("* all changes re-applied.\n");
         } else {
            if (bGlblQuickSum) {
               printf("* patch applied: %s - %d target files\n",pszPatchFileName,nGlblPatchedFiles);
            } else {
               printf("* all done.\n");
            }
         }
         // patch applied: save the patchfile for future revoke
         if (!bGlblNoPID)
         {
         #ifdef _WIN32
         _mkdir("save_patch");
         #else
         mkdir("save_patch", S_IREAD | S_IWRITE | S_IEXEC);
         #endif
         FILE *fout = fopen(pszPatchFileBackup,"w");
         if (fout) { fprintf(fout,"dummy"); fclose(fout); }
         #ifdef _WIN32
         sprintf(szCmdBuf, "xcopy /Q /K /Y %s %s >nul",pszPatchFileName,pszPatchFileBackup);
         #else
         sprintf(szCmdBuf, "cp -p %s %s",pszPatchFileName,pszPatchFileBackup);
         #endif
         iRC = system(szCmdBuf);
         if (iRC)
            log(0, "error  : cannot backup patchfile to: %s\n",pszPatchFileBackup);
         }
      } else {
         if (iRC & 4)
            log(0, "info   : make sure you are above the \"%s\" directory.\n",pszRoot);
      }
   } else {
      if (bGlblQuickSum)
         log(0, "patch  : %s\n",pszPatchFileName);
      if (iRC & 4) {
         log(0, "info   : make sure you are in the \"%s\" directory.\n",pszRoot);
         return iRC;
      }
      printf("* NOTHING CHANGED: i will either apply ALL changes, or NONE.\n"
             "* you may also try sfk patch -revoke or -redo.\n"
            );
      fflush(stdout);
   }

   return iRC;
}

char *SFKPatch :: skipspace(char *psz) {
   while (*psz && *psz == ' ')
      psz++;
   return psz;
}

int SFKPatch :: processPatchFile(char *pszPatchFileName)
{
   int iRC = 0;

   fpatch = fopen(pszPatchFileName, "r");
   if (!fpatch) { log(0, "error  : cannot open patchfile: %s\n",pszPatchFileName); return 2+4; }
   nGlblLine = 0;

   // parse patch file, exec commands
   int bWithinSkip = 0;
   char szBuf[MAX_LINE_LEN];
   while (fgets(szBuf,sizeof(szBuf)-10,fpatch) != NULL)
   {
      nGlblLine++;

      // determine line endings of the command file
      if (nCmdFileLineEndings == -1)
      {
         if (strstr(szBuf,"\r\n") != 0)
            nCmdFileLineEndings = 2;   // CR/LF
         else
            nCmdFileLineEndings = 1;   // LF only
      }

      if (strBegins(szBuf,":#"))
         continue;

      if (strBegins(szBuf,":skip-end")) {
         if (!bWithinSkip) {
            log(0, "error  : skip-end without skip-begin in line %d\n",nGlblLine);
            return 2;
         }
         bWithinSkip = 0;
         continue;
      }

      if (strBegins(szBuf,":skip-begin")) {
         if (bWithinSkip) {
            log(0, "error  : skip-begin twice in line %d\n",nGlblLine);
            return 2;
         }
         bWithinSkip = 1;
         continue;
      }

      if (bWithinSkip)
         continue;

      if (strBegins(szBuf,":patch ")) {
         iRC |= processCmdPatch(skipspace(szBuf+7));
         continue;
      }
      if (   strBegins(szBuf,":info ")
          || strBegins(szBuf,":info\r")
          || strBegins(szBuf,":info\n")
         )
      {
         iRC |= processCmdInfo(skipspace(szBuf+6));
         continue;
      }
      if (strBegins(szBuf,":root ")) {
         if (processCmdRoot(skipspace(szBuf+6)))
            return 1+4;
         continue;
      }
      if (strBegins(szBuf,":select-replace "))
      {
         bGlblAnySelRep = 1;
         if (iGlobalChange < MAX_GLOBAL_CHANGES-2) {
            char *psz = skipspace(szBuf+strlen(":select-replace "));
            char  nCC = *psz++;
            char *pszMaskBegin = psz;
            while (*psz && (*psz != nCC)) psz++;
            char *pszMaskEnd   = psz;
            if (*psz) psz++;
            char *pszFromBegin = psz;
            while (*psz && (*psz != nCC)) psz++;
            char *pszFromEnd   = psz;
            if (*psz) psz++;
            char *pszToBegin   = psz;
            while (*psz && (*psz != nCC)) psz++;
            char *pszToEnd     = psz;
            *pszMaskEnd = 0;
            *pszFromEnd = 0;
            *pszToEnd   = 0;
            if (strlen(pszFromBegin) > 0) {
               char *pszMaskDup = strdup(pszMaskBegin);
               char *pszFromDup = strdup(pszFromBegin);
               char *pszToDup   = strdup(pszToBegin);
               apGlobalChange[iGlobalChange][0] = pszMaskDup;
               apGlobalChange[iGlobalChange][1] = pszFromDup;
               apGlobalChange[iGlobalChange][2] = pszToDup;
               iGlobalChange++;
            }
         } else {
            log(0, "error  : too many global select-replace, only up to %d supported.\n",MAX_GLOBAL_CHANGES);
            return 2;
         }
         continue;
      }
      if (strBegins(szBuf,":create ")) {
         iRC |= processCreateFile(skipspace(szBuf+8));
         continue;
      }
      if (strBegins(szBuf,":mkdir ")) {
         iRC |= processCreateDir(skipspace(szBuf+7));
         continue;
      }
      if (strBegins(szBuf,":file ")) {
         iRC |= processCmdFile(skipspace(szBuf+6));
         continue;
      }
      if (strBegins(szBuf,":set only-lf-output")) {
         bGlblUnixOutput = 1;
         continue;
      }
      if (strBegins(szBuf,":set detab=")) {
         nGlblDetabOutput = atol(szBuf+strlen(":set detab="));
         log(5, "setting detab to %d\n",nGlblDetabOutput);
         continue;
      }
      if (strBegins(szBuf,":")) {
         log(0, "error  : unknown command in line %d: %s\n",nGlblLine,szBuf);
         return 1;
      }

      // everything else handled by processCmdFile
   }

   fclose(fpatch);

   // now that all was processed, cleanup table of global changes
   for (int i=0;i<iGlobalChange;i++) {
      if (bGlblCheckSelRep) {
         if (!anGlobalChange[i])
            log(5, "info   : global select-replace never applied: %s, %s, %s\n",apGlobalChange[i][0],apGlobalChange[i][1],apGlobalChange[i][2]);
         else
         if (bGlblStats || bGlblVerbose)
            log(5, "global select-replace applied %d times: %s, %s, %s\n",anGlobalChange[i],apGlobalChange[i][0],apGlobalChange[i][1],apGlobalChange[i][2]);
      }
      free(apGlobalChange[i][0]);
      free(apGlobalChange[i][1]);
      free(apGlobalChange[i][2]);
   }
   iGlobalChange = 0;

   return iRC;
}

int SFKPatch :: processCmdFile(char *pszIn)
{
   if (strlen(pszIn) < 1) { log(0, "error  : missing filename after :file\n"); return 2; }

   strcpy(szCmdBuf, pszIn);
   char *psz = 0;
   if ((psz = strchr(szCmdBuf, '\r')) != 0) *psz = 0;
   if ((psz = strchr(szCmdBuf, '\n')) != 0) *psz = 0;
   char *pszFile = strdup(szCmdBuf);
 
   strcpy(szCmdBuf, pszFile);

   // check if file exists
   if (!bGlblRevoke)
      if (!fileExists(szCmdBuf)) {
         log(0, "error  : unable to open file: %s\n",szCmdBuf);
         return 2+4;
      }

   iLocalChange = 0;
   int iRC = processFileUntilDone(szCmdBuf);

   // always cleanup this table, which is feeded by processFileUntilDone
   for (int i=0;i<iLocalChange;i++) {
      if (bGlblCheckSelRep) {
         if (!anLocalChange[i])
            log(5, "info   : local select-replace never applied: %s, %s, %s\n",apLocalChange[i][0],apLocalChange[i][1],apLocalChange[i][2]);
         else
         if (bGlblStats || bGlblVerbose)
            log(5, "local select-replace applied %d times: %s, %s, %s\n",anLocalChange[i],apLocalChange[i][0],apLocalChange[i][1],apLocalChange[i][2]);
      }
      free(apLocalChange[i][0]);
      free(apLocalChange[i][1]);
      free(apLocalChange[i][2]);
   }
   iLocalChange = 0;

   return iRC;
}

int SFKPatch :: processCreateFile(char *pszIn)
{
   if (strlen(pszIn) < 1) { log(0, "error  : missing filename after :create\n"); return 2; }

   strcpy(szCmdBuf, pszIn);
   char *psz = 0;
   if ((psz = strchr(szCmdBuf, '\r')) != 0) *psz = 0;
   if ((psz = strchr(szCmdBuf, '\n')) != 0) *psz = 0;
   char *pszFile = strdup(szCmdBuf);
 
   strcpy(szCmdBuf, pszFile);

   // any existing file?
   if (!bGlblRevoke && !bGlblBackup) {
      if (fileExists(szCmdBuf)) {
         if (!bGlblVerify)
            log(0, "warning: file already exists: %s\n", szCmdBuf);
      } else {
         if (bGlblVerify) {
            log(0, "error  : file no longer exists: %s\n", szCmdBuf);
            return 1;
         }
      }
   }

   FILE *fout = 0;

   if (!bGlblSimulate && bGlblRevoke) {
      // do exact opposite: delete old file
      if (remove(szCmdBuf))
         log(0, "warning: cannot remove: %s\n", szCmdBuf);
      else {
         nGlblRevokedFiles++;
         log(5, "removed: %s\n", szCmdBuf);
      }
   }
   else
   {
      // create new output file
      if (!bGlblSimulate && !bGlblRevoke && !bGlblBackup) {
         fout = fopen(szCmdBuf, "w");
         if (!fout) {
            log(0, "error  : cannot create file: %s\n", szCmdBuf); // return 2+4;
            // fall-through, continue to allow creation of other files.
         }
      }
   }

   // copy-through contents from patchfile until :done
   char szBuf[MAX_LINE_LEN];
   int iout = 0;
   int bGotDone = 0;
   int nStartLine = nGlblLine;
   while (fgets(szBuf,sizeof(szBuf)-10,fpatch) != NULL)
   {
      nGlblLine++;
      if (!strncmp(szBuf, ":done", 5)) {
         bGotDone = 1;
         break;
      }
      if (!bGlblSimulate && !bGlblRevoke && !bGlblBackup && fout) {
         fputs(szBuf, fout);
         iout++;
      }
   }

   // close output file
   if (!bGlblSimulate && !bGlblRevoke && !bGlblBackup && fout) {
      fflush(fout);
      fclose(fout);
      nGlblPatchedFiles++;
      log(5, "written: %s, %d lines.\n",szCmdBuf,iout);
   }

   if (!bGotDone) {
      if (!fout) { log(0, "error  : missing :done after :create of line %d\n", nStartLine); return 2; }
   }

   return 0;
}

int SFKPatch :: processCreateDir(char *pszIn)
{
   if (strlen(pszIn) < 1) { log(0, "error  : missing filename after :mkdir\n"); return 2; }

   strcpy(szCmdBuf, pszIn);
   char *psz = 0;
   if ((psz = strchr(szCmdBuf, '\r')) != 0) *psz = 0;
   if ((psz = strchr(szCmdBuf, '\n')) != 0) *psz = 0;
   char *pszFile = strdup(szCmdBuf);
 
   strcpy(szCmdBuf, pszFile);

   if (!bGlblSimulate && bGlblRevoke) {
      // not yet supported: remove dir on revoke (sequence problem)
      // _rmdir(szCmdBuf);
      return 0;
   }

   // create dir. have to use Win-specific API.
   if (!bGlblSimulate && !bGlblRevoke && !bGlblBackup) {
      #ifdef _WIN32
      int iRC = _mkdir(szCmdBuf);
      #else
      int iRC = mkdir(szCmdBuf, S_IREAD | S_IWRITE | S_IEXEC);
      #endif
      if (!iRC) log(5, "created: %s\n",szCmdBuf);
   }

   return 0;
}

void SFKPatch :: shrinkLine(char *psz, char *pszOut)
{
   int bWithinString = 0;
   int nWhiteCnt = 0;
   for (;*psz; psz++)
   {
      if (*psz == '\r' || *psz == '\n')
         break;   // strip also CR, LF from shrinked strings

      if (*psz == '\"' || *psz == '\'')
         bWithinString = 1-bWithinString;

      if (bWithinString)
         *pszOut++ = *psz;
      else
      if (*psz != ' ' && *psz != '\t') {
         *pszOut++ = *psz;
         nWhiteCnt = 0;
      }
      else {
         // always apply first whitespace
         nWhiteCnt++;
         if (nWhiteCnt == 1)
            *pszOut++ = ' ';
      }
   }
   *pszOut = 0;
}

int SFKPatch :: compareLines(char *psz1, char *psz2)
{
   if (bGlblIgnoreWhiteSpace)
   {
      shrinkLine(psz1, szLine1);
      shrinkLine(psz2, szLine2);
      int iRC = strcmp(szLine1,szLine2);
      if (bGlblVerbose)
      {
         if (iRC)
            printf("src-> %s\npat-> %s\n",szLine1,szLine2);
         else
            printf("src*> %s\npat*> %s\n",szLine1,szLine2);
      }
      return iRC;
   }
   return strcmp(psz1, psz2);
}

int SFKPatch :: processFileUntilDone(char *pszTargFileName)
{
   // INPUT implicite: fpatch stream

   // 1. read next block from patchfile until ":done"
   char szBuf[MAX_LINE_LEN];
   char szBuf2[MAX_LINE_LEN];
   int ipatch = 0;
   while (fgets(szBuf,sizeof(szBuf)-10,fpatch) != NULL)
   {
      nGlblLine++;
      aPatch[ipatch++] = strdup(szBuf);
      aPatch[ipatch] = 0;
      if (ipatch > SFKPATCH_MAX_CMDLINES) { log(0, "command block too large, max %d lines supported.\n",(int)SFKPATCH_MAX_CMDLINES); return 2; }
      if (!strncmp(szBuf, ":done", 5))
         break;
   }

   // 2. find commands
   int icmd=0; int iline=0; int bNextCmd=0; int bDone=0;
   int bWithinToBlock=0, bHavePatchIDForThisFile=0;
   int bFromPassed=0, nLocalDetabOutput=0;
   while (!bDone)
   {
      aifrom   [icmd]=-1;
      aifromlen[icmd]=-1;
      aito     [icmd]=-1;
      aitolen  [icmd]=-1;
      for (;iline<ipatch;iline++)
      {
         if (!strncmp(aPatch[iline],":select-replace ",strlen(":select-replace ")))
         {
            bGlblAnySelRep = 1;
            if (iLocalChange < MAX_LOCAL_CHANGES-2) {
               char *psz = aPatch[iline]+strlen(":select-replace ");
               char  nCC = *psz++;
               char *pszMaskBegin = psz;
               while (*psz && (*psz != nCC)) psz++;
               char *pszMaskEnd   = psz;
               if (*psz) psz++;
               char *pszFromBegin = psz;
               while (*psz && (*psz != nCC)) psz++;
               char *pszFromEnd   = psz;
               if (*psz) psz++;
               char *pszToBegin   = psz;
               while (*psz && (*psz != nCC)) psz++;
               char *pszToEnd     = psz;
               *pszMaskEnd = 0;
               *pszFromEnd = 0;
               *pszToEnd   = 0;
               if (strlen(pszFromBegin) > 0) {
                  char *pszMaskDup = strdup(pszMaskBegin);
                  char *pszFromDup = strdup(pszFromBegin);
                  char *pszToDup   = strdup(pszToBegin);
                  apLocalChange[iLocalChange][0] = pszMaskDup;
                  apLocalChange[iLocalChange][1] = pszFromDup;
                  apLocalChange[iLocalChange][2] = pszToDup;
                  iLocalChange++;
               }
            } else {
               log(0, "error  : too many select-replace in one :file, only up to %d supported.\n",MAX_LOCAL_CHANGES);
               return 2;
            }
            continue;
         }

         if (!strncmp(aPatch[iline],":set detab=",strlen(":set detab="))) {
            nLocalDetabOutput = atol(aPatch[iline]+strlen(":set detab="));
            // log(5, "setting local detab to %d\n",nLocalDetabOutput);
            continue;
         }

         if (!strncmp(aPatch[iline],":from",strlen(":from")))
         {
            bFromPassed = 1;
            bWithinToBlock = 0;
            if (aifrom[icmd] == -1) {
               // starts new command (only at file start)
               aifrom[icmd]    = iline+1;
            } else {
               // ends current command
               aitolen[icmd]   = iline-aito[icmd];
               bNextCmd = 1;
            }
         }

         if (!bFromPassed && !strncmp(aPatch[iline], ":", 1)) {
            log(0, "unexpected command: %s\n", aPatch[iline]);
            return 9;
         }

         if (!strncmp(aPatch[iline],":to",strlen(":to"))) {
            aito[icmd]      = iline+1;
            aifromlen[icmd] = aito[icmd]-aifrom[icmd]-1;
            bWithinToBlock  = 1;
         }

         if (!strncmp(aPatch[iline],":done",strlen(":done"))) {
            // only at EOF
            bWithinToBlock = 0;
            aitolen[icmd]   = iline-aito[icmd];
            bNextCmd = 1;
            bDone = 1;
         }

         if (strstr(aPatch[iline], "[patch-id]") != 0) {
            if (bWithinToBlock)
               bHavePatchIDForThisFile = 1;
            else {
               log(0, "error  : line %d: [patch-id] not allowed within :from block. expected in :to block.\n",nGlblLine);
               return 2;
            }
         }

         if (bNextCmd)
         {
            bNextCmd=0;
            if (aifrom[icmd]   ==-1) { log(0, "error  : line %d: :from block missing\n",nGlblLine); return 2; }
            if (aito[icmd]     ==-1) { log(0, "error  : line %d: :to block missing\n",nGlblLine); return 2; }
            if (aifromlen[icmd]<= 0) { log(0, "error  : line %d: :from block empty\n",nGlblLine); return 2; }
            if (aitolen[icmd]  <= 0) { log(0, "error  : line %d: :to block empty\n",nGlblLine);   return 2; }
            icmd++;
            if (icmd >= SFKPATCH_MAX_NUMCMD-2)   { log(0, "error  : too many commands, only %d supported\n",SFKPATCH_MAX_NUMCMD); return 2; }
            aifrom[icmd]    = iline+1;
            break;
         }

      }  // endfor inner loop

   }  // endfor outer loop (bDone)

   if (!bHavePatchIDForThisFile && !bGlblNoPID) {
      log(0, "error  : line %d: [patch-id] missing!\n",nGlblLine);
      log(0, "info   : you must supply at least one [patch-id] within a :to block per :file,\n"
             "info   : otherwise i cannot identify already-patched files.\n");
      return 2;
   }

   // =========== pre-scan the target file, find out if it's patched =====================

   int bTargetIsPatched = 0;

   FILE *ftarg2 = fopen(pszTargFileName, "r");

   if (!ftarg2) { log(0, "error  : cannot read target file: %s\n", pszTargFileName); return 2+4; }

   while (fgets(szBuf,sizeof(szBuf)-10,ftarg2) != NULL)
      if (strstr(szBuf,"[patch-id]") != 0)
         bTargetIsPatched = 1;

   fclose(ftarg2);

   // ====================================================================================

   char szProbeCmd[MAX_LINE_LEN];

 if (!bGlblSimulate)
 { // begin backup block

   // make a backup of the target file
   char szRelFileName[1024];
   char szBackupDir[MAX_LINE_LEN];
   strcpy(szBackupDir, pszTargFileName);
   char *pszLastDir = strrchr(szBackupDir, glblPathChar);
   // if (!pszLastDir) { log(0, "error  : no '%c' path character in %s, cannot create backup dir.\n", glblPathChar, szBackupDir); return 2; }
   if (pszLastDir) {
      pszLastDir++;
      strcpy(szRelFileName,pszLastDir);
      *pszLastDir = 0;
      strcat(szBackupDir, "save_patch");
   } else {
      // file name without any path:
      strcpy(szRelFileName, pszTargFileName);
      strcpy(szBackupDir, "save_patch");
   }

   // IS a backup file already existing?
   sprintf(szProbeCmd,"%s%c%s",szBackupDir,glblPathChar,szRelFileName);

   // are we by any chance in REVOKE mode?
   if (bGlblRevoke && bTargetIsPatched)
   {
      // YES: copy backup file back.
      char szCopyCmd[MAX_LINE_LEN];
      // 0. ensure backup exists
      if (!fileExists(szProbeCmd)) { log(0, "error  : cannot revoke, no backup file: %s\n",szProbeCmd); return 1+4; }
      // is there an old target?
      if (fileExists(pszTargFileName))
      {
         // 1. ensure target is writeable
         #ifdef _WIN32
         sprintf(szCopyCmd, "attrib -R %s",pszTargFileName);
         #else
         sprintf(szCopyCmd, "chmod +w %s", pszTargFileName);
         #endif
         if (system(szCopyCmd)) { }
         #ifdef _WIN32
         // 2. delete target to ensure copy will work
         sprintf(szCopyCmd, "del %s",pszTargFileName);
         if (system(szCopyCmd)) { }
         #endif
      }
      // 3. copy backup over target
      if (bGlblTouchOnRevoke) {
         // make sure target has current timestamp
         FILE *fsrc = fopen(szProbeCmd, "rb");
         if (!fsrc) { log(0, "error  : cannot open %s\n",szProbeCmd); return 2+4; }
         FILE *fdst = fopen(pszTargFileName,"wb");
         if (!fdst) { log(0, "error  : cannot open %s\n",pszTargFileName); return 2+4; }
         // quick binary block copy
         // size_t fread( void *buffer, size_t size, size_t count, FILE *stream );
         // size_t fwrite( const void *buffer, size_t size, size_t count, FILE *stream );
         size_t ntotal = 0;
         while (1) {
            size_t nread = fread(szCopyCmd, 1, sizeof(szCopyCmd), fsrc);
            if (nread <= 0)
               break;
            fwrite(szCopyCmd, 1, nread, fdst);
            ntotal += nread;
         }
         fclose(fdst);
         fclose(fsrc);
         // write-protect target
         #ifdef _WIN32
         sprintf(szCopyCmd, "attrib +R %s",pszTargFileName);
         #else
         sprintf(szCopyCmd, "chmod -w %s", pszTargFileName);
         #endif
         if (system(szCopyCmd)) { }
         log(5, "revoked: %s, %u bytes\n",pszTargFileName,(unsigned int)ntotal);
         nGlblRevokedFiles++;
      } else {
         // xcopy requires us to create a dummy, otherwise we get a prompting
         FILE *fdst = fopen(pszTargFileName,"w");
         if (!fdst) { log(0, "error  : cannot open %s\n",pszTargFileName); return 2+4; }
         fprintf(fdst, "tmp\n\n"); fflush(fdst);
         fclose(fdst);
         // no overwrite this with /K, keeping potential +R attributes
         #ifdef _WIN32
         sprintf(szCopyCmd, "xcopy /Q /K /Y %s %s >nul",szProbeCmd,pszTargFileName);
         #else
         sprintf(szCopyCmd, "cp -p %s %s",szProbeCmd,pszTargFileName);
         #endif
         int iRC = system(szCopyCmd);
         if (!iRC) {
            log(5, "revoked: %s\n",pszTargFileName);
            nGlblRevokedFiles++;
         } else {
            log(0, "revoke failed: %s\n",pszTargFileName);
            return 1;
         }
      }
      // 4. make backup writeable, and remove
      #ifdef _WIN32
      sprintf(szCopyCmd, "attrib -R %s",szProbeCmd);
      #else
      sprintf(szCopyCmd, "chmod +w %s", szProbeCmd);
      #endif
      if (system(szCopyCmd)) { }
      if (remove(szProbeCmd)) {
         log(0, "error  : cannot delete stale backup: %s\n",szProbeCmd);
         return 1;
      }
      return 0;
   }

   if (bGlblRevoke && !bTargetIsPatched)
   {
      if (!bTargetIsPatched) {
         log(0, "warning: isn't patched, will not revoke: %s\n",pszTargFileName);
      }
   }

   // otherwise continue creating a backup
   if (bGlblBackup)
   {
      // prepare: if stale backup, delete first
      if (fileExists(szProbeCmd))
      {
         // remove old backup, most probably stale
         char szCopyCmd[MAX_LINE_LEN];
         log(5, "del.bup: %s\n",szProbeCmd);
         #ifdef _WIN32
         sprintf(szCopyCmd, "attrib -R %s",szProbeCmd);
         #else
         sprintf(szCopyCmd, "chmod +w %s", szProbeCmd);
         #endif
         if (system(szCopyCmd)) { }
         if (remove(szProbeCmd)) {
            log(0, "error  : cannot delete stale backup: %s\n",szProbeCmd);
            return 1;
         }
      }

      // create backup dir and file
      char szCopyCmd[MAX_LINE_LEN];
      int iRC = 0;
      // sprintf(szCopyCmd,"mkdir %s",szBackupDir);
      // int iRC = system(szCopyCmd); // create backup dir, if not done yet
      #ifdef _WIN32
      if (_mkdir(szBackupDir))
      #else
      if (mkdir(szBackupDir, S_IREAD | S_IWRITE | S_IEXEC))
      #endif
      {
         // log(0, "warning: cannot create backup dir: %s\n",szBackupDir);
         // return 1;
      }
      #ifdef _WIN32
      sprintf(szCopyCmd,"xcopy /Q /K %s %s >nul",pszTargFileName,szBackupDir);
      #else
      sprintf(szCopyCmd,"cp -p %s %s",pszTargFileName,szBackupDir);
      #endif
      iRC = system(szCopyCmd); // create backup file
      if (iRC) {log(0, "error  : creation of backup file failed: RC %d\n",iRC); return 2+4; }
      if (fileExists(szProbeCmd)) {
         log(5, "backupd: %s\n",szProbeCmd);
      } else {
         log(0, "error  : verify of backup file failed: %s\n",szProbeCmd); return 2+4;
      }
      return 0;
   }

 } // end backup block

   if (bGlblRevoke)
      return 0;

   // 3. read and patch the target file
   FILE *ftarg = fopen(pszTargFileName, "r");
   if (!ftarg) { log(0, "error  : cannot read target file: %s\n", pszTargFileName); return 2+4; }

   // we cache the output in memory
   int iout = 0;

   int isrcbuf=0, ipatmatch=0;
   aBuf[0] = 0;
   int icmd2 = 0, nLine = 0, nTargLineEndings = -1;
   while (fgets(szBuf,sizeof(szBuf)-10,ftarg) != NULL)
   {
      nLine++;

      // make sure target file has SAME line endings as cmd file.
      if (nTargLineEndings == -1)
      {
         if (strstr(szBuf,"\r\n") != 0)
            nTargLineEndings = 2;   // CR/LF
         else
            nTargLineEndings = 1;   // LF only
         if (nCmdFileLineEndings != nTargLineEndings)
         {
            log(0, "error  : different line endings!\n");
            log(0, "error  :   the patch file uses %s line endings.\n",(nCmdFileLineEndings==2)?"CR/LF":"LF");
            log(0, "error  :   this target file uses %s line endings: %s\n",(nTargLineEndings==2)?"CR/LF":"LF",pszTargFileName);
            return 2;
         }
      }

      if (strstr(szBuf,"[patch-id]") != 0) {
         if (bGlblVerify) {
            if (!bGlblQuickSum)
               log(5, "checked: %s is still patched\n",pszTargFileName);
            return 0;
         }
         if (!bGlblNoPID) {
            log(0, "error  : %s already patched\n",pszTargFileName);
            return 1;
         }
      }

      // does current in-line match the CURRENT pattern's current line?
      if ((icmd2<icmd) && !compareLines(szBuf,aPatch[aifrom[icmd2]+ipatmatch]))
      {
         ipatmatch++;
         aBuf[isrcbuf++] = strdup(szBuf);
         if (isrcbuf > SFKPATCH_MAX_CACHELINES-10) { log(0, "pattern cache overflow, %d lines exceeded\n",(int)SFKPATCH_MAX_CACHELINES); return 2; }
         if (ipatmatch == aifromlen[icmd2]) {
            // full pattern match:
            // write replacement pattern
            for (int i3=0;i3<aitolen[icmd2];i3++) {
               apOut[iout++] = strdup(aPatch[aito[icmd2]+i3]);
               if (iout > SFKPATCH_MAX_OUTLINES-10) { log(0, "output cache overflow, %d lines exceeded\n",(int)SFKPATCH_MAX_OUTLINES); return 2; }
            }
            // drop the cache
            for (int i4=0;i4<isrcbuf;i4++)
               free(aBuf[i4]);
            // reset stats
            isrcbuf=0;
            ipatmatch=0;
            // switch to next command
            icmd2++;
         }
      } else {
         // flush and clear the cache, if any
         for (int i2=0;i2<isrcbuf;i2++) {
            apOut[iout++] = strdup(aBuf[i2]);
            if (iout > SFKPATCH_MAX_OUTLINES-10) { log(0, "output cache overflow, %d lines exceeded\n",(int)SFKPATCH_MAX_OUTLINES); return 2; }
            free(aBuf[i2]);
         }
         // flush also current line, which wasn't cached
         apOut[iout++] = strdup(szBuf);
         if (iout > SFKPATCH_MAX_OUTLINES-10) { log(0, "output cache overflow, %d lines exceeded\n",(int)SFKPATCH_MAX_OUTLINES); return 2; }
         // reset stats
         isrcbuf=0;
         ipatmatch=0;
      }
   }

   if (icmd2 != icmd) {
      log(0, "error  : from-pattern %d of %d mismatch, in file %s\n",icmd2+1,icmd,pszTargFileName);
      log(0, "info   : check pattern content and SEQUENCE (must match target sequence).\n");
      return 2;
   }

   fclose(ftarg);

 if (!bGlblSimulate)
 {
   // now, we hold the patched target file in apOut.
   // overwrite the target.
   int bSwitchedAttrib = 0;
   cchar *pszFileMode = "w";
   if (bGlblUnixOutput)
         pszFileMode = "wb";
   ftarg = fopen(pszTargFileName, pszFileMode);
   if (!ftarg) {
      // open for write failed? try switching attributes
      #ifdef _WIN32
      sprintf(szProbeCmd, "attrib -R %s",pszTargFileName);
      #else
      sprintf(szProbeCmd, "chmod +w %s", pszTargFileName);
      #endif
      if (system(szProbeCmd)) { }
      bSwitchedAttrib = 1;
      ftarg = fopen(pszTargFileName, pszFileMode);
   }
   if (!ftarg) {
      log(0, "error  : cannot overwrite target file: %s\n", pszTargFileName); return 2+4;
   }

   // write and free all memory lines
   for (int n=0; n<iout; n++)
   {
      strcpy(szBuf, apOut[n]);

      // apply local changes, which have priority over globals
      int ipat=0;
      for (ipat=0; ipat<iLocalChange; ipat++) {
         if (   (strstr(szBuf, apLocalChange[ipat][0]) != 0)
             && (strstr(szBuf, apLocalChange[ipat][1]) != 0)
            )
         {
            // replace pattern within line
            if (bGlblVerbose) { printf("lc1>%s",szBuf);fflush(stdout); }

            char *psz = strstr(szBuf, apLocalChange[ipat][1]);
            *psz = 0;
            strcpy(szBuf2,szBuf);
            strcat(szBuf2,apLocalChange[ipat][2]);
            psz += strlen(apLocalChange[ipat][1]);
            strcat(szBuf2,psz);
            strcpy(szBuf,szBuf2);

            if (bGlblVerbose) { printf("lc2>%s",szBuf);fflush(stdout); }

            anLocalChange[ipat]++;
         }
      }

      // apply global changes, if anything left to change
      for (ipat=0; ipat<iGlobalChange; ipat++) {
         if (   (strstr(szBuf, apGlobalChange[ipat][0]) != 0)
             && (strstr(szBuf, apGlobalChange[ipat][1]) != 0)
            )
         {
            // replace pattern within line
            if (bGlblVerbose) { printf("gc1>%s",szBuf);fflush(stdout); }

            char *psz = strstr(szBuf, apGlobalChange[ipat][1]);
            *psz = 0;
            strcpy(szBuf2,szBuf);
            strcat(szBuf2,apGlobalChange[ipat][2]);
            psz += strlen(apGlobalChange[ipat][1]);
            strcat(szBuf2,psz);
            strcpy(szBuf,szBuf2);

            if (bGlblVerbose) { printf("gc2>%s",szBuf);fflush(stdout); }

            anGlobalChange[ipat]++;
         }
      }

      // apply detabbing, if selected
      if (nLocalDetabOutput) {
         if (detabLine(szBuf, szBuf2, MAX_LINE_LEN, nLocalDetabOutput))
            return 9;
      }
      if (nGlblDetabOutput) {
         if (detabLine(szBuf, szBuf2, MAX_LINE_LEN, nGlblDetabOutput))
            return 9;
      }

      // apply unix conversion or not, and finally write
      if (bGlblUnixOutput) {
         char *psz = strrchr(szBuf,'\n'); if (psz) *psz = 0;
               psz = strrchr(szBuf,'\r'); if (psz) *psz = 0;
         fprintf(ftarg, "%s\n", szBuf);
      } else {
         fputs(szBuf,ftarg);
      }

      free(apOut[n]);
   }

   fclose(ftarg);

   // have to re-enable write protection?
   if (bSwitchedAttrib) {
      #ifdef _WIN32
      sprintf(szProbeCmd, "attrib +R %s",pszTargFileName);
      #else
      sprintf(szProbeCmd, "chmod -w %s", pszTargFileName);
      #endif
      if (system(szProbeCmd)) { }
   }

   if (bSwitchedAttrib) {
      log(5, "Patched: %s, %d lines.\n",pszTargFileName,iout);
   } else {
      log(5, "patched: %s, %d lines.\n",pszTargFileName,iout);
   }
   nGlblPatchedFiles++;
 }
 else {
   log(5, "checked: %s, %d lines.\n",pszTargFileName,iout);
 }

   if (bGlblVerify) {
      log(0, "error  : %s no longer patched - probably overwritten\n",pszTargFileName);
      return 1;
   }

   return 0;
}

// ----------- sfk inst - c++ source code instrumentation support --------------

#define SFKINST_TRBSIZE    200 // token ring buffer
#define SFKINST_BLINESIZE 1024
#define SFKINST_BLINEMAX    50

class SrcParse
{
public:
   SrcParse ( );
   uint processFile (char *pText, bool bSimulate, FILE *foutOptional);
   void  processLine (char *pBuf, bool bSimulate);

protected:
   void addtok    (char c);
   void addWord   ( );
   void addColon  ( );
   void addScope  ( );
   void addBra    ( );
   void addKet    ( );
   void addCBra   ( );
   void addCKet   ( );
   void addSemi   ( );
   void addRemark ( );
   bool hasFunctionStart (uint &rn1stline);
   char pretok    (uint &itok2, uint &nstepdowncnt, uint &rnline);
   void addDetectKeyword   (char **rpsz);
   void reduceSignature    (char *pszIn, char *pszOut);

   enum eSFKInstScanStates {
      ess_idle  = 1,
      ess_word  = 2,
      ess_num   = 3,
      ess_colon = 4,
      ess_slash = 5
      };

private:
   uchar atok[SFKINST_TRBSIZE+10];
   uint itok;
   uchar altok[SFKINST_TRBSIZE+10]; // just for the current line
   uint iltok;
   uint atokline[SFKINST_TRBSIZE+10]; // line number of token
   uint nline;
   char  abline[SFKINST_BLINEMAX][SFKINST_BLINESIZE+2];
   uint ibline;
   uint ibackscope;
   bool  bbackscope;
   FILE *clOut;
   uint nClHits;

   char szLineBuf[MAX_LINE_LEN+10];
   char szLineBuf2[MAX_LINE_LEN+10];
   char szLineBuf3[MAX_LINE_LEN+10];
   char szBupDir[MAX_LINE_LEN+10];
   char szBupFile[MAX_LINE_LEN+10];
   char szCopyCmd[MAX_LINE_LEN+10];

public:
static bool
   bdebug,
   binsteol;

static cchar
   *pszGlblInclude,
   *pszGlblMacro;
};

bool   SrcParse::bdebug = 0;
bool   SrcParse::binsteol = 0;
cchar *SrcParse::pszGlblInclude = "";
cchar *SrcParse::pszGlblMacro   = "";

SrcParse::SrcParse()
{
   memset(this, 0, sizeof(*this));
}

uint SrcParse::processFile(char *pText, bool bSimulate, FILE *fout)
{
   if (!bSimulate) {
      clOut = fout;
      fprintf(fout, "#include \"%s\" // [instrumented]\n", pszGlblInclude);
   }

   // char *pCopy = strdup(pText);
   uint nTextLen = strlen(pText);
   char *pCopy = new char[nTextLen+10];
   if (!pCopy) { fprintf(stderr, "error  : out of memory at %d\n", __LINE__); return 0; }
   memcpy(pCopy, pText, nTextLen+1);

   nClHits = 0;

   // NO RETURNS FROM HERE

   uint nMaxLineLen = sizeof(szLineBuf)-10;
   char *psz1 = pCopy;
   char *pszContinue = 0;
   while (psz1)
   {
      char *psz2 = strchr(psz1, '\n');
      if (psz2)
      {
         pszContinue = psz2+1;
         int nLineLen = psz2 - psz1;
         if (nLineLen > (int)nMaxLineLen) nLineLen = nMaxLineLen;
         strncpy(szLineBuf, psz1, nLineLen);
         szLineBuf[nLineLen] = '\0';
         char *pszCR = strchr(szLineBuf, '\r');
         if (pszCR) *pszCR = '\0';
      }
      else
      {
         memset(szLineBuf, 0, sizeof(szLineBuf));
         strncpy(szLineBuf, psz1, nMaxLineLen);
         pszContinue = 0;
      }

      processLine(szLineBuf, bSimulate);
      psz1 = pszContinue;
   }

   // NO RETURNS UNTIL HERE

   delete [] pCopy;

   return nClHits;
}

bool isatoz(char c) { char c2=tolower(c); return (c2>='a' && c2<='z'); }
bool isnum_(char c) { return (c>='0' && c<='9') || (c=='_'); }

void SrcParse::reduceSignature(char *psz1, char *psz2)
{
   // reduce [type] class::method([parms])
   //     to class::method

   // goto last scope (in case there are many)
   //     cls1::type1 & cls2::type2::method3(
   char *pszs = strstr(psz1, "::");
   if (!pszs) { strcpy(psz2, psz1); return; }
   char *prbra = strrchr(psz1, '(');
   char *pszn;
   while ((pszn = strstr(pszs+1, "::")) && (pszn < prbra))
      pszs = pszn;
   // find head
   char *pszh = pszs-1;
   while (pszh >= psz1) {
      char c = *pszh;
      // if (!isnum_(c) && !isatoz(c))
      //    break;
      if (c == ' ' || c == '\t')
         break;
      pszh--;
   }
   while ((pszh<pszs) && !isatoz(*pszh))
      pszh++;
   // find tail
   char *pszt = pszs+2;
   while (*pszt) {
      char c = *pszt;
      // if ((c != '~') && !isnum_(c) && !isatoz(c))
      //   break;
      if (c == '(')
         break;
      pszt++;
   }
   while ((pszt > pszh) && (*pszt == ' ' || *pszt == '('))
      pszt--;
   pszt++;
   // check and copy
   if (pszh < pszt) {
      strncpy(psz2, pszh, pszt-pszh);
      psz2[pszt-pszh] = '\0';
   } else {
      strcpy(psz2, "?::?");
   }
}

void SrcParse::processLine(char *pszLine, bool bSimulate)
{
   nline++;

   char *pszx;
   if ((pszx = strchr(pszLine, '\n'))) *pszx = '\0';
   if ((pszx = strchr(pszLine, '\r'))) *pszx = '\0';

   // store in backline buffer
   strncpy(abline[ibline], pszLine, SFKINST_BLINESIZE-10);
   abline[ibline][SFKINST_BLINESIZE-10] = '\0';
   if (ibackscope == ibline) {
      bbackscope = 0; // got overwritten
   }

   // reset current line token buffer.
   // this is handled like a string, with zero terminator.
   iltok = 0;
   memset(altok, 0, sizeof(altok));

   // printf("] %s\n", pszLine);
   // printf("] %s\n", abline[ibline]);

   char *psz1 = pszLine;
   char c1;
   uchar nstate = ess_idle;
   do
   {
      c1 = *psz1; // incl. null at eol

		mtklog(("inst: %c %d",c1,nstate));

      if (isatoz(c1)) {
         switch (nstate) {
            case ess_idle:
               nstate=ess_word;
               addDetectKeyword(&psz1);
               break;
         }
      }
      else
      if (isnum_(c1)) {
         switch (nstate) {
            case ess_idle: nstate=ess_num ; break;
         }
      }
      else
      {
         // NOT alphanumeric
         switch (nstate) {
            case ess_word : nstate=ess_idle; addWord(); break;
         }

         if (c1 == '/') {
            switch (nstate) {
               case ess_idle : nstate=ess_slash; break;
               case ess_slash:
                  nstate=ess_idle ; addRemark(); c1=0; break;
            }
         }
         else
         if (c1 == '*') {
            switch (nstate) {
               case ess_slash:
                  nstate=ess_idle; addRemark(); break;
            }
         }
         else
         if (nstate == ess_slash)
             nstate = ess_idle;

         if (c1 == ':') {
            switch (nstate) {
               case ess_idle : nstate=ess_colon; break;
               case ess_colon: nstate=ess_idle ; addScope(); break;
            }
         }
         else
         if (nstate == ess_colon) {
             nstate = ess_idle;
             addColon();
         }
 
         if (c1 == '(') { nstate=ess_idle; addBra(); }
         else
         if (c1 == ')') { nstate=ess_idle; addKet(); }
         else
         if (c1 == '{') { nstate=ess_idle; addCBra(); }
         else
         if (c1 == '}') { nstate=ess_idle; addCKet(); }
         else
         if (c1 == ';') { nstate=ess_idle; addSemi(); }
      }

      psz1++;
   }
   while (c1);
   // printf("\n");

   // assumed function start in current line?
   if (   strstr((char*)altok, "wsw(")
       || strstr((char*)altok, "wsww(")
      )
   {
      ibackscope = ibline;
      bbackscope = 1;
   }

   strcpy(szLineBuf2, szLineBuf);

   // REDUCTION: so far, this accepts only "{" lines
   //            without anything else in it.
   if (hasFunctionStart(ibackscope))
   {
      char *pszMethodStartLine = abline[ibackscope];

		mtklog(("inst:   msline \"%.20s\"", pszMethodStartLine));

     if (bdebug)
     {
      printf("FN BODY at %d: %s\n", nline, (char*)atok);
      char *psz1;
      uint i2 = ibackscope;
      while ((psz1 = abline[i2])) {
         if (strlen(psz1))
            printf("] %s\n", psz1);
         if (i2 == ibline)
            break;
         if (++i2 >=  SFKINST_BLINEMAX)
            i2 = 0;
      }
     }

      // the current line contains the relevant "{" somewhere.
      // for now, we instrument only simple lines:
		mtklog(("inst:   lbuf2  \"%s\"", szLineBuf2));
		// accept "{" but also "[anywhitespace]{"
		char *pszs = szLineBuf2;
		while (*pszs && (*pszs==' ' || *pszs=='\t')) pszs++;
      if (!strcmp(pszs, "{")) {
         reduceSignature(pszMethodStartLine, szLineBuf3);
         sprintf(pszs, "{%s(\"%s\");", pszGlblMacro, szLineBuf3);
         // printf("=> %s \"%s\"\n", szLineBuf2, pszMethodStartLine);
         nClHits++;
      }
      else
      if (binsteol)
      do
      {
         // also instrument "{" at end of line
         char *pcur = strrchr(szLineBuf2, '{');
         if (!pcur) break;
         if (!strcmp(pcur, "{")) {
            reduceSignature(pszMethodStartLine, szLineBuf3);
            sprintf(pcur, "{%s(\"%s\");", pszGlblMacro, szLineBuf3);
            // printf("=> %s \"%s\"\n", szLineBuf2, pszMethodStartLine);
            nClHits++;
         }
      }
      while (0);
   }
   // else keep szLineBuf2 unchanged

   if (!bSimulate) {
      fputs(szLineBuf2, clOut);
      fputc('\n', clOut);
   }

   if (++ibline >= SFKINST_BLINEMAX)
      ibline = 0;
}

void SrcParse::addDetectKeyword(char **rpsz)
{
   char *psz1 = *rpsz;
   if (!strncmp(psz1, "class ", strlen("class ")))
      {  addtok('k'); psz1+=strlen("class "); }
   else
   if (!strncmp(psz1, "struct ", strlen("struct ")))
      {  addtok('k'); psz1+=strlen("struct "); }
   *rpsz = psz1;
}

void SrcParse::addWord()   { addtok('w'); }
void SrcParse::addColon()  { addtok(':'); }
void SrcParse::addScope()  { addtok('s'); }
void SrcParse::addBra()    { addtok('('); }
void SrcParse::addKet()    { addtok(')'); }
void SrcParse::addCBra()   { addtok('{'); }
void SrcParse::addCKet()   { addtok('}'); }
void SrcParse::addSemi()   { addtok(';'); }
void SrcParse::addRemark() { addtok('r'); }

void SrcParse::addtok(char c)
{
	mtklog(("inst:  addtok %c", c));

   atok[itok] = c;
   atokline[itok] = ibline;
   if (++itok >= SFKINST_TRBSIZE)
      itok = 0;
   atok[itok] = '*';

   altok[iltok] = c;
   if (++iltok >= SFKINST_TRBSIZE)
      iltok = 0;
   altok[iltok] = '*';
   // printf("%c", c);
}

char SrcParse::pretok(uint &itok2, uint &nstepcnt, uint &rnline)
{
   if (nstepcnt == 0)
      return 0;
   else
      nstepcnt--;

   if (itok2 == 0)
      itok2 = SFKINST_TRBSIZE-1;
   else
      itok2--;

   rnline = atokline[itok2];
   return atok[itok2];
}

bool SrcParse::hasFunctionStart(uint &rnline)
{
   uint itok2 = itok;
   uint ncnt  = SFKINST_TRBSIZE;

   // W* S W B W* K ** {

   uint ntline = 0;
   char c1 = pretok(itok2, ncnt, ntline);
   if (!c1) return false;
   if (c1 != '{') {
		mtklog(("inst:   hasfs %c, false", c1));
		return false;
	}

   // have a curly braket:

   uint nBra = 0;
   uint nKet = 0;
   uint nrc  = 0;
   uint nScope = 0;
   uint nBraDist = 0; // word distance from last bra
 
   while (!nrc && (c1 = pretok(itok2, ncnt, ntline))) {
      switch (c1) {
         case 's':
            if (bdebug) printf("s-hit %d %d\n",nBra,nKet);
            if (nBra > 0 && (nBra == nKet) && (nBraDist==1))
            {
               rnline = ntline;
               // nrc = 1;
               nScope++;
            }
            break;
         case '{': nrc=2; break;
         case '}': nrc=3; break;
         case ';': nrc=4; break;
         case '(': nBra++; nBraDist=0; break;
         case ')': nKet++; break;
         case ':': nBra=nKet=0; break;
         case 'k': nrc=5; break; // class or struct
         case 'w':
            nBraDist++;
            if (nScope == 1) {
               // probably standing on wsw(
               nrc = 1;
            }
            break;
      }
   }

   if (nrc == 1) {
		mtklog(("inst:   hasfs true"));
      return true;
	}

   if (bdebug) {
      printf("MISS.%d: %s %d %d\n", nrc, (char*)atok,nBra,nKet);
      char *psz1;
      uint i2=ibackscope;
      while ((psz1 = abline[i2])) {
         if (strlen(psz1))
            printf("] %s\n", psz1);
         if (i2 == ibline)
            break;
         if (++i2 >=  SFKINST_BLINEMAX)
            i2 = 0;
      }
   }

   return false;
}

static int fileSize(char *pszFile)
{
   struct stat sinfo;
   if (stat(pszFile, &sinfo))
      return -1;
   return sinfo.st_size;
}

// NOTE: DO NOT FORGET TO DELETE RESULT
static char *loadFile(char *pszFile, int nLine)
{
   int lFileSize = fileSize(pszFile);
   if (lFileSize < 0)
      return 0;
   char *pOut = new char[lFileSize+10];
   // printf("loadFile %p %d\n", pOut, nLine);
   FILE *fin = fopen(pszFile, "rb");
   if (!fin) { fprintf(stderr, "error  : cannot read: %s\n", pszFile); return 0; }
   int nRead = fread(pOut, 1, lFileSize, fin);
   fclose(fin);
   if (nRead != lFileSize) {
      fprintf(stderr, "error  : cannot read: %s (%d %d)\n", pszFile, nRead, lFileSize);
      delete [] pOut;
      return 0;
   }
   pOut[lFileSize] = '\0';
   return pOut;
}

int sfkInstrument(char *pszFile, cchar *pszInc, cchar *pszMac, bool bRevoke, bool bRedo, bool bTouchOnRevoke, int nmode)
{
   char szLineBuf[MAX_LINE_LEN+10];
   char szLineBuf2[MAX_LINE_LEN+10];
   char szLineBuf3[MAX_LINE_LEN+10];
   char szBupDir[SFK_MAX_PATH+100];
   char szBupFile[SFK_MAX_PATH+100];
   char szCopyCmd[MAX_LINE_LEN+10];

   // printf("INST %s %u %s %s\n",pszFile,bRevoke,pszInc,pszMac);

   SrcParse::binsteol = (nmode & 1) ? 1 : 0;

   SrcParse::pszGlblInclude = pszInc;
   SrcParse::pszGlblMacro   = pszMac;

   #ifdef _WIN32
   if (strstr(pszFile, "save_inst\\"))
   #else
   if (strstr(pszFile, "save_inst/"))
   #endif
   {
      fprintf(stderr, "warning: exclude, cannot instrument: %s\n", pszFile);
      return 5;
   }

   // create backup infos
   strcpy(szBupDir, pszFile);
   char *pszPath = strrchr(szBupDir, glblPathChar);
   if (pszPath) *pszPath = '\0';
   else strcpy(szBupDir, ".");
   char *pszRelFile = strrchr(pszFile, glblPathChar);
   if (pszRelFile) pszRelFile++;
   else pszRelFile = pszFile;
   strcat(szBupDir, glblPathStr);
   strcat(szBupDir, "save_inst");
   sprintf(szBupFile, "%s%c%s", szBupDir, glblPathChar, pszRelFile);
   // printf("BUPDIR: %s\n", szBupDir);
   // printf("BUPFIL: %s\n", szBupFile);

   if (bRevoke)
   {
      // first check if the file is really instrumented
      char *pFile = loadFile(pszFile, __LINE__);
      bool bisins = 1;
      if (pFile) {
         if (!strstr(pFile, "// [instrumented]"))
            bisins = 0;
         delete [] pFile;
      }

      if (!bisins)
      {
         if (!bRedo) {
            fprintf(stderr, "skipped: %s - not instrumented\n", pszFile);
            return 1;
         }  // else fall through
      }
      else
      {
         if (!fileExists(szBupFile)) { fprintf(stderr, "warning: cannot revoke, no backup: %s\n", pszFile); return 5; }
 
         if (fileExists(pszFile)) {
            // 1. ensure target is writeable
            #ifdef _WIN32
            sprintf(szCopyCmd, "attrib -R %s",pszFile);
            #else
            sprintf(szCopyCmd, "chmod +w %s", pszFile);
            #endif
            if (system(szCopyCmd)) { }
            #ifdef _WIN32
            // 2. delete target to ensure copy will work
            sprintf(szCopyCmd, "del %s",pszFile);
            if (system(szCopyCmd)) { }
            #endif
         }

         char *pszTargFileName = pszFile;

         if (bTouchOnRevoke)
         {
            // make sure target has current timestamp
            FILE *fsrc = fopen(szBupFile, "rb");
            if (!fsrc) { fprintf(stderr, "error  : cannot open %s\n",szBupFile); return 2+4; }
            FILE *fdst = fopen(pszTargFileName,"wb");
            if (!fdst) { fprintf(stderr, "error  : cannot open %s\n",pszTargFileName); return 2+4; }
            // quick binary block copy
            // size_t fread( void *buffer, size_t size, size_t count, FILE *stream );
            // size_t fwrite( const void *buffer, size_t size, size_t count, FILE *stream );
            size_t ntotal = 0;
            while (1) {
               size_t nread = fread(szCopyCmd, 1, sizeof(szCopyCmd), fsrc);
               if (nread <= 0)
                  break;
               fwrite(szCopyCmd, 1, nread, fdst);
               ntotal += nread;
            }
            fclose(fdst);
            fclose(fsrc);
            // write-protect target
            #ifdef _WIN32
            sprintf(szCopyCmd, "attrib +R %s",pszTargFileName);
            #else
            sprintf(szCopyCmd, "chmod -w %s", pszTargFileName);
            #endif
            if (system(szCopyCmd)) { }
            if (!bRedo) {
               printf("revoked: %s, %u bytes\n",pszTargFileName,(unsigned int)ntotal);
               return 0;
            }  // else fall through
         }
         else
         {
            // xcopy requires us to create a dummy, otherwise we get a prompting
            FILE *fdst = fopen(pszTargFileName,"w");
            if (!fdst) { fprintf(stderr, "error  : cannot open %s\n",pszTargFileName); return 9; }
            fprintf(fdst, "tmp\n\n"); fflush(fdst);
            fclose(fdst);
            // now overwrite this with /K, keeping potential +R attributes
            #ifdef _WIN32
            sprintf(szCopyCmd, "xcopy /Q /K /Y /R %s %s >nul",szBupFile,pszTargFileName);
            #else
            sprintf(szCopyCmd, "cp -p %s %s",szBupFile,pszTargFileName);
            #endif
            int iRC = system(szCopyCmd);
            if (!iRC) {
               if (!bRedo) {
                  printf("revoked: %s\n",pszTargFileName);
                  return 0;
               }  // else fall through
            } else {
               fprintf(stderr, "error  : revoke failed: %s\n",pszTargFileName);
               return 1;
            }
         }   // endelse touch-on-revoke
      }  // endelse is-instrumented
   }

   // instrument the target file

   int nsize = fileSize(pszFile);
   if (nsize <= 0) { fprintf(stderr, "skipped: %s - empty file\n", pszFile); return 5; }
   if (nsize > 50 * 1048576) { fprintf(stderr, "warning: too large, skipping: %s\n", pszFile); return 5; }

   char *pFile = loadFile(pszFile, __LINE__);
   if (!pFile)
      return 9;

   // COVER ALL RETURNS WITH pFILE CLEANUP FROM HERE:

   if (strstr(pFile, "// [instrumented]")) {
      fprintf(stderr, "warning: already instrumented, skipping: %s\n", pszFile);
      delete [] pFile;
      return 5;
   }

   SrcParse *pparse1 = new SrcParse();
   if (pparse1->processFile(pFile, 1, 0) == 0) {
      fprintf(stderr, "skipped: %s - nothing to change\n", pszFile);
      delete [] pFile;
      delete pparse1;
      return 5;
   }
   delete pparse1;

   // create backup file
   #ifdef _WIN32
   _mkdir(szBupDir);
   #else
   mkdir(szBupDir, S_IREAD | S_IWRITE | S_IEXEC);
   #endif
   FILE *ftmp = fopen(szBupFile,"w");
   if (ftmp) { fprintf(ftmp,"dummy"); fclose(ftmp); }
   #ifdef _WIN32
   sprintf(szLineBuf, "xcopy /Q /K /Y /R %s %s >nul",pszFile,szBupFile);
   #else
   sprintf(szLineBuf, "cp -p %s %s",pszFile,szBupFile);
   #endif
   int iRC = system(szLineBuf);
   if (iRC) {
      fprintf(stderr, "error  : cannot backup file to: %s\n",szBupFile);
      delete [] pFile;
      return 9;
   }

   // reopen target file for write
   FILE *fout = fopen(pszFile, "w");
   if (!fout) {
      // probably write protected
      #ifdef _WIN32
      sprintf(szLineBuf, "attrib -R %s", pszFile);
      #else
      sprintf(szLineBuf, "chmod +w %s",  pszFile);
      #endif
      if (system(szLineBuf)) { }
      // retry
      if (!(fout = fopen(pszFile, "w"))) {
         fprintf(stderr, "error  : unable to write: %s\n", pszFile);
         delete [] pFile;
         return 9;
      }
   }

   SrcParse *pparse2 = new SrcParse();
   uint nHits = pparse2->processFile(pFile, 0, fout);

   // cleanup
   delete pparse2;
   delete [] pFile;
   fclose(fout);

   if (bRedo)
      printf("redone : %s, %d hits\n", pszFile, nHits);
   else
      printf("inst'ed: %s, %d hits\n", pszFile, nHits);

   return 0;
}

static char abLineEditPartInfo[1024];
char abLineEditPartInfo2[1024];

int pointedit(char *pszMaskIn, char *pszSrc, int *pOutMatchLen, char *pszDst, int iMaxDst, bool verb)
{
   ushort aFrom[1024];
   ushort aInfoIdx[1024];
   ushort aTo[1024];
   char   aLitPart[22][610];
   char   aPart[22][610];
   int    iPartIdx= 0, iPartIdxMax = 12-2;
   int    iPartChr= 0, iPartChrMax = 610-10;

   char *pszMask = pszMaskIn;

   int iMaskLen = strlen(pszMask);
   if (iMaskLen < 3)
      return 10+(verb?perr("/from/to/ too short"):0);

   int iFromCur = 0;
   int iFromMax = (sizeof(aFrom)/sizeof(ushort))-10;
   int iToCur   = 0;
   int iToMax   = (sizeof(aTo)/sizeof(ushort))-10;

   // --- split from/to and tokenize ---

   char csep = *pszMask++;
   char ccur = 0, csub = 0;
   char *pszCur = 0;

   while (*pszMask!=0 && *pszMask!=csep)
   {
      if (iFromCur >= iFromMax)
         return 11+(verb?perr("from text too long"):0);

      pszCur = pszMask;

      ccur = *pszMask++;

      if (cs.spat!=0 && ccur=='\\') {
         int iskip=1;
         ccur=0;
         switch (*pszMask) {
            case 't': ccur='\t'; break;
            case 'q': ccur='"'; break;
            case '\\': case '*': case '?':
            case '[': case ']': case '#':
               ccur=*pszMask; break;
            case 'x':
               if (pszMask[1] && pszMask[2])
                  ccur = (char)getTwoDigitHex(pszMask+1);
               iskip=3;
               break;
         }
         if (!ccur)
            return 12+(verb?perr("invalid slash pattern: %s",pszMask-1):0);
         pszMask+=iskip;
         aInfoIdx[iFromCur] = (ushort)(pszMask-1-pszMaskIn);
         aFrom[iFromCur++] = ccur;
         continue;
      }

      if (ccur == '*') {
         aFrom[iFromCur] = 0x2000U;
         // determine stop literal and store as part
         if (iPartIdx >= 20)
            return 13+(verb?perr("too many parts"):0);
         int iLitChrIdx = pszMask-pszMaskIn;
         int iLitChrLen = 0;
         for (; pszMask[iLitChrLen]; iLitChrLen++) {
            csub = pszMask[iLitChrLen];
            if (csub==csep || csub=='*' || csub=='?')
               break;
            aLitPart[iPartIdx][iLitChrLen] = csub;
         }
         aLitPart[iPartIdx][iLitChrLen] = '\0';
         aFrom[iFromCur] += iPartIdx;
         aInfoIdx[iFromCur] = (ushort)(pszMask-1-pszMaskIn);
         iFromCur++;
         iPartIdx++;
         continue;
      }

      if (ccur == '?') {
         int ilen = 1;
         while (*pszMask!=0 && *pszMask=='?') {
            ilen++;
            pszMask++;
         }
         aInfoIdx[iFromCur] = (ushort)(pszMask-1-pszMaskIn);
         aFrom[iFromCur++] = 0x1000U + ilen;
         continue;
      }

      if (ccur == '[') {
         // [2 chars]
         int ilen = atoi(pszMask);
         while (isdigit(*pszMask))
            pszMask++;
         if (*pszMask!=' ')
            return 14+(verb?perr("wrong [] syntax: %s",pszCur):0);
         pszMask++;
         if (strBegins(pszMask, "chars]")) {
            pszMask += strlen("chars]");
            aInfoIdx[iFromCur] = (ushort)(pszMask-1-pszMaskIn);
            aFrom[iFromCur++] = 0x1000U + ilen;
            continue;
         }
         return 15+(verb?perr("wrong [] syntax: %s",pszCur):0);
      }

      aInfoIdx[iFromCur] = (ushort)(pszMask-1-pszMaskIn);
      aFrom[iFromCur++] = tolower(ccur);
   }
   aFrom[iFromCur] = 0;
   iFromMax = iFromCur;
   if (*pszMask!=csep)
      return 16+(verb?perr("missing end separator '%c'",csep):0);
   pszMask++;

   int iToPart = 0;

   while (*pszMask!=0 && *pszMask!=csep)
   {
      if (iToCur >= iToMax)
         return 17+(verb?perr("to text too long"):0);

      if (cs.spat!=0 && *pszMask=='\\') {
         pszMask++;
         int iskip=1;
         ccur=0;
         switch (*pszMask) {
            case 't': ccur='\t'; break;
            case 'q': ccur='"'; break;
            case '\\': case '*': case '?':
            case '[': case ']': case '#': 
               ccur=*pszMask; break;
            case 'x':
               if (pszMask[1] && pszMask[2])
                  ccur = (char)getTwoDigitHex(pszMask+1);
               iskip=3;
               break;
         }
         if (!ccur)
            return 18+(verb?perr("invalid slash pattern: %s",pszMask-1):0);
         pszMask+=iskip;
         aTo[iToCur++] = ccur;
         continue;
      }

      if (*pszMask=='#') {
         pszMask++;
         if (isdigit(*pszMask)) {
            iToPart = atoi(pszMask);
            while (isdigit(*pszMask))
               pszMask++;
         } else {
            iToPart++;
         }
         int ipart = iToPart;
         if (ipart > 0)
             ipart--;
         aTo[iToCur++] = 0x1000U + ipart;
         continue;
      }

      if (!strncmp(pszMask, "[parts ", 7))
      {
         pszMask += 7;
         bool bfill = 0;
         while (1)
         {
            // expect partno or ]
            if (isdigit(*pszMask)) {
               int ipart = atoi(pszMask);
               if (ipart < 1)
                  return 19+(verb?perr("wrong part number: %d",ipart):0);
               while (isdigit(*pszMask))
                  pszMask++;
               if (bfill) {
                  bfill=0;
                  iToPart++;
                  for (; iToPart<ipart; iToPart++)
                     aTo[iToCur++] = 0x1000U + (iToPart-1);
                  aTo[iToCur++] = 0x1000U + (ipart-1);
                  iToPart=ipart;
               } else {
                  iToPart=ipart;
                  aTo[iToCur++] = 0x1000U + (ipart-1);
               }
               continue;
            }
            if (*pszMask==',') {
               pszMask++;
               bfill=0;
               continue;
            }
            if (*pszMask=='-') {
               pszMask++;
               bfill=1;
               continue;
            }
            if (*pszMask==']') {
               pszMask++;
               break;
            }
            return 20+(verb?perr("wrong parts syntax"):0);
         }
         continue;
      }

      if (!strncmp(pszMask, "[part", 5)) {
         pszMask += 5;
         if (*pszMask==' ')
            pszMask++;
         int ipart = atoi(pszMask);
         if (ipart > 0)
             ipart--;
         iToPart=ipart+1;
         while (*pszMask!=0 && isdigit(*pszMask))
            pszMask++;
         if (*pszMask != ']')
            return 21+(verb?perr("missing ]"):0);
         pszMask++;
         aTo[iToCur++] = 0x1000U + ipart;
         continue;
      }

      aTo[iToCur++] = *pszMask++;
   }
   aTo[iToCur] = 0;
   iToMax = iToCur;
   if (*pszMask!=csep)
      return 22+(verb?perr("missing end separator '%c'",csep):0);

   pszMask++;
   if (*pszMask)
      return 23+(verb?perr("unexpected text after end separator '%c': %s",csep,pszMask):0);

   // --- match source and collect dynamic parts ---

   char *pSrcCur = pszSrc;
   int   iFrom   = 0;
   int   istate  = 0;
   int   imaxcol = 0;
   int   iLitPart= 0;

   iPartIdx = 0;
   iPartChr = 0;

   if (aFrom[iFrom] >= 0x2000U) {
      istate   = 2;
      iLitPart = aFrom[iFrom] & 0x0FFU;
      if (aLitPart[iLitPart][0])
         istate = 3;
   }
   else
   if (aFrom[iFrom] >= 0x1000U) {
      istate  = 1;
      imaxcol = aFrom[iFrom] & 0x0FFFU;
   }

   bool btrace = 0;

   if (iMaskLen+10 < sizeof(abLineEditPartInfo))
     memset(abLineEditPartInfo, ' ', iMaskLen);

   while (*pSrcCur && aFrom[iFrom])
   {
      // int ipinf = pSrcCur - pszSrc;
      // if (ipinf+10 < sizeof(abLineEditPartInfo))
      //    abLineEditPartInfo[ipinf] = '1'+(iPartIdx%10);

      ushort ninfidx = aInfoIdx[iFrom];
      if (ninfidx+10 < sizeof(abLineEditPartInfo))
         abLineEditPartInfo[ninfidx] = '1'+(iPartIdx%10);

      char csrc = *pSrcCur++;

      if (iPartIdx >= iPartIdxMax)
         return 24+(verb?perr("too many match parts"):0);
      if (iPartChr >= iPartChrMax)
         return 25+(verb?perr("match part overflow"):0);

      switch (istate)
      {
         case 0: // any or single char
            if (aFrom[iFrom] >= 0x1000U) {
               pSrcCur--;
               break;
            }
            if (tolower(csrc) != aFrom[iFrom]) {
               if (btrace) {
                  printf("miss %c %c\n",csrc,aFrom[iFrom]);
                  for (int i=0; i<iPartIdx; i++)
                     printf("p%d %s\n",i+1,aPart[i]);
               }
               return 1; // no match
            }
            // single char matched
            aPart[iPartIdx][iPartChr++] = csrc;
            iFrom++;
            continue;

         case 1: // group of ?
            aPart[iPartIdx][iPartChr++] = csrc;
            imaxcol--;
            if (imaxcol > 0)
               continue;
            iFrom++;
            break;

         case 2: // open *
            aPart[iPartIdx][iPartChr++] = csrc;
            continue;

         case 3: // * then literal
            // look for stop literal part
            char *psz  = pSrcCur-1;
            int   ichr = 0;
            char  c1=0, c2=0;
            for (;;ichr++) {
               c1 = tolower(psz[ichr]);
               c2 = tolower(aLitPart[iLitPart][ichr]);
               // if (btrace) printf("cmp %c %c\n",c1,c2);
               if (!c1 || !c2) break;
               if (c1 != c2) break;
            }
            if (c2) {
               aPart[iPartIdx][iPartChr++] = csrc;
               continue;
            }
            // stop literal match
            istate = 4;
            pSrcCur--;
            if (btrace) printf("stoplit match: %s\n",pSrcCur);
            iFrom++;
            break;
      }

      if (istate > 0 || iPartChr > 0) {
         aPart[iPartIdx][iPartChr] = '\0';
         iPartIdx++;
         iPartChr = 0;
      }

      if (aFrom[iFrom] >= 0x2000U) {
         istate   = 2;
         iLitPart = aFrom[iFrom] & 0x0FFU;
         if (aLitPart[iLitPart][0])
            istate = 3;
         if (btrace) printf("to state %d seeking \"%s\"\n",istate,aLitPart[iLitPart]);
      }
      else
      if (aFrom[iFrom] >= 0x1000U) {
         istate  = 1;
         imaxcol = aFrom[iFrom] & 0x0FFFU;
         if (btrace) printf("to state %d over %d chars on %s\n",istate,imaxcol,pSrcCur);
      }
      else if (aFrom[iFrom])
      {
         istate  = 0;
         if (btrace) printf("to state %d on %s mask %c\n",istate,pSrcCur,aFrom[iFrom]);
      }
   }
   if (iMaskLen+10 < sizeof(abLineEditPartInfo))
      abLineEditPartInfo[iMaskLen] = '\0';

   if (istate > 0) {
      aPart[iPartIdx][iPartChr] = '\0';
      iPartIdx++;
   }

   // is it a match?
   if (aFrom[iFrom]!=0 && istate!=2) {
      if (btrace) printf("mask not done at end of src\n");
      return 1;
   }
   // if (aFrom[iFrom]==0 && *pSrcCur!=0) {
   //    if (btrace) printf("src not done at end of mask\n");
   //    return 1;
   // }
   *pOutMatchLen = (int)(pSrcCur - pszSrc);
   if (btrace) printf("point match len=%d\n", *pOutMatchLen);

   if (btrace) {
      for (int i=0; i<iPartIdx; i++)
         printf("p%d %s\n",i+1,aPart[i]);
   }

   // --- build output from to mask and parts ---
 
   ushort *pToSrc = aTo;
   char *pDstCur = pszDst;
   char *pDstMax = pszDst+iMaxDst-10;
   while (*pToSrc)
   {
      if (pDstCur >= pDstMax)
         return 26+(verb?perr("output buffer overflow"):0);

      ushort csrc = *pToSrc++;

      if (csrc < 0x1000) {
         *pDstCur++ = (char)csrc;
         continue;
      }

      int ipart = csrc & 0xFFFU;
      if (ipart < 0 || ipart >= iPartIdxMax)
         return 27+(verb?perr("invalid part number: %d",ipart+1):0);

      char *ppart = aPart[ipart];
      while (*ppart) {
         if (pDstCur >= pDstMax)
            return 28+(verb?perr("output buffer overflow"):0);
         *pDstCur++ = *ppart++;
      }
   }
   *pDstCur = '\0';

   return 0;
}

void lineeditinit()
{
   memset(abLineEditPartInfo2, 0, sizeof(abLineEditPartInfo2));
}

char *lineeditinfo()
{
   return abLineEditPartInfo2;
}

int lineedit(char *pszMaskIn, char *pszSrc, char *pszDst, int iMaxDst, char *pAtt1, char *pAtt2, 
   uint flags, int *poff, int *plen)
{
   bool bexact = (flags & 1) ? 1 : 0;
   bool bverb  = (flags & 2) ? 1 : 0;

   char aPointBuf[1024];
   mclear(aPointBuf);

   char *pSrcOld = pszSrc;
   char *pSrcCur = pszSrc;
   char *pDstCur = pszDst;
   char *pDstMax = pszDst+iMaxDst-10;

   bool  bAnyDone = 0;

   while (*pSrcCur)
   {
      int imatch = 0;
      int isubrc = pointedit(pszMaskIn, pSrcCur, &imatch, aPointBuf, sizeof(aPointBuf), bverb);
      if (isubrc >= 10)
         return isubrc;
      if (isubrc > 0) {
         if (bexact)
            return 1;
         if (pDstCur > pDstMax)
            return 30+perr("ledit: output overflow");
         *pDstCur++ = *pSrcCur++;
         continue;
      }
      if (imatch < 1)
         return 31+perr("ledit: invalid match");
      int iedit = strlen(aPointBuf);
      // mark match src
      if (pAtt1) memset(pAtt1+(pSrcCur-pszSrc),'i',imatch);
      // mark match dst
      if (pAtt2) memset(pAtt2+(pDstCur-pszDst),'a',iedit);
      // copy edited part
      if (pDstCur+iedit > pDstMax)
         return 32+perr("ledit: output overflow");
      memcpy(pDstCur, aPointBuf, iedit);
      pDstCur += iedit;
      if (!bAnyDone)
      {
         bAnyDone = 1;
         if (!abLineEditPartInfo2[0]) {
            memcpy(abLineEditPartInfo2, abLineEditPartInfo, sizeof(abLineEditPartInfo));
            myrtrim(abLineEditPartInfo2);
         }
         if (poff) *poff = (int)(pSrcCur-pszSrc);
         if (plen) *plen = imatch;
      }
      // skip matched source
      pSrcCur += imatch;
      if (bexact && *pSrcCur)
         return 1;
   }
   *pDstCur = '\0';

   return bAnyDone ? 0 : 1;
}

int execRename(Coi *pcoi)
{
   char abSrcBuf[1024];
   char abDstRel[1024];
   char abSrcRelAtt[1024];
   char abDstRelAtt[1024];
   char abDstAbs[1024];
   char abSrcAbsAtt[1024];
   char abDstAbsAtt[1024];
   char abSrcPrint[1024];
   char abDstPrint[1024];

   mclear(abDstRel);
   mclear(abDstAbs);

   strcopy(abSrcBuf, pcoi->name());

   char *pszSrcPath = 0;
   char *pszRelFile = 0;

   char *prel = strrchr(abSrcBuf, glblPathChar);
   if (prel) {
      *prel++ = '\0';
      pszRelFile = prel;
      pszSrcPath = abSrcBuf;
   } else {
      pszRelFile = abSrcBuf;
      pszSrcPath = str("");
   }

   memset(abSrcRelAtt, 0, sizeof(abSrcRelAtt));
   memset(abSrcRelAtt, ' ', sizeof(abSrcRelAtt)-10);
   memset(abDstRelAtt, 0, sizeof(abDstRelAtt));
   memset(abDstRelAtt, ' ', sizeof(abDstRelAtt)-10);

   memset(abSrcAbsAtt, 0, sizeof(abSrcAbsAtt));
   memset(abSrcAbsAtt, ' ', sizeof(abSrcAbsAtt)-10);
   memset(abDstAbsAtt, 0, sizeof(abDstAbsAtt));
   memset(abDstAbsAtt, ' ', sizeof(abDstAbsAtt)-10);

   memset(abLineEditPartInfo, 0, sizeof(abLineEditPartInfo));

   if (cs.listfiles)
   {
      // just print selected files
      strcopy(abSrcPrint, pcoi->name());
      int iFullLen = strlen(abSrcPrint);
      int iPathLen = strlen(pszSrcPath)+1;
      int iRelOff  = iPathLen;
      int iRelLen  = iFullLen-iPathLen;
      if (iRelOff>=0 && iRelLen>0)
         memset(abSrcAbsAtt+iRelOff, 'i', iRelLen);
      printColorText(abSrcPrint, abSrcAbsAtt);
      return 0;
   }

   int iMatchOff=0,iMatchLen=0;
   int isubrc = lineedit(cs.renexp, pszRelFile,
                  abDstRel, sizeof(abDstRel),
                  abSrcRelAtt, abDstRelAtt,
                  cs.exact|2, &iMatchOff, &iMatchLen);

   if (isubrc) {
      // printf("miss: %s\n", pszFile);
      return 0; // no match
   }
   // printf("match: %s\n", pszFile);

   // fromtext matches
   int iDstRelSkip=0;
   if (cs.rentodir)
      joinPath(abDstAbs, sizeof(abDstAbs), cs.rentodir, abDstRel, &iDstRelSkip);
   else
      joinPath(abDstAbs, sizeof(abDstAbs), pszSrcPath, abDstRel, &iDstRelSkip);

   // ignore if no name change
   if (!strcmp(pcoi->name(), abDstAbs))
      return 0;

   // isolate output folder
   char szOutPath[1024];
   strcopy(szOutPath, abDstAbs);
   prel = strrchr(szOutPath, glblPathChar);
   #ifdef _WIN32
   if (!prel) {
      prel = strrchr(szOutPath, ':');
      if (prel)
         prel++;
   }
   #endif
   if (prel)
      *prel = '\0';
   else
      szOutPath[0] = '\0';
   // is output folder same as input?
   bool bMayCheckOutPath=1;
   if (!strcmp(szOutPath, pszSrcPath))
      bMayCheckOutPath=0;

   // highlight source match
   {
      int iCopyLen = strlen(pszRelFile);
      int iCopyPos = strlen(pcoi->name()) - iCopyLen;
      if (iCopyPos >= 0)
         memcpy(abSrcAbsAtt+iCopyPos+6, abSrcRelAtt, iCopyLen);
   }

   // highlight dst match
   {
      int iCopyLen = (int)strlen(abDstRel) - iDstRelSkip;
      int iCopyPos = (int)strlen(abDstAbs) - iCopyLen;
      if (iCopyPos >= 0 && iCopyLen > 0)
         memcpy(abDstAbsAtt+iCopyPos+6, abDstRelAtt+iDstRelSkip, iCopyLen);
   }

   snprintf(abSrcPrint, sizeof(abSrcPrint)-10, "FROM: %s", pcoi->name());
   abSrcPrint[sizeof(abSrcPrint)-10] = '\0';
   memset(abSrcAbsAtt, 'f', 6);
   printColorText(abSrcPrint, abSrcAbsAtt);

   if (cs.sim < 2)
   {
      bool bredundant=0;
      bool bexists=0;
      bool bnooutdir=0;
      bool bbadoutdir=0;

      if (cs.sim)
      {
         if (glblOutFileMap.isset(abDstAbs)) {
            bredundant=1;
            cs.filesRedundant++;
         } else {
            glblOutFileMap.put(abDstAbs);
         }

         Coi ocoi(abDstAbs, 0);
         if (ocoi.existsFile(1)) {
            bexists=1;
            cs.filesExisting++;
         }

         if (bMayCheckOutPath)
         {
            Coi opath(szOutPath, 0);
            if (opath.existsFile(0)) {
               if (cs.verbose>1)
                  printf("ERR : file exist with name: %s\n", szOutPath);
               bbadoutdir=1;
               cs.badOutDir++;
            }
            else if (!opath.existsFile(1)) {
               if (cs.verbose>1)
                  printf("ERR : dir does not exist: %s\n", szOutPath);
               bnooutdir=1;
               cs.noOutDir++;
            }
         }
      }

      if (bredundant || bexists || bnooutdir || bbadoutdir)
      {
         char szinfo[200];
         szinfo[0]='\0';
         cchar *szcont="";

         if (bredundant)
            { strcat(szinfo, "redundant"); szcont=","; }
         if (bexists)
            { strcat(szinfo, szcont); strcat(szinfo, "exists"); szcont=","; }
         if (bnooutdir)
            { strcat(szinfo, szcont); strcat(szinfo, "no outdir"); szcont=","; }
         if (bbadoutdir)
            { strcat(szinfo, szcont); strcat(szinfo, "bad outdir"); szcont=","; }

         snprintf(abDstPrint, sizeof(abDstPrint)-10, "ERR : %s - %s", abDstAbs, szinfo);
         abDstPrint[sizeof(abDstPrint)-10] = '\0';
         memset(abDstAbsAtt, 'e', 6);
         printColorText(abDstPrint, abDstAbsAtt);
      }
      else
      {
         snprintf(abDstPrint, sizeof(abDstPrint)-10, "TO  : %s", abDstAbs);
         abDstPrint[sizeof(abDstPrint)-10] = '\0';
         memset(abDstAbsAtt, 'f', 6);
         printColorText(abDstPrint, abDstAbsAtt);
      }
   }

   if (!cs.sim && cs.yes)
   {
      isubrc = pcoi->renameto(abDstAbs);
      if (isubrc)
         return 1+perr("... rename failed, rc=%d\n", isubrc);
   }

   return 0;
}

bool getistr(char *psz, int idigits, int ifrom, int ito, int &rout)
{
   int r=0;
   for (int i=0; i<idigits; i++)
   {
      char c = *psz++;
      if (!isdigit(c))
         return 0;
      r = r * 10 + (c - '0');
   }
   if (r < ifrom) return 0;
   if (r > ito) return 0;
   rout = r;
   return 1;
}

bool matchdate(char *pszsrcio, cchar *pszmask, int *adate, bool breorder=0)
{
   char *pszsrc = pszsrcio;

   int Y=0,M=0,D=0,h=0,m=0,s=0;
   int i=0,istate=0;

   int ireqdigits = 0;
   for (char *psz=(char*)pszmask; *psz; psz++)
      switch (*psz) {
         case 'Y': ireqdigits += 4; break;
         case 'M': ireqdigits += 2; break;
         case 'D': ireqdigits += 2; break;
         case 'h': ireqdigits += 2; break;
         case 'm': ireqdigits += 2; break;
         case 's': ireqdigits += 2; break;
      }

   int ihavedigits = 0;
   for (char *psz=pszsrc; *psz; psz++)
      if (isdigit(*psz))
         ihavedigits++;
      else
         break;

   if (ihavedigits < ireqdigits)
      return 0;

   while (*pszmask && *pszsrc)
   {
      switch (*pszmask)
      {
         case 'Y':
            if (!getistr(pszsrc,4,1970,3000,Y)) {
               if (istate) return 0;
               pszsrc++;
               continue;
            }
            adate[0] = Y;
            pszmask++;
            pszsrc += 4;
            istate=1;
            continue;
         case 'M':
            if (!getistr(pszsrc,2,01,12,M)) {
               if (istate) return 0;
               pszsrc++;
               continue;
            }
            adate[1] = M;
            pszmask++;
            pszsrc += 2;
            istate=1;
            continue;
         case 'D':
            if (!getistr(pszsrc,2,01,31,D)) {
               if (istate) return 0;
               pszsrc++;
               continue;
            }
            adate[2] = D;
            pszmask++;
            pszsrc += 2;
            istate=1;
            continue;
         case 'h':
            if (!getistr(pszsrc,2,00,23,h)) {
               if (istate) return 0;
               pszsrc++;
               continue;
            }
            adate[3] = h;
            pszmask++;
            pszsrc += 2;
            istate=1;
            continue;
         case 'm':
            if (!getistr(pszsrc,2,00,59,m)) {
               if (istate) return 0;
               pszsrc++;
               continue;
            }
            adate[4] = m;
            pszmask++;
            pszsrc += 2;
            istate=1;
            continue;
         case 's':
            if (!getistr(pszsrc,2,00,59,s)) {
               if (istate) return 0;
               pszsrc++;
               continue;
            }
            adate[5] = s;
            pszmask++;
            pszsrc += 2;
            istate=1;
            continue;
      }
   }

   if (!*pszmask)
   {
      if (breorder==1 && (pszsrc-pszsrcio)==8)
      {
         // rebuild date in order: YMD
         pszsrcio[0] = ((adate[0] / 1000) % 10) + '0';
         pszsrcio[1] = ((adate[0] /  100) % 10) + '0';
         pszsrcio[2] = ((adate[0] /   10) % 10) + '0';
         pszsrcio[3] = ((adate[0] /    1) % 10) + '0';
         pszsrcio[4] = ((adate[1] /   10) % 10) + '0';
         pszsrcio[5] = ((adate[1] /    1) % 10) + '0';
         pszsrcio[6] = ((adate[2] /   10) % 10) + '0';
         pszsrcio[7] = ((adate[2] /    1) % 10) + '0';
      }
      return 1;
   }

   return 0;
}

#ifdef SFK_W64
char *dataAsTraceQ(ushort *pAnyData)
{
   static char szBuf[300];

   char *pszBuf = szBuf;
   int  iMaxBuf = sizeof(szBuf);
 
   ushort *pSrcCur = (ushort *)pAnyData;
 
   char *pszDstCur = pszBuf;
   char *pszDstMax = pszBuf + iMaxBuf - 20;
 
   int i=0;
   for (; pSrcCur[i] != 0 && pszDstCur < pszDstMax; i++)
   {
      ushort uc = pSrcCur[i];
 
      if (uc < 0x100U && isprint((char)uc))
      {
         *pszDstCur++ = (char)uc;
         continue;
      }

      *pszDstCur++ = '?';
   }
 
   *pszDstCur = '\0';
 
   return pszBuf;
}

int execFixFile(ushort *ain, __wfinddata64_t *pdata)
{
   cs.files++;

   // derive all possibly needed data

   ushort apure[1024];  // for dewide, rewide
   char   szpure[1024]; // same in ascii
   int    adate[20];    // for setftime
   uchar  szcp[50];

   int isrc=0,idst=0,iuni=0;
   int msrc=1000,mdst=1000;
   mclear(adate);

   while (ain[isrc]!=0 && isrc<msrc && idst<mdst)
   {
      ushort uc = ain[isrc++];
      if (uc >= 0x100U)
      {
         iuni++;

         if (cs.dewide)
            continue;

         if (cs.rewide)
         {
            sprintf((char*)szcp, "{%04x}", uc);
            for (int i=0; szcp[i]!=0 && idst<mdst; i++)
            {
               apure[idst] = szcp[i];
               szpure[idst] = szcp[i];
               idst++;
            }
            continue;
         }

         apure[idst] = uc;
         szpure[idst] = '?';
         idst++;
         continue;
      }
      apure[idst] = uc;
      szpure[idst] = (char)uc;
      idst++;
   }
   apure[idst] = 0;
   szpure[idst] = '\0';

   /*
      scan for date, time
      -------------------
      01312015    MDY   201501   hms
      20150131    YMD   2015     hm

      01172015-0737  MDYhm
      20150131201501 YMDhms
   */
   bool bdate=0,btime=0,btsdiff=0;
   char *psz = strrchr(szpure, glblPathChar);
   if (!psz)
         psz = szpure;
   while (*psz)
   {
      if (!bdate) {
         if (matchdate(psz, "MDY", adate, cs.setndate)) {
            if (cs.setndate) {
               // write back possible reordered date
               int ioff = psz-szpure;
               for (int i=0; i<8; i++) {
                  if (apure[ioff+i] != ((ushort)psz[i]&0xFFU))
                     btsdiff = 1;
                  apure[ioff+i] = psz[i];
               }
            }
            bdate=1; 
            psz += 8;
            continue; 
         }
         if (matchdate(psz, "YMD", adate))
            { bdate=1; psz += 8; continue; }
         psz++;
         continue;
      }
      if (!btime) {
         if (matchdate(psz, "hms", adate))
            { btime=1; psz += 6; continue; }
         if (matchdate(psz, "hm", adate))
            { btime=1; psz += 4; continue; }
         psz++;
         continue;
      }
      break;
   }

   bool bNameDiffers = memcmp(ain, apure, (isrc+1)*2) ? 1 : 0;

   mytime_t now = mytime(NULL);
   struct tm *tm = 0;
   tm = mylocaltime(&now);
   tm->tm_isdst = -1;

   tm->tm_year = adate[0] - 1900;
   tm->tm_mon  = adate[1] - 1;
   tm->tm_mday = adate[2];
   tm->tm_hour = adate[3];
   tm->tm_min  = adate[4];
   tm->tm_sec  = adate[5];

   mytime_t nTime = mymktime(tm);
   num nTime2 = (num)nTime;

   bool bTimeDiffers = 0;

   if (nTime2 > 0 && nTime2 != pdata->time_write)
      bTimeDiffers = 1;

   // ignore, list, change?
   bool bChgName=0, bChgTime=0;
   
   if ((cs.dewide || cs.rewide) && bNameDiffers)
      bChgName = 1;

   if (cs.setftime && bTimeDiffers)
      bChgTime = 1;

   if (btsdiff)
      bChgName = 1;

   if (!bChgName && !bChgTime)
      return 0;

   printx("$FROM:<def> <time>%s<def> %s\n", 
      pdata->time_write > 0 ? timeAsString(pdata->time_write,1) : "[invalid time]",
      dataAsTraceQ(ain));

   // printx("$DIFF:<def> name=%d time=%d %llu %llu\n",bChgName,bChgTime,pdata->time_write,nTime2);

   cchar *ptimecol = (bChgTime != 0 && nTime2 > 0) ? "<rep>":"<time>";
   cchar *pnamecol = bChgName ? "<rep>":"";
   
   printx("$  TO:<def> %s%s<def> %s%s<def>\n", ptimecol, timeAsString(nTime2,1),
      pnamecol, dataAsTraceW(apure));

   if (idst < 1) {
      pwarn("... cannot change, name would be empty: %s\n", dataAsTraceW(ain));
      return 0;
   }

   if (cs.yes)
   {
      do
      {
         if (bChgName && _wrename(ain, apure))
         {
            perr("... rename failed: %s\n", dataAsTraceW(ain));
            break;
         }

         bool berr = 0;

         if (bChgTime != 0 && nTime2 > 0)
         {
            HANDLE hDst = CreateFileW(
               apure,
               FILE_WRITE_ATTRIBUTES,
               0,    // share
               0,    // security
               OPEN_EXISTING,
               FILE_ATTRIBUTE_NORMAL,
               0     // template file
               );
            if (hDst == INVALID_HANDLE_VALUE)
               { perr("... set filetime failed (1)\n"); break; }

            FILETIME nDstMTime, nDstCTime;
            FILETIME *pMTime=0, *pCTime=0;
 
            if (!makeWinFileTime(nTime2, nDstMTime))
               pMTime = &nDstMTime;
 
            // if (nClCTime > 0)
            // {
            //    if (!makeWinFileTime(nClCTime, nDstCTime))
            //       pCTime = &nDstCTime;
            // }

            if (pMTime || pCTime)
               if (!SetFileTime(hDst, pCTime, 0, pMTime))
                  { perr("... set filetime failed (2)\n"); berr=1; }

            CloseHandle(hDst);
         }

         if (!berr)
            cs.filesChg++;
      }
      while (0);
   }
   else
   {
      cs.filesChg++;
   }

   return 0;
}
#endif

int iGlblMovTotalTime = 0;
num nGlblMovFileSize  = 0;

int movtimetoms(int i)
{
   int ifrac = i % 100;
       i /= 100;
 
   int isec = i % 60;
       i /= 60;

   int imin = i % 60;
       i /= 60;

   int ihou = i;

   return (ihou * 100 + imin) * 100 + isec;
}

char *movtimetoa(int i, char *szBuf=0)
{
   static char szBuf2[100];
 
   if (!szBuf)
      szBuf = szBuf2;

   int ifrac = i % 100;
       i /= 100;
 
   int isec = i % 60;
       i /= 60;

   int imin = i % 60;
       i /= 60;

   int ihou = i;

   sprintf(szBuf, "%02u:%02u:%02u", ihou, imin, isec);

   return szBuf;
}

// RC 0: error
int atomovtime(char *psz, int ioptlen, bool bUseBytes)
{
   char szBuf[100];

   int ilen = ioptlen ? ioptlen : strlen(psz);

   if (ilen > sizeof(szBuf)-10)
      return 0+perr("buffer overflow");

   if (bUseBytes)
   {
      if (!nGlblMovFileSize)
         return 0+perr("missing filesize. specify as first parameter.");

      if (!iGlblMovTotalTime)
         return 0+perr("missing total time. specify as second parameter.");

      memcpy(szBuf, psz, ilen);
      szBuf[ilen] = '\0';

      num nBytePos = atonum(szBuf);

      int ires = nBytePos * iGlblMovTotalTime / nGlblMovFileSize;
 
      // printf("# time %08u = \"%s\" * %u / %s (inlen=%d)\n",
      //   ires, szBuf, iGlblMovTotalTime, numtoa(nGlblMovFileSize), ilen);

      if (cs.verbose > 1)
         printf("itime : %s = %s bytes\n", movtimetoa(ires, szBuf), numtoa(nBytePos));
 
      return ires;
   }

   // 3053     -> ((30 * 60) + 53) * 100
   // 305395   -> ((30 * 60) + 53) * 100 + 95

   char szHou[10]; mclear(szHou);
   char szMin[10]; mclear(szMin);
   char szSec[10]; mclear(szSec);
   char szFrc[10]; mclear(szFrc);

   if (strchr(psz, ':'))
   {
      // 30:53 and 01:30:53
      switch (ilen)
      {
         case 5:
            memcpy(szMin, psz, 2);
            memcpy(szSec, psz+3, 2);
            break;
 
         case 8:
            memcpy(szHou, psz, 2);
            memcpy(szMin, psz+3, 2);
            memcpy(szSec, psz+6, 2);
            break;
      }
   }
   else
   {
      // 3053 and 013053
      switch (ilen)
      {
         case 4:
            memcpy(szMin, psz, 2);
            memcpy(szSec, psz+2, 2);
            break;
 
         case 6:
            memcpy(szHou, psz, 2);
            memcpy(szMin, psz+2, 2);
            memcpy(szSec, psz+4, 2);
            break;
      }
   }

   int iTime =
         (atoi(szHou) * 3600) * 100
      +  (atoi(szMin) *   60) * 100
      +  (atoi(szSec)       ) * 100
      ;

   return iTime;
}

// RC 0: OK
int atomovrange(char *psz, num *pstart, num *pend)
{
   char *pszStart = psz;
   char *pszEnd   = strchr(psz, '-');
   if (!pszEnd)
      return 9+perr("wrong time range format: %s", psz);

   *pstart = atonum(pszStart);

   pszEnd++;

   if (!(*pend = atonum(pszEnd)))
      return 10+perr("wrong time range end: %s", psz);

   return 0;
}

// RC 0: OK
int atomovrange(char *psz, int *pstart, int *pend, bool bUseBytes)
{
   char *pszStart = psz;
   char *pszEnd   = strchr(psz, '-');
   if (!pszEnd)
      return 9+perr("wrong time range format: %s", psz);

   *pstart = atomovtime(pszStart, pszEnd-pszStart, bUseBytes);

   pszEnd++;

   if (!(*pend = atomovtime(pszEnd, 0, bUseBytes)))
      return 10+perr("wrong time range end: %s", psz);

   return 0;
}

int Media::findSeconds(num n)
{
   for (int i=0; i<iCmd; i++) {
      if (aBeg[i]==n && aBegSec[i]!=-1)
         return aBegSec[i];
      if (aEnd[i]==n && aEndSec[i]!=-1)
         return aEndSec[i];
   }
   return -1;
}

Media *Media::pClCurrent = 0;

Media::Media( )
{
   memset(this, 0, sizeof(*this));
}

void Media::reset( )
{
   iClInvalidFiles = 0;
   iClDoneFiles = 0;
   nClGlobalBytes = 0;
   bClHaveKeep = 0;
   bClKeepAll = 0;
   bClJoinOutput = 0;
   bClFixOutput = 0;
   iClDoneTS = 0;
   if (fClOut)
      perr("media: output file open on init\n");
   fClOut = 0;
   mclear(szClTmpOutFile);
   mclear(szClRecentOutFile);
   mclear(szClFinalFile);
   mclear(szClFixParms);
   clearCommands();
}

void Media::setFixParms(char *psz)
{
   if (!strcmp(psz, "pal-dvd") || !strcmp(psz, "dvd-pal"))
      strcopy(szClFixParms, "-target pal-dvd");
   else
   if (!strcmp(psz, "ntsc-dvd") || !strcmp(psz, "dvd-ntsc"))
      strcopy(szClFixParms, "-target ntsc-dvd");
   else
      strcopy(szClFixParms, psz);
}

void Media::closeOutput( )
{
   char szCmd[SFK_MAX_PATH+100];

   if (bClFixOutput)
   {
      snprintf(szCmd, sizeof(szCmd)-10,
         "ffmpeg -y -i \"%s\" %s -codec copy \"%s\"",
         szClTmpOutFile, szClFixParms, szClFinalFile);

      if (cs.sim && (cs.quiet<2))
         printx("$%s\n", szCmd);
      else
      if (!cs.sim && fClOut)
         printx("$%s\n", szCmd);
   }

   if (!fClOut)
      return;

   fclose(fClOut);
   fClOut = 0;
 
   if (bClFixOutput)
   {
      int iSubRC = system(szCmd);

      if (iSubRC > 0) {
         pwarn("ffmpeg possible error, RC=%d\n", iSubRC);
         pinf("use -keeptmp to keep temporary output file.\n");
      }
 
      if (!bClKeepTmp) {
         if (cs.verbose)
            printf("cleaning up temporary: %s\n", szClTmpOutFile);
         remove(szClTmpOutFile);
      }
   }

   if (cs.keeptime)
   {
      // this checks if it was really set
      clOutStat.writeStat(__LINE__);
   }
}

void Media::clearCommands( )
{
   bClHaveM3UCommands=0;
   mclear(aCmd);
   mclear(aBeg);
   mclear(aEnd);
   mclear(aBegSec);
   mclear(aEndSec);
   iFirstCmd=0;
   iCmd=0;
   for (int i=0; i<MAX_MOV_CMD; i++) {
      aBegSec[i]=-1;
      aEndSec[i]=-1;
   }
}

void Media::shutdown( )
{
   if (pszClM3UText) {
      delete [] pszClM3UText;
      pszClM3UText = 0;
   }
   if (!pClCurrent)
      return; // safety
   Media *pThis = pClCurrent;
   pClCurrent = 0;
   delete pThis;
   // no further processing
}

Media &Media::current( )
{
   if (!pClCurrent)
      pClCurrent = new Media();
   return *pClCurrent;
}

int Media::parseM3UFile(char *pszm3u)
{
   bClHaveM3UCommands=1;

   iFirstCmd=0;
   iCmd=0;

   if (pszClM3UText)
      delete [] pszClM3UText;
   if (!(pszClM3UText = loadFile(pszm3u, 0)))
      return 9;
   if (cs.verbose)
      printf("--- Using bookmarks: ---\n");
   /*
      #EXTVLCOPT:bookmarks={name=the.mpg0,bytes=56119296,time=93}
      #EXTVLCOPT:bookmarks={name=the.mpg0,bytes=56119296,time=93},{name=the.mpg1,bytes=1334441410,time=2354},...
      C:\video\the.mpg
   */
   bool  bNextIsFile = 0;
   char *pszMeta = 0;
   char *pszCur  = pszClM3UText;
   char *pszNext = 0;
   char *pszTime = 0;
   int iCurLine=0,iMetaLine=0,iFileLine=0;

   char  szTimeBuf1[100],szTimeBuf2[100],szTimeBuf3[100];

   // find last EXTVLCOPT entry
   while (pszCur && *pszCur) {
      pszNext = strchr(pszCur, '\n');
      if (pszNext)
         *pszNext++ = '\0';
      removeCRLF(pszCur);
      iCurLine++;
      if (strBegins(pszCur, "#EXTVLCOPT:bookmarks=")) {
         pszMeta = pszCur;
         bNextIsFile = 1;
         iMetaLine = iCurLine;
      }
      else
      if (bNextIsFile) {
         bNextIsFile = 0;
         pszClM3UFileEntry = pszCur;
         iFileLine = iCurLine;
      }
      pszCur = pszNext;
   }
   if (!pszMeta)
      return 9+perr("no #EXTVLCOPT:bookmarks= found within %s\n", pszm3u);
   if (!pszClM3UFileEntry || !strlen(pszClM3UFileEntry))
      return 9+perr("no input file filename found within %s\n", pszm3u);
   if (!fileExists(pszClM3UFileEntry)) {
      perr("input file not found: %s\n", pszClM3UFileEntry);
      pinf("from line %03u of M3U: %s\n", iFileLine, pszm3u);
      return 9;
   }
   if (cs.verbose) {
      printf("using: line %u, %.60s ...\n", iMetaLine, pszMeta);
      printf("using: line %u, %s\n", iFileLine, pszClM3UFileEntry);
   }
 
   // parse that entry
   pszCur = pszMeta;
   int istate2 = 0;
   int nTimeSec = 0;
   while (pszCur && *pszCur) {
      // convert bookmarks into keep commands
      if (!(pszNext = strstr(pszCur, "bytes=")))
         break;
      pszNext += 6;
      num nPos = atonum(pszNext);
      if (pszTime = strstr(pszNext, "time="))
         nTimeSec = atoi(pszTime+5);
      if (!(istate2++ & 1)){
         aCmd[iCmd] = SFKMOV_KEEP;
         aBeg[iCmd] = nPos;
         aBegSec[iCmd] = nTimeSec;
      } else {
         aEnd[iCmd] = nPos;
         aEndSec[iCmd] = nTimeSec;
         if (cs.verbose)
            printf("%s : %s-%s (%s-%s)\n",
               aCmd[iCmd] == SFKMOV_KEEP ? "keep":"skip",
               numtoa(aBeg[iCmd],10,szTimeBuf1), numtoa(aEnd[iCmd],10,szTimeBuf2),
               movtimetoa(aBegSec[iCmd]*100,szTimeBuf3),movtimetoa(aEndSec[iCmd]*100));
         iCmd++;
      }
      pszCur = pszNext;
   }
 
   // plausi check
   if (istate2 & 1)
      return 9+perr("uneven number of boomarks found in %s\n", pszm3u);
   if (cs.verbose) {
      printf("------------------------\n");
   }

   return 0;
}

int Media::analyze(uchar *pbuf, int isize)
{
   return 0;
}

int Media::renderTempName(char *pszFinal)
{
   // from: pszFinal == output.mpg
   // to  : pszFinal == output-tmp.mpg
   //       szClFinalFile  == output.mpg
   strcopy(szClFinalFile, pszFinal);

   char szNameBuf[SFK_MAX_PATH+10];
   strcopy(szNameBuf, pszFinal);

   char *pszBase = szNameBuf;
   char *pszExt  = strrchr(szNameBuf, '.');

   if (!pszExt)
      return 9+perr("missing file extension on output name: %s\n", pszFinal);

   if (SFTmpFile::tmpDirWasSet()) {
      // NO *pszExt del here, using full ".ext"
      SFTmpFile otmp(pszExt, 1);
      char *psz = otmp.name();
      strcopy(szClTmpOutFile, psz);
   } else {
      *pszExt++ = '\0';
      snprintf(szClTmpOutFile, sizeof(szClTmpOutFile)-10,
         "%s-tmp.%s", pszBase, pszExt);
   }

   if (bClShowTmp)
      printf("using temporary: %s\n", szClTmpOutFile);

   return 0;
}

int Media::processMediaFile(char *pszSrc, char *pszOutFile)
{
   if (!bClKeepAll && !iCmd) {
      pwarn("no commands given, skipping: %s\n", pszSrc);
      return 5;
   }

   const char *pszind = bClHaveM3UCommands ? "  ":"";
   if (cs.quiet >= 2) pszind = "";

   printf("input: %s%s\n", pszind, pszSrc);

   if (bClFixOutput)
   do
   {
      if (bClJoinOutput && szClTmpOutFile[0]) {
         pszOutFile = szClTmpOutFile;
         break; // names already rendered
      }

      if (!pszOutFile)
         return 9+perr("missing output filename");

      if (renderTempName(pszOutFile))
         return 9;
      // szClOut   is now TEMPORARY name
      // szClFinal is the final filename

      pszOutFile = szClTmpOutFile;
   }
   while (0);

   if (pszOutFile)
      strcopy(szClRecentOutFile, pszOutFile);

   if ((cs.quiet<2) && pszOutFile)
      printf("out  : %s%s%s\n", pszind, pszOutFile, bClJoinOutput ? " (joined)":"");

   FileStat ofsSrc;
   if (ofsSrc.readFrom(pszSrc, 0, 1))
      return 9+perr("no such input file: %s", pszSrc);

   num nFileSize = ofsSrc.getSize();

   if (nFileSize <= 0)
      return 9+perr("empty input file: %s", pszSrc);

   if (!cs.sim && !pszOutFile)
      return 9+perr("missing output filename");

   int iRC = 0;

   num nTotalTime = nFileSize;

   FILE *fin = fopen(pszSrc, "rb");
   if (!fin) return 9+perr("unable to open input file: %s\n", pszSrc);

   if (!cs.sim) {
      if (!fClOut) {
         fClOut = fopen(pszOutFile, "wb");
         if (!fClOut) { fclose(fin); return 9+perr("unable to write: %s\n", pszOutFile); }
         clOutStat.copyFrom(ofsSrc);
         clOutStat.setFilename(pszOutFile);
      }
   }

   num nTotalBytes=0;

   // set start state
   int iCopyState = 1; // keep
   if (   aCmd[iFirstCmd] == SFKMOV_KEEP
       && aBeg[iFirstCmd] > 0
      )
       iCopyState = 0; // keep, but not from start
   else
   if (   aCmd[iFirstCmd] == SFKMOV_CUT
       && aBeg[iFirstCmd] == 0
      )
       iCopyState = 0; // cut from start

   // define current switch point
   num nCurBeg = 0;
   num nCurEnd = nTotalTime;
 
   char szBuf1[100],szBuf2[100],szBuf3[100];
   char szInfoBuf[100];
 
   num tLastInfo = 0;

   while (iRC==0 && nCurBeg<nTotalTime)
   {
      // BEGIN state of current cmd was set already.
      // find next switch point.
      int iNextCmd   = -1;
      int bNextIsBeg =  0;
      num iNextTime  = nTotalTime;
      for (int iTmp=iFirstCmd; aCmd[iTmp]; iTmp++)
      {
         num ibeg = aBeg[iTmp];
         num iend = aEnd[iTmp];

         if (ibeg > nCurBeg && ibeg < iNextTime) {
            iNextTime=ibeg; iNextCmd=iTmp; bNextIsBeg=1;
            // printf("curmin: #%d with beg %s\n", iTmp, numtoa(ibeg, 10));
            continue;
         }

         if (iend > nCurBeg && iend < iNextTime) {
            iNextTime=iend; iNextCmd=iTmp; bNextIsBeg=0;
            // printf("curmin: #%d with end %s\n", iTmp, numtoa(iend, 10));
            continue;
         }
      }

      // calc current range to keep or cut
      if (iNextCmd >= 0)
         nCurEnd = iNextTime;
      else
         nCurEnd = nTotalTime;

      if (!cs.quiet)
      {
         info.clear();

         int iBegSec = findSeconds(nCurBeg);
         int iEndSec = findSeconds(nCurEnd);

         if (iBegSec >= 0 && iEndSec >= 0)
            sprintf(szInfoBuf, "%s-%s (%s-%s)",
               numtoa(nCurBeg, 10, szBuf1), numtoa(nCurEnd, 10, szBuf2),
               movtimetoa(iBegSec*100,szBuf3),movtimetoa(iEndSec*100));
         else
            sprintf(szInfoBuf, "%s-%s",
               numtoa(nCurBeg, 10, szBuf1), numtoa(nCurEnd, 10, szBuf2));

         if (iCopyState)
            printx("$copy : %s\n", szInfoBuf);
         else
            printx("#skip : %s\n", szInfoBuf);
      }

      // apply current range
      while (1)
      {
         info.setProgress(nTotalTime, nCurBeg, "bytes");
         info.setStatus(iCopyState ? "copy":"skip", pszOutFile ? pszOutFile : pszSrc);

         if (iCopyState)
         {
            // copy part
            int iMaxRead = sizeof(abBuf)-100;
 
            if (nCurEnd < nCurBeg + iMaxRead)
               iMaxRead = (int)(nCurEnd - nCurBeg);
 
            if (iMaxRead <= 0) {
               perr("invalid section length: %d at %s\n", iMaxRead, numtoa(nCurBeg, 10));
               iRC = 9;
               break;
            }
 
            int nread = (cs.sim && !bClScan) ? iMaxRead : fread(abBuf, 1, iMaxRead, fin);
 
            if (nread <= 0)
            {
               info.print("file end reached.\n");
               iRC = 9;
               break; // EOF on input
            }

            if (bClScan)
               analyze(abBuf, nread);
 
            if (!cs.sim && fClOut) {
               int nwrite = myfwrite(abBuf, nread, fClOut);
               if (nwrite != nread) {
                  esys("fwrite", "error while writing: %s   \n", pszOutFile);
                  iRC = 9;
                  break;
               }
            }

            nTotalBytes += nread;
            nClGlobalBytes += nread;
            nCurBeg += nread;
         }
         else
         {
            // skip part
            if (myfseek(fin, nCurEnd, SEEK_SET)) {
               iRC = 9;
               break;
            }
            nCurBeg = nCurEnd;
         }

         if (nCurBeg >= nCurEnd)
            break;

      }  // endwhile part read loop

      if (iRC > 0)
         break;

      // switch copy state
      if (iNextCmd >= 0)
      {
         if (aCmd[iNextCmd]==SFKMOV_KEEP && bNextIsBeg) {
            iCopyState = 1; // start of keep
            if (cs.verbose > 2)
               info.print("part : kb.KEEP from %s\n", numtoa(nCurBeg, 10));
         }
         else
         if (aCmd[iNextCmd]==SFKMOV_KEEP && !bNextIsBeg) {
            iCopyState = 0; // end of keep
            if (cs.verbose > 2)
               info.print("part : ke.SKIP from %s\n", numtoa(nCurBeg, 10));
         }
         else
         if (aCmd[iNextCmd]==SFKMOV_CUT && bNextIsBeg) {
            iCopyState = 0; // start of cut
            if (cs.verbose > 2)
               info.print("part : cb.SKIP from %s\n", numtoa(nCurBeg, 10));
         }
         else
         if (aCmd[iNextCmd]==SFKMOV_CUT && !bNextIsBeg) {
            iCopyState = 1; // end of cut
            if (cs.verbose > 2)
               info.print("part : ce.KEEP from %s\n", numtoa(nCurBeg, 10));
         }
      }

   }  // endwhile overall I/O loop

   info.clear();

   if (!bClJoinOutput)
      closeOutput();
   // else called later

   fclose(fin);

   if (!iRC && cs.verbose) {
      if (cs.sim) {
         printf("would copy %d mb (%s bytes).\n",
            (int)(nTotalBytes/1000000), numtoa(nTotalBytes));
      } else {
         printf("copied %s bytes.\n", numtoa(nTotalBytes));
      }
   }

   if (!iRC && szClMoveSrcOutDir[0]) {
      char *pszSrcRelName = strrchr(pszSrc, glblPathChar);
      if (pszSrcRelName)
         pszSrcRelName++;
      else
         pszSrcRelName = pszSrc;
      joinPath(szClMoveSrcOutFile, sizeof(szClMoveSrcOutDir),
               szClMoveSrcOutDir, pszSrcRelName);
      const char *pszInfo = "";
      if (fileExists(szClMoveSrcOutFile)) {
         if (cs.sim) {
            pszInfo = " (output file exists)";
         } else {
            if (cs.force) {
               remove(szClMoveSrcOutFile);
               pszInfo = " (overwritten)";
            } else {
               pszInfo = " "; // dummy for following error
            }
         }
      }
      if (!cs.quiet)
         printx("$move : %s -> %s%s\n", pszSrc, szClMoveSrcOutFile, pszInfo);
      if (!cs.sim) {
         if (rename(pszSrc, szClMoveSrcOutFile)) {
            perr("cannot move %s -> %s\n", pszSrc, szClMoveSrcOutFile);
            static bool btold=0;
            if (!btold) {
               btold=1;
               if (pszInfo[0])
                  pinf("add option -force to overwrite existing output file.\n");
               else
                  pinf("output dir must be on same file system as input file.\n");
            }
         }
      }
   }

   iClDoneFiles++;

   return iRC;
}

int execMedia(char *pszSrc, char *pszOutFile)
{
   Media &m = Media::current();
 
   int iRC = 0;

   if (m.bClHaveM3UCommands) {
      // these are valid per single file only
      m.clearCommands();
   }
 
   if (endsWithExt(pszSrc, str(".m3u"))) {
      if (!m.bClHaveKeep) {
         pwarn("missing \"-keepbook\" command, skipping: %s\n", pszSrc);
         return 5;
      }
      if (cs.quiet<2)
         printf("using: %s\n", pszSrc);
      if (iRC = m.parseM3UFile(pszSrc)) {
         // a single M3U batch is invalid,
         // but overall processing may keep running
         m.iClInvalidFiles++;
         return iRC;
      }
      if (!(pszSrc = m.pszClM3UFileEntry)) {
         m.iClInvalidFiles++;
         return 9; // safety
      }
   }

   if (iRC = m.processMediaFile(pszSrc, pszOutFile))
      m.iClInvalidFiles++;

   return iRC;
}

int ispuretext(char *psz, char ccurdelim, char coutdelim, char cquote)
{
   while (*psz)
   {
      char c = *psz++;
      if (c == ccurdelim)
         return 1;
      if (c == coutdelim)
         return 0;
      if (c == cquote)
         return 0;
   }

   return 1;
}

int ispurenum(char *psz, char ccurdelim, char coutdelim)
{
   /*
      +123.456e+10
      -234.387e3
      -234,387e-3
   */

   if (*psz=='+' || *psz=='-')
      psz++;

   int istate = 1;

   while (*psz != 0 && *psz != ccurdelim)
   {
      char c = *psz++;

      if (c == coutdelim)
         return 0;

      switch (istate)
      {
         case 1: // expect just digit
            if (isdigit(c)) {
               istate = 2;
               continue;
            }
            return 0;

         case 2: // expect digit . , e
            if (isdigit(c))
               continue;
            if (c == '.' || c == ',')
               continue;
            if (tolower(c) == 'e') {
               istate = 3;
               continue;
            }
            return 0;

         case 3: // after e: expect + - digit
            if (c == '+' || c == '-') {
               istate = 4;
               continue;
            }
         case 4:
            if (isdigit(c))
               continue;
            return 0;
      }
   }

   return 1;
}

int csvToTab(char *psrc, char *pdst, int imaxdst)
{
   char cdelim     = cs.cinsep;
   char ctab       = cs.coutsep;
   char cquote     = cs.cquote;
   char ctabescape = cs.coutsepesc;

   /*
      5324,John Smith,Los Angeles
      7936,"James Foo, Dr.",Atlanta
      3297,"Jack ""Goo"" Bar",San Diego
      5239,,Nowhere
      5240,Empty{TAB}City,
   */

   char *pSrcCur = psrc;
   char *pSrcMax = psrc + strlen(psrc);
   char *pDstCur = pdst;
   char *pDstMax = pdst + (imaxdst - 20);

   int istate   = 0;
   int binquote = 0;

   while (pSrcCur < pSrcMax && pDstCur < pDstMax)
   {
      char c  = *pSrcCur++;
      char c2 = *pSrcCur;  // NULL if at end

      switch (istate)
      {
         case 0:  // expect quote, start of data, delimiter
            if (c == cquote) {
               binquote = 1;
               istate = 1;
               continue;
            }
            if (c == cdelim) {
               *pDstCur++ = ctab;
               continue;
            }
            istate = 1;
            // fall through

         case 1:  // expect data byte, escaped quote, delimiter, EOR
            if (c == cquote && c2 == cquote) {
               *pDstCur++ = cquote;
               pSrcCur++;
               continue;
            }
            if (binquote == 0 && c == cdelim) {
               *pDstCur++ = ctab;
               istate = 0;
               continue;
            }
            if (binquote == 1 && c == cquote) {
               // delimiter or EOR must be next
               binquote = 0;
               continue;
            }
            if (c == ctab) {
               // remove tabs from input
               c = ctabescape;
            }
            *pDstCur++ = c;
            continue;
      }
   }

   if (pSrcCur >= pSrcMax)
   {
      // end of record checks
      if (pDstCur >= pDstMax)
         return 10;

      switch (istate)
      {
         case 0:  // expect quote, data, delimiter
            *pDstCur++ = '\0';
            return 0;

         case 1:  // expect data byte, escaped quote, delimiter, EOR
            if (binquote)
               return 11;
            *pDstCur++ = '\0';
            return 0;
      }
   }

   return 12;
}

int tabToCsv(char *psrc, char *pdst, int imaxdst)
{
   char ctab       = cs.cinsep;
   char cdelim     = cs.coutsep;
   char cquote     = cs.cquote;
   bool bquotetext = cs.quotetext;
   bool bfullquote = cs.quoteall;

   char *pSrcCur = psrc;
   char *pSrcMax = psrc + strlen(psrc);
   char *pDstCur = pdst;
   char *pDstMax = pdst + (imaxdst - 20);

   int istate   = 0;
   int binquote = 0;
   int boutquoteopen = 0;

   while (pSrcCur < pSrcMax && pDstCur < pDstMax)
   {
      char c  = *pSrcCur++;
      char c2 = *pSrcCur;  // NULL if at end

      switch (istate)
      {
         case 0:  // expect start of data, tab, quote-to-escape
            if (c == ctab) {
               if (bfullquote) {
                  *pDstCur++ = cquote;
                  *pDstCur++ = cquote;
               }
               *pDstCur++ = cdelim;
               continue;
            }
            // start of data, quote-to-escape
            if (   bfullquote == 1
                || (bquotetext && !ispurenum(pSrcCur-1, ctab, cdelim))
                || (!ispuretext(pSrcCur-1, ctab, cdelim, cquote))
               )
            {
               *pDstCur++ = cquote;
               boutquoteopen = 1;
            }
            istate = 1;
            // fall through

         case 1:  // expect tab, data byte, quote-to-escape
            if (c == ctab) {
               if (boutquoteopen)
                  *pDstCur++ = cquote;
               *pDstCur++ = cdelim;
               boutquoteopen = 0;
               istate = 0;
               // tab at end of record?
               if (pSrcCur >= pSrcMax) {
                  // then it means a trailing empty field
                  if (bfullquote) {
                     *pDstCur++ = cquote;
                     *pDstCur++ = cquote;
                  }
                  // but without extra separator
               }
               continue;
            }
            if (c == cquote) {
               // escape quote
               *pDstCur++ = cquote;
               *pDstCur++ = cquote;
               continue;
            }
            // continue data
            *pDstCur++ = c;
            continue;
      }
   }

   if (pSrcCur >= pSrcMax)
   {
      // end of record checks
      if (pDstCur >= pDstMax)
         return 10;

      switch (istate)
      {
         case 0:  // expect start of data, tab
            *pDstCur++ = '\0';
            return 0;

         case 1:  // expect tab, data byte
            if (boutquoteopen)
               *pDstCur++ = cquote;
            *pDstCur++ = '\0';
            return 0;
      }
   }

   return 12;
}

int execCsvConv(bool bToCsv, Coi *pcoi, FILE *fin, StringPipe *pInData, int nMaxLines, char *pszOutFile)
{
   FILE *fout = 0;

   // INPUT FILE CLOSES AUTOMATICALLY.
   FileCloser fcin(pcoi); // does nothing if pcoi == NULL

   if (pcoi)
   {
      if (pcoi->open("rb"))
         return 9+perr("cannot read: %s %s\n", pcoi->name(),pcoi->lasterr());
   }

   if (pszOutFile)
      if (!(fout = fopen(pszOutFile, "w")))
         return 10+perr("cannot write: %s\n", pszOutFile);

   int nMaxLineLen = sizeof(szLineBuf)-10;
   int ncheckcnt   = 0;
   int lRC         = 0;
   int nLine       = 0;

   myfgets_init();
   while (1)
   {
      // --- get next line to linebuf ---

      memset(szAttrBuf, ' ', MAX_LINE_LEN-10);
      szAttrBuf[MAX_LINE_LEN-10] = '\0';

      int nRead = 0;

      if (pInData) {
         if (pInData->eod())
            break;
         char *pattr = 0;
         char *psz = pInData->read(&pattr);
         mystrcopy(szLineBuf, psz, MAX_LINE_LEN);
         strcat(szLineBuf, "\n"); // force LF
         int nlen = strlen(szLineBuf);
         if (pattr) {
            mystrcopy(szAttrBuf, pattr, MAX_LINE_LEN);
            if ((int)strlen(szAttrBuf) < nlen-1) {
               memset(szAttrBuf, ' ', nlen);
               szAttrBuf[nlen] = '\0';
            }
         }
         nRead = nlen;
      } else if (pcoi) { // native or virtual file
         nRead = pcoi->readLine(szLineBuf, nMaxLineLen);
      } else if (fin)  { // probably stdin
         nRead = myfgets(szLineBuf, nMaxLineLen, fin, 0, szAttrBuf);
      }
      if (!nRead)
         break; // EOD

      if ((++ncheckcnt > 100000) && ((ncheckcnt & 65535)==0))
         if (userInterrupt())   // costs a bit of time
            {  lRC=9; break;  } // stop by escape

      szLineBuf[nRead] = '\0';
      szAttrBuf[nRead] = '\0';
      nLine++;
      removeCRLF(szLineBuf);

      // --- process line ---
      int isubrc = 0;
      if (bToCsv == 0)
         isubrc = csvToTab(szLineBuf, szLineBuf2, MAX_LINE_LEN);
      else
         isubrc = tabToCsv(szLineBuf, szLineBuf2, MAX_LINE_LEN);
      if (isubrc) {
         if (!cs.nowarn)
            pwarn("input line %d wrong format (%d): %s", nLine, isubrc, szLineBuf);
         snprintf(szLineBuf2, MAX_LINE_LEN, "[warning:%d]\t%s", isubrc, szLineBuf);
      }

      // -- write to output ---
      if (fout)
         fprintf(fout, "%s\n", szLineBuf2);
      else
      if (chain.coldata)
         chain.addLine(szLineBuf2, str(""));
      else
         printf("%s\n", szLineBuf2);
   }

   if (fout)
      fclose(fout);

   return lRC;
}

UDPCore::UDPCore(int iMaxWait)
{
   memset(this, 0, sizeof(*this));

   for (int i=0; i<nClCon; i++)
      aClCon[i].sock = INVALID_SOCKET;

   nClMaxSock = 0;

   bpure = 1;
   
   tstart = getCurrentTime();

   imaxwait = iMaxWait;
}

UDPCore::~UDPCore( )
{
   if (nClCon > 0)
      perr("missing shutdown() on udpcore %p",this);

   shutdown();
}

int UDPCore::lastError( )
{
   #ifdef _WIN32
   return WSAGetLastError();
   #else
   return errno;
   #endif
}

void UDPCore::shutdown( )
{
   for (int i=0; i<nClCon; i++) 
      if (aClCon[i].sock != INVALID_SOCKET)
         closesocket(aClCon[i].sock);

   memset(this, 0, sizeof(*this));

   for (int i=0; i<nClCon; i++)
      aClCon[i].sock = INVALID_SOCKET;

   nClMaxSock = 0;
}

int UDPCore::makeSocket(int iMode, char *pszHost, int nportin)
{__
   if (nClCon >= MAX_UDP_CON-2)
      return 9+perr("too many sockets, cannot connect more");

   mclear(aClCon[nClCon]);
   aClCon[nClCon].sock = INVALID_SOCKET;

   uint nPort = nportin;

   struct hostent *pTarget;
   struct sockaddr_in oaddr;
   SOCKET hSock = 0;

   switch (iMode)
   {
      case 1: hSock = socket(AF_INET, SOCK_DGRAM, 0); break;
      case 2: hSock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP); break;
   }

   if (hSock == INVALID_SOCKET)
      return 9+perr("cannot create socket, probably missing admin rights");

   UDPCon *pcon = &aClCon[nClCon];

   if ((pTarget = sfkhostbyname(pszHost)) == NULL)
   {
      closesocket(hSock);
      return 9+perr("cannot get host\n");
   }

   char szBuf[100];

   strcopy(pcon->host, pszHost);
   pcon->port = nportin;

   pcon->sock = hSock;

   memcpy(&pcon->addr.sin_addr.s_addr, pTarget->h_addr, pTarget->h_length);
   pcon->addr.sin_family = AF_INET;
   pcon->addr.sin_port = htons((unsigned short)nPort);

   char *pstr = ipAsString((struct sockaddr_in *)&pcon->addr,szBuf,sizeof(szBuf),bpure?0:2);
   strcopy(pcon->ipstr, pstr);

   FD_SET(pcon->sock, &clReadSet);

   nClMaxSock = mymax(nClMaxSock, pcon->sock);

   nClCon++;

   return 0;
}

int UDPCore::selectInput(int *pIndex, int iMaxWaitMSec)
{
   struct timeval tv;
	tv.tv_sec  = iMaxWaitMSec/1000;
	tv.tv_usec = (iMaxWaitMSec < 1000) ? iMaxWaitMSec * 1000 : 0;

   memcpy(&clSetCopy, &clReadSet, sizeof(fd_set));

   int nrc = select(nClMaxSock+1,
      &clSetCopy,
      NULL,
      NULL,
      &tv
      );

   if (nrc == 0)
      return 1; // nothing received

   if (nrc < 0)
      return 9+perr("select() failed, %d",lastError());

   for (int i=0; i<nClCon; i++)
   {
      if (FD_ISSET(aClCon[i].sock, &clSetCopy)) 
      {
         *pIndex = i;
         return 0;
      }
   }

   return 1; // nothing to do
}

void printSamp(int nlang, char *pszOutFile, char *pszClassName, int bWriteFile)
{
      /*
         sfk fromclip +filt -srep "_\\_\\\\_" -srep "_\q_\\\q_" -sform "          \q$col1\\n\q"
         sfk filt src -rep "_%_%%_" -srep "_\\_\\\\_" -srep "_\q_\\\q_" -sform "         \q$col1\\n\q" +toclip
      */

      switch (nlang) {
      case 1:
      chain.print(
          "import java.io.*;\n"
          "\n"
          "public class %s\n"
          "{\n"
          "    static void log(String s) { System.out.println(\"main: \"+s); }\n"
          "\n"
          "    public static void main(String args[]) throws Throwable\n"
          "    {\n"
          "        if (args.length < 2)\n"
          "            { log(\"supply in- and output filename.\"); return; }\n"
          "\n"
          "        // copy or convert text file\n"
          "        BufferedReader rin = new BufferedReader(\n"
          "            new InputStreamReader(\n"
          "                new FileInputStream(args[0]), \"ISO-8859-1\"\n"
          "                // or US-ASCII,UTF-8,UTF-16BE,UTF-16LE,UTF-16\n"
          "                ));\n"
          "\n"
          "        PrintWriter pout = new PrintWriter(\n"
          "            new OutputStreamWriter(\n"
          "                new FileOutputStream(args[1]), \"ISO-8859-1\"\n"
          "                ));\n"
          "\n"
          "        while (true) {\n"
          "            String sline = rin.readLine();\n"
          "            if (sline == null) break; // EOD\n"
          "            log(\"copying line: \"+sline);\n"
          "            pout.println(sline);\n"
          "        }\n"
          "\n"
          "        pout.close();\n"
          "        rin.close();\n"
          "    }\n"
          "};\n",
          pszClassName
        );
         break;

      case 2:
      chain.print(
         "#include <stdio.h>\n"
         "#include <string.h>\n"
         "#include <stdarg.h>\n"
         "\n"
         "// print error message with variable parameters.\n"
         "int perr(const char *pszFormat, ...) {\n"
         "   va_list argList;\n"
         "   va_start(argList, pszFormat);\n"
         "   char szBuf[1024];\n"
         "   ::vsprintf(szBuf, pszFormat, argList);\n"
         "   fprintf(stderr, \"error: %%s\", szBuf);\n"
         "   return 0;\n"
         "}\n"
         "\n"
         "// copy text lines from one file into another.\n"
         "int main(int argc, char *argv[]) \n"
         "{\n"
         "  if (argc < 2) return 9+perr(\"specify input and output filename.\\n\");\n"
         "\n"
         "  char *pszInFile  = argv[1];\n"
         "  char *pszOutFile = argv[2];\n"
         "\n"
         "  FILE *fin  = fopen(pszInFile , \"rb\"); if (!fin ) return 9+perr(\"cannot read %%s\\n\" , pszInFile);\n"
         "  FILE *fout = fopen(pszOutFile, \"wb\"); if (!fout) return 9+perr(\"cannot write %%s\\n\", pszOutFile);\n"
         "\n"
         "  char szBuf[1024];\n"
         "  memset(szBuf, 0, sizeof(szBuf));\n"
         "  while (fgets(szBuf, sizeof(szBuf)-10, fin)) \n"
         "  {\n"
         "     char *psz = strchr(szBuf, '\\r'); if (psz) *psz = '\\0'; // strip cr\n"
         "           psz = strchr(szBuf, '\\n'); if (psz) *psz = '\\0'; // strip lf\n"
         "     printf(\"line: \\\"%%s\\\"\\n\", szBuf);\n"
         "     strcat(szBuf, \"\\n\");\n"
         "     int nlen = strlen(szBuf);\n"
         "     if (fwrite(szBuf, 1, nlen, fout) != nlen)\n"
         "        return 9+perr(\"failed to fully write %%s\\n\", pszOutFile);\n"
         "  }\n"
         "\n"
         "  fclose(fout);\n"
         "  fclose(fin);\n"
         "\n"
         "  return 0;\n"
         "}\n"
        );
         break;

      case 3:
      chain.print(
         "@rem windows command shell batch example\n"
         "@echo off\n"
         "IF \"%%1\"==\"\" GOTO xerr01\n"
         "echo \"parameter is %%1\"\n"
         "GOTO xdone\n"
         "\n"
         ":xerr01\n"
         "echo \"please supply a parameter.\"\n"
         "echo \"example: mybat parm123\"\n"
         "GOTO xdone\n"
         "\n"
         ":xdone\n"
         );
         break;

      case 4:
      chain.print(
         "#!/bin/bash\n"
         "\n"
         "function pmsg {\n"
         "   # uses a local variable mystr\n"
         "   local mystr=\"info: $1\"\n"
         "   echo $mystr\n"
         "}\n"
         "\n"
         "myparm1=\"$1 and $2\"       # no blanks around \"=\"\n"
         "\n"
         "if [ \"$2\" = \"\" ]; then    # requires all blanks\n"
         "   pmsg \"please supply two parameters.\"\n"
         "else\n"
         "   pmsg \"you supplied \\\"$myparm1\\\".\"\n"
         "\n"
         "   #  < -lt   > -gt   <= -le   >= -ge   == -eq   != -ne\n"
         "   i=1\n"
         "   while [ $i -le 5 ]; do # not \"$i < 5\"\n"
         "      echo counting: $i   # quotes are optional\n"
         "      let i+=1            # not \"i += 1\" or \"$i+=1\"\n"
         "   done\n"
         "fi\n"
         );
         break;

      case 5:
      chain.print(
         "sfk select testfiles .txt .hpp .cpp\n"
         "\n"
         "   // find words supplied by user.\n"
         "   // note that %%1 is the same as $1.\n"
         "   +find\n"
         "      %%1 %%2 %%3 $4 $5 $6\n"
         "\n"
         "   // process files containing hits\n"
         "   +run -quiet \"sfk echo \\\"Found hit in: [green]$file[def]\\\"\" -yes\n"
         "\n"
         "   // run the script by:\n"
         "   // \"sfk script %s pattern1 [pattern2 ...]\"\n",
         pszOutFile ? pszOutFile : "thisfile"
         );
         break;

      case 6:
      chain.print(
         "@echo off\n"
         "sfk script %s -from begin %%*\n"
         "GOTO xend\n"
         "\n"
         "sfk label begin\n"
         "\n"
         "   // select text files from testfiles:\n"
         "   +select testfiles .txt\n"
         "\n"
         "   // filter words foo, and user-supplied:\n"
         "   +ffilter\n"
         "      -+foo\n"
         "      %%1\n"
         "      %%2\n"
         "\n"
         "   // display results in depeche view:\n"
         "   +view\n"
         "\n"
         "   // end of sfk script:\n"
         "   +end\n"
         "\n"
         ":xend\n",
         pszOutFile ? pszOutFile : "thisfile.bat"
         );
         break;

      case 7:
      chain.print(
         "#!/bin/bash\n"
         "sfk script %s -from begin $@\n"
         "function skip_block\n"
         "{\n"
         "sfk label begin\n"
         "\n"
         "   // select text files from testfiles:\n"
         "   +select testfiles .txt\n"
         "\n"
         "#  // filter words foo, and user-supplied.\n"
         "#  // note that # lines are skipped by bash,\n"
         "#  // but not by sfk.\n"
         "#  +ffilter\n"
         "#     -+foo\n"
         "#     $1\n"
         "#     $2\n"
         "\n"
         "   // display results in depeche view:\n"
         "   +view\n"
         "\n"
         "   // end of sfk script:\n"
         "   +end\n"
         "}\n",
         pszOutFile ? pszOutFile : "thisfile.bat"
         );
         break;

      case 8:
      chain.print(
         "\n"
         "// Example source code for image file conversion\n"
         "// and simple image processing with Java.\n"
         "// Requires SUN's Java Advanced Imaging I/O Tools.\n"
         "// Usage: java imtool input.png outbase\n"
         "// Creates: outbase-jpg.jpg and further.\n"
         "\n"
         "import java.io.*;\n"
         "import java.awt.image.*;\n"
         "import javax.imageio.*;\n"
         "import com.sun.image.codec.jpeg.*;\n"
         "\n"
         "public class %s\n"
         "{\n"
         "    static void log(String s) { System.out.println(s); }\n"
         "\n"
         "    public static void main(String args[]) throws Throwable\n"
         "    {\n"
         "        if (args.length < 2)\n"
         "            throw new Exception(\"specify input filename and output basename.\");\n"
         "\n"
         "        String src  = args[0];\n"
         "        String dst1 = args[1]+\"-jpg.jpg\";\n"
         "        String dst2 = args[1]+\"-png.png\";\n"
         "        String dst3 = args[1]+\"-green.jpg\";\n"
         "        String dst4 = args[1]+\"-green.png\";\n"
         "\n"
         "        // load a PNG image, with or without transparency (alpha channel).\n"
         "        BufferedImage buf = ImageIO.read(new File(src));\n"
         "        int nwidth  = buf.getWidth();\n"
         "        int nheight = buf.getHeight();\n"
         "        log(\"width = \"+nwidth+\" pixels, height = \"+nheight);\n"
         "\n"
         "        // trivial file conversion: save as a JPEG or PNG image.\n"
         "        // JPEG will work only if input contained no transparency.\n"
         "        ImageIO.write(buf, \"jpg\", new File(dst1)); log(dst1);\n"
         "        ImageIO.write(buf, \"png\", new File(dst2)); log(dst2);\n"
         "\n"
         "        // image processing: turn all transparent pixels into green.\n"
         "\n"
         "        // 1. get access to main pixels, and transparency.\n"
         "        WritableRaster rmain = buf.getRaster();\n"
         "        WritableRaster rtran = buf.getAlphaRaster();\n"
         "\n"
         "        // 2. create a memory image to write to, WITHOUT transparency.\n"
         "        BufferedImage bout  = new BufferedImage(nwidth,nheight,BufferedImage.TYPE_INT_RGB);\n"
         "        WritableRaster rout = bout.getRaster();\n"
         "\n"
         "        int apixm[] = new int[4]; // main  pixel\n"
         "        int apixt[] = new int[4]; // trans pixel\n"
         "        int apixr[] = new int[4]; // repl. color\n"
         "\n"
         "        apixt[0] = 0xFF; // default is non-transparent\n"
         "        apixr[1] = 0xFF; // set replacement color to green\n"
         "\n"
         "        for (int y=0; y<nheight; y++)\n"
         "         for (int x=0; x<nwidth; x++) \n"
         "         {\n"
         "            rmain.getPixel(x,y,apixm);\n"
         "            if (rtran != null)\n"
         "                rtran.getPixel(x,y,apixt);\n"
         "            if (apixt[0] == 0x00)\n"
         "                // pixel is fully transparent: set to green\n"
         "                rout.setPixel(x,y,apixr);\n"
         "            else\n"
         "                // else copy through, do not change.\n"
         "                rout.setPixel(x,y,apixm);\n"
         "         }\n"
         "\n"
         "        // save memory image as JPEG, with control of quality.\n"
         "        File file = new File(dst3);\n"
         "        FileOutputStream out = new FileOutputStream(file);\n"
         "        JPEGImageEncoder encoder = JPEGCodec.createJPEGEncoder(out);\n"
         "        JPEGEncodeParam param = encoder.getDefaultJPEGEncodeParam(bout);\n"
         "        param.setQuality((float)90.0, false); // 90 percent quality\n"
         "        encoder.setJPEGEncodeParam(param);\n"
         "        encoder.encode(bout);\n"
         "        out.close();\n"
         "        log(dst3);\n"
         "\n"
         "        // save memory image as PNG.\n"
         "        ImageIO.write(bout, \"png\", new File(dst4)); log(dst4);\n"
         "    }\n"
         "};\n"
         ,pszClassName
         );
         break;

      case 9:
      chain.print(
         "<?php\n"
         "   // simple text file read and write in php.\n"
         "   // requires the php command line interface:\n"
         "   // 1. get the php 5.x zip package\n"
         "   // 2. unzip into a dir like c:\\app\\php\n"
         "   // 3. set PATH=%PATH%;c:\\app\\php;c:\\app\\php\\ext\n"
         "   // then run this script by \"php %s\"\n"
         "\n"
         "   if ($argc < 3) {\n"
         "      print(\"usage: php %s infile outfile\\n\");\n"
         "      return;\n"
         "   }\n"
         "\n"
         "   $ssrc = $argv[1];\n"
         "   $sdst = $argv[2];\n"
         "\n"
         "   if (($fsrc = fopen($ssrc, \"r\")) === false) die(\"cannot read $ssrc\\n\");\n"
         "   if (($fdst = fopen($sdst, \"w\")) === false) die(\"cannot write $sdst\\n\");\n"
         "\n"
         "   $nlines = 0;\n"
         "   while (!feof($fsrc)) {\n"
         "      $sline = fgets($fsrc, 4096);\n"
         "      if (fputs($fdst, $sline) === false)\n"
         "         { print(\"failed to write (disk full?)\\n\"); break; }\n"
         "      $nlines++;\n"
         "   }\n"
         "\n"
         "   fclose($fdst);\n"
         "   fclose($fsrc);\n"
         "\n"
         "   print(\"$nlines lines copied from $ssrc to $sdst.\\n\");\n"
         "?>\n"
         ,pszOutFile ? pszOutFile : "thisfile.php"
         ,pszOutFile ? pszOutFile : "thisfile.php"
         );
         break;

      case 10:
      chain.print(
         "<?php\n"
         "   // create a thumbnail image from a large image.\n"
         "   // requires the php command line interface:\n"
         "   // 1. get the php 5.x zip package\n"
         "   // 2. unzip into a dir like c:\\app\\php\n"
         "   // 3. set PATH=PATH;c:\\app\\php;c:\\app\\php\\ext\n"
         "   // then run this script by \"php %s in.jpg out.jpg\"\n"
         "\n"
         "   if ($argc < 3) {\n"
         "      print(\"usage: php %s input.jpg output.jpg [targetwidth quality]\\n\");\n"
         "      return;\n"
         "   }\n"
         "\n"
         "   $ssrc = $argv[1];\n"
         "   $sdst = $argv[2];\n"
         "   $wdst = isset($argv[3]) ? $argv[3] : 100;\n"
         "   $nqty = isset($argv[4]) ? $argv[4] :  80;\n"
         "\n"
         "   if (strstr($ssrc, \".jpg\"))\n"
         "      $isrc = ImageCreateFromJPEG($ssrc);\n"
         "   else\n"
         "      $isrc = ImageCreateFromPNG($ssrc);\n"
         "   if ($isrc === false) die(\"cannot load: $ssrc\");\n"
         "\n"
         "   $nsrcw = ImageSX($isrc);\n"
         "   $nsrch = ImageSY($isrc);\n"
         "   print(\"input: $ssrc with $nsrcw\".\"x$nsrch pixels\\n\");\n"
         "\n"
         "   $hdst  = intval($wdst * $nsrch / $nsrcw);\n"
         "   $idst  = ImageCreateTrueColor($wdst, $hdst);\n"
         "   if ($idst === false) die(\"cannot create thumb\");\n"
         "\n"
         "   imagecopyresampled($idst, $isrc, 0,0,0,0, $wdst,$hdst, $nsrcw, $nsrch);\n"
         "   imagejpeg($idst, $sdst, $nqty);\n"
         "   print(\"thumb: $sdst with $wdst\".\"x$hdst pixels, quality=$nqty\\n\");\n"
         "\n"
         "   imagedestroy($idst);\n"
         "   imagedestroy($isrc);\n"
         "?>\n"
         ,pszOutFile ? pszOutFile : "thisfile.php"
         ,pszOutFile ? pszOutFile : "thisfile.php"
         );
         break;

      case 11:
      chain.print(
         "<html>\n"
         " <head>\n"
         "  <title>Welcome to FooBar</title>\n"
         "   <style type=\"text/css\">\n"
         "      body     { font: 12px verdana,arial; }\n"
         "      table    { font: 12px verdana,arial; }\n"
         "      h1       { font: 16px verdana,arial; font-weight: bold; }\n"
         "      b.red    { color: #ee6622; }\n"
         "   </style>\n"
         "   <script type=\"text/javascript\">\n"
         "      function hello() {\n"
         "         document.write(\"hello from JavaScript.\");\n"
         "      }\n"
         "   </script>\n"
         " </head>\n"
         "<body leftmargin=\"0\" topmargin=\"0\" marginwidth=\"0\" marginheight=\"0\">\n"
         "\n"
         "<table width=\"980\" cellspacing=\"0\" cellpadding=\"0\" align=\"center\" border=\"0\">\n"
         "\n"
         " <tr>\n"
         "  <td width=\"120\" align=\"center\" valign=\"middle\">\n"
         "  &nbsp;<br>\n"
         "  home\n"
         "  </td>\n"
         "  <td width=\"740\" align=\"center\" valign=\"top\">\n"
         "  &nbsp;<br>\n"
         "  <h1>Welcome to FooBar.</h1>\n"
         "  </td>\n"
         "  <td width=\"120\" align=\"center\" valign=\"middle\">\n"
         "  &nbsp;<br>\n"
         "  other\n"
         "  </td>\n"
         " </tr>\n"
         "\n"
         " <tr>\n"
         "  <td align=\"center\" valign=\"top\">&nbsp;</td>\n"
         "  <td>\n"
         "      <b class=\"red\">bold</b> and normal text.\n"
         "      <p>\n"
         "      <script type=\"text/javascript\">\n"
         "         hello();\n"
         "      </script>\n"
         "  </td>\n"
         "  <td align=\"center\" valign=\"top\">&nbsp;</td>\n"
         " </tr>\n"
         "\n"
         "</table>\n"
         "\n"
         "</body>\n"
         "</html>\n"
         );
         break;

      case 12:
      chain.print(
         "@rem <?php print(\"\\r\"); /*\n"
         "@echo off\n"
         "\n"
         "IF \"%%1\"==\"\" GOTO xerr01\n"
         "\n"
         "sfk script %s -from begin %%*\n"
         "GOTO xend\n"
         "\n"
         ":xerr01\n"
         "sfk echo \"[green]jpeg image size lister.[def]\"\n"
         "sfk echo \"lists width, height of all .jpg in a dir.\"\n"
         "sfk echo \"usage: %s dirname\"\n"
         "GOTO xend\n"
         "\n"
         "   // this script requires the php command line interface:\n"
         "   // 1. get the php 5.x zip package\n"
         "   // 2. unzip into a dir like c:\\app\\php\n"
         "   // 3. set PATH=PATH;c:\\app\\php;c:\\app\\php\\ext\n"
         "\n"
         "   // ----- begin of sfk script code -----\n"
         "\n"
         "sfk label begin\n"
         "\n"
         "   +sel %%1 .jpg\n"
         "\n"
         "   +run -quiet -yes \"php %s $file\"\n"
         "\n"
         "   +end\n"
         "\n"
         "*/ // ----- end of sfk, begin of php script -----\n"
         "\n"
         "   // print the width and height in pixel\n"
         "   // of the supplied image file name:\n"
         "\n"
         "   $asize = getimagesize($argv[1]);\n"
         "   printf(\"w=%%04d h=%%04d %%s\\n\", $asize[0], $asize[1], $argv[1]);\n"
         "\n"
         "/* // ----- end of php script, end of batch -----\n"
         ":xend\n"
         "@rem */ ?>\n"
         ,pszOutFile ? pszOutFile : "thisfile.bat"
         ,pszOutFile ? pszOutFile : "thisfile.bat"
         ,pszOutFile ? pszOutFile : "thisfile.bat"
         );
         break;

      case 13:
      chain.print(
         "import java.io.*;\n"
         "\n"
         "public class %s\n"
         "{\n"
         "    static void log(String s) { System.out.println(s); }\n"
         "\n"
         "    // convert a single byte record into a hexdump record.\n"
         "    // by default, set nrec to 16, and ndoff to 0.\n"
         "    public static String hexRecord(byte ab[], int noffset, int nlen, int nrec, int ndoff)\n"
         "    {\n"
         "        // create hex and text representation\n"
         "        StringBuffer sline = new StringBuffer();\n"
         "        StringBuffer stext = new StringBuffer();\n"
         "        for (int i=0; i<nlen; i++) {\n"
         "            int nval = ab[noffset+i] & 0xFF;\n"
         "            if (nval < 0x10) sline.append(\"0\");\n"
         "            sline.append(Integer.toString(nval, 0x10));\n"
         "            sline.append(\" \");\n"
         "            if (Character.isLetter(nval))\n"
         "                stext.append((char)nval);\n"
         "            else\n"
         "                stext.append('.');\n"
         "        }\n"
         "        // fill rest of line, if any\n"
         "        int npadlen = nrec;\n"
         "        while (stext.length() < npadlen) stext.append(' ');\n"
         "        npadlen *= 3;\n"
         "        while (sline.length() < npadlen) sline.append(' ');\n"
         "        sline.setLength(sline.length()-1);\n"
         "        // create offset as hex value\n"
         "        String soffset = \"\";\n"
         "        soffset = Integer.toString(ndoff, 0x10).toUpperCase();\n"
         "        while (soffset.length() < 10) soffset = \"0\"+soffset;\n"
         "        // combine hex, text and offset\n"
         "        return \">\"+sline.toString().toUpperCase()+\"< \"+stext+\" \"+soffset;\n"
         "    }\n"
         "    \n"
         "    // hexdump a whole byte array, from a given offset.\n"
         "    // nrec is the number of bytes per output record, use 16 by default.\n"
         "    // ndosv is the display offset start value, use 0 by default.\n"
         "    public static void hexDump(byte ab[], int noffset, int nlen, int nrec, int ndoff)\n"
         "    {\n"
         "        while (nlen > 0) {\n"
         "            int nblock  = (nlen < nrec) ? nlen : nrec;\n"
         "            String srec = hexRecord(ab, noffset, nblock, nrec, ndoff);\n"
         "            System.out.println(srec);\n"
         "            noffset += nblock;\n"
         "            ndoff   += nblock;\n"
         "            nlen    -= nblock;\n"
         "        }\n"
         "    }\n"
         "\n"
         "    // hex dump a whole binary file's content\n"
         "    public static void main(String args[]) throws Throwable\n"
         "    {\n"
         "        if (args.length < 1)\n"
         "            { log(\"usage: java %s inputfilename\"); return; }\n"
         "\n"
         "        byte abBuf[] = new byte[1600];\n"
         "\n"
         "        FileInputStream oin = new FileInputStream(args[0]);\n"
         "\n"
         "        int nread  = 0;\n"
         "        int ntotal = 0;\n"
         "        do {\n"
         "            nread = oin.read(abBuf, 0, abBuf.length);\n"
         "            if (nread > 0) hexDump(abBuf, 0, nread, 16, ntotal);\n"
         "            ntotal += nread;\n"
         "        }   while (nread > 0);\n"
         "\n"
         "        oin.close();\n"
         "    }\n"
         "}\n"
          ,pszClassName,pszClassName
        );
         break;

      case 14:
      chain.print(
         "\n"
         "import java.awt.*;\n"
         "import java.awt.event.*;\n"
         "import java.io.*;\n"
         "import javax.swing.*;\n"
         "import java.util.*;\n"
         "\n"
         "// can be run both as an applet and a command line application\n"
         "public class %s extends JApplet implements ActionListener\n"
         "{\n"
         "    // a panel to collect all objects for display\n"
         "    Container clPane = null;\n"
         "\n"
         "    // a hashmap to collect the same objects for retrieval by an id\n"
         "    HashMap<String,Component> clComp = new HashMap<String,Component>();\n"
         "    // HashMap clComp = new HashMap(); // for JDK 1.4.2\n"
         "\n"
         "    // the current add position\n"
         "    int xadd = 0, yadd = 0;\n"
         "\n"
         "    // set component placement cursor to a position\n"
         "    void setPos(int x,int y) { xadd=x; yadd=y; }\n"
         "    \n"
         "    // add and remember a generic component for display.\n"
         "    // steps the placement cursor w pixels to the right.\n"
         "    void add(int x,int y,int w,int h,String id,Component o) {\n"
         "        o.setBounds(x,y,w,h);   // set absolute position of object\n"
         "        clPane.add(o);          // add to panel to display object\n"
         "        clComp.put(id, o);      // and remember object in a hashmap\n"
         "        xadd += w;              // step add position horizontally\n"
         "    }\n"
         "    \n"
         "    // add a non-editable text label\n"
         "    void addLabel(int w, int h, String id, String text)\n"
         "        { add(xadd,yadd,w,h, id, new JLabel(text)); }\n"
         "\n"
         "    // add a single line editable text field    \n"
         "    void addTextField(int w, int h, String id, String text)\n"
         "        { add(xadd,yadd,w,h, id, new JTextField(text)); }\n"
         "\n"
         "    // add a multi linex editable text area\n"
         "    void addTextArea(int w, int h, String id, String text) {\n"
         "        JTextArea   oarea   = new JTextArea(text);\n"
         "        JScrollPane oscroll = new JScrollPane(oarea);\n"
         "        oscroll.setBounds(xadd,yadd,w,h);\n"
         "        clPane.add(oscroll);\n"
         "        clComp.put(id,oarea);\n"
         "    }\n"
         "\n"
         "    // add a push button\n"
         "    void addButton(int w, int h, String id, String text) {\n"
         "        JButton o = new JButton(text);\n"
         "        o.addActionListener(this);\n"
         "        add(xadd,yadd,w,h, id, o);\n"
         "    }\n"
         "\n"
         "    // easy access to any object by its id\n"
         "    JTextField getTextField(String id) { return (JTextField)clComp.get(id); }\n"
         "    JTextArea  getTextArea (String id) { return (JTextArea )clComp.get(id); }\n"
         "    JLabel     getLabel    (String id) { return (JLabel    )clComp.get(id); }\n"
         "\n"
         "    // setup visible objects at absolute positions\n"
         "    private void fillPane() \n"
         "    {        \n"
         "        clPane.setLayout(null); // absolute positioning layout\n"
         "        \n"
         "        setPos      ( 20, 20);\n"
         "        addLabel    ( 70, 20, \"lname\", \"filename\");\n"
         "        addTextField(620, 20, \"tname\", \"c:\\\\test.txt\");\n"
         "\n"
         "        setPos      ( 90, yadd + 30);\n"
         "        addButton   (620, 20, \"bload\", \"load\");\n"
         "\n"
         "        setPos      ( 20, yadd + 30);\n"
         "        addLabel    ( 70, 20, \"lcont\", \"content\");\n"
         "        addTextArea (620,400, \"acont\", \"\");\n"
         "    }\n"
         "\n"
         ,pszClassName);
    chain.print(
         "    // process push button command    \n"
         "    public void actionPerformed(ActionEvent e) \n"
         "    {\n"
         "        String sout = \"\";\n"
         "        try {\n"
         "            String scmd = e.getActionCommand();\n"
         "            if (scmd.equals(\"load\")) \n"
         "            {\n"
         "                String sFilename = getTextField(\"tname\").getText();\n"
         "                BufferedReader rin = new BufferedReader(\n"
         "                    new InputStreamReader(new FileInputStream(sFilename), \"ISO-8859-1\"));\n"
         "                while (true) {\n"
         "                    String sline = rin.readLine();\n"
         "                    if (sline == null) break; // EOD\n"
         "                    sout += sline + \"\\n\";\n"
         "                }\n"
         "                rin.close();\n"
         "            }\n"
         "        } catch (Throwable t) {\n"
         "            sout = \"\"+t;\n"
         "        }\n"
         "        getTextArea(\"acont\").setText(sout);\n"
         "    }\n"
         "\n"
         "    // to run it from the command line:\n"
         "    public static void main(String[] args)\n"
         "        { new %s().main2(args); }\n"
         "\n"
         "    public void main2(String[] args) {\n"
         "        JFrame frame = new JFrame(\"Simple File Viewer\");\n"
         "        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);\n"
         "        clPane = frame.getContentPane();\n"
         "        fillPane();\n"
         "        frame.setSize(760, 560);\n"
         "        frame.setVisible(true);\n"
         "    }\n"
         "\n"
         "    // to run it as an applet:\n"
         "  public void init() {\n"
         "     clPane = new JPanel();\n"
         "     this.setContentPane(clPane);\n"
         "     fillPane();\n"
         "  }\n"
         "    /*\n"
         "       create a page \"show.html\" containing\n"
         "\n"
         "       <html><body>\n"
         "          <Applet Code=\"%s.class\" width=800 height=600></Applet>\n"
         "       </body></html>\n"
         "      \n"
         "       and then type \"appletviewer show.html\"\n"
         "    */\n"
         "}\n"
          ,pszClassName,pszClassName
        );
        break;

      case 15:
      chain.printFile(
"myext@mydomain.org\\chrome\\content\\myext.xul", bWriteFile,
"<overlay id=\"myextOverlay\" xmlns=\"http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul\">\n"
"   <script type=\"application/x-javascript\" src=\"chrome://myext/content/myextOverlay.js\"/>\n"
"   <popup id=\"contentAreaContextMenu\">\n"
"      <menuitem id=\"myext-sayhello\" label=\"Say Hello\" oncommand=\"sayHello(event);\" />\n"
"   </popup>\n"
"</overlay>\n"
         );
      chain.printFile(
"myext@mydomain.org\\chrome\\content\\myextOverlay.js", bWriteFile,
"function sayHello(event) {\n"
"   alert(\"hello.\");\n"
"}\n"
         );
      chain.printFile(
"myext@mydomain.org\\chrome.manifest", bWriteFile,
"content myext chrome/content/\n"
"overlay chrome://browser/content/browser.xul chrome://myext/content/myext.xul\n"
         );
      chain.printFile(
"myext@mydomain.org\\install.rdf", bWriteFile,
"<?xml version=\"1.0\"?>\n"
"<RDF xmlns=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\"\n"
"     xmlns:em=\"http://www.mozilla.org/2004/em-rdf#\">\n"
"  <Description about=\"urn:mozilla:install-manifest\">\n"
"    <em:id>myext@mydomain.org</em:id>\n"
"    <em:version>0.1.0</em:version>\n"
"    <em:type>2</em:type>\n"
"    <em:targetApplication>\n"
"      <Description>\n"
"        <em:id>{ec8030f7-c20a-464f-9b0e-13a3a9e97384}</em:id> <!-- Firefox -->\n"
"        <em:minVersion>1.0</em:minVersion>\n"
"        <em:maxVersion>3.0.*</em:maxVersion>\n"
"      </Description>\n"
"    </em:targetApplication>\n"
"    <em:name>MyExt</em:name>\n"
"    <em:description>A very simple demo extension</em:description>\n"
"    <em:creator>My Self</em:creator>\n"
"    <em:homepageURL>http://mydomain.org/myext/</em:homepageURL>\n"
"  </Description>      \n"
"</RDF>\n"
         );
         break;
 
      case 16:
      chain.print(
          "/*\n"
          "   Example for sending UDP color text in C++.\n"
          "   For more details read \"sfk netlog\".\n"
          "\n"
          "   Compile like:\n"
          "      Windows gcc : g++ %s -lws2_32\n"
          "      Windows VC  : cl  %s ws2_32.lib\n"
          "      Linux/Mac   : g++ %s\n"
          "*/\n"
          "\n"
          "#include <stdio.h>\n"
          "#include <stdlib.h>\n"
          "#include <string.h>\n"
          "#include <stdarg.h>\n"
          "#include <errno.h>\n"
          "\n"
          "#ifdef _WIN32\n"
          "  #include <windows.h>\n"
          "  #ifdef _MSC_VER\n"
          "    #define snprintf  _snprintf \n"
          "    #define vsnprintf _vsnprintf \n"
          "    #define sockerrno WSAGetLastError()\n"
          "  #else\n"
          "    #include <ws2tcpip.h>\n"
          "    #define sockerrno errno\n"
          "  #endif\n"
          "  #define socklen_t int\n"
          "#else\n"
          "  #include <sys/socket.h>\n"
          "  #include <netdb.h>\n"
          "  #ifdef __APPLE__\n"
          "    #define SOL_IP IPPROTO_IP\n"
          "  #endif\n"
          "  #ifndef INVALID_SOCKET\n"
          "    #define INVALID_SOCKET -1\n"
          "  #endif\n"
          "  #define sockerrno errno\n"
          "#endif\n"
          "\n"
          "char szLineBuf[500];\n"
          "\n"
          "int iNetSock = INVALID_SOCKET;\n"
          "int iRequest = 1;\n"
          "struct sockaddr_in oAddr;\n"
          "socklen_t iAddrLen = sizeof(oAddr);\n"
          "\n"
          "int perr(const char *pszFormat, ...)\n"
          "{\n"
          "   va_list argList;\n"
          "   va_start(argList, pszFormat);\n"
          "   vsnprintf(szLineBuf, sizeof(szLineBuf)-10, pszFormat, argList);\n"
          "   szLineBuf[sizeof(szLineBuf)-10] = '\\0';\n"
          "   printf(\"Error: %s\\n\", szLineBuf);\n"
          "   return 0;\n"
          "}\n"
          "\n"
          "int netlog(const char *pszFormat, ...)\n"
          "{\n"
          "   char szHeadBuf[100];\n"
          "   int  iHeadLen = 0;\n"
          "\n"
          "   va_list argList;\n"
          "   va_start(argList, pszFormat);\n"
          "   vsnprintf(szLineBuf+100, sizeof(szLineBuf)-110, pszFormat, argList);\n"
          "   szLineBuf[sizeof(szLineBuf)-10] = '\\0';\n"
          "   \n"
          "   // change all [red] to compact color codes \\x1Fr\n"
          "   for (char *psz=szLineBuf+100; *psz; psz++)\n"
          "      if (psz[0]=='[')\n"
          "         for (int i=1; psz[i]; i++)\n"
          "            if (i>=2 && psz[i]==']')\n"
          "               { psz[0]=0x1F; memmove(psz+2, psz+i+1, strlen(psz+i+1)+1); break; }\n"
          "\n"
          "   // add sfktxt header before text\n"
          "   snprintf(szHeadBuf, sizeof(szHeadBuf)-10, \":sfktxt:v100,req%s,cs1\\n\\n\", iRequest++);\n"
          "   iHeadLen = strlen(szHeadBuf);\n"
          "   char *pData = szLineBuf+100-iHeadLen;\n"
          "   memcpy(pData, szHeadBuf, iHeadLen);\n"
          "\n"
          "   sendto(iNetSock, pData, strlen(pData), 0, (struct sockaddr *)&oAddr, iAddrLen);\n"
          "\n"
          "   return 0;\n"
          "}\n"
          "\n"
          "int main(int argc, char *argv[])\n"
          "{\n"
          "   const char *pszHost = \"localhost\";\n"
          "   unsigned short iPort = 21323;\n"
          "   \n"
          "   #ifdef _MSC_VER\n"
          "   WORD wVersionRequested = MAKEWORD(1,1);\n"
          "   WSADATA wsaData;\n"
          "   if (WSAStartup(wVersionRequested, &wsaData)!=0)\n"
          "      return 9+perr(\"WSAStartup failed\");\n"
          "   #endif\n"
          "\n"
          "   memset((char *)&oAddr, 0,sizeof(oAddr));\n"
          "   oAddr.sin_family      = AF_INET;\n"
          "   oAddr.sin_port        = htons(iPort);\n"
          "\n"
          "   struct hostent *pHost = gethostbyname(pszHost);\n"
          "   memcpy(&oAddr.sin_addr.s_addr, pHost->h_addr, pHost->h_length);\n"
          "\n"
          "   if ((iNetSock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)\n"
          "      return 9+perr(\"cannot create socket\");\n"
          "\n"
          "   netlog(\"[Red]Foo[def] and [Blue]bar[def] went to the [Green]zoo[def].\\n\");\n"
          "   \n"
          "   return 0;\n"
          "}\n"
          "\n"
          , pszOutFile ? pszOutFile : "netlog.cpp"
          , pszOutFile ? pszOutFile : "netlog.cpp"
          , pszOutFile ? pszOutFile : "netlog.cpp"
          , "%s" // literally
          , "%d" // literally
         );
         if (!pszOutFile)
            printf("// to write .cpp file use:\n"
                   "// sfk samp cppnetlog netlog.cpp\n");
         break;
 
      case 17:
      if (!pszOutFile) pszClassName = str("netlog");
      chain.print(
          "/*\n"
          "   Example for sending UDP color text in Java.\n"
          "   For more details read \"sfk netlog\".\n"
          "*/\n"
          "\n"
          "import java.io.*;\n"
          "import java.net.*;\n"
          "\n"
          "public class %s\n"
          "{\n"
          "   public static DatagramSocket clSocket = null;\n"
          "   public static InetAddress clAddress = null;\n"
          "   public static int iClPort = -1;\n"
          "   static int iClRequest = 1;\n"
          "\n"
          "   public static void init(String sHost, int iPort) throws Throwable\n"
          "   {\n"
          "      clAddress = InetAddress.getByName(sHost);\n"
          "      iClPort = iPort;\n"
          "      clSocket = new DatagramSocket();\n"
          "   }\n"
          "\n"
          "   public static void log(String sTextIn) throws Throwable\n"
          "   {\n"
          "      String sText   = sTextIn+\"\\n\";\n"
          "\n"
          "      // change all [red] to compact color codes \\x1Fr\n"
          "      byte[] abData1 = sText.getBytes();\n"
          "      int    iSize1  = abData1.length;\n"
          "      byte[] abData2 = new byte[iSize1+100];\n"
          "\n"
          "      // keep 100 bytes space for header\n"
          "      int i2=100;\n"
          "      for (int i1=0; i1<iSize1;)\n"
          "      {\n"
          "         if (abData1[i1]=='[') {\n"
          "            i1++;\n"
          "            if (i1>=iSize1)\n"
          "               break;\n"
          "            abData2[i2++] = (byte)0x1F;\n"
          "            abData2[i2++] = abData1[i1++];\n"
          "            while (i1<iSize1 && abData1[i1]!=']')\n"
          "               i1++;\n"
          "            if (i1<iSize1)\n"
          "               i1++;\n"
          "         } else {\n"
          "            abData2[i2++] = abData1[i1++];\n"
          "         }\n"
          "      }\n"
          "      int iTextSize = i2-100;\n"
          "\n"
          "      // add sfktxt header before text\n"
          "      String sHead = \":sfktxt:v100,req\"+iClRequest+\",cs1\\n\\n\";\n"
          "      iClRequest++;\n"
          "      byte abHead[] = sHead.getBytes();\n"
          "      int iHeadLen  = abHead.length;\n"
          "      for (int i=0; i<iHeadLen; i++)\n"
          "         abData2[100-iHeadLen+i] = abHead[i];\n"
          "      int iStartOff = 100-iHeadLen;\n"
          "      int iFullSize = iHeadLen+iTextSize;\n"
          "\n"
          "      DatagramPacket packet = new DatagramPacket(abData2, iStartOff, iFullSize, clAddress, iClPort);\n"
          "      clSocket.send(packet);\n"
          "   }\n"
          "\n"
          "   public static void main(String args[]) throws Throwable\n"
          "   {\n"
          "      %s.init(\"localhost\", 21323);\n"
          "      %s.log(\"[Red]Foo[def] and [Blue]bar[def] went to the [Green]zoo[def].\");\n"
          "   }\n"
          "}\n"
          "\n"
          , pszClassName
          , pszClassName
          , pszClassName
          );
         if (!pszOutFile)
            printf("// to write .java file use:\n"
                   "// sfk samp javanetlog netlog.java\n");
         break;
      }
}

// internal check: structure alignments must be same as in sfk.cpp
void getAlignSizes2(int &n1, int &n2, int &n3)
{
   n1 = (int)sizeof(AlignTest1);
   n2 = (int)sizeof(AlignTest2);
   n3 = (int)sizeof(AlignTest3);
}

/*
   =====================================================================================
   SFK ping support code. Must be LAST in sfkext.cpp as it changes structure alignments!
   =====================================================================================
*/

#ifdef _WIN32

#define ICMP_ECHO_REPLY    0
#define ICMP_DEST_UNREACH  3
#define ICMP_ECHO_REQUEST  8
#define ICMP_TTL_EXPIRE    11

#ifndef ICMP_MINLEN
 #define ICMP_MINLEN 8
#endif
#ifndef ICMP_ECHOREPLY
 #define ICMP_ECHOREPLY 0
#endif
#ifndef ICMP_ECHO
 #define ICMP_ECHO 8
#endif

#ifdef _MSC_VER
 #pragma pack(1)  // CHANGES STRUCTURE ALIGNMENTS FROM HERE!
#endif

struct icmp {
    BYTE icmp_type;          // ICMP packet type
    BYTE icmp_code;          // Type sub code
    USHORT icmp_cksum;
    USHORT icmp_id;
    USHORT icmp_seq;
    ULONG timestamp;    // not part of ICMP, but we need it
};

struct ip { // IPHeader {
    BYTE h_len:4;           // Length of the header in dwords
    BYTE version:4;         // Version of IP
    BYTE tos;               // Type of service
    USHORT total_len;       // Length of the packet in dwords
    USHORT ident;           // unique identifier
    USHORT flags;           // Flags
    BYTE ttl;               // Time to live
    BYTE proto;             // Protocol number (TCP, UDP etc)
    USHORT checksum;        // IP checksum
    ULONG source_ip;
    ULONG dest_ip;
};

#else

 #include <sys/param.h>
 #include <sys/file.h>
 #include <netinet/in_systm.h>
 #include <netinet/ip.h>
 #include <netinet/ip_icmp.h>

// some linux/mac sys\param.h define that:
#ifdef isset
 #undef isset
#endif

#endif

#define	DEFDATALEN	(64-ICMP_MINLEN)	/* default data length */
#define	MAXIPLEN	60
#define	MAXICMPLEN	76
#define	MAXPACKET	(65536 - 60 - ICMP_MINLEN)/* max packet size */

static ushort in_cksum(ushort *addr, unsigned len)
{
   ushort answer = 0;
   uint sum = 0;
   while (len > 1)  {
      sum += *addr++;
      len -= 2;
   }
   if (len == 1) {
      *(unsigned char *)&answer = *(unsigned char *)addr ;
      sum += answer;
   }
   sum = (sum >> 16) + (sum & 0xffff);
   sum += (sum >> 16);
   answer = ~sum;
   return answer;
}

void UDPCore::stepPing(int i)
{
   int iResendTime = imaxwait/2;
   if (iResendTime < 400)
       iResendTime = 400;

   switch (aClCon[i].istate)
   {
      case 0:
         sendPing(i);
         aClCon[i].istate = 1;
         break;

      case 1:
         if (getCurrentTime() - tstart < iResendTime)
            break;
         // sendPing(i);
         aClCon[i].istate = 2;
         break;
   }
}

int UDPCore::sendPing(int i)
{
   int datalen = DEFDATALEN;
   u_char outpack[MAXPACKET];
   char szSenderInfo[200];

   mclear(outpack);
   mclear(szSenderInfo);

   aClCon[i].idelay = -1;

   struct icmp *icp = (struct icmp *)outpack;

	icp->icmp_type   = ICMP_ECHO;
	icp->icmp_code   = 0;
	icp->icmp_cksum  = 0;
	icp->icmp_seq    = 123+i;	/* seq and id must be reflected */
	icp->icmp_id     = getpid()+i;

	int cc = datalen + ICMP_MINLEN;
	icp->icmp_cksum = in_cksum((unsigned short *)icp,cc);

   num tstart = getCurrentTime();

	int isent = sendto(aClCon[i].sock, (char *)outpack, cc, 0,
      (struct sockaddr*)&aClCon[i].addr, (socklen_t)sizeof(struct sockaddr_in));

   aClCon[i].tsent = getCurrentTime();

	if (isent != cc)
	{
      perr("ping: send error (%d/%d) to %s\n", isent, cc, aClCon[i].ipstr);
      return -5;
	}

   if (bverbose)
	  printf("ping: sent to #%u at %s\n", i, ipAsString(&aClCon[i].addr, szSenderInfo, sizeof(szSenderInfo)-10, 1));

   return 0;
}

int UDPCore::recvPing(int i, int *pDelay)
{
   int packlen = DEFDATALEN + MAXIPLEN + MAXICMPLEN;
	u_char packet[DEFDATALEN + MAXIPLEN + MAXICMPLEN + 10];
   char szSenderInfo[200];

   struct sockaddr_in from;
   int hlen = 0, end_t = 0;
   num tend = 0;

   mclear(packet);
   mclear(szSenderInfo);

	int fromlen = sizeof(sockaddr_in);
	int ret = 0;
	if ((ret = recvfrom(aClCon[i].sock, (char *)packet, packlen, 0,(struct sockaddr *)&from, (socklen_t*)&fromlen)) < 0)
	   return -7+perr("ping: receive error\n");
	
   if (bverbose)
	  printf("%d = recv\n", ret);

	struct ip *ip = (struct ip *)((char*)packet);
	
	hlen = sizeof(struct ip);
	
	if (ret < (hlen + ICMP_MINLEN))
	   return -8+perr("ping: reply too short\n");
	
	struct icmp *icp = (struct icmp *)(packet + hlen);

   char *pszfrom = ipAsString(&from, szSenderInfo, sizeof(szSenderInfo)-10, 1);
	
	if (icp->icmp_type != ICMP_ECHOREPLY)
   {
	   if (bverbose)
			printf("ping: got an invalid reply type %u from %s\n", icp->icmp_type, pszfrom);
         return 7;
   }

   if (bverbose)
		printf("ping: got an echo reply from %s\n", pszfrom);

	if (icp->icmp_seq != 123+i)
	{
      if (bverbose)
			printf("ping: received invalid sequence (%u/%u) from %s\n", icp->icmp_seq, 123+i, pszfrom);
      return 5;
	}

	if (icp->icmp_id != getpid()+i)
	{
      if (bverbose)
			printf("ping: received id %u from %s\n", icp->icmp_id, pszfrom);
      return 6;
	}

   aClCon[i].idelay = (int)(getCurrentTime() - aClCon[i].tsent);

   *pDelay = aClCon[i].idelay;

   int icopy = ret;
   if (icopy > sizeof(aClCon[i].reply))
       icopy = sizeof(aClCon[i].reply);
   memcpy(aClCon[i].reply, packet, icopy);
   aClCon[i].replylen = icopy;

   aClCon[i].istate = 3;

   iClResponses++;

   return 0;
}

#endif // USE_SFK_BASE

