/*
   source instrumentation support

	sfk 1.5.4:	support for blocks starting with "   {"
               instead of just "{".
*/

#include "sfkbase.hpp"

// #define SFK_MEMTRACE
#ifdef _WIN32
 #ifdef SFK_MEMTRACE
  #define  MEMDEB_JUST_DECLARE
  #include "memdeb.cpp"
 #endif
#endif

#ifdef WINCE
   #undef stat
   #undef stat64
   extern int mystat(const char *_Name, struct stat *_Stat);
   extern char *strdup(char *);
   extern int remove(char *);
#endif

static char szLineBuf[4096];
static char szLineBuf2[4096];
static char szLineBuf3[4096];
static char szBupDir[4096];
static char szBupFile[4096];
static char szCopyCmd[4096];

static char *pszGlblInclude = "";
static char *pszGlblMacro   = "";
static bool  bdebug=0;

#define TRB_SIZE    200 // token ring buffer
#define BLINE_SIZE 1024
#define BLINE_MAX    50

class SrcParse
{
public:
   SrcParse ( );
   ulong processFile (char *pText, bool bSimulate, FILE *foutOptional);
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
   bool hasFunctionStart (ulong &rn1stline);
   char pretok    (ulong &itok2, ulong &nstepdowncnt, ulong &rnline);
   void addDetectKeyword   (char **rpsz);
   void reduceSignature    (char *pszIn, char *pszOut);

private:
   uchar atok[TRB_SIZE+10];
   ulong itok;
   uchar altok[TRB_SIZE+10]; // just for the current line
   ulong iltok;
   ulong atokline[TRB_SIZE+10]; // line number of token
   ulong nline;
   char  abline[BLINE_MAX][BLINE_SIZE+2];
   ulong ibline;
   ulong ibackscope;
   bool  bbackscope;
   FILE *clOut;
   ulong nClHits;
};

SrcParse::SrcParse() {
   itok   = 0;
   memset(atok, 0, sizeof(atok));
   iltok  = 0;
   memset(altok, 0, sizeof(altok));
   nline  = 0;
   ibline = 0;
   memset(abline, 0, sizeof(abline));
   ibackscope = 0;
   bbackscope = 0;
   clOut = 0;
   nClHits = 0;
}

ulong SrcParse::processFile(char *pText, bool bSimulate, FILE *fout)
{
   if (!bSimulate) {
      clOut = fout;
      fprintf(fout, "#include \"%s\" // [instrumented]\n", pszGlblInclude);
   }

   // char *pCopy = strdup(pText);
   ulong nTextLen = strlen(pText);
   char *pCopy = new char[nTextLen+10];
   if (!pCopy) { fprintf(stderr, "error  : out of memory at %d\n", __LINE__); return 0; }
   memcpy(pCopy, pText, nTextLen+1);

   nClHits = 0;

   // NO RETURNS FROM HERE

   ulong nMaxLineLen = sizeof(szLineBuf)-10;
   char *psz1 = pCopy;
   char *pszContinue = 0;
   while (psz1)
   {
      char *psz2 = strchr(psz1, '\n');
      if (psz2)
      {
         pszContinue = psz2+1;
         int nLineLen = psz2 - psz1;
         if (nLineLen > (long)nMaxLineLen) nLineLen = nMaxLineLen;
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

enum eScanStates {
   ess_idle  = 1,
   ess_word  = 2,
   ess_num   = 3,
   ess_colon = 4,
   ess_slash = 5
};

bool isatoz(char c) { char c2=tolower(c); return (c2>='a' && c2<='z'); }
bool isnum_(char c) { return (c>='0' && c<='9') || (c=='_'); }

void SrcParse::reduceSignature(char *psz1, char *psz2)
{
   // reduce [type] class::method([parms])
   //     to class::method
   ulong nlen = strlen(psz1);
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
   if (pszx = strchr(pszLine, '\n')) *pszx = '\0';
   if (pszx = strchr(pszLine, '\r')) *pszx = '\0';

   // store in backline buffer
   strncpy(abline[ibline], pszLine, BLINE_SIZE-10);
   abline[ibline][BLINE_SIZE-10] = '\0';
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

		mtklog("inst: %c %d",c1,nstate);

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

		mtklog("inst:   msline \"%.20s\"", pszMethodStartLine);

     if (bdebug)
     {
      printf("FN BODY at %d: %s\n", nline, (char*)atok);
      ulong ibline2 = ibline;
      ulong nblcnt  = 3; // BLINE_MAX;
      char *psz1;
      ulong i2 = ibackscope;
      while (psz1 = abline[i2]) {
         if (strlen(psz1))
            printf("] %s\n", psz1);
         if (i2 == ibline)
            break;
         if (++i2 >=  BLINE_MAX)
            i2 = 0;
      }
     }

      // the current line contains the relevant "{" somewhere.
      // for now, we instrument only simple lines:
		mtklog("inst:   lbuf2  \"%s\"", szLineBuf2);
		// accept "{" but also "[anywhitespace]{"
		char *pszs = szLineBuf2;
		while (*pszs && (*pszs==' ' || *pszs=='\t')) pszs++;
      if (!strcmp(pszs, "{")) {
         reduceSignature(pszMethodStartLine, szLineBuf3);
         sprintf(pszs, "{%s(\"%s\");", pszGlblMacro, szLineBuf3);
         // printf("=> %s \"%s\"\n", szLineBuf2, pszMethodStartLine);
         nClHits++;
      }
   }
   // else keep szLineBuf2 unchanged

   if (!bSimulate) {
      fputs(szLineBuf2, clOut);
      fputc('\n', clOut);
   }

   if (++ibline >= BLINE_MAX)
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
	mtklog("inst:  addtok %c", c);

   atok[itok] = c;
   atokline[itok] = ibline;
   if (++itok >= TRB_SIZE)
      itok = 0;
   atok[itok] = '*';

   altok[iltok] = c;
   if (++iltok >= TRB_SIZE)
      iltok = 0;
   altok[iltok] = '*';
   // printf("%c", c);
}

char SrcParse::pretok(ulong &itok2, ulong &nstepcnt, ulong &rnline)
{
   if (nstepcnt == 0)
      return 0;
   else
      nstepcnt--;

   if (itok2 == 0)
      itok2 = TRB_SIZE-1;
   else
      itok2--;

   rnline = atokline[itok2];
   return atok[itok2];
}

bool SrcParse::hasFunctionStart(ulong &rnline)
{
   ulong itok2 = itok;
   ulong ncnt  = TRB_SIZE;

   // W* S W B W* K ** {

   ulong ntline = 0;
   char c1 = pretok(itok2, ncnt, ntline);
   if (!c1) return false;
   if (c1 != '{') {
		mtklog("inst:   hasfs %c, false", c1);
		return false;
	}

   // have a curly braket:

   ulong nBra = 0;
   ulong nKet = 0;
   ulong nrc  = 0;
   ulong nScope = 0;
   ulong nBraDist = 0; // word distance from last bra
   
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
		mtklog("inst:   hasfs true");
      return true;
	}

   if (bdebug) {
      printf("MISS.%d: %s %d %d\n", nrc, (char*)atok,nBra,nKet);
      ulong ibline2 = ibline;
      ulong nblcnt  = 3; // BLINE_MAX;
      char *psz1;
      ulong i2=ibackscope;
      while (psz1 = abline[i2]) {
         if (strlen(psz1))
            printf("] %s\n", psz1);
         if (i2 == ibline)
            break;
         if (++i2 >=  BLINE_MAX)
            i2 = 0;
      }
   }

   return false;
}

static long fileSize(char *pszFile) 
{
   struct stat sinfo;
   #ifdef WINCE
   if (mystat(pszFile, &sinfo))
      return -1;
   #else
   if (stat(pszFile, &sinfo))
      return -1;
   #endif
   return sinfo.st_size;
}

// NOTE: DO NOT FORGET TO DELETE RESULT
static char *loadFile(char *pszFile, int nLine)
{
   long lFileSize = fileSize(pszFile);
   if (lFileSize < 0)
      return 0;
   char *pOut = new char[lFileSize+10];
   // printf("loadFile %p %d\n", pOut, nLine);
   FILE *fin = fopen(pszFile, "rb");
   if (!fin) { fprintf(stderr, "error  : cannot read: %s\n", pszFile); return 0; }
   long nRead = fread(pOut, 1, lFileSize, fin);
   fclose(fin);
   if (nRead != lFileSize) {
      fprintf(stderr, "error  : cannot read: %s (%d %d)\n", pszFile, nRead, lFileSize);
      delete [] pOut;
      return 0;
   }
   pOut[lFileSize] = '\0';
   return pOut;
}

int sfkInstrument(char *pszFile, char *pszInc, char *pszMac, bool bRevoke, bool bRedo, bool bTouchOnRevoke)
{
   // printf("INST %s %u %s %s\n",pszFile,bRevoke,pszInc,pszMac);

   pszGlblInclude = pszInc;
   pszGlblMacro   = pszMac;

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
            system(szCopyCmd);
            #ifdef _WIN32
            // 2. delete target to ensure copy will work
            sprintf(szCopyCmd, "del %s",pszFile);
            system(szCopyCmd);
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
            system(szCopyCmd);
            if (!bRedo) {
               printf("revoked: %s, %lu bytes\n",pszTargFileName,(unsigned long)ntotal);
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

   long nsize = fileSize(pszFile);
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
      system(szLineBuf);
      // retry 
      if (!(fout = fopen(pszFile, "w"))) {
         fprintf(stderr, "error  : unable to write: %s\n", pszFile);
         delete [] pFile;
         return 9;
      }
   }

   SrcParse *pparse2 = new SrcParse();
   ulong nHits = pparse2->processFile(pFile, 0, fout);

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
