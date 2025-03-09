/*
   sfk md5 implementation
*/

class SFKMD5
{
public:
    SFKMD5  ( );
   ~SFKMD5  ( );

   void   update  (uchar *pData, ulong nLen);
   uchar *digest  ( );

private:
   void   update     (uchar c);
   void   transform  ( );

   ulong nClCntHigh, nClCntLow;
   uchar *pClBuf;
   ulong nClBufLen,nClBufCnt;
   uchar *pClDig;
   ulong nClDigLen;
   uchar aClBuf[100];
   uchar aClDig[100];
   ulong alClSta[4];
   ulong alClBuf[16];
   bool  bClDigDone;
};

SFKMD5::SFKMD5()
{
   bClDigDone  = 0;

   nClDigLen   = 16;
   nClBufLen   = 64;
   pClBuf      = aClBuf;
   pClDig      = aClDig;

   nClCntHigh  = nClCntLow = nClBufCnt = 0;

   alClSta[0] = 0x67452301;
   alClSta[1] = 0xefcdab89;
   alClSta[2] = 0x98badcfe;
   alClSta[3] = 0x10325476;
}

SFKMD5::~SFKMD5() { }

void SFKMD5::update(uchar *pData, ulong nLen)
{
   for (ulong i=0; i<nLen; i++)
      update(pData[i]);
}

void SFKMD5::update(uchar c)
{
   pClBuf[nClBufCnt++] = c;
   if (nClBufCnt == nClBufLen)
   {
      transform();
      nClBufCnt = 0;
   }
   if (++nClCntLow == 0)
      nClCntHigh++;
}

void SFKMD5::transform()
{
   #define MD5XOR0(x, y, z) (z ^ (x & (y ^ z)))
   #define MD5XOR1(x, y, z) (y ^ (z & (x ^ y)))
   #define MD5XOR2(x, y, z) (x ^ y ^ z)
   #define MD5XOR3(x, y, z) (y  ^  (x | ~z))
   #define MD5ROT(a, n) a = (a << n) | (a >> (32 - n))
   #define MD5PR0(a, b, c, d, k, s, t) { a += k + t + MD5XOR0(b, c, d); MD5ROT(a, s); a += b; }
   #define MD5PR1(a, b, c, d, k, s, t) { a += k + t + MD5XOR1(b, c, d); MD5ROT(a, s); a += b; }
   #define MD5PR2(a, b, c, d, k, s, t) { a += k + t + MD5XOR2(b, c, d); MD5ROT(a, s); a += b; }
   #define MD5PR3(a, b, c, d, k, s, t) { a += k + t + MD5XOR3(b, c, d); MD5ROT(a, s); a += b; }

   uchar *pBuf = pClBuf;
   for (ulong i = 0; i < 16; i++)
   {
      ulong ul;
      ul = pBuf[0];
      ul |= pBuf[1] << 8;
      ul |= pBuf[2] << 16;
      ul |= pBuf[3] << 24;
      pBuf += 4;
      alClBuf[i] = ul;
   }

   ulong a = alClSta[0];
   ulong b = alClSta[1];
   ulong c = alClSta[2];
   ulong d = alClSta[3];

   MD5PR0(a, b, c, d, alClBuf[ 0],  7, 0xd76aa478);
   MD5PR0(d, a, b, c, alClBuf[ 1], 12, 0xe8c7b756);
   MD5PR0(c, d, a, b, alClBuf[ 2], 17, 0x242070db);
   MD5PR0(b, c, d, a, alClBuf[ 3], 22, 0xc1bdceee);
   MD5PR0(a, b, c, d, alClBuf[ 4],  7, 0xf57c0faf);
   MD5PR0(d, a, b, c, alClBuf[ 5], 12, 0x4787c62a);
   MD5PR0(c, d, a, b, alClBuf[ 6], 17, 0xa8304613);
   MD5PR0(b, c, d, a, alClBuf[ 7], 22, 0xfd469501);
   MD5PR0(a, b, c, d, alClBuf[ 8],  7, 0x698098d8);
   MD5PR0(d, a, b, c, alClBuf[ 9], 12, 0x8b44f7af);
   MD5PR0(c, d, a, b, alClBuf[10], 17, 0xffff5bb1);
   MD5PR0(b, c, d, a, alClBuf[11], 22, 0x895cd7be);
   MD5PR0(a, b, c, d, alClBuf[12],  7, 0x6b901122);
   MD5PR0(d, a, b, c, alClBuf[13], 12, 0xfd987193);
   MD5PR0(c, d, a, b, alClBuf[14], 17, 0xa679438e);
   MD5PR0(b, c, d, a, alClBuf[15], 22, 0x49b40821);

   MD5PR1(a, b, c, d, alClBuf[ 1],  5, 0xf61e2562);
   MD5PR1(d, a, b, c, alClBuf[ 6],  9, 0xc040b340);
   MD5PR1(c, d, a, b, alClBuf[11], 14, 0x265e5a51);
   MD5PR1(b, c, d, a, alClBuf[ 0], 20, 0xe9b6c7aa);
   MD5PR1(a, b, c, d, alClBuf[ 5],  5, 0xd62f105d);
   MD5PR1(d, a, b, c, alClBuf[10],  9, 0x02441453);
   MD5PR1(c, d, a, b, alClBuf[15], 14, 0xd8a1e681);
   MD5PR1(b, c, d, a, alClBuf[ 4], 20, 0xe7d3fbc8);
   MD5PR1(a, b, c, d, alClBuf[ 9],  5, 0x21e1cde6);
   MD5PR1(d, a, b, c, alClBuf[14],  9, 0xc33707d6);
   MD5PR1(c, d, a, b, alClBuf[ 3], 14, 0xf4d50d87);
   MD5PR1(b, c, d, a, alClBuf[ 8], 20, 0x455a14ed);
   MD5PR1(a, b, c, d, alClBuf[13],  5, 0xa9e3e905);
   MD5PR1(d, a, b, c, alClBuf[ 2],  9, 0xfcefa3f8);
   MD5PR1(c, d, a, b, alClBuf[ 7], 14, 0x676f02d9);
   MD5PR1(b, c, d, a, alClBuf[12], 20, 0x8d2a4c8a);

   MD5PR2(a, b, c, d, alClBuf[ 5],  4, 0xfffa3942);
   MD5PR2(d, a, b, c, alClBuf[ 8], 11, 0x8771f681);
   MD5PR2(c, d, a, b, alClBuf[11], 16, 0x6d9d6122);
   MD5PR2(b, c, d, a, alClBuf[14], 23, 0xfde5380c);
   MD5PR2(a, b, c, d, alClBuf[ 1],  4, 0xa4beea44);
   MD5PR2(d, a, b, c, alClBuf[ 4], 11, 0x4bdecfa9);
   MD5PR2(c, d, a, b, alClBuf[ 7], 16, 0xf6bb4b60);
   MD5PR2(b, c, d, a, alClBuf[10], 23, 0xbebfbc70);
   MD5PR2(a, b, c, d, alClBuf[13],  4, 0x289b7ec6);
   MD5PR2(d, a, b, c, alClBuf[ 0], 11, 0xeaa127fa);
   MD5PR2(c, d, a, b, alClBuf[ 3], 16, 0xd4ef3085);
   MD5PR2(b, c, d, a, alClBuf[ 6], 23, 0x04881d05);
   MD5PR2(a, b, c, d, alClBuf[ 9],  4, 0xd9d4d039);
   MD5PR2(d, a, b, c, alClBuf[12], 11, 0xe6db99e5);
   MD5PR2(c, d, a, b, alClBuf[15], 16, 0x1fa27cf8);
   MD5PR2(b, c, d, a, alClBuf[ 2], 23, 0xc4ac5665);

   MD5PR3(a, b, c, d, alClBuf[ 0],  6, 0xf4292244);
   MD5PR3(d, a, b, c, alClBuf[ 7], 10, 0x432aff97);
   MD5PR3(c, d, a, b, alClBuf[14], 15, 0xab9423a7);
   MD5PR3(b, c, d, a, alClBuf[ 5], 21, 0xfc93a039);
   MD5PR3(a, b, c, d, alClBuf[12],  6, 0x655b59c3);
   MD5PR3(d, a, b, c, alClBuf[ 3], 10, 0x8f0ccc92);
   MD5PR3(c, d, a, b, alClBuf[10], 15, 0xffeff47d);
   MD5PR3(b, c, d, a, alClBuf[ 1], 21, 0x85845dd1);
   MD5PR3(a, b, c, d, alClBuf[ 8],  6, 0x6fa87e4f);
   MD5PR3(d, a, b, c, alClBuf[15], 10, 0xfe2ce6e0);
   MD5PR3(c, d, a, b, alClBuf[ 6], 15, 0xa3014314);
   MD5PR3(b, c, d, a, alClBuf[13], 21, 0x4e0811a1);
   MD5PR3(a, b, c, d, alClBuf[ 4],  6, 0xf7537e82);
   MD5PR3(d, a, b, c, alClBuf[11], 10, 0xbd3af235);
   MD5PR3(c, d, a, b, alClBuf[ 2], 15, 0x2ad7d2bb);
   MD5PR3(b, c, d, a, alClBuf[ 9], 21, 0xeb86d391);

   alClSta[0] += a;
   alClSta[1] += b;
   alClSta[2] += c;
   alClSta[3] += d;
}

uchar *SFKMD5::digest()
{
   if (bClDigDone)
      return pClDig;

   ulong lLow  = nClCntLow << 3;
   ulong lHigh = (nClCntLow >> 29) | (nClCntHigh << 3);

   update(128);

   while ((nClCntLow & 63) != 56)
      update(0);

   update((uchar)lLow);
   update((uchar)(lLow >> 8));
   update((uchar)(lLow >> 16));
   update((uchar)(lLow >> 24));

   update((uchar)lHigh);
   update((uchar)(lHigh >> 8));
   update((uchar)(lHigh >> 16));
   update((uchar)(lHigh >> 24));

   // above code should ensure that last update() lead to empty buffer.
   if (nClBufCnt != 0) { fprintf(stderr, "ERROR: SFKMD5 internal #1\n"); }

   uchar *pDigest = pClDig;
   for (ulong i = 0; i < 4; i++)
   {
      ulong ul = alClSta[i];
      pDigest[0] = (uchar)ul; ul >>= 8;
      pDigest[1] = (uchar)ul; ul >>= 8;
      pDigest[2] = (uchar)ul; ul >>= 8;
      pDigest[3] = (uchar)ul;
      pDigest += 4;
   }

   bClDigDone = 1;
   return pClDig;
}
