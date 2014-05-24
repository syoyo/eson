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

  eson::Object subO;
  subO["muda"] = vd2;

  eson::Object o;
  o["abora"] = vd;
  o["dora"] = ival;
  o["name"] = sval;
  o["bin"] = bval;
  o["sub"] = eson::Value(subO); // compound object.

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
  std::string err = eson::Parse(ret, (const uint8_t*)buf);
  if (!err.empty()) {
    std::cout << "err:" << err << std::endl;
  }

  eson::Value doval = ret.Get("dora");
  printf("dora = %f\n", doval.Get<double>());

  eson::Value dval = ret.Get("muda");
  printf("muda = %f\n", dval.Get<double>());

  eson::Value sub = ret.Get("sub");
  printf("sub.isObj = %d\n", sub.IsObject());

  eson::Binary bin = ret.Get("bin").Get<eson::Binary>();
  assert(bin.size == 12);
  printf("bin len = %lld\n", bin.size);
  for (int i = 0; i < bin.size; i++) {
    printf("    bin[%d] = %d\n", i, bin.ptr[i]);
  } 

  if (ret.Get("bin1").IsBinary()) {
    eson::Binary bin1 = ret.Get("bin1").Get<eson::Binary>();
    printf("bin1.size = %d\n", (int)bin1.size);
    //assert(bin1.size == 0);
  }
  assert(ret.Get("bin1").IsBinary() == false);

  delete buf;
}

int
main(
  int argc,
  char** argv)
{
  printf("Testing ESON C++ binding...\n");
  ESONTest();
  printf("Test DONE\n");

  return EXIT_SUCCESS;
}
