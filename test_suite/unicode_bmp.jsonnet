// This test proves that Jsonnet can handle non "Basic Multilingual Plane" (BMP) characters, in this
// case U+1F4D8 BLUE BOOK
// It is tested via UTF8 and UTF16 and via explicit parseJson as well as basically Jsonnet parsing.
{
  'unicode_bmp1.json (import)': import 'unicode_bmp1.jsonnet.in',
  'unicode_bmp1.json (importstr+parseJson)': std.parseJson(importstr 'unicode_bmp1.jsonnet.in'),
  'unicode_bmp2.json (import)': import 'unicode_bmp2.jsonnet.in',
  'unicode_bmp2.json (importstr+parseJson)': std.parseJson(importstr 'unicode_bmp2.jsonnet.in'),
}
