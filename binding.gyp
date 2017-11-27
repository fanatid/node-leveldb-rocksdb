{
  "targets": [{
    "target_name": "node-leveldb-rocksdb",
    "sources": [
      "./src/addon.cc",
      "./src/leveldb.cc"
    ],
    "include_dirs": [
      "./deps/leveldb/include",
      "./deps/rocksdb/include",
      "<!(node -e \"require('nan')\")"
    ],
    "defines": [
    ],
    "cflags": [
      "-Wall",
      "-Wno-maybe-uninitialized",
      "-Wno-uninitialized",
      "-Wno-unused-function",
      "-Wextra"
    ],
    "libraries": [
      "<!(pwd)/deps/leveldb/out-static/libleveldb.a",
      "<!(pwd)/deps/rocksdb/librocksdb.a"
    ]
  }]
}
