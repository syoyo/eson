# ESON, Exa-scale Storage Object Notation

ESON is simple but powerful schema-less binary data format designed to handle Exa-scale data. Example includes graphics(e.g. geometry, volume and textures) and may also applicable for in-memory database and scientific data.

ESON is also designed to handle large-scale data efficiently for comming NVM(non-volatile memory) or SCM(storage-class memory) era.

C++ API is primarily provided.

# Version

0.2.0 (Jan 2015)
0.1.0 (Jul 2013)

# Status

Very unstable. Spec and API will change in the future.

## Quick tutorial

    $ make
    $ ./eson_test

## Specification

See [SPECIFICATION.md](SPECIFICATION.md)

## Design and implementation references

ESON design is strongly affected by

* BSON http://bsonspec.org/

ESON C++ API is strongly affected by

* picojson https://github.com/kazuho/picojson

## Example

```
#include "eson.h"

#include <iostream>
#include <cstdlib>
#include <cstdio>

static void
ESONTest()
{
  eson::Value v;
  double dbl = 1.234;
  eson::Value vd(dbl);

  double dbl2 = 3.4;
  eson::Value vd2(dbl2);

  int64_t i = 144;
  eson::Value ival(i);

  std::string name("jojo");
  eson::Value sval(name);

  char bindata[12];
  for (int i = 0; i < 12; i++) {
    bindata[i] = i;
  }
  eson::Value bval((const uint8_t*)bindata, 12);

  eson::Object o;
  o["abora"] = vd;
  o["muda"] = vd2;
  o["dora"] = ival;
  o["name"] = sval;
  o["bin"] = bval;

  v = eson::Value(o);

  // First calcuate required size for serialized data.
  int64_t sz = v.Size();

  uint8_t* buf = new uint8_t[sz]; // or use mmap() if sz is large.
  uint8_t* ptr = &buf[0];

  ptr = v.Serialize(ptr);
  assert((ptr-&buf[0]) == sz);

  FILE* fp = fopen("output.eson", "wb");
  fwrite(buf, 1, sz, fp);
  fclose(fp);

  eson::Value ret;
  std::string err = eson::Parse(ret, buf);
  if (!err.empty()) {
    std::cout << "err:" << err << std::endl;
  }

  eson::Value dval = ret.Get("muda");
  printf("muda = %f\n", dval.Get<double>());

  eson::Binary bin = ret.Get("bin").Get<eson::Binary>();
  printf("bin len = %d\n", bin.size);
  for (int i = 0; i < bin.size; i++) {
    printf("    bin[%d] = %d\n", i, bin.ptr[i]);
  } 

  delete buf;
}
```

## TODO

* Efficiently serialize key table for better search performance.
* Make API Zero-Copy to reduce memory.

## Compression

Currently we are planning to use lz4 compression for lossless binary data.
Lossy compression is also planned. Sengcom seems good idea for lossy compression.

* Wavelet compression for floating point data – Sengcom http://www.unidata.ucar.edu/software/netcdf/papers/sengcom.pdf

## Author(s)

Syoyo Fujita(syoyo@lighttransport.com)

## License

ESON C++ API library is licensed under 3-clause BSD license.

### Third-party licenses

* lz4 is licensed under 2-clause BSD license.
