/*
Copyright 2014 Google Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/


// TODO(dcunnin): width and precision from *

// #
std.assertEqual(std.format("No format chars\n", []), "No format chars\n") &&
std.assertEqual(std.format("", []), "") &&
std.assertEqual(std.format("%#%", []), "%") &&
std.assertEqual(std.format("%# +05.3%", []), "    %") &&
std.assertEqual(std.format("%# -+05.3%", []), "%    ") &&

// s
std.assertEqual(std.format("%s", ["test"]), "test") &&
std.assertEqual(std.format("%s", [true]), "true") &&
std.assertEqual(std.format("%5s", ["test"]), " test") &&

// c
std.assertEqual(std.format("%c", ["a"]), "a") &&
std.assertEqual(std.format("%# +05.3c", ["a"]), "    a") &&
std.assertEqual(std.format("%c", [std.codepoint("a")]), "a") &&

// d (also a quick test of i and u)
std.assertEqual(std.format("thing-%d", [10]), "thing-10") &&
std.assertEqual(std.format("thing-%#ld", [10]), "thing-10") &&
std.assertEqual(std.format("thing-%d", [-10]), "thing--10") &&
std.assertEqual(std.format("thing-%4d", [10]), "thing-  10") &&
std.assertEqual(std.format("thing-%04d", [10]), "thing-0010") &&
std.assertEqual(std.format("thing-% d", [10]), "thing- 10") &&
std.assertEqual(std.format("thing-%- 4d", [10]), "thing- 10 ") &&
std.assertEqual(std.format("thing-% d", [-10]), "thing--10") &&
std.assertEqual(std.format("thing-%4.4d", [10.3]), "thing-  10") &&
std.assertEqual(std.format("thing-%+4.4d", [10.3]), "thing- +10") &&
std.assertEqual(std.format("thing-%+-4.4d", [10.3]), "thing-+10 ") &&
std.assertEqual(std.format("thing-%-4.4d", [10.3]), "thing-10  ") &&
std.assertEqual(std.format("thing-%#-4.4d", [10.3]), "thing-10  ") &&
std.assertEqual(std.format("thing-%#-4.4i", [10.3]), "thing-10  ") &&
std.assertEqual(std.format("thing-%#-4.4u", [10.3]), "thing-10  ") &&

// o
std.assertEqual(std.format("thing-%o", [10]), "thing-12") &&
std.assertEqual(std.format("thing-%lo", [10]), "thing-12") &&
std.assertEqual(std.format("thing-%o", [-10]), "thing--12") &&
std.assertEqual(std.format("thing-%4o", [10]), "thing-  12") &&
std.assertEqual(std.format("thing-%04o", [10]), "thing-0012") &&
std.assertEqual(std.format("thing-% o", [10]), "thing- 12") &&
std.assertEqual(std.format("thing-%- 4o", [10]), "thing- 12 ") &&
std.assertEqual(std.format("thing-% o", [-10]), "thing--12") &&
std.assertEqual(std.format("thing-%4.4o", [10.3]), "thing-  12") &&
std.assertEqual(std.format("thing-%+4.4o", [10.3]), "thing- +12") &&
std.assertEqual(std.format("thing-%+-4.4o", [10.3]), "thing-+12 ") &&
std.assertEqual(std.format("thing-%-4.4o", [10.3]), "thing-12  ") &&
std.assertEqual(std.format("thing-%#o", [10]), "thing-012") &&
std.assertEqual(std.format("thing-%#lo", [10]), "thing-012") &&
std.assertEqual(std.format("thing-%#o", [-10]), "thing--012") &&
std.assertEqual(std.format("thing-%#4o", [10]), "thing- 012") &&
std.assertEqual(std.format("thing-%#04o", [10]), "thing-0012") &&
std.assertEqual(std.format("thing-%# o", [10]), "thing- 012") &&
std.assertEqual(std.format("thing-%#- 4o", [10]), "thing- 012") &&
std.assertEqual(std.format("thing-%# o", [-10]), "thing--012") &&
std.assertEqual(std.format("thing-%#4.4o", [10.3]), "thing- 012") &&
std.assertEqual(std.format("thing-%#+4.4o", [10.3]), "thing-+012") &&
std.assertEqual(std.format("thing-%#+-4.4o", [10.3]), "thing-+012") &&
std.assertEqual(std.format("thing-%#-4.4o", [10.3]), "thing-012 ") &&

// x
std.assertEqual(std.format("thing-%x", [910]), "thing-38e") &&
std.assertEqual(std.format("thing-%lx", [910]), "thing-38e") &&
std.assertEqual(std.format("thing-%x", [-910]), "thing--38e") &&
std.assertEqual(std.format("thing-%5x", [910]), "thing-  38e") &&
std.assertEqual(std.format("thing-%05x", [910]), "thing-0038e") &&
std.assertEqual(std.format("thing-% x", [910]), "thing- 38e") &&
std.assertEqual(std.format("thing-%- 5x", [910]), "thing- 38e ") &&
std.assertEqual(std.format("thing-% x", [-910]), "thing--38e") &&
std.assertEqual(std.format("thing-%5.4x", [910.3]), "thing-  38e") &&
std.assertEqual(std.format("thing-%+5.4x", [910.3]), "thing- +38e") &&
std.assertEqual(std.format("thing-%+-5.4x", [910.3]), "thing-+38e ") &&
std.assertEqual(std.format("thing-%-5.4x", [910.3]), "thing-38e  ") &&
std.assertEqual(std.format("thing-%#x", [910]), "thing-0x38e") &&
std.assertEqual(std.format("thing-%#lx", [910]), "thing-0x38e") &&
std.assertEqual(std.format("thing-%#x", [-910]), "thing--0x38e") &&
std.assertEqual(std.format("thing-%#7x", [910]), "thing-  0x38e") &&
std.assertEqual(std.format("thing-%#07x", [910]), "thing-0x0038e") &&
std.assertEqual(std.format("thing-%# x", [910]), "thing- 0x38e") &&
std.assertEqual(std.format("thing-%#- 7x", [910]), "thing- 0x38e ") &&
std.assertEqual(std.format("thing-%# x", [-910]), "thing--0x38e") &&
std.assertEqual(std.format("thing-%#7.4x", [910.3]), "thing-  0x38e") &&
std.assertEqual(std.format("thing-%#+7.4x", [910.3]), "thing- +0x38e") &&
std.assertEqual(std.format("thing-%#+-7.4x", [910.3]), "thing-+0x38e ") &&
std.assertEqual(std.format("thing-%#-7.4x", [910.3]), "thing-0x38e  ") &&

// X
std.assertEqual(std.format("thing-%X", [910]), "thing-38E") &&
std.assertEqual(std.format("thing-%lX", [910]), "thing-38E") &&
std.assertEqual(std.format("thing-%X", [-910]), "thing--38E") &&
std.assertEqual(std.format("thing-%5X", [910]), "thing-  38E") &&
std.assertEqual(std.format("thing-%05X", [910]), "thing-0038E") &&
std.assertEqual(std.format("thing-% X", [910]), "thing- 38E") &&
std.assertEqual(std.format("thing-%- 5X", [910]), "thing- 38E ") &&
std.assertEqual(std.format("thing-% X", [-910]), "thing--38E") &&
std.assertEqual(std.format("thing-%5.4X", [910.3]), "thing-  38E") &&
std.assertEqual(std.format("thing-%+5.4X", [910.3]), "thing- +38E") &&
std.assertEqual(std.format("thing-%+-5.4X", [910.3]), "thing-+38E ") &&
std.assertEqual(std.format("thing-%-5.4X", [910.3]), "thing-38E  ") &&
std.assertEqual(std.format("thing-%#X", [910]), "thing-0X38E") &&
std.assertEqual(std.format("thing-%#lX", [910]), "thing-0X38E") &&
std.assertEqual(std.format("thing-%#X", [-910]), "thing--0X38E") &&
std.assertEqual(std.format("thing-%#7X", [910]), "thing-  0X38E") &&
std.assertEqual(std.format("thing-%#07X", [910]), "thing-0X0038E") &&
std.assertEqual(std.format("thing-%# X", [910]), "thing- 0X38E") &&
std.assertEqual(std.format("thing-%#- 7X", [910]), "thing- 0X38E ") &&
std.assertEqual(std.format("thing-%# X", [-910]), "thing--0X38E") &&
std.assertEqual(std.format("thing-%#7.4X", [910.3]), "thing-  0X38E") &&
std.assertEqual(std.format("thing-%#+7.4X", [910.3]), "thing- +0X38E") &&
std.assertEqual(std.format("thing-%#+-7.4X", [910.3]), "thing-+0X38E ") &&
std.assertEqual(std.format("thing-%#-7.4X", [910.3]), "thing-0X38E  ") &&

// e
std.assertEqual(std.format("%e", [910]), "9.100000e+02") &&
std.assertEqual(std.format("%.0le", [910]), "9e+02") &&
std.assertEqual(std.format("%#e", [-910]), "-9.100000e+02") &&
std.assertEqual(std.format("%16e", [910]), "    9.100000e+02") &&
std.assertEqual(std.format("%016e", [910]), "00009.100000e+02") &&
std.assertEqual(std.format("% e", [910]), " 9.100000e+02") &&
std.assertEqual(std.format("%- 16e", [910]), " 9.100000e+02   ") &&
std.assertEqual(std.format("% e", [-910]), "-9.100000e+02") &&
std.assertEqual(std.format("%16.4e", [910.3]), "      9.1030e+02") &&
std.assertEqual(std.format("%+16.4e", [910.3]), "     +9.1030e+02") &&
std.assertEqual(std.format("%+-16.4e", [910.3]), "+9.1030e+02     ") &&
std.assertEqual(std.format("%-16.4e", [910.3]), "9.1030e+02      ") &&
std.assertEqual(std.format("%#.0e", [910.3]), "9.e+02") &&
std.assertEqual(std.format("%#.0e", [900]), "9.e+02") &&

// E
std.assertEqual(std.format("%E", [910]), "9.100000E+02") &&
std.assertEqual(std.format("%.0lE", [910]), "9E+02") &&
std.assertEqual(std.format("%#E", [-910]), "-9.100000E+02") &&
std.assertEqual(std.format("%16E", [910]), "    9.100000E+02") &&
std.assertEqual(std.format("%016E", [910]), "00009.100000E+02") &&
std.assertEqual(std.format("% E", [910]), " 9.100000E+02") &&
std.assertEqual(std.format("%- 16E", [910]), " 9.100000E+02   ") &&
std.assertEqual(std.format("% E", [-910]), "-9.100000E+02") &&
std.assertEqual(std.format("%16.4E", [910.3]), "      9.1030E+02") &&
std.assertEqual(std.format("%+16.4E", [910.3]), "     +9.1030E+02") &&
std.assertEqual(std.format("%+-16.4E", [910.3]), "+9.1030E+02     ") &&
std.assertEqual(std.format("%-16.4E", [910.3]), "9.1030E+02      ") &&
std.assertEqual(std.format("%#.0E", [910.3]), "9.E+02") &&
std.assertEqual(std.format("%#.0E", [900]), "9.E+02") &&

// f
std.assertEqual(std.format("%f", [910]), "910.000000") &&
std.assertEqual(std.format("%.0lf", [910]), "910") &&
std.assertEqual(std.format("%#f", [-910]), "-910.000000") &&
std.assertEqual(std.format("%12f", [910]), "  910.000000") &&
std.assertEqual(std.format("%012f", [910]), "00910.000000") &&
std.assertEqual(std.format("% f", [910]), " 910.000000") &&
std.assertEqual(std.format("%- 12f", [910]), " 910.000000 ") &&
std.assertEqual(std.format("% f", [-910]), "-910.000000") &&
std.assertEqual(std.format("%12.4f", [910.3]), "    910.3000") &&
std.assertEqual(std.format("%+12.4f", [910.3]), "   +910.3000") &&
std.assertEqual(std.format("%+-12.4f", [910.3]), "+910.3000   ") &&
std.assertEqual(std.format("%-12.4f", [910.3]), "910.3000    ") &&
std.assertEqual(std.format("%#.0f", [910.3]), "910.") &&
std.assertEqual(std.format("%#.0f", [910]), "910.") &&

// g
std.assertEqual(std.format("%.3g", [1000000001]), "1.000e+09") &&
std.assertEqual(std.format("%.3g", [1001]), "1.001e+03") &&
std.assertEqual(std.format("%.3g", [0.001]), "0.001") &&

// lots together, also test % operator
std.assertEqual("%s[%05d]-%2x%2x%2x%c" % ["foo", 3991, 17, 18, 17, 100], "foo[03991]-111211d") &&

// use of *
std.assertEqual("%*d" % [10, 8], "%10d" % [8]) &&
std.assertEqual("%*.*f" % [10, 3, 1/3], "%10.3f" % [1/3]) &&

true
