#include "eson.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>

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

  int64_t num_vertices = v.Get("num_vertices").Get<int64_t>();
  int64_t num_faces = v.Get("num_faces").Get<int64_t>();
  printf("# of vertices: %lld\n", num_vertices);
  printf("# of faces   : %lld\n", num_faces);

  eson::Binary vertices_data = v.Get("vertices").Get<eson::Binary>();
  const float* vertices = reinterpret_cast<float*>(const_cast<uint8_t*>(vertices_data.ptr));

  eson::Binary faces_data = v.Get("faces").Get<eson::Binary>();
  const int* faces = reinterpret_cast<int*>(const_cast<uint8_t*>(faces_data.ptr));

  for (int64_t i = 0; i < num_vertices; i++) {
    printf("  vtx[%lld] = %f, %f, %f\n", i, vertices[3*i+0], vertices[3*i+1], vertices[3*i+2]);
  }

  for (int64_t i = 0; i < num_faces; i++) {
    printf("  face[%lld] = %d, %d, %d\n", i, faces[3*i+0], faces[3*i+1], faces[3*i+2]);
  }

  return 0;
}
