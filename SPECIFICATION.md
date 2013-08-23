# ESON speficifation

## Version

0.1.0 (Jul 2013)

## Overview

ESON is a Exa-scale binary format in which key-value pairs are stored.

ESON format is strongly affected by BSON, and ESON can be considered 64bit binary extension of BSON. So readers are welcome to refer BSON specification (http://bsonspec.org/)

## Specification

### Endian

ESON uses `little-endian`.

### Types

type    | bytes  | comment
--------|--------|--------------------------------
byte	| 1      | 8-bits byte
int64	| 8      | 64-bit signed integer
double	| 8      | 64-bit IEEE 754 floating point


### Format

symbol       |    | expression                | comment
-------------|----|---------------------------|-------------------------------------------------------------------
document     | := | int64 elems               | ESON document. The int64 is total number of bytes in the document.
elems        | := | element elems             | Sequence of elements
             | :  | nil                       | 
element      | := | "\x01" key double         | floating point value
             | :  | "\x02" key int64          | integer value
             | :  | "\x03" key byte           | bool value
             | :  | "\x04" key string         | UTF-8 string
             | :  | "\x05" key int64 document | Array value(not yet implemented)
             | :  | "\x06" key binary         | Binary value
             | :  | "\x07" key document       | Object value
key          | :  | chars + '\0'              | Null terminated string
string       | := | N chars                   | Number of chars(int64) + char(byte) array
binary       | := | N bytes                   | Number of bytes(int64) + byte array
 










