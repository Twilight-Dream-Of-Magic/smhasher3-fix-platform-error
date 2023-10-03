set(HASH_SRC_FILES
  hashes/donothing.cpp
  hashes/badhash.cpp
  hashes/aesrng.cpp
#
#########################################
# Normal hashes are sorted biggest->smallest to try to minimize
# parallel compilation times
#########################################
#
#  hashes/pmp_multilinear.cpp
  hashes/xxhash.cpp
  hashes/t1ha.cpp
  hashes/farmhash.cpp
  hashes/halftimehash.cpp
  hashes/blake2.cpp
  hashes/blake3.cpp
  hashes/umash.cpp
  hashes/mum_mir.cpp
  hashes/cityhash.cpp
  hashes/nmhash.cpp
  hashes/ascon.cpp
  hashes/rmd.cpp
  hashes/beamsplitter.cpp
  hashes/sha2.cpp
  hashes/sha1.cpp
  hashes/pearson.cpp
  hashes/vmac.cpp
  hashes/clhash.cpp
  hashes/spookyhash.cpp
  hashes/meowhash.cpp
  hashes/metrohash.cpp
  hashes/farsh.cpp
  hashes/siphash.cpp
  hashes/crc.cpp
  hashes/chaskey.cpp
  hashes/tabulation.cpp
  hashes/komihash.cpp
  hashes/multiply_shift.cpp
  hashes/md5.cpp
  hashes/fnv.cpp
  hashes/hasshe2.cpp
  hashes/prvhash.cpp
  hashes/murmurhash3.cpp
  hashes/wyhash.cpp
  hashes/discohash.cpp
  hashes/sha3.cpp
  hashes/poly_mersenne.cpp
  hashes/murmurhash2.cpp
  hashes/falkhash.cpp
  hashes/aquahash.cpp
  hashes/fletcher.cpp
  hashes/crap.cpp
  hashes/blockpearson.cpp
  hashes/perlhashes.cpp
  hashes/jodyhash.cpp
  hashes/khash.cpp
  hashes/floppsyhash.cpp
  hashes/seahash.cpp
  hashes/rainstorm.cpp
  hashes/rainbow.cpp
  hashes/falcon_oaat.cpp
  hashes/lookup3.cpp
  hashes/fasthash.cpp
  hashes/aesnihash-peterrk.cpp
  hashes/aesnihash-majek.cpp
  hashes/o1hash.cpp
  hashes/pengyhash.cpp
  hashes/mx3.cpp
  hashes/superfasthash.cpp
  hashes/khashv.cpp
  hashes/murmurhash1.cpp
  hashes/murmur_oaat.cpp
  hashes/x17.cpp
  hashes/highwayhash.cpp
  hashes/abseil.cpp
  hashes/polymur.cpp
)
