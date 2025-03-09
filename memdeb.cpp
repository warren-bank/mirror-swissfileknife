/*
   sfk simple memory tracing
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

static void *operator new[](size_t size, char *src, int line);
static void *operator new(size_t size, char *src, int line);
static void *operator new(size_t size);

static char *sfkmem_file = 0;
static long  sfkmem_line = 0;
static long  sfkmem_news = 0;
static long  sfkmem_dels = 0;
static long  sfkmem_errs = 0;
static size_t sfkmem_bytes = 0;

static void setPreDelete(char *file, long line) {
   sfkmem_file = file;
   sfkmem_line = line;
}

class SFKMemoryListEntry
{
public:
   SFKMemoryListEntry   ( ) { pClNext = pClPrevious = 0; }

   SFKMemoryListEntry *next      ( ) { return pClNext; }
   SFKMemoryListEntry *previous  ( ) { return pClPrevious; }

   SFKMemoryListEntry *pClNext;
   SFKMemoryListEntry *pClPrevious;
};

class SFKMemoryList
{
public:
   SFKMemoryList  ( )   { pClFirst = pClLast = 0; }

   SFKMemoryListEntry *first  ( ) { return pClFirst; }
   SFKMemoryListEntry *last   ( ) { return pClLast; }

   void add                (SFKMemoryListEntry *);
   void addAsFirst         (SFKMemoryListEntry *);
   void addAfter           (SFKMemoryListEntry *after, SFKMemoryListEntry *toadd);
   void remove             (SFKMemoryListEntry *entry);  // w/o delete
   void removeAndDeleteAll ( );

private:
   SFKMemoryListEntry *pClFirst;
   SFKMemoryListEntry *pClLast;
};

#ifndef MEMDEB_JUST_DECLARE
void SFKMemoryList :: add(SFKMemoryListEntry* pNew)
{
   if (!pClFirst)
   {
      // yet empty list:
      pClFirst = pClLast = pNew;
      pNew->pClNext = pNew->pClPrevious = 0;
      return;
   }

   // append node at end of list:
   pClLast->pClNext   = pNew;
    pNew->pClPrevious = pClLast;
    pNew->pClNext     = 0;
   pClLast            = pNew;
}

void SFKMemoryList :: addAsFirst(SFKMemoryListEntry* pNew)
{
   // yet empty list?
   if (!pClFirst)
   {
      pClFirst = pClLast = pNew;
      pNew->pClNext = pNew->pClPrevious = 0;
      return;
   }

   // make node new front of list:
   SFKMemoryListEntry *n2 = pClFirst;
   n2->pClPrevious      = pNew;
    pNew->pClPrevious   = 0;
    pNew->pClNext       = n2;
   pClFirst             = pNew;
}

void SFKMemoryList :: addAfter(SFKMemoryListEntry *pAfter, SFKMemoryListEntry *pNew)
{
   SFKMemoryListEntry *pNext = pAfter->pClNext;  // might be 0

   pAfter->pClNext   = pNew;
   pNew->pClPrevious = pAfter;
   pNew->pClNext     = pNext;

   if (pNext)
      pNext->pClPrevious = pNew;
   else
      pClLast  = pNew;
}

void SFKMemoryList :: remove(SFKMemoryListEntry* pRemove)
{
   SFKMemoryListEntry *pPrevious = pRemove->pClPrevious;  // might be 0
   SFKMemoryListEntry *pNext     = pRemove->pClNext;      // might be 0

   pRemove->pClNext = pRemove->pClPrevious = 0;

   if (!pPrevious)   // if 'pRemove' at start of list
   {
      if (pClFirst = pNext)      // new list start becomes pNext ...
         pNext->pClPrevious = 0; // ... and if pNext exists, adjust it,
      else
         pClLast  = 0;           // else list is empty.
   }
   else
   {
      // at least a 'pClPrevious' is given.
      if (pPrevious->pClNext = pNext)     // let pPrevious' 'pClNext' ptr bypass 'pRemove' ...
         pNext->pClPrevious = pPrevious;  // ... and if pNext exists, adjust it,
      else
         pClLast  = pPrevious;            // else set new listend.
   }
}
#endif

struct SFKMemoryBlock
 : public SFKMemoryListEntry
{
void  *pAddress;
long   lSize;
long   lLine;
char  *file;
};

static void sfkmem_setZone(void *p,size_t size)
{
   unsigned char *puc = (unsigned char *)p;   
   for (unsigned long i=0; i<size; i++)
      puc[i] = 0xEE;
}

static long sfkmem_checkZone(void *p,size_t size)
{
   unsigned char *puc = (unsigned char *)p;   
   for (unsigned long i=0; i<size; i++)
      if (puc[i] != (unsigned char)0xEE)
         return -1;
   return 0;         
}

static void sfkmem_hexdump(void *pAddressIn, long  lSize)
{
 char szBuf[100];
 long lIndent = 1;
 long lOutLen, lIndex, lIndex2, lOutLen2;
 long lRelPos;
 struct { char *pData; unsigned long lSize; } buf;
 unsigned char *pTmp,ucTmp;
 unsigned char *pAddress = (unsigned char *)pAddressIn;

   buf.pData   = (char *)pAddress;
   buf.lSize   = lSize;

   while (buf.lSize > 0)
   {
      pTmp     = (unsigned char *)buf.pData;
      lOutLen  = (int)buf.lSize;
      if (lOutLen > 16)
          lOutLen = 16;

      /*                        1         2         3         4          */
      /*              01234567890123456789012345678901234567890123456789 */
      sprintf(szBuf, " >                                                  "
                     "    %08lX", pTmp-pAddress);
      lOutLen2 = lOutLen;

      for(lIndex = 1+lIndent, lIndex2 = 53-15+lIndent, lRelPos = 0;
          lOutLen2;
          lOutLen2--, lIndex += 2, lIndex2++
         )
      {
         ucTmp = *pTmp++;

         sprintf(szBuf + lIndex, "%02X ", (unsigned short)ucTmp);
         if(!isprint(ucTmp))  ucTmp = '.'; // nonprintable char
         szBuf[lIndex2] = ucTmp;

         if (!(++lRelPos & 3))     // extra blank after 4 bytes
         {  lIndex++; szBuf[lIndex+2] = ' '; }
      }

      if (!(lRelPos & 3)) lIndex--;

      szBuf[lIndex  ]   = '<';
      szBuf[lIndex+1]   = ' ';

      printf("%s\n", szBuf);

      buf.pData   += lOutLen;
      buf.lSize   -= lOutLen;
   }
}

static SFKMemoryList glblMem;

static void *memDebNew(size_t size, char *pszSource, int line)
{
   // calculate red zone size
   long lrs = size / 10;
   if (lrs < 4) lrs = 4;

   void *p = malloc(size+2*lrs);
   
   #ifdef VERBOSE_MEM
   printf("%lxh = ALLOC %ld (%s, %ld)\n", p, size, pszSource, line);
   #endif
   
   sfkmem_setZone(p,lrs);
   sfkmem_setZone((char*)p+lrs+size,lrs);
   
   SFKMemoryBlock *pBlock = (SFKMemoryBlock *)malloc(sizeof(SFKMemoryBlock));
   memset(pBlock,0,sizeof(SFKMemoryBlock));
   pBlock->pAddress = (char*)p+lrs;
   pBlock->lSize    = size;
   pBlock->lLine    = line;
   pBlock->file     = pszSource;
   glblMem.add(pBlock);

   sfkmem_news++;
   sfkmem_bytes += size;
   
   return (char*)p+lrs;
}

static void *operator new(size_t size, char *pszSource, int line)
{  return memDebNew(size, pszSource, line);  }

static void *operator new[](size_t size, char *pszSource, int line)
{  return memDebNew(size, pszSource, line);  }

static void operator delete(void *pUserMemory)
{   
   if (!pUserMemory)
      return;

   long lrs = 0;
   
   #ifdef VERBOSE_MEM
   printf("%lxh   DELETE (%s, %ld)\n", pUserMemory, sfkmem_file, sfkmem_line);
   #endif
   
   SFKMemoryBlock *p = 0;
   for (p=(SFKMemoryBlock*)glblMem.first(); p; p=(SFKMemoryBlock*)p->next())
      if (p->pAddress == pUserMemory)
         break;
   
   if (p)
   {
      // calculate red zone size
      lrs = p->lSize / 10;
      if (lrs < 4) lrs = 4;
   
      if (sfkmem_checkZone((char*)pUserMemory-lrs,lrs))
      {
         printf("MEMORY OVERWRITE:\n"
            "  before block: %lxh\n"
            "  alloc'ed in : %s line %ld\n",
            pUserMemory,
            p->file, p->lLine);
            
         printf("hexdump of block start follows.\n"
            "first %ld (%lxh) bytes should be '0xEE',\n"
            "those not being 0xEE got hit by something.\n",
            lrs, lrs);
            
         long lMaxDump = p->lSize;
         if (lMaxDump > 100)
             lMaxDump = 100;
         sfkmem_hexdump((char*)pUserMemory-lrs, lrs+lMaxDump);
         
         sfkmem_errs++;
      }
      
      if (sfkmem_checkZone((char*)pUserMemory+p->lSize,lrs))
      {
         printf("MEMORY OVERWRITE:\n"
            "  after block: %lxh\n"
            "  alloc'ed in: %s line %ld\n",
            pUserMemory,
            p->file, p->lLine);
            
         printf("hexdump of block end follows.\n"
            "last %ld (%lxh) bytes should be '0xEE',\n"
            "those not being 0xEE got hit by something.\n",
            lrs, lrs);
            
         long lMaxDump = p->lSize;
         if (lMaxDump > 100)
             lMaxDump = 100;
         sfkmem_hexdump((char*)pUserMemory
            +p->lSize
            -lMaxDump,
            lMaxDump+lrs);
            
         sfkmem_errs++;
      }
   
      sfkmem_bytes -= p->lSize;

      glblMem.remove(p);
      free(p);
   }
   else
   {
      printf("DELETE TWICE:\n"
         "  of block: %lxh\n"
         "  done in : %s line %ld\n",
         pUserMemory,
         sfkmem_file,
         sfkmem_line
         );
         
      sfkmem_errs++;
   }         
   
   sfkmem_dels++;
   
   free((char*)pUserMemory-lrs);
}

static void listMemoryLeaks( )
{
   long bAnyLeak = 0;
   for (SFKMemoryBlock *p=(SFKMemoryBlock*)glblMem.first(); p; p=(SFKMemoryBlock*)p->next())
   {
      bAnyLeak = 1;
      printf("MEM LEAK: adr %lxh, size %ld, alloc'ed in %s %ld\n",
         p->pAddress,
         p->lSize,
         p->file,
         p->lLine
         );
   }
   
   if (bAnyLeak)
      printf("[SMEMDEBUG: %ld new's, %ld delete's, %ld errors, %ld leaks]\n",
         sfkmem_news, sfkmem_dels, sfkmem_errs, sfkmem_news-sfkmem_dels);
}

#ifdef MEMDEB_JUST_DECLARE
extern char *memDebStrDup(const char *strSource, char *pszFile, int nLine);
#else
char *memDebStrDup(const char *strSource, char *pszFile, int nLine) {
   long lLen  = strlen(strSource);
   char *pOut = new(pszFile,nLine) char[lLen+2];
   strcpy(pOut, strSource);
   return pOut;
}
#endif

#define newTmpPreProc50796 new(__FILE__,__LINE__)
#define new newTmpPreProc50796

#define delTmpPreProc50796 delete
#define delete setPreDelete(__FILE__,__LINE__),delTmpPreProc50796

#define strdup(x) memDebStrDup(x,__FILE__,__LINE__)
