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

#ifndef USE_SFK_BASE
 #if defined(WINFULL) && defined(_MSC_VER)
  #define SFK_MEMTRACE
 #endif
#endif

#ifdef _WIN32
 #ifdef SFK_MEMTRACE
  #define  MEMDEB_JUST_DECLARE
  #include "memdeb.cpp"
 #endif
#endif

#include "sfknet.hpp"

int perr(const char *pszFormat, ...);
int pwarn(const char *pszFormat, ...);
int pinf(const char *pszFormat, ...);
extern bool endsWithArcExt(char *pname);
extern num nGlblMemLimit;
extern cchar *arcExtList[];
extern void tellMemLimitInfo();
extern int quietMode();
extern bool bGlblEscape;
extern char *getHTTPUserAgent();
extern int createOutDirTree(char *pszOutFile);
extern size_t myfwrite(uchar *pBuf, size_t nBytes, FILE *fout, num nMaxInfo=0, num nCur=0, SFKMD5 *pmd5=0);

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
         { perr(0, "int. 187281850"); ncmp=-1; break; }

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

void  TCPCon::setBlocking (bool bYesNo) 
{
   // in case of server socket, make accept non-blocking:
   unsigned long ulParm = bYesNo ? 0 : 1;
   ioctlsocket(clSock, FIONBIO, &ulParm);
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

int TCPCore::lastError( ) {
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
   unsigned long ulParm = 1;
   ioctlsocket(hServSock, FIONBIO, &ulParm);

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
		if (endsWithArcExt(purl)) {
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
		if (endsWithArcExt(purl)) {
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

      // prefix every entry by it's source url
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
      // °sender will wait now until we confirm successful transfer.
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
            if (   mystrstri(psz2, " href=\"", &npos)
                || mystrstri(psz2, " href='", &npos)
               )
            {
               ceref = psz2[npos+6];
               psz2 += npos+7;
               break; 
            }
            if (mystrstri(psz2, " href=", &npos))
            {
               ceref = ' ';
               psz2 += npos+6;
               break; 
            }
         }
         if (!mystrnicmp(psz2, "<img ", 5)) {
            int npos = 0;
            if (   mystrstri(psz2, " src=\"", &npos)
                || mystrstri(psz2, " src='", &npos)
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
   if (isNet() && isTravelZip(1))
      {_ } // accept, need to cache whole file
   else
   if (isZipSubEntry())
      {_ } // accept, need to cache sub entry via parent
   else {
      mtklog(("coi::provideInput not needed: %s net=%d tz=%d", name(),isNet(),isTravelZip(1)));
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
   if (bWithRootZips && isTravelZip()) return 1;
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
