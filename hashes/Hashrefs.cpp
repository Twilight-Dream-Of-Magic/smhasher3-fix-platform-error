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
#include "Platform.h"
#include "Hashlib.h"

unsigned refs() {
    USE_FAMILY(donothing);
    USE_FAMILY(badhash);
    USE_FAMILY(aesrng);

    USE_FAMILY(md5);
    USE_FAMILY(sha1);
    USE_FAMILY(sha2);
    USE_FAMILY(sha3);
    USE_FAMILY(rmd);
    USE_FAMILY(blake2);
    USE_FAMILY(blake3);
    USE_FAMILY(vmac);
    USE_FAMILY(ascon);
    USE_FAMILY(chaskey);
    USE_FAMILY(crc);

    USE_FAMILY(aesnihash);
    USE_FAMILY(fnv);
    USE_FAMILY(poly_mersenne);
    USE_FAMILY(discohash);
    USE_FAMILY(beamsplitter);
    USE_FAMILY(murmur1);
    USE_FAMILY(murmur2);
    USE_FAMILY(murmur3);
    USE_FAMILY(mum_mir);
    USE_FAMILY(tabulation);
    USE_FAMILY(wyhash);
    USE_FAMILY(PMP_mutilinear);
    USE_FAMILY(falkhash);
    USE_FAMILY(spookyhash);
    USE_FAMILY(metrohash);
    USE_FAMILY(pearson);
    USE_FAMILY(blockpearson);
    USE_FAMILY(multiply_shift);
    USE_FAMILY(t1ha);
    USE_FAMILY(farmhash);
    USE_FAMILY(farsh);
    USE_FAMILY(prvhash);
    USE_FAMILY(cityhash);
    USE_FAMILY(halftimehash);
    USE_FAMILY(siphash);
    USE_FAMILY(seahash);
    USE_FAMILY(hasshe2);
    USE_FAMILY(clhash);
    USE_FAMILY(umash);
    USE_FAMILY(o1hash);
    USE_FAMILY(floppsy);
    USE_FAMILY(nmhash);
    USE_FAMILY(komihash);
    USE_FAMILY(mx3);
    USE_FAMILY(jodyhash);
    USE_FAMILY(pengyhash);
    USE_FAMILY(fasthash);
    USE_FAMILY(khash);
    USE_FAMILY(superfasthash);
    USE_FAMILY(meowhash);
    USE_FAMILY(xxhash);
    USE_FAMILY(lookup3);
    USE_FAMILY(perloldhashes);
    USE_FAMILY(MurmurOAAT);
    USE_FAMILY(x17);
    USE_FAMILY(falcon_oaat);
    USE_FAMILY(crap);
    USE_FAMILY(fletcher);

    return 0;
}
