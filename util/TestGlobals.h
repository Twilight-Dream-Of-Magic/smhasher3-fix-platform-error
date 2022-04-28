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
 */

//-----------------------------------------------------------------------------
// Basic infrastructure that basically all tests use
#include <vector>
#include <set>
#include "Hashinfo.h"

//-----------------------------------------------------------------------------
// Global variables from main.cpp

// To be able to sample different statistics sets from the same hash,
// a seed can be supplied which will be used in each test where a seed
// is not explicitly part of that test.
extern seed_t g_seed;

// The user can select which endian-ness of the hash implementation to test
extern HashInfo::endianness g_hashEndian;

extern const char * g_failstr;

//-----------------------------------------------------------------------------
// Recording test results for final summary printout

#define COUNT_MAX_PVALUE 18
extern uint32_t g_log2pValueCounts[COUNT_MAX_PVALUE+2];

static inline void recordLog2PValue(uint32_t log_pvalue) {
  if (log_pvalue <= COUNT_MAX_PVALUE) {
    g_log2pValueCounts[log_pvalue]++;
  } else {
    g_log2pValueCounts[COUNT_MAX_PVALUE+1]++;
  }
}

extern uint32_t g_testPass, g_testFail;
extern std::vector< std::pair<const char *, char *> > g_testFailures;

static inline void recordTestResult(bool pass, const char * suitename, const char * testname) {
  if (pass) {
    g_testPass++;
    return;
  }
  g_testFail++;

  char * ntestname = NULL;
  if (testname != NULL) {
    testname += strspn(testname, " ");
    ntestname = strdup(testname);
    if (!ntestname) {
        printf("OOM\n");
        exit(1);
    }
  }
  g_testFailures.push_back(std::pair<const char *, char *>(suitename, ntestname));
}

static inline void recordTestResult(bool pass, const char * suitename, uint64_t testnum) {
  const uint64_t maxlen = sizeof("18446744073709551615"); // UINT64_MAX
  char testname[maxlen];
  snprintf(testname, maxlen, "%lu", testnum);
  recordTestResult(pass, suitename, testname);
}

