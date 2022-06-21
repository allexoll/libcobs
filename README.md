# cobs_lib

This repository implements a [COBS encoding & decoding](https://en.wikipedia.org/wiki/Consistent_Overhead_Byte_Stuffing) library, with an accumulator for decoding. 


You can compile a unit tester with 
```sh
make CFLAGS="-DCOBS_LIB_UTEST_MAIN"
./cobs_test
```

inspired by [cobs.rs](https://github.com/jamesmunns/cobs.rs) & [postcard-rs](https://github.com/jamesmunns/postcard)