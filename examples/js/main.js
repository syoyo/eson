//
// Read ESON file in Node.js 
//
var eson = require('../../eson-binary.js');
var fs = require('fs');

if (process.argv.length < 3) {
  console.log("needs input.eson");
  process.exit(-1);
}

var buf = fs.readFileSync(process.argv[2])
var b = eson.parse(buf);

console.log(b)
