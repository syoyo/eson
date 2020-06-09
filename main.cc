#define ESON_IMPLEMENTATION
#include "eson.h"

#include <iostream>
#include <cstdlib>
#include <cstdio>

static void
ESONTest()
{
  double dbl = 1.234;
  eson::Value vd(dbl);

  double dbl2 = 3.4;
  eson::Value vd2(dbl2);

  int64_t i = 144;
  eson::Value ival(i);

  std::string name("jojo");
  eson::Value sval(name);

  char bindata[12];
  for (int j = 0; j < 12; j++) {
    bindata[j] = static_cast<char>(j);
  }
  eson::Value bval(reinterpret_cast<const uint8_t*>(bindata), 12);

  eson::Object subO;
  subO["muda"] = vd2;

  eson::Object o;
  o["abora"] = vd;
  o["dora"] = ival;
  o["name"] = sval;
  o["bin"] = bval;
  o["sub"] = eson::Value(subO); // compound object.

  eson::Value ret;
  {
    eson::Value v = eson::Value(o);

    // First calcuate required size for serialized data.
    int64_t sz = static_cast<int64_t>(v.Size());
    std::cout << "sz = " << sz << "\n";

    uint8_t* buf = new uint8_t[size_t(sz)]; // or use mmap() if sz is large.
    uint8_t* ptr = &buf[0];

    ptr = v.Serialize(ptr);
    assert((ptr-&buf[0]) == sz);

    FILE* fp = fopen("output.eson", "wb");
    fwrite(buf, 1, static_cast<size_t>(sz), fp);
    fclose(fp);

    std::string err = eson::Parse(ret, static_cast<const uint8_t*>(buf));
    if (!err.empty()) {
      std::cout << "err:" << err << std::endl;
    }
    delete [] buf;

    eson::Value doval = ret.Get("dora");
    printf("dora = %f\n", doval.Get<double>());

    eson::Value dval = ret.Get("muda");
    printf("muda = %f\n", dval.Get<double>());

    eson::Value sub = ret.Get("sub");
    printf("sub.isObj = %d\n", sub.IsObject());

    eson::Binary bin = ret.Get("bin").Get<eson::Binary>();
    assert(bin.size == 12);
    printf("bin len = %ld\n", bin.size);
    for (int j = 0; j < bin.size; j++) {
      printf("    bin[%d] = %d\n", j, bin.ptr[j]);
    }

    if (ret.Get("bin1").IsBinary()) {
      eson::Binary bin1 = ret.Get("bin1").Get<eson::Binary>();
      printf("bin1.size = %d\n", static_cast<int>(bin1.size));
      //assert(bin1.size == 0);
    }
    assert(ret.Get("bin1").IsBinary() == false);
  }

  // append & modify
  {
    eson::Value v2;
    v2.DeepCopyFrom(ret.Get<eson::Object>());

    o2["dora"] = eson::Value(int64_t(2)); // modify
    o2["ari"] = eson::Value(3.14); // append

    eson::Value v = eson::Value(o2);

    // First calcuate required size for serialized data.
    int64_t sz = static_cast<int64_t>(v.Size());
    std::cout << "sz = " << sz << "\n";

    uint8_t* buf = new uint8_t[size_t(sz)]; // or use mmap() if sz is large.
    uint8_t* ptr = &buf[0];

    ptr = v.Serialize(ptr);
    assert((ptr-&buf[0]) == sz);

    FILE* fp = fopen("appended_output.eson", "wb");
    fwrite(buf, 1, static_cast<size_t>(sz), fp);
    fclose(fp);
    delete [] buf;
  }


}

int
main(
  int argc,
  char** argv)
{
  (void)argc;
  (void)argv;
  printf("Testing ESON C++ binding...\n");
  ESONTest();
  printf("Test DONE\n");

  return EXIT_SUCCESS;
}
