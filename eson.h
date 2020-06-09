/*
MIT License

Copyright (c) 2013-2020 Light Transport Entertainment Inc. and contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#ifndef ESON_H_
#define ESON_H_

#include <stdint.h>

#include <cassert>
#include <cstdio>
#include <cstring>
#include <cstdlib>

#include <map>
#include <string>
#include <vector>

namespace eson {

typedef enum {
  NULL_TYPE = 0,
  FLOAT64_TYPE = 1,
  INT64_TYPE = 2,
  BOOL_TYPE = 3,
  STRING_TYPE = 4,
  ARRAY_TYPE = 5,
  BINARY_TYPE = 6,
  OBJECT_TYPE = 7
} Type;

class Value {
 public:
  typedef struct {
    const uint8_t *ptr;
    int64_t size;
    bool is_external; // true: Do not free `ptr`. false: free `ptr` in destructor.
    int8_t __pad[7];
  } Binary;

  typedef std::vector<Value> Array;
  typedef std::map<std::string, Value> Object;

 protected:
  int type_;  // Data type
  int pad0_;
  mutable uint64_t size_;  // Data size
  mutable bool dirty_;

  // union {
  bool boolean_;
  char pad6_[6];
  int64_t int64_;
  double float64_;
  std::string string_;
  Binary binary_;
  Array array_;
  Object object_;
  //};

 public:
  Value() : type_(NULL_TYPE), dirty_(true) {}
  Value(const Value &rhs);
  Value &operator=(const Value &rhs);

  ///
  /// Deep copy Value object.
  ///
  /// allocate binary for BINARY_TYPE data.
  ///
  void DeepCopyFrom(const Value &rhs);

  explicit Value(bool b) : type_(BOOL_TYPE), dirty_(false) {
    boolean_ = b;
    size_ = 1;
  }
  explicit Value(int64_t i) : type_(INT64_TYPE), dirty_(false) {
    int64_ = i;
    size_ = 8;
  }
  explicit Value(double n) : type_(FLOAT64_TYPE), dirty_(false) {
    float64_ = n;
    size_ = 8;
  }
  explicit Value(const std::string &s) : type_(STRING_TYPE), dirty_(false) {
    string_ = std::string(s);
    size_ = string_.size();
  }
  explicit Value(const uint8_t *p, uint64_t n, bool is_external = true)
      : type_(BINARY_TYPE), dirty_(false) {
    binary_ = Binary();
    if (is_external) {
      binary_.ptr = p;  // Just save a pointer.
    } else {
      binary_.ptr = new uint8_t[n];
      memcpy(const_cast<uint8_t *>(binary_.ptr), p, n);
    }
    binary_.size = static_cast<int64_t>(n);
    binary_.is_external = is_external;
    size_ = n;
  }
  explicit Value(const Array &a) : type_(ARRAY_TYPE), dirty_(true) {
    array_ = Array(a);
    size_ = ComputeArraySize();
  }
  explicit Value(const Object &o) : type_(OBJECT_TYPE), dirty_(true) {
    object_ = Object(o);
    size_ = ComputeObjectSize();
  }
  ~Value();

  /// Compute size of array element.
  uint64_t ComputeArraySize() const {
    assert(type_ == ARRAY_TYPE);

    assert(array_.size() > 0);

    char base_element_type = array_[0].Type();

    //
    // Elements in the array must be all same type.
    //

    uint64_t sum = 0;
    for (size_t i = 0; i < array_.size(); i++) {
      char element_type = array_[i].Type();
      assert(base_element_type == element_type);
      (void)element_type;
      sum += array_[i].ComputeSize();
    }
    (void)base_element_type;

    return sum + 1 + sizeof(int64_t);
  }

  /// Compute object size.
  uint64_t ComputeObjectSize() const {
    assert(type_ == OBJECT_TYPE);

    uint64_t object_size = 0;

    for (Object::const_iterator it = object_.begin(); it != object_.end();
         ++it) {
      const std::string &key = it->first;
      uint64_t key_len = key.length() + 1;  // + '\0'
      uint64_t data_len = it->second.ComputeSize();
      object_size += key_len + data_len + 1;  // +1 = tag size.
    }

    return object_size;
  }

  /// Compute data size.
  uint64_t ComputeSize() const {
    switch (type_) {
      case INT64_TYPE:
        return 8;
        break;
      case FLOAT64_TYPE:
        return 8;
        break;
      case STRING_TYPE:
        return string_.size() + sizeof(int64_t);  // N + str data
        break;
      case BINARY_TYPE:
        return size_ + sizeof(int64_t);  // N + bin data
        break;
      case ARRAY_TYPE:
        return ComputeArraySize() + sizeof(int64_t);  // datalen + N
        break;
      case OBJECT_TYPE:
        return ComputeObjectSize() + sizeof(int64_t);  // datalen + N
        break;
      default:
        assert(0);
        break;
    }
    assert(0);
    return 0;  // Never come here.
  }

  uint64_t Size() const {
    if (!dirty_) {
      return size_;
    } else {
      // Recompute data size.
      size_ = ComputeSize();
      dirty_ = false;
      return size_;
    }
  }

  char Type() const { return static_cast<const char>(type_); }

  bool IsBool() const { return (type_ == BOOL_TYPE); }

  bool IsInt64() const { return (type_ == INT64_TYPE); }

  bool IsFloat64() const { return (type_ == FLOAT64_TYPE); }

  bool IsString() const { return (type_ == STRING_TYPE); }

  bool IsBinary() const { return (type_ == BINARY_TYPE); }

  bool IsArray() const { return (type_ == ARRAY_TYPE); }

  bool IsObject() const { return (type_ == OBJECT_TYPE); }

  // Accessor
  template <typename T>
  const T &Get() const;
  template <typename T>
  T &Get();

  // Lookup value from an array
  const Value &Get(int64_t idx) const {
    static Value &null_value = *(new Value());
    assert(IsArray());
    assert(idx >= 0);
    return (static_cast<uint64_t>(idx) < array_.size())
               ? array_[static_cast<uint64_t>(idx)]
               : null_value;
  }

  // Lookup value from a key-value pair
  const Value &Get(const std::string &key) const {
    static Value &null_value = *(new Value());
    assert(IsObject());
    Object::const_iterator it = object_.find(key);
    return (it != object_.end()) ? it->second : null_value;
  }

  size_t ArrayLen() const {
    if (!IsArray()) return 0;
    return array_.size();
  }

  // Valid only for object type.
  bool Has(const std::string &key) const {
    if (!IsObject()) return false;
    Object::const_iterator it = object_.find(key);
    return (it != object_.end()) ? true : false;
  }

  // List keys
  std::vector<std::string> Keys() const {
    std::vector<std::string> keys;
    if (!IsObject()) return keys;  // empty

    for (Object::const_iterator it = object_.begin(); it != object_.end();
         ++it) {
      keys.push_back(it->first);
    }

    return keys;
  }

  // Serialize data to memory 'p'.
  // Memory of 'p' must be allocated by app before calling this function.
  // (size can be obtained by calling 'Size' function.
  // Return next data location.
  uint8_t *Serialize(uint8_t *p) const;

 private:

  void Copy_(const Value &rhs, bool is_deep);

  static Value null_value();
};

// Alias
typedef Value::Array Array;
typedef Value::Object Object;
typedef Value::Binary Binary;

#define GET(ctype, var)                           \
  template <>                                     \
  inline const ctype &Value::Get<ctype>() const { \
    return var;                                   \
  }                                               \
  template <>                                     \
  inline ctype &Value::Get<ctype>() {             \
    return var;                                   \
  }
GET(bool, boolean_)
GET(double, float64_)
GET(int64_t, int64_)
GET(std::string, string_)
GET(Binary, binary_)
GET(Array, array_)
GET(Object, object_)
#undef GET

// Deserialize data from memory 'p'.
// Returns error string. Empty if success.
std::string Parse(Value &v, const uint8_t *p);
std::string Parse(Array &v, const uint8_t *p);

class ESON {
 public:
  ESON();
  ~ESON();

  /// Load data from a file.
  bool Load(const char *filename);

  /// Dump data to a file.
  bool Dump(const char *filename);

 private:
  uint8_t *data_;  /// Pointer to data
  uint64_t size_;  /// Total data size

  bool valid_;
};

}  // namespace eson

#ifdef ESON_IMPLEMENTATION
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <sstream>

#ifdef _WIN32
#include <windows.h>  //  File mapping
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace eson {

Value::~Value()
{
  if (type_ == BINARY_TYPE) {
    if (binary_.ptr && (!binary_.is_external)) {
      delete [] binary_.ptr;
    }
  }

}

void Value::Copy_(const Value &rhs, bool is_deep) {

  type_ = rhs.type_;
  size_ = rhs.size_;
  dirty_ = rhs.dirty_;

  switch (type_) {
    case FLOAT64_TYPE: {
      float64_ = rhs.float64_;
    } break;
    case INT64_TYPE: {
      int64_ = rhs.int64_;
    } break;
    case STRING_TYPE: {
      string_ = rhs.string_;
    } break;
    case BINARY_TYPE: {
      if ((!is_deep) && rhs.binary_.is_external) {
        // just copy pointer
        binary_.ptr = rhs.binary_.ptr;
      } else {
        memcpy(const_cast<uint8_t *>(binary_.ptr), rhs.binary_.ptr, size_t(rhs.binary_.size));
      }
      binary_.size = rhs.binary_.size;
      binary_.is_external = is_deep ? false : rhs.binary_.is_external;
    } break;
    case OBJECT_TYPE: {
      for (Object::const_iterator it = rhs.object_.begin(); it != rhs.object_.end();
           ++it) {
        object_[it->first] = it->second;
      }
    } break;
    case NULL_TYPE: {
    } break;
    case BOOL_TYPE: {
      // @todo
      assert(0);
    } break;
    case ARRAY_TYPE: {
      // @todo
      assert(0);
    } break;
  }
}

void DeepCopyFrom(const Value &rhs);

Value::Value(const Value &rhs) {
  Copy_(rhs, false);
}

Value &Value::operator=(const Value &rhs) {
  Copy_(rhs, false);

  return *this;
}

uint8_t *Value::Serialize(uint8_t *p) const {
  uint8_t *ptr = p;
  switch (type_) {
    case FLOAT64_TYPE:
      memcpy(ptr, &float64_, sizeof(double));
      ptr += sizeof(double);
      break;
    case INT64_TYPE: {
      // (*(reinterpret_cast<int64_t *>(ptr))) = int64_;
      memcpy(ptr, &int64_, sizeof(int64_t));
      ptr += sizeof(int64_t);
    } break;
    case STRING_TYPE: {
      // len(64bit) + string
      // (*(reinterpret_cast<int64_t *>(ptr))) = size_;
      memcpy(ptr, &size_, sizeof(int64_t));
      ptr += sizeof(int64_t);
      memcpy(ptr, string_.c_str(), size_);
      ptr += size_;
    } break;
    case BINARY_TYPE: {
      // len(64bit) + bindata
      // (*(reinterpret_cast<int64_t *>(ptr))) = size_;
      memcpy(ptr, &size_, sizeof(int64_t));
      ptr += sizeof(int64_t);
      memcpy(ptr, binary_.ptr, size_);
      ptr += size_;
    } break;
    case OBJECT_TYPE: {
      // (*(reinterpret_cast<int64_t *>(ptr))) = size_;
      memcpy(ptr, &size_, sizeof(int64_t));
      ptr += sizeof(int64_t);

      // Serialize key-value pairs.
      for (Object::const_iterator it = object_.begin(); it != object_.end();
           ++it) {
        // Emit type tag.
        char ty = static_cast<char>(it->second.type_);
        (*(reinterpret_cast<char *>(ptr))) = ty;
        ptr++;

        // Emit key
        const std::string &key = it->first;
        memcpy(ptr, key.c_str(), key.size());
        ptr += key.size();
        (*(reinterpret_cast<char *>(ptr))) = '\0';  // null terminate
        ptr++;

        // Emit element
        ptr = it->second.Serialize(ptr);
      }
    } break;
    case ARRAY_TYPE: {
      // (*(reinterpret_cast<int64_t *>(ptr))) = size_;
      memcpy(ptr, &size_, sizeof(int64_t));
      ptr += sizeof(int64_t);

      char ty = static_cast<char>(type_);
      (*(reinterpret_cast<char *>(ptr))) = ty;
      ptr++;

      // (*(reinterpret_cast<int64_t *>(ptr))) = array_.size();
      uint64_t arraySize = array_.size();
      memcpy(ptr, &arraySize, sizeof(int64_t));
      ptr += sizeof(int64_t);

      for (size_t i = 0; i < array_.size(); i++) {
        ptr = array_[i].Serialize(ptr);
      }
    } break;
    default:
      assert(0);
      break;
  }

  return ptr;
}

// Forward decl.
static const uint8_t *ParseElement(std::stringstream &err, Object &o,
                                   const uint8_t *p);

static const uint8_t *ReadKey(std::string &key, const uint8_t *p) {
  key = std::string(reinterpret_cast<const char *>(p));

  p += key.length() + 1;  // + '\0'

  return p;
}

static const uint8_t *ReadFloat64(double &v, const uint8_t *p) {
  double val = 0.0;
  memcpy(&val, p, sizeof(double));
  v = val;
  p += sizeof(double);

  return p;
}

static const uint8_t *ReadInt64(int64_t &v, const uint8_t *p) {
  int64_t val = 0;
  memcpy(&val, p, sizeof(int64_t));
  v = val;
  p += sizeof(int64_t);

  return p;
}

static const uint8_t *ReadString(std::string &s, const uint8_t *p) {
  // N + string data.
  int64_t val;
  memcpy(&val, p, sizeof(int64_t));
  int64_t n = val;
  p += sizeof(int64_t);

  assert(n >= 0);

  s = std::string(reinterpret_cast<const char *>(p), static_cast<size_t>(n));
  p += n;

  return p;
}

static const uint8_t *ReadBinary(const uint8_t *&b, int64_t &n,
                                 const uint8_t *p) {
  // N + bin data.
  int64_t val;
  memcpy(&val, p, sizeof(int64_t));

  n = val;
  p += sizeof(int64_t);

  assert(n >= 0);

  // Just returns pointer address.
  b = p;
  p += n;

  return p;
}

static const uint8_t *ReadObject(Object &o, const uint8_t *p) {
  // N + object data.
  int64_t val;
  memcpy(&val, p, sizeof(int64_t));
  int64_t n = val;
  p += sizeof(int64_t);

  assert(n >= 0);
  (void)n;

  std::stringstream ss;
  const uint8_t *ptr = ParseElement(ss, o, p);

  return ptr;
}

static const uint8_t *ParseElement(std::stringstream &err, Object &o,
                                   const uint8_t *p) {
  const uint8_t *ptr = p;
  (void)err;

  // Read tag;
  Type type = static_cast<Type>(*(reinterpret_cast<const char *>(ptr)));
  ptr++;

  std::string key;
  ptr = ReadKey(key, ptr);

  switch (type) {
    case FLOAT64_TYPE: {
      double val;
      ptr = ReadFloat64(val, ptr);
      o[key] = Value(val);
    } break;
    case INT64_TYPE: {
      int64_t val;
      ptr = ReadInt64(val, ptr);
      o[key] = Value(val);
    } break;
    case STRING_TYPE: {
      std::string str;
      ptr = ReadString(str, ptr);
      o[key] = Value(str);
    } break;
    case BINARY_TYPE: {
      const uint8_t *bin_ptr;
      int64_t bin_size;
      ptr = ReadBinary(bin_ptr, bin_size, ptr);
      o[key] = Value(bin_ptr, static_cast<uint64_t>(bin_size));
    } break;
    case OBJECT_TYPE: {
      Object obj;
      ptr = ReadObject(obj, ptr);

      o[key] = Value(obj);
    } break;
    case NULL_TYPE: {
    } break;
    case BOOL_TYPE: {
      // @todo
      assert(0);
    } break;
    case ARRAY_TYPE: {
      // @todo
      assert(0);
    } break;
  }

  return ptr;
}

static const uint8_t *ParseElement(std::stringstream &err, Value &v,
                                   const uint8_t *ptr) {
  const uint8_t *p = ptr;
  // Read total size.
  int64_t val = 0;
  memcpy(&val, ptr, sizeof(int64_t));
  int64_t sz = val;
  ptr += sizeof(int64_t);
  assert(sz > 0);

  Object obj;

  int64_t read_size = 0;
  while (read_size < sz) {
    ptr = ParseElement(err, obj, ptr);
    read_size = ptr - p;
  }
  v = Value(obj);
  return ptr;
}

static const uint8_t *ParseElement(std::stringstream &err, Array &o,
                                   const uint8_t *p) {
  const uint8_t *ptr = p;

  // Read tag;
  Type type = static_cast<Type>(*(reinterpret_cast<const char *>(ptr)));
  ptr++;

  std::string key;
  if (type != ARRAY_TYPE) ptr = ReadKey(key, ptr);

  assert(type == ARRAY_TYPE);

  int64_t nElems;
  ptr = ReadInt64(nElems, ptr);

  for (size_t i = 0; i < static_cast<size_t>(nElems); i++) {
    Value value;
    ptr = ParseElement(err, value, ptr);
    o.push_back(value);
  }

  return ptr;
}

std::string Parse(Value &v, const uint8_t *p) {
  std::stringstream err;

  const uint8_t *ptr = p;

  //
  // == toplevel element
  //

  // Read total size.
  int64_t val = 0;
  memcpy(&val, ptr, sizeof(int64_t));
  int64_t sz = val;
  ptr += sizeof(int64_t);
  assert(sz > 0);

  Object obj;

  int64_t read_size = 0;
  while (read_size < sz) {
    ptr = ParseElement(err, obj, ptr);
    read_size = ptr - p;
  }

  v = Value(obj);

  return err.str();
}

std::string Parse(Array &v, const uint8_t *p) {
  std::stringstream err;

  const uint8_t *ptr = p;

  //
  // == toplevel element
  //

  // Read total size.
  int64_t val = 0;
  memcpy(&val, ptr, sizeof(int64_t));
  int64_t sz = val;
  ptr += sizeof(int64_t);
  assert(sz > 0);

  int64_t read_size = 0;
  while (read_size < sz) {
    ptr = ParseElement(err, v, ptr);
    read_size = ptr - p;
  }

  return err.str();
}

}  // namespace eson
#endif

#endif  // ESON_H_
