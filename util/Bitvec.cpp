/*
 * SMHasher3
 * Copyright (C) 2021-2022  Frank J. T. Wojcik
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *     Copyright (c) 2010-2012 Austin Appleby
 *     Copyright (c) 2019-2020 Yann Collet
 *     Copyright (c) 2020      Reini Urban
 *
 *     Permission is hereby granted, free of charge, to any person
 *     obtaining a copy of this software and associated documentation
 *     files (the "Software"), to deal in the Software without
 *     restriction, including without limitation the rights to use,
 *     copy, modify, merge, publish, distribute, sublicense, and/or
 *     sell copies of the Software, and to permit persons to whom the
 *     Software is furnished to do so, subject to the following
 *     conditions:
 *
 *     The above copyright notice and this permission notice shall be
 *     included in all copies or substantial portions of the Software.
 *
 *     THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *     EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 *     OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *     NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 *     HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *     WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *     FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *     OTHER DEALINGS IN THE SOFTWARE.
 */
#include "Platform.h"
#include "Types.h"
#include "Random.h"
#include "Bitvec.h"

#include <cstdio>
#include <algorithm>
#include <memory>
#include <cassert>

//----------------------------------------------------------------------------

#ifdef DEBUG
#undef assume
#define assume(x) assert(x)
#define verify(x) assert(x)
#else
static void warn_if ( bool x, const char * s, const char * fn, uint64_t ln )
{
  if (!x)
    printf("Statement %s is not true: %s:%ld\n", s, fn, ln);
}
#define verify(x) warn_if(x, #x, __FILE__, __LINE__)
#endif

//----------------------------------------------------------------------------

static void printKey(const void* key, size_t len)
{
    const unsigned char* const p = (const unsigned char*)key;
    size_t s;
    printf("\n0x");
    for (s=0; s<len; s++) printf("%02X", p[s]);
    printf("\n  ");
    for (s=0; s<len; s+=8) printf("%-16zu", s);
}

void printHash(const void* key, size_t len)
{
    const unsigned char* const p = (const unsigned char*)key;
    assert(len < 2048);
    for (int i=(int)len-1; i >= 0 ; i--) printf("%02x", p[i]);
    printf("  ");
}

void printbits ( const void * blob, int len )
{
  const uint8_t * data = (const uint8_t *)blob;

  printf("[");
  for(int i = 0; i < len; i++)
  {
    unsigned char byte = data[i];

    int hi = (byte >> 4);
    int lo = (byte & 0xF);

    if(hi) printf("%01x",hi);
    else   printf(".");

    if(lo) printf("%01x",lo);
    else   printf(".");

    if(i != len-1) printf(" ");
  }
  printf("]");
}

void printbits2 ( const uint8_t * k, int nbytes )
{
  printf("[");

  for(int i = nbytes-1; i >= 0; i--)
  {
    uint8_t b = k[i];

    for(int j = 7; j >= 0; j--)
    {
      uint8_t c = (b & (1 << j)) ? '#' : ' ';

      putc(c,stdout);
    }
  }
  printf("]");
}

void printhex ( const void * blob, int len )
{
  assert((len & 3) == 0);
  uint8_t * d = (uint8_t*)blob;
  for(int i = 0; i < len; i++)
  {
    printf("%02x",d[i]);
  }
}

void printhex32 ( const void * blob, int len )
{
  assert((len & 3) == 0);

  uint32_t * d = (uint32_t*)blob;

  printf("{ ");

  for(int i = 0; i < len/4; i++)
  {
    printf("0x%08x, ",d[i]);
  }

  printf("}");
}

void printbytes ( const void * blob, int len )
{
  uint8_t * d = (uint8_t*)blob;

  printf("{ ");

  for(int i = 0; i < len; i++)
  {
    printf("0x%02x, ",d[i]);
  }

  printf(" };");
}

void printbytes2 ( const void * blob, int len )
{
  uint8_t * d = (uint8_t*)blob;

  for(int i = 0; i < len; i++)
  {
    printf("%02x ",d[i]);
  }
}

//-----------------------------------------------------------------------------
// Bit-level manipulation

uint32_t getbit_wrap ( const void * block, int len, uint32_t bit )
{
  uint8_t * b = (uint8_t*)block;

  int byte = bit >> 3;
  bit = bit & 0x7;

  byte %= len;

  return (b[byte] >> bit) & 1;
}

void setbit ( void * block, int len, uint32_t bit )
{
  uint8_t * b = (uint8_t*)block;

  int byte = bit >> 3;
  bit = bit & 0x7;

  if(byte < len) b[byte] |= (1 << bit);
}

void     clearbit    ( void * blob, int len, uint32_t bit );

void setbit ( void * block, int len, uint32_t bit, uint32_t val )
{
  val ? setbit(block,len,bit) : clearbit(block,len,bit);
}

void clearbit ( void * block, int len, uint32_t bit )
{
  uint8_t * b = (uint8_t*)block;

  int byte = bit >> 3;
  bit = bit & 0x7;

  if(byte < len) b[byte] &= ~(1 << bit);
}

// from the "Bit Twiddling Hacks" webpage
int countbits ( uint32_t v )
{
  v = v - ((v >> 1) & 0x55555555);                    // reuse input as temporary
  v = (v & 0x33333333) + ((v >> 2) & 0x33333333);     // temp
  int c = ((v + ((v >> 4) & 0xF0F0F0F)) * 0x1010101) >> 24; // count

  return c;
}

// For highzerobits()
const uint32_t hzb[256] = {
    8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

//-----------------------------------------------------------------------------

static NEVER_INLINE uint32_t window1 ( void * blob, int len, int start, int count )
{
  int nbits = len*8;
  start %= nbits;

  uint32_t t = 0;

  for(int i = 0; i < count; i++)
  {
    setbit(&t,sizeof(t),i, getbit_wrap(blob,len,start+i));
  }

  return t;
}

static NEVER_INLINE uint32_t window8 ( void * blob, int len, int start, int count )
{
  int nbits = len*8;
  start %= nbits;

  uint32_t t = 0;
  uint8_t * k = (uint8_t*)blob;

  if(count == 0) return 0;

  int c = start & (8-1);
  int d = start / 8;

  for(int i = 0; i < 4; i++)
  {
    int ia = (i + d + 1) % len;
    int ib = (i + d + 0) % len;

    uint32_t a = k[ia];
    uint32_t b = k[ib];

    uint32_t m = (a << (8-c)) | (b >> c);

    t |= (m << (8*i));

  }

  t &= ((1 << count)-1);

  return t;
}

static NEVER_INLINE uint32_t window32 ( void * blob, int len, int start, int count )
{
  int nbits = len*8;
  start %= nbits;

  assert((len & 3) == 0);

  int ndwords = len / 4;

  uint32_t * k = (uint32_t*)blob;

  if(count == 0) return 0;

  int c = start & (32-1);
  int d = start / 32;

  if(c == 0) return (k[d] & ((1 << count) - 1));

  int ia = (d + 1) % ndwords;
  int ib = (d + 0) % ndwords;

  uint32_t a = k[ia];
  uint32_t b = k[ib];

  uint32_t t = (a << (32-c)) | (b >> c);

  t &= ((1 << count)-1);

  return t;
}

//-----------------------------------------------------------------------------

template < int nbits >
bool test_window2 ( void )
{
  Rand r(83874);

  struct keytype
  {
    uint8_t bytes[nbits/8];
  };

  int nbytes = nbits / 8;
  int reps = 10000;

  for(int j = 0; j < reps; j++)
  {
    if(j % (reps/10) == 0) printf(".");

    keytype k;

    r.rand_p(&k,nbytes);

    for(int start = 0; start < nbits; start++)
    {
      for(int count = 0; count < std::min(nbits, 24); count++)
      {
        uint32_t a = window1(&k,nbytes,start,count);
        uint32_t b = window8(&k,nbytes,start,count);
        uint32_t c = ((nbytes&3) != 0) ? b :window32(&k,nbytes,start,count);
        uint32_t d = window(&k,start,count);

        verify(a == b);
        verify(a == c);
        verify(a == d);
      }
    }
  }

  printf("PASS %d\n",nbits);

  return true;
}

bool test_window ( void )
{
  Rand r(48402);

  int reps = 10000;

  for(int j = 0; j < reps; j++)
  {
    if(j % (reps/10) == 0) printf(".");

    int nbits   = 64;
    int nbytes  = nbits / 8;

    uint64_t x = r.rand_u64();
    Blob<64> xb = x;

    for(int start = 0; start < nbits; start++)
    {
      for(int count = 0; count <= 24; count++)
      {
        uint32_t a = (uint32_t)ROTR64(x,start);
        a &= ((1 << count)-1);

        uint32_t b = window1   (&xb,nbytes,start,count);
        uint32_t c = window8   (&xb,nbytes,start,count);
        uint32_t d = window32  (&xb,nbytes,start,count);
        uint32_t e = window<64>(&xb,start,count);
        uint32_t f = window    (&xb,start,count);

        verify(a == b);
        verify(a == c);
        verify(a == d);
        verify(a == e);
        verify(a == f);
      }
    }
  }

  printf("PASS 64\n");

  test_window2<8>();
  test_window2<16>();
  test_window2<24>();
  test_window2<32>();
  test_window2<40>();
  test_window2<48>();
  test_window2<56>();
  test_window2<64>();
  test_window2<128>();
  test_window2<256>();

  return true;
}

//-----------------------------------------------------------------------------
