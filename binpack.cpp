/*
   support code for the sfk bin-to-src function.
   this will rarely be used by end users,
   but it's needed by sfk itself for popup image encoding.
*/

#define lmsg

#ifdef LOCALTEST

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "raw.cpp"

#define uchar unsigned char
#define ulong unsigned long

static void bmem_hexdump
 (
   void *pAddress,
   long  lSize
 )
{
 char szBuf[100];
 long lIndent = 1;
 long lOutLen, lIndex, lIndex2, lOutLen2;
 long lRelPos;
 struct { char *pData; unsigned long lSize; } buf;
 unsigned char *pTmp,ucTmp;

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

#endif

uchar *binPack(uchar *pIn, ulong nInSize, ulong &rnOutSize)
{
   ulong nOutSize = 0;
   uchar *pOut = 0;
   uchar *pMem = 0;
   uchar *pMax = pIn + nInSize;
   uchar *pCur = 0;
   uchar *pOld = 0;

   for (uchar npass=1; npass <= 2; npass++)
   {
      if (npass == 1)
         pOut = 0;
      else {
         pMem = new uchar[nOutSize];
         pOut = pMem;
      }

      pCur = pIn;
      pOld = pCur;

      while (pCur < pMax)
      {
         // detect repetition of patterns up to size 3
         uchar nbestsize = 0;
         uchar nbestgain = 0;
         uchar nbestrep  = 0;
         for (uchar isize = 1; isize <= 3; isize++) {
            uchar nrep  = 0;
            uchar bbail = 0;
            for (; (pCur+(nrep+1)*isize < pMax) && (nrep < 60) && !bbail; nrep++)
            {
               for (uchar i1=0; (i1<isize) && !bbail; i1++)
                  if (pCur[nrep*isize+i1] != pCur[i1])
                     bbail = 1;
               if (bbail)
                  break;
            }
            // this always results in nrep >= 1.
            uchar ngain = (nrep-1)*isize;
            if (ngain >= 3) {
               // there is a repetition, saving at least 3 bytes.
               // determine max savings accross all sizes.
               if (ngain > nbestgain) {
                  nbestgain = ngain;
                  nbestsize = isize;
                  nbestrep  = nrep;
               }
            }
         }
         // if (nbestrep > 0)
         //   printf("size %02u rep %02u gain %02u at %x\n", nbestsize, nbestrep, nbestgain, pCur-pIn);

         // if repeat pattern found,
         // OR if non-repeat exceeds maxsize
         if ( (nbestrep > 0) || ((pCur - pOld) >= 60) ) {
            // flush non-packable, if any
            if (pCur > pOld) {
               lmsg("[flush non-pack %x]\n", pCur-pOld);
               uchar nDist = pCur-pOld;
               if (npass == 1) {
                  pOut++;
                  pOut += (pCur - pOld);
               } else {
                  *pOut++ = 0x00 | nDist;
                  while (pOld < pCur)
                     *pOut++ = *pOld++;
               }
               pOld  = pCur;
            }
            // flush packable, if any
            if (nbestrep > 0) {
               lmsg("[pack %x %x]\n", nbestsize, nbestrep);
               if (npass == 1) {
                  pOut++;
                  pOut += nbestsize;
                  pCur += nbestsize;
               } else {
                  *pOut++ = (nbestsize << 6) | nbestrep;
                  for (uchar i1=0; i1<nbestsize; i1++)
                     *pOut++ = *pCur++;
               }
               pCur += (nbestrep-1)*nbestsize;
               pOld  = pCur;
            }
         } else {
            // count non-packable
            pCur++;
         }
      }

      // flush remainder, if any
      if (pCur > pOld) {
         lmsg("[flush trailer %x]\n", pCur-pOld);
         uchar nDist = pCur-pOld;
         if (npass == 1) {
            pOut++;
            pOut += (pCur - pOld);
         } else {
            *pOut++ = 0x00 | nDist;
            while (pOld < pCur)
               *pOut++ = *pOld++;
         }
         pOld  = pCur;
      }

      if (npass == 1) {
         nOutSize = (ulong)(pOut - 0);
         lmsg("packsize %u\n", nOutSize);
      }
   }

   rnOutSize = nOutSize;
   return pMem;
}

uchar *binUnpack(uchar *pIn, ulong nInSize, ulong &rnOutSize)
{
   ulong nOutSize = 0;
   uchar *pOut = 0;
   uchar *pMem = 0;
   uchar *pMax = pIn + nInSize;
   uchar *pCur = 0;
   uchar *pOld = 0;

   for (uchar npass=1; npass <= 2; npass++)
   {
      if (npass == 1)
         pOut = 0;
      else {
         pMem = new uchar[nOutSize];
         pOut = pMem;
      }

      pCur = pIn;
      pOld = pCur;

      while (pCur < pMax)
      {
         uchar ncmd = *pCur++;
         if (ncmd >= 64) {
            // unpack repeat block
            uchar nsiz = ncmd >> 6;
            uchar nrep = ncmd & 0x3F;
            lmsg("[upack-rep %x %x (%x)]\n", nsiz, nrep, ncmd);
            // reproduce reference pattern nrep times
            if (npass == 1)
               pOut += nrep * nsiz;
            else
            for (; nrep > 0; nrep--)
               for (uchar i1=0; i1<nsiz; i1++)
                  *pOut++ = *(pCur+i1);
            // skip reference pattern
            pCur += nsiz;
         } else {
            // unpack skip block
            uchar nrep = ncmd;
            lmsg("[upack-skip %x]\n", nrep);
            if (npass == 1) {
               pOut += nrep;
               pCur += nrep;
            }
            else
            for (; (nrep > 0) && (pCur < pMax); nrep--)
               *pOut++ = *pCur++;
         }
      }

      if (npass == 1) {
         nOutSize = (ulong)(pOut - 0);
         lmsg("unpacksize %u\n", nOutSize);
      }
   }

   rnOutSize = nOutSize;
   return pMem;
}

long simpleUnpack(uchar *pCur, ulong nInSize, uchar *pOut, ulong nOutSize)
{
   uchar *pMax    = pCur + nInSize;
   uchar *pOutMax = pOut + nOutSize;

   while (pCur < pMax)
   {
      uchar ncmd = *pCur++;
      if (ncmd >= 64) {
         uchar nsiz = ncmd >> 6;
         uchar nrep = ncmd & 0x3F;
         for (; nrep > 0; nrep--)
            for (uchar i1=0; i1<nsiz; i1++)
               *pOut++ = *(pCur+i1);
         pCur += nsiz;
      } else {
         uchar nrep = ncmd;
         for (; (nrep > 0) && (pCur < pMax); nrep--)
            *pOut++ = *pCur++;
      }
   }

   return 0;
}

#ifdef LOCALTEST

uchar abUnpack[100000];

uchar abNoRep[50] = {
   0x10, 0x20, 0x30, 0x10, 0x20, 0x30, 0x50, 0x80, 0x90, 0xA0,
   0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xA0,
   0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xA0,
   0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xA0,
   0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0x99,
};

int main(int argc, char *argv[])
{
   // bmem_hexdump(abRawBlock, 24000);
   // bmem_hexdump(abNoRep, 50);

   ulong nPackSize = 0;
   uchar *pPack = binPack(abRawBlock, 24000, nPackSize);
   // uchar *pPack = binPack(abNoRep, 50, nPackSize);

   // bmem_hexdump(pPack, nPackSize);

   ulong nUnpackSize = 0;
   uchar *pUnpack = binUnpack(pPack, nPackSize, nUnpackSize);

   // bmem_hexdump(pUnpack, nUnpackSize);

   simpleUnpack(pPack, nPackSize, abUnpack, 24000);

   for (ulong n1=0; n1<24000; n1++)
      if (abRawBlock[n1] != pUnpack[n1]) {
         printf("mismatch.1 at offset %x\n", n1);
         exit(1);
      }

   for (ulong n2=0; n2<24000; n2++)
      if (abRawBlock[n2] != abUnpack[n2]) {
         printf("mismatch.2 at offset %x\n", n2);
         exit(1);
      }

   printf("OK %u %u %u\n", 24000, nPackSize, nUnpackSize);

   return 0;
}
#endif
