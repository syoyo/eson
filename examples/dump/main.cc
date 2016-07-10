#include "eson.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>

namespace {

void
PrintIndent(int indent)
{
  for (int i = 0; i < indent; i++) {
    printf("  ");
  }
}

void
PrintEntity(int indent, eson::Value& p)
{
  PrintIndent(indent);

}

static void VisitValue(const eson::Value& p, int indent);

void
VisitObject(const eson::Value& p, int indent)
{
  assert(p.IsObject());
  std::vector<std::string> keys;
  keys = p.Keys();

  PrintIndent(indent); printf("[Object]\n");

  for (size_t i = 0; i < keys.size(); i++) {
    const eson::Value& v = p.Get(keys[i]);
    PrintIndent(indent+1); printf("%s\n", keys[i].c_str());
    VisitValue(v, indent+2);
  }
}

void
VisitArray(const eson::Value& p, int indent)
{
  assert(p.IsArray());

  PrintIndent(indent); printf("[Array]\n");

  for (size_t i = 0; i < p.ArrayLen(); i++) {
    const eson::Value& v = p.Get(i);
    VisitValue(v, indent+1);
  }
}

void
VisitValue(const eson::Value& p, int indent)
{
  if (p.IsObject()) { VisitObject(p, indent); }
  else if (p.IsArray()) { VisitArray(p, indent); }
  else if (p.IsBinary()) { 
    PrintIndent(indent); printf("[Binary] length = %lld\n", p.Size());
  } else if (p.IsFloat64()) { 
    PrintIndent(indent); printf("%lf(float64)\n", p.Get<double>());
  } else if (p.IsInt64()) { 
    PrintIndent(indent); printf("%lld(int64)\n", p.Get<int64_t>());
  } else if (p.IsString()) { 
    PrintIndent(indent); printf("\"%s\"\n", p.Get<std::string>().c_str());
  } else if (p.IsBool()) { 
    bool ret = p.Get<bool>();
    if (ret == true) {
      PrintIndent(indent); printf("true\n");
    } else {
      PrintIndent(indent); printf("false\n");
    }
  } else {
    printf("ty: %d\n", p.Type());    
    assert(0); // ???
  }
}

}  // namespace

int
main(
  int argc,
  char** argv)
{
  if (argc < 2) {
    printf("Usage: %s input.eson\n", argv[0]);
    exit(1);
  }

  // @todo { Use mmap() }
  std::vector<uint8_t> buf;

  FILE* fp = fopen(argv[1], "rb");
  fseek(fp, 0, SEEK_END);
  size_t len = ftell(fp);
  rewind(fp);
  buf.resize(len);
  len = fread(&buf[0], 1, len, fp);
  fclose(fp);

  eson::Value v;

  std::string err = eson::Parse(v, &buf[0]);
  if (!err.empty()) {
    std::cout << "Err: " << err << std::endl;
    exit(1);
  }

  VisitValue(v, 0);

  return 0;
}
