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
 *     Copyright (c) 2014-2021 Reini Urban
 *     Copyright (c) 2019-2020 Yann Collet
 *     Copyright (c) 2021      Jim Apple
 *     Copyright (c) 2021      Ori Livneh
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
#include "Bitvec.h"
#include "Stats.h"
#include "Analyze.h"
#include "Instantiate.h"
#include "VCode.h"

#include "BadSeedsTest.h"

#include <inttypes.h>
#ifdef HAVE_THREADS
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#endif

//-----------------------------------------------------------------------------
// Find bad seeds, and test against the known secrets/bad seeds.

template< typename hashtype >
static bool TestSeed(const HashInfo * hinfo, const seed_t seed) {
    const HashFn hash = hinfo->hashFn(g_hashEndian);
    const std::vector<int>     testlens  = {1,2,4,8,12,16,32,64,128};
    const std::vector<uint8_t> testbytes = {0,32,'0',127,128,255};
    const unsigned numtestbytes = testbytes.size();
    const hashtype zero = {0};
    std::vector<hashtype> hashes(numtestbytes);
    HashSet<hashtype> dummy_collisions;
    bool result = true;

    if (hinfo->is32BitSeed() && (seed > UINT64_C(0xffffffff)))
        return true;

    /* Premake all the test keys */
    uint8_t keys[numtestbytes][128];
    for (int i = 0; i < numtestbytes; i++) {
        memset(&keys[i][0], testbytes[i], 128);
    }

    printf("0x%" PRIx64 "\n", seed);
    const seed_t hseed = hinfo->Seed(seed, true);

    for (int len : testlens) {
        memset(&hashes[0], 0, numtestbytes * sizeof(hashtype));
        for (int i = 0; i < numtestbytes; i++) {
            hash(&keys[i][0], len, hseed, &hashes[i]);
            if (hashes[0] == zero) {
                printf("Confirmed broken seed 0x%" PRIx64 " => hash of 0"  \
                        " with key[%d] of all 0x%02x\n",
                        seed, len, testbytes[i]);
                result = false;
            }
        }

        if (FindCollisions(hashes, dummy_collisions, 0, false) > 0) {
            printf("Confirmed bad seed 0x%" PRIx64 " for len %d\n", seed, len);
            for (int i = 0; i < numtestbytes; i++) {
                printf("  ");
                printhex(&hashes[i], sizeof(hashtype));
                printf("\n");
            }
            result = false;
        }
    }

    return result;
}

template< typename hashtype >
static bool BadSeedsKnown(const HashInfo * hinfo) {
    bool result = true;
    const std::set<seed_t> & seeds = hinfo->badseeds;

    printf("Testing %lu known bad seeds:\n", seeds.size());

    for (seed_t seed : seeds) {
        bool thisresult = true;
        thisresult &= TestSeed<hashtype>(hinfo, seed);
        if (!hinfo->is32BitSeed() && (seed <= 0xffffffff) && (seed != 0)) {
            thisresult &= TestSeed<hashtype>(hinfo, seed << 32);
        }
        if (thisresult) {
            printf("Huh! \"Known\" bad seed %016lx isn't bad\n", seed);
        }
        result &= thisresult;
    }

    if (!seeds.size()) {
        result &= TestSeed<hashtype>(hinfo, 0x0);
    }

    if (result) {
        printf("PASS\n");
    }

    return result;
}

//-----------------------------------------------------------------------------
#ifdef HAVE_THREADS
// For keeping track of progress printouts across threads
static std::atomic<unsigned> seed_progress;
static std::mutex print_mutex;
#else
static unsigned seed_progress;
#endif

// Process part of a 2^32 range, split into g_NCPU threads
template< typename hashtype >
static void TestSeedRangeThread(const HashInfo * hinfo, const uint64_t hi,
                             const uint32_t start, const uint32_t endlow,
                             bool &result, bool &newresult) {
    const std::set<seed_t> & seeds = hinfo->badseeds;
    const HashFn hash = hinfo->hashFn(g_hashEndian);
    const seed_t last = hi | endlow;
    const hashtype zero = {0};
    //static_assert(testbytes[0] == 0, "Code assumes first test byte is 0");
    const std::vector<uint8_t> testbytes = {0,32,127,255};
    const unsigned numtestbytes = testbytes.size();
    std::vector<hashtype> hashes(numtestbytes);
    HashSet<hashtype> collisions;

    const char * progress_fmt =
        (last <= UINT64_C(0xffffffff)) ?
            "%8" PRIx64 "%c"  : "%16" PRIx64 "%c";
    const uint64_t progress_nl_every =
        (last <= UINT64_C(0xffffffff)) ? 8 : 4;

    int fails = 0;
    result = true;

    {
#ifdef HAVE_THREADS
        std::lock_guard<std::mutex> lock(print_mutex);
#endif
        printf("Testing [0x%016" PRIx64 ", 0x%016" PRIx64 "] ... \n", hi | start, last);
    }

    /* Premake all the test keys */
    uint8_t keys[numtestbytes][16];
    for (int i = 0; i < numtestbytes; i++) {
        memset(&keys[i][0], testbytes[i], 16);
    }

    seed_t seed = (hi | start);
    do {
        /*
         * Print out progress using *one* printf() statement (for
         * thread friendliness). Add newlines periodically to make
         * output friendlier to humans, keeping track of printf()s
         * across all threads.
         */
        if ((seed & UINT64_C(0x1ffffff)) == UINT64_C(0x1ffffff)) {
#ifdef HAVE_THREADS
            std::lock_guard<std::mutex> lock(print_mutex);
#endif
            unsigned count = ++seed_progress;
            const char spacer = ((count % progress_nl_every) == 0) ? '\n' : ' ';
            printf (progress_fmt, seed, spacer);
        }

        /* Test the next seed against 16 copies of each test byte */
        const seed_t hseed = hinfo->Seed(seed, true, 1);
        memset(&hashes[0], 0, numtestbytes * sizeof(hashtype));
        for (int i = 0; i < numtestbytes; i++) {
            hash(&keys[i][0], 16, hseed, &hashes[i]);
        }

        /* Check for a broken seed */
        if (hashes[0] == zero) {
            bool known_seed = (std::find(seeds.begin(), seeds.end(), seed) != seeds.end());
            {
#ifdef HAVE_THREADS
                std::lock_guard<std::mutex> lock(print_mutex);
#endif
                if (known_seed) {
                    printf("\nVerified broken seed 0x%" PRIx64 " => 0 with key[16] of all 0 bytes\n", seed);
                } else {
                    printf("\nNew broken seed 0x%" PRIx64 " => 0 with key[16] of all 0 bytes\n", seed);
                }
            }
            fails++;
            result = false;
            if (!known_seed) {
                newresult = true;
            }
        }

        /* Report if any collisions were found */
        if (FindCollisions(hashes, collisions, 1000, true) > 0) {
#ifdef HAVE_THREADS
            std::lock_guard<std::mutex> lock(print_mutex);
#endif
            bool known_seed = (std::find(seeds.begin(), seeds.end(), seed) != seeds.end());
            if (known_seed) {
                printf("\nVerified bad seed 0x%" PRIx64 "\n", seed);
            } else {
                printf("\nNew bad seed 0x%" PRIx64 "\n", seed);
            }
            fails++;
            if (fails > 300) {
                fprintf(stderr, "Too many bad seeds, ending test\n");
                if (g_NCPU > 1) {
                    exit(1);
                }
                goto out;
            }
            if (!known_seed && (fails < 32)) { // don't print too many lines
                PrintCollisions(collisions);
            }
            collisions.clear();
            result = false;
            if (!known_seed)
                newresult = true;
        }
    } while (seed++ != last);

 out:
    return;
}

// Test a full 2**32 range [hi + 0, hi + 0xffffffff].
// If no new bad seed is found, then newresult must be left unchanged.
template< typename hashtype >
static bool TestManySeeds(const HashInfo * hinfo, const uint64_t hi, bool &newresult) {
  bool result = true;
  seed_progress = 0;

  if (g_NCPU == 1) {
      TestSeedRangeThread<hashtype>(hinfo, hi, 0x0, 0xffffffff, result, newresult);
      printf("\n");
  } else {
#ifdef HAVE_THREADS
      // split into g_NCPU threads
      std::thread t[g_NCPU];
      const uint64_t len = UINT64_C(0x100000000) / g_NCPU;
      // Can't make VLAs in C++, so have to use vectors, but can't
      // pass a ref of a bool in a vector to a thread... :-<
      bool * results    = new bool[g_NCPU]();
      bool * newresults = new bool[g_NCPU]();

      printf("%d threads starting...\n", g_NCPU);
      for (int i=0; i < g_NCPU; i++) {
          const uint32_t start = i * len;
          const uint32_t end = (i < (g_NCPU - 1)) ? start + (len - 1) : 0xffffffff;
          t[i] = std::thread {TestSeedRangeThread<hashtype>, hinfo, hi, start, end,
                              std::ref(results[i]), std::ref(newresults[i])};
      }

      std::this_thread::sleep_for(std::chrono::seconds(1));

      for (int i=0; i < g_NCPU; i++) {
          t[i].join();
      }

      printf("All %d threads ended\n", g_NCPU);

      for (int i=0; i < g_NCPU; i++) {
          result &= results[i];
          newresult |= newresults[i];
      }

      delete [] results;
      delete [] newresults;
#endif
  }

  // Since this can be threaded, just use the test parameters for the
  // VCode input data.
  addVCodeInput(hi);         // hi
  addVCodeInput(0);          // lo start
  addVCodeInput(0xffffffff); // lo end
  // Nothing to add to VCodeOutput
  addVCodeResult(result);

  return result;
}

template< typename hashtype >
static bool BadSeedsFind(const HashInfo * hinfo) {
    bool result = true;
    bool newresult = false;

    printf("Testing the first 2**32 seeds ...\n");
    result &= TestManySeeds<hashtype>(hinfo, UINT64_C(0x0), newresult);

    if (!hinfo->is32BitSeed()) {
        printf("And the last 2**32 seeds ...\n");
        result &= TestManySeeds<hashtype>(hinfo, UINT64_C(0xffffffff00000000), newresult);
    }

    if (result) {
        printf("PASS\n");
    } else {
        printf("FAIL\n");
        if (newresult) {
            printf("Consider adding any new bad seeds to this hash's list of badseeds in main.cpp\n");
        }
    }

    return result;
}

//-----------------------------------------------------------------------------
template < typename hashtype >
bool BadSeedsTest(const HashInfo * hinfo, bool find_new_seeds) {
    const HashFn hash = hinfo->hashFn(g_hashEndian);
    bool result = true;

    // Never find new bad seeds for mock hashes, except for aesrng
    if (hinfo->isMock() && (strncmp(hinfo->name, "aesrng", 6) != 0)) {
        find_new_seeds = false;
    }

    printf("[[[ BadSeeds Tests ]]]\n\n");

    /*
     * FIXME - With the current definition of a "bad" seed, some
     * failures on 32-bit hashes are expected by chance. Either the
     * bad seed test needs to change, or the pass/fail needs to be
     * based on the count of bad seeds.
     *
     * For now, just don't test 32-bit hashes.
     */
    if (sizeof(hashtype) <= 4) {
        printf("Skipping BadSeeds test on 32-bit hash\n\n");
        return result;
    }

    hinfo->Seed(0);

    result &= BadSeedsKnown<hashtype>(hinfo);
    if (find_new_seeds) {
        result &= BadSeedsFind<hashtype>(hinfo);
    }

    recordTestResult(result, "BadSeeds", (const char *)NULL);

    if(!result) printf("\n*********FAIL*********\n");
    printf("\n");

    return result;
}

INSTANTIATE(BadSeedsTest, HASHTYPELIST);
