var Int64 = require('node-int64');

exports.parse = function(/* Buffer */ buffer) {
 
  var parseBinary = function(buf, offset) {

    // @note { Assume binary size is less than 2GB. } 

    var N0 = buf.readUInt32LE(offset+0);
    var N1 = buf.readUInt32LE(offset+4);

    var data = buf.slice(offset+8, offset+8+N0);

    return {n: N0, data: data}
  }

  var parseArray = function(buf, offset) {

    // @note { Assume data size is less than 2GB. } 

    var N0 = buf.readUInt32LE(offset+0);
    var N1 = buf.readUInt32LE(offset+4);

    var data = buf.slice(offset+8, offset+8+N0);

    return {n: N0, data: null} // @todo
  }

  var parseString = function(buf, offset) {

    var N0 = buf.readUInt32LE(offset+0);
    var N1 = buf.readUInt32LE(offset+4);

    var data = buf.slice(offset+8, offset+8+N0);

    return { n: N0, str: data.toString('utf8') }
  }

  var parseKey = function(buf, offset) {
    var key = []
    var i = 0;
    while (true) {
      var c = buf.readUInt8(offset+i);
      i = i + 1;
      key.push(c)
      if (c == 0) {
        break;
      }
    }

    var s = new Buffer(key.length);
    for (var i = 0; i < key.length; i++) {
      s[i] = key[i];
    }

    return {n: key.length, str: s.toString('utf8')}
  }

  var parseElement = function(buf, offset) {

    var elemID = buf.readUInt8(offset);

    var e = 'unknown';
    if (elemID == 0) {
      e = 'end';
    } else if (elemID == 1) {
      e = 'float64';
    } else if (elemID == 2) {
      e = 'int64';
    } else if (elemID == 3) {
      e = 'boolean';
    } else if (elemID == 4) {
      e = 'string';
    } else if (elemID == 5) {
      e = 'array';
    } else if (elemID == 6) {
      e = 'binary';
    } else if (elemID == 6) {
      e = 'object';
    }
    
    return e;
      
  }

  function parseESONObject(buf, offset)
  {

    // @note { Assume eson object is less than 2GB }
    var N0 = buf.readUInt32LE(offset+0);
    var N1 = buf.readUInt32LE(offset+4);
    var N = new Int64(N1, N0);

    if (N0 == 0) {
      return null;
    }

    offset += 8

    var elems = []

    var Ntotal = N0;

    while (offset < Ntotal) {

      var e = parseElement(buf, offset);

      if (e == 'end') {
        return elems
      }

      var k = parseKey(buf, offset+1);

      var ret = {};
      ret['element'] = e;
      ret['key'] = k;

      offset += 1 + k.n;

      if (e == 'float64') {
        var val = buf.readDoubleLE(offset);
        offset += 8 + 8;
        ret['value'] = val;
      } else if (e == 'int64') {
        var N0 = buf.readUInt32LE(offset+0);
        var N1 = buf.readUInt32LE(offset+4);
        var val = new Int64(N1, N0);
        offset += 8;
        ret['value'] = val;
      } else if (e == 'boolean') {
        var b = buf.readUInt8(offset);
        var val = (b == 0) ? false : true;
        offset += 1;
        ret['value'] = val;
      } else if (e == 'string') {
        var b = parseString(buf, offset)
        offset += b.n+8;
        ret['value'] = b;
      } else if (e == 'binary') {
        var b = parseBinary(buf, offset)
        offset += b.n+8;
        ret['value'] = b;
      } else if (e == 'array') {
        var b = parseArray(buf, offset)
        offset += b.n+8;
        ret['value'] = b;
      } else if (e == 'object') {
        var b = parseESONObject(buf, offset)
        offset += b.n+8;
        ret['value'] = b;
      }

      elems.push(ret);

    }

    return elems;

  }

  var elements = parseESONObject(buffer, 0);

  return elements

}

