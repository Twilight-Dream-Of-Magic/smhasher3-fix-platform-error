/*
 * hash he2
 * This is free and unencumbered software released into the public
 * domain under The Unlicense (http://unlicense.org/).
 *
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a
 * compiled binary, for any purpose, commercial or non-commercial, and
 * by any means.
 *
 * In jurisdictions that recognize copyright laws, the author or
 * authors of this software dedicate any and all copyright interest in
 * the software to the public domain. We make this dedication for the
 * benefit of the public at large and to the detriment of our heirs
 * and successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to
 * this software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <http://unlicense.org/>
 */
/* From http://cessu.blogspot.com/2008/11/hashing-with-sse2-revisited-or-my-hash.html */
#include "Platform.h"
#include "Types.h"
#include "Hashlib.h"

#if defined(NEW_HAVE_SSE_2)
#include <immintrin.h>
#endif

alignas(16) const static uint32_t coeffs[12] = {
  /* Four carefully selected coefficients and interleaving zeros. */
  0x98b365a1, 0, 0x52c69cab, 0,
  0xb76a9a41, 0, 0xcc4d2c7b, 0,
  /* 128 bits of random data. */
  0x564a4447, 0xc7265595, 0xe20c241d, 0x128fa608,
};

#define COMBINE_AND_MIX(c_1, c_2, s_1, s_2, in)                              \
  /* Phase 1: Perform four 32x32->64 bit multiplication with the             \
     input block and words 1 and 3 coeffs, respectively.  This               \
     effectively propagates a bit change in input to 32 more                 \
     significant bit positions.  Combine into internal state by              \
     subtracting the result of multiplications from the internal             \
     state. */                                                               \
  s_1 = _mm_sub_epi64(s_1, _mm_mul_epu32(c_1, _mm_unpackhi_epi32(in, in)));  \
  s_2 = _mm_sub_epi64(s_2, _mm_mul_epu32(c_2, _mm_unpacklo_epi32(in, in)));  \
                                                                             \
  /* Phase 2: Perform shifts and xors to propagate the 32-bit                \
     changes produced above into 64-bit (and even a little larger)           \
     changes in the internal state. */                                       \
  /* state ^= state >64> 29; */                                              \
  s_1 = _mm_xor_si128(s_1, _mm_srli_epi64(s_1, 29));                         \
  s_2 = _mm_xor_si128(s_2, _mm_srli_epi64(s_2, 29));                         \
  /* state +64= state <64< 16; */                                            \
  s_1 = _mm_add_epi64(s_1, _mm_slli_epi64(s_1, 16));                         \
  s_2 = _mm_add_epi64(s_2, _mm_slli_epi64(s_2, 16));                         \
  /* state ^= state >64> 21; */                                              \
  s_1 = _mm_xor_si128(s_1, _mm_srli_epi64(s_1, 21));                         \
  s_2 = _mm_xor_si128(s_2, _mm_srli_epi64(s_2, 21));                         \
  /* state +64= state <128< 32; */                                           \
  s_1 = _mm_add_epi64(s_1, _mm_slli_si128(s_1, 4));                          \
  s_2 = _mm_add_epi64(s_2, _mm_slli_si128(s_2, 4));                          \
                                                                             \
  /* Phase 3: Propagate the changes among the four 64-bit words by           \
     performing 64-bit subtractions and 32-bit word shuffling. */            \
  s_1 = _mm_sub_epi64(s_1, s_2);                                             \
  s_2 = _mm_sub_epi64(_mm_shuffle_epi32(s_2, _MM_SHUFFLE(0, 3, 2, 1)), s_1); \
  s_1 = _mm_sub_epi64(_mm_shuffle_epi32(s_1, _MM_SHUFFLE(0, 1, 3, 2)), s_2); \
  s_2 = _mm_sub_epi64(_mm_shuffle_epi32(s_2, _MM_SHUFFLE(2, 1, 0, 3)), s_1); \
  s_1 = _mm_sub_epi64(_mm_shuffle_epi32(s_1, _MM_SHUFFLE(2, 1, 0, 3)), s_2); \
                                                                             \
  /* With good coefficients any one-bit flip in the input has now            \
     changed all bits in the internal state with a probability               \
     between 45% to 55%. */

static void hasshe2(const uint8_t * input_buf, size_t n_bytes, uint64_t seed, void *output_state) {
  __m128i coeffs_1, coeffs_2, rnd_data, seed_xmm, input, state_1, state_2;

#if 0
#else  
  if (n_bytes % 16) {
      //add pad NUL
      n_bytes += 16 - (n_bytes % 16);
  }
#endif  
  
  coeffs_1 = _mm_load_si128((__m128i *) coeffs);
  coeffs_2 = _mm_load_si128((__m128i *) (coeffs + 4));
  rnd_data = _mm_load_si128((__m128i *) (coeffs + 8));
  seed_xmm = _mm_set1_epi64x(seed);

  /* Initialize internal state to something random.  (Alternatively,
     if hashing a chain of data, read in the previous hash result from
     somewhere.) */
#if 0
  state_1 = state_2 = _mm_xor_si128(rnd_data, seed_xmm);
#else
  state_1 = state_2 = rnd_data;
#endif

  while (n_bytes >= 16) {
      /* Read in 16 bytes, or 128 bits, from buf.  Advance buf and
         decrement n_bytes accordingly. */
      input = _mm_loadu_si128((__m128i *) input_buf);
      input_buf += 16;
      n_bytes -= 16;

      COMBINE_AND_MIX(coeffs_1, coeffs_2, state_1, state_2, input);
  }
#if 0
  if (n_bytes > 0) {
      uint8_t buf[16];
      memcpy(buf, input_buf, n_bytes);
      memset(buf + n_bytes, 0, 16 - n_bytes);
      input = _mm_loadu_si128((__m128i *) buf);
      COMBINE_AND_MIX(coeffs_1, coeffs_2, state_1, state_2, input);
  }
#endif

  /* Postprocessing.  Copy half of the internal state into fake input,
     replace it with the constant rnd_data, and do one combine and mix
     phase more. */
  input = state_1;
  state_1 = rnd_data;
  COMBINE_AND_MIX(coeffs_1, coeffs_2, state_1, state_2, input);

  _mm_storeu_si128((__m128i *)output_state, state_1);
  _mm_storeu_si128((__m128i *)((char*)output_state + 16), state_2);
}

void Hasshe2(const void * in, const size_t len, const seed_t seed, void * out) {
    if (len == 0) {
        PUT_U32<false>(0, (uint8_t *)out, 0);
    } else {
        hasshe2((const uint8_t *)in, len, (uint64_t)seed, out);
    }
}

REGISTER_FAMILY(hasshe2);

REGISTER_HASH(hasshe2,
  $.desc = "hasshe2 (SSE2-oriented hash)",
  $.hash_flags =
        0,
  $.impl_flags =
        FLAG_IMPL_LICENSE_PUBLIC_DOMAIN,
  $.bits = 256,
  $.verification_LE = 0xF5D39DFE,
  $.verification_BE = 0x0,
  $.hashfn_native = Hasshe2,
  $.hashfn_bswap = Hasshe2
);
