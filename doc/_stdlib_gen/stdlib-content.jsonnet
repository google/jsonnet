local html = import 'html.libsonnet';

{
  intro: html.paragraphs([
    |||
      This page describes the functions available in Jsonnet's standard library, i.e. the object
      implicitly bound to the <code>std</code> variable. Some of the standard library functions
      can be implemented in Jsonnet. Their code can be found in the <tt>std.jsonnet</tt> file.
      The behavior of some of the other functions, i.e. the ones that expose extra functionality
      not otherwise available to programmers, is described formally in the <a href="/language/spec.html">specification</a>.
    |||,
    |||
      The standard library is implicitly added to all Jsonnet programs by enclosing them in a
      local construct. For example, if the program given by the user is <code>{x: "foo"}</code>,
      then the actual code executed would be <code>local std = { ... }; {x: "foo"}</code>. The
      functions in the standard library are all hidden fields of the <code>std</code> object.
    |||,
  ]),
  prefix: 'std',
  groups: [
    {
      name: 'External Variables',
      id: 'ext_vars',
      fields: [
        {
          name: 'extVar',
          params: ['x'],
          description: 'If an external variable with the given name was defined, return its string value. Otherwise, raise an error.',
        },
      ],
    },
    {
      name: 'Types and Reflection',
      id: 'types_reflection',
      fields: [
        {
          name: 'thisFile',
          description: 'Note that this is a field. It contains the current Jsonnet filename as a string.',
        },
        {
          name: 'type',
          params: ['x'],
          description: html.paragraphs([
            |||
              Return a string that indicates the type of the value. The possible return values are:
              "array", "boolean", "function", "null", "number", "object", and "string".
            |||,
            |||
              The following functions are also available and return a boolean:
              <code>std.isArray(v)</code>, <code>std.isBoolean(v)</code>, <code>std.isFunction(v)</code>,
              <code>std.isNumber(v)</code>, <code>std.isObject(v)</code>, and
              <code>std.isString(v)</code>.
            |||,
          ]),
        },
        {
          name: 'length',
          params: ['x'],
          description: |||
            Depending on the type of the value given, either returns the number of elements in the
            array, the number of codepoints in the string, the number of parameters in the function, or
            the number of fields in the object. Raises an error if given a primitive value, i.e.
            <code>null</code>, <code>true</code> or <code>false</code>.
          |||,
        },
        {
          name: 'objectHas',
          params: ['o', 'f'],
          description: |||
            Returns <code>true</code> if the given object has the field (given as a string), otherwise
            <code>false</code>. Raises an error if the arguments are not object and string
            respectively. Returns false if the field is hidden.
          |||,
        },
        {
          name: 'objectFields',
          params: ['o'],
          description: |||
            Returns an array of strings, each element being a field from the given object. Does not include
            hidden fields.
          |||,
        },
        {
          name: 'objectHasAll',
          params: ['o', 'f'],
          description: |||
            As <code>std.objectHas</code> but also includes hidden fields.
          |||,
        },
        {
          name: 'objectFieldsAll',
          params: ['o'],
          description: |||
            As <code>std.objectFields</code> but also includes hidden fields.
          |||,
        },
        {
          name: 'prune',
          params: ['a'],
          description: |||
            Recursively remove all "empty" members of <code>a</code>. "Empty" is defined as zero
            length `arrays`, zero length `objects`, or `null` values.
            The argument <code>a</code> may have any type.
          |||,
        },
        {
          name: 'mapWithKey',
          params: ['func', 'obj'],
          description: |||
            Apply the given function to all fields of the given object, also passing
            the field name. The function <code>func</code> is expected to take the
            field name as the first parameter and the field value as the second.
          |||,
        },
      ],
    },
    {
      name: 'Mathematical Utilities',
      id: 'math',
      intro: [
        |||
          <p>
              The following mathematical functions are available:
          </p>
          <ul>
              <ul><code>std.abs(n)</code></ul>
              <ul><code>std.sign(n)</code></ul>
              <ul><code>std.max(a, b)</code></ul>
              <ul><code>std.min(a, b)</code></ul>
              <ul><code>std.pow(x, n)</code></ul>
              <ul><code>std.exp(x)</code></ul>
              <ul><code>std.log(x)</code></ul>
              <ul><code>std.exponent(x)</code></ul>
              <ul><code>std.mantissa(x)</code></ul>
              <ul><code>std.floor(x)</code></ul>
              <ul><code>std.ceil(x)</code></ul>
              <ul><code>std.sqrt(x)</code></ul>
              <ul><code>std.sin(x)</code></ul>
              <ul><code>std.cos(x)</code></ul>
              <ul><code>std.tan(x)</code></ul>
              <ul><code>std.asin(x)</code></ul>
              <ul><code>std.acos(x)</code></ul>
              <ul><code>std.atan(x)</code></ul>
          </ul>
          <p>
              The function <code>std.mod(a, b)</code> is what the % operator is desugared to. It performs
              modulo arithmetic if the left hand side is a number, or if the left hand side is a string,
              it does Python-style string formatting with <code>std.format()</code>.
          </p>
        |||,
      ],
      fields: [
        {
          name: 'std.clamp',
          params: ['x', 'minVal', 'maxVal'],
          availableSince: '0.15.0',
          description: |||
            Clamp a value to fit within the range [<code>minVal</code>, <code>maxVal</code>].
            Equivalent to <code>std.max(minVal, std.min(x, maxVal))</code>.
          |||,
          examples: [
            {
              input: 'std.clamp(-3, 0, 5)',
              output: std.clamp(-3, 0, 5),
            },
            {
              input: 'std.clamp(4, 0, 5)',
              output: std.clamp(4, 0, 5),
            },
            {
              input: 'std.clamp(7, 0, 5)',
              output: std.clamp(7, 0, 5),
            },
          ],
        },

      ],
    },
    {
      name: 'Assertions and Debugging',
      id: 'assertions_debugging',
      fields: [
        {
          name: 'assertEqual',
          params: ['a', 'b'],
          description: 'Ensure that <code>a == b</code>. Returns <code>true</code> or throws an error message.',
        },
      ],
    },
    {
      name: 'String Manipulation',
      id: 'string',
      fields: [
        {
          name: 'toString',
          params: ['a'],
          description: |||
            Convert the given argument to a string.
          |||,
        },
        {
          name: 'codepoint',
          params: ['str'],
          description: |||
            Returns the positive integer representing the unicode codepoint of the character in the
            given single-character string. This function is the inverse of <code>std.char(n)</code>.
          |||,
        },
        {
          name: 'char',
          params: ['n'],
          description: |||
            Returns a string of length one whose only unicode codepoint has integer id <code>n</code>.
            This function is the inverse of <code>std.codepoint(str)</code>.
          |||,
        },
        {
          name: 'substr',
          params: ['str', 'from', 'len'],
          description: |||
            Returns a string that is the part of <code>s</code> that starts at offset <code>from</code>
            and is <code>len</code> codepoints long. If the string <code>s</code> is shorter than
            <code>from+len</code>, the suffix starting at position <code>from</code> will be returned.
          |||,
        },
        {
          name: 'findSubstr',
          params: ['pat', 'str'],
          description: |||
            Returns an array that contains the indexes of all occurances of <code>pat</code> in
            <code>str</code>.
          |||,
        },
        {
          name: 'startsWith',
          params: ['a', 'b'],
          description: |||
            Returns whether the string a is prefixed by the string b.
          |||,
        },
        {
          name: 'endsWith',
          params: ['a', 'b'],
          description: |||
            Returns whether the string a is suffixed by the string b.
          |||,
        },
        {
          name: 'stripChars',
          params: ['str', 'chars'],
          availableSince: '0.15.0',
          description: |||
            Removes characters <code>chars</code> from the beginning and from the end of <code>str</code>.
          |||,
          examples: [
            {
              input: 'std.stripChars(" test test test     ", " ")',
              output: std.stripChars(' test test test     ', ' '),
            },
            {
              input: 'std.stripChars("aaabbbbcccc", "ac")',
              output: std.stripChars('aaabbbbcccc', 'ac'),
            },
            {
              input: 'std.stripChars("cacabbbbaacc", "ac")',
              output: std.stripChars('cacabbbbaacc', 'ac'),
            },
          ],
        },
        {
          name: 'lstripChars',
          params: ['str', 'chars'],
          availableSince: '0.15.0',
          description: |||
            Removes characters <code>chars</code> from the beginning of <code>str</code>.
          |||,
          examples: [
            {
              input: 'std.lstripChars(" test test test     ", " ")',
              output: std.lstripChars(' test test test     ', ' '),
            },
            {
              input: 'std.lstripChars("aaabbbbcccc", "ac")',
              output: std.lstripChars('aaabbbbcccc', 'ac'),
            },
            {
              input: 'std.lstripChars("cacabbbbaacc", "ac")',
              output: std.lstripChars('cacabbbbaacc', 'ac'),
            },
          ],
        },
        {
          name: 'rstripChars',
          params: ['str', 'chars'],
          availableSince: '0.15.0',
          description: |||
            Removes characters <code>chars</code> from the end of <code>str</code>.
          |||,
          examples: [
            {
              input: 'std.rstripChars(" test test test     ", " ")',
              output: std.rstripChars(' test test test     ', ' '),
            },
            {
              input: 'std.rstripChars("aaabbbbcccc", "ac")',
              output: std.rstripChars('aaabbbbcccc', 'ac'),
            },
            {
              input: 'std.rstripChars("cacabbbbaacc", "ac")',
              output: std.rstripChars('cacabbbbaacc', 'ac'),
            },
          ],
        },
        {
          name: 'split',
          params: ['str', 'c'],
          description: |||
            Split the string <code>str</code> into an array of strings, divided by the single character
            <code>c</code>.
          |||,
          examples: [
            {
              input: @'std.split("foo/bar", "/")',
              output: std.split('foo/bar', '/'),
            },
            {
              input: @'std.split("/foo/", "/")',
              output: std.split('/foo/', '/'),
            },
          ],
        },
        {
          name: 'splitLimit',
          params: ['str', 'c', 'maxsplits'],
          description: |||
            As std.split(str, c) but will stop after <code>maxsplits</code> splits, thereby the largest
            array it will return has length <code>maxsplits + 1</code>.  A limit of -1 means unlimited.
          |||,
          examples: [
            {
              input: @'std.splitLimit("foo/bar", "/", 1)',
              output: std.splitLimit('foo/bar', '/', 1),
            },
            {
              input: @'std.splitLimit("/foo/", "/", 1)',
              output: std.splitLimit('foo/bar', '/', 1),
            },
          ],
        },
        {
          name: 'strReplace',
          params: ['str', 'from', 'to'],
          description: |||
            Returns a copy of the string in which all occurrences of string <code>from</code> have been
            replaced with string <code>to</code>.
          |||,
          examples: [
            {
              input: @"std.strReplace('I like to skate with my skateboard', 'skate', 'surf')",
              output: std.strReplace('I like to skate with my skateboard', 'skate', 'surf'),
            },
          ],
        },
        {
          name: 'asciiUpper',
          params: ['str'],
          description: |||
            Returns a copy of the string in which all ASCII letters are capitalized.
          |||,
          examples: [
            {
              input: "std.asciiUpper('100 Cats!')",
              output: std.asciiUpper('100 Cats!'),
            },
          ],
        },
        {
          name: 'asciiLower',
          params: ['str'],
          description: |||
            Returns a copy of the string in which all ASCII letters are lower cased.
          |||,
          examples: [
            {
              input: "std.asciiLower('100 Cats!')",
              output: std.asciiLower('100 Cats!'),
            },
          ],
        },
        {
          name: 'stringChars',
          params: ['str'],
          description: |||
            Split the string <code>str</code> into an array of strings, each containing a single
            codepoint.
          |||,
          examples: [
            {
              input: 'std.stringChars("foo")',
              output: std.stringChars('foo'),
            },
          ],
        },
        {
          name: 'format',
          params: ['str', 'vals'],
          description: |||
            Format the string <code>str</code> using the values in <code>vals</code>. The values can be
            an array, an object, or in other cases are treated as if they were provided in a singleton
            array. The string formatting follows the <a
            href="https://docs.python.org/2/library/stdtypes.html#string-formatting">same rules</a> as
            Python. The <code>%</code> operator can be used as a shorthand for this function.
          |||,
          examples: [
            {
              input: 'std.format("Hello %03d", 12)',
              output: std.format('Hello %03d', 12),
            },
            {
              input: '"Hello %03d" % 12',
              output: 'Hello %03d' % 12,
            },
            {
              input: '"Hello %s, age %d" % ["Foo", 25]',
              output: 'Hello %s, age %d' % ['Foo', 25],
            },
            {
              input: '"Hello %(name)s, age %(age)d" % {age: 25, name: "Foo"}',
              output: 'Hello %(name)s, age %(age)d' % { age: 25, name: 'Foo' },
            },
          ],
        },
        {
          name: 'escapeStringBash',
          params: ['str'],
          description: |||
            Wrap <code>str</code> in single quotes, and escape any single quotes within <code>str</code>
            by changing them to a sequence <tt>'"'"'</tt>. This allows injection of arbitrary strings
            as arguments of commands in bash scripts.
          |||,
        },
        {
          name: 'escapeStringDollars',
          params: ['str'],
          description: |||
            Convert $ to $$ in <code>str</code>. This allows injection of arbitrary strings into
            systems that use $ for string interpolation (like Terraform).
          |||,
        },
        {
          name: 'escapeStringJson',
          params: ['str'],
          description: |||
            Convert <code>str</code> to allow it to be embedded in a JSON representation, within a
            string. This adds quotes, escapes backslashes, and escapes unprintable characters.
          |||,
          examples: [
            {
              input: |||
                local description = "Multiline\nc:\\path";
                "{name: %s}" % std.escapeStringJson(description)
              |||,
              output: (
                local description = 'Multiline\nc:\\path';
                '{name: %s}' % std.escapeStringJson(description)
              ),
            },
          ],
        },
        {
          name: 'escapeStringPython',
          params: ['str'],
          description: |||
            Convert <code>str</code> to allow it to be embedded in Python. This is an alias for
            <code>std.escapeStringJson</code>.
          |||,
        },
      ],
    },
    {
      name: 'Parsing',
      id: 'parsing',
      fields: [
        {
          name: 'parseInt',
          params: ['str'],
          description: |||
            Parses a signed decimal integer from the input string.
          |||,
          examples: [
            {
              input: 'std.parseInt("123")',
              output: std.parseInt('123'),
            },
            {
              input: 'std.parseInt("-123")',
              output: std.parseInt('-123'),
            },
          ],
        },
        {
          name: 'parseOctal',
          params: ['str'],
          description: |||
            Parses an unsigned octal integer from the input string. Initial zeroes are tolerated.
          |||,
          examples: [
            {
              input: 'std.parseOctal("755")',
              output: std.parseOctal('755'),
            },
          ],
        },
        {
          name: 'parseHex',
          params: ['str'],
          description: |||
            Parses an unsigned hexadecimal integer, from the input string. Case insensitive.
          |||,
          examples: [
            {
              input: 'std.parseHex("ff")',
              output: std.parseHex('ff'),
            },
          ],
        },
        {
          name: 'parseJson',
          availableSince: '0.13.0',
          params: ['str'],
          description: |||
            Parses a JSON string.
          |||,
          examples: [
            {
              input: 'std.parseJson(\'{"foo": "bar"}\')',
              output: std.parseJson('{"foo": "bar"}'),
            },
          ],
        },
        {
          name: 'encodeUTF8',
          params: ['str'],
          availableSince: '0.13.0',
          description: |||
            Encode a string using <a href="https://en.wikipedia.org/wiki/UTF-8">UTF8</a>. Returns an array of numbers
            representing bytes.
          |||,
        },
        {
          name: 'decodeUTF8',
          params: ['arr'],
          availableSince: '0.13.0',
          description: |||
            Decode an array of numbers representing bytes using <a href="https://en.wikipedia.org/wiki/UTF-8">UTF8</a>.
            Returns a string.
          |||,
        },
      ],
    },
    {
      name: 'Manifestation',
      id: 'manifestation',
      // TODO(sbarzowski): Clean up the example's representation
      fields: [
        {
          name: 'manifestIni',
          params: ['ini'],
          description: [
            html.p({}, |||
                Convert the given structure to a string in <a href="http://en.wikipedia.org/wiki/INI_file">INI format</a>. This
                allows using Jsonnet's
                object model to build a configuration to be consumed by an application expecting an INI
                file. The data is in the form of a set of sections, each containing a key/value mapping.
                These examples should make it clear:
            |||),
            html.pre({}, |||
              {
                  main: { a: "1", b: "2" },
                  sections: {
                      s1: {x: "11", y: "22", z: "33"},
                      s2: {p: "yes", q: ""},
                      empty: {},
                  }
              }
            |||),

            html.p({}, |||
              Yields a string containing this INI file:
            |||),

            html.pre({}, |||
              a = 1
              b = 2
              [empty]
              [s1]
              x = 11
              y = 22
              z = 33
              [s2]
              p = yes
              q =
            |||),
          ]
        },
        {
          name: 'manifestPython',
          params: ['v'],
          description: [
            html.p({}, |||
              Convert the given value to a JSON-like form that is compatible with Python. The chief
              differences are True / False / None instead of true / false / null.
            |||),

            html.pre({}, |||
              {
                  b: ["foo", "bar"],
                  c: true,
                  d: null,
                  e: { f1: false, f2: 42 },
              }
            |||),

            html.p({}, |||
                Yields a string containing Python code like:
            |||),

            html.pre({}, |||
              {
                  "b": ["foo", "bar"],
                  "c": True,
                  "d": None,
                  "e": {"f1": False, "f2": 42}
              }
            |||),
          ]
        },
        {
          name: 'manifestPythonVars',
          params: ['conf'],
          description: [
            html.p({}, |||
                Convert the given object to a JSON-like form that is compatible with Python. The key
                difference to <code>std.manifestPython</code> is that the top level is represented as a list
                of Python global variables.
            |||),

            html.pre({}, |||
              {
                  b: ["foo", "bar"],
                  c: true,
                  d: null,
                  e: { f1: false, f2: 42 },
              }
            |||),

            html.p({}, |||
                Yields a string containing this Python code:
            |||),

            html.pre({}, |||
              b = ["foo", "bar"]
              c = True
              d = None
              e = {"f1": False, "f2": 42}
            |||),
          ],
        },
        {
          name: 'manifestJsonEx',
          params: ['value', 'indent'],
          description: [
            html.p({}, |||
                Convert the given object to a JSON form. <code>indent</code> is a string containing
                one or more whitespaces that are used for indentation:
            |||),
            html.pre({}, |||
              std.manifestJsonEx(
                {
                    x: [1, 2, 3, true, false, null,
                        "string\nstring"],
                    y: { a: 1, b: 2, c: [1, 2] },
                }, "    ")
            |||),
            html.p({}, |||
              Yields a string containing this JSON object:
            |||),
            html.pre({}, |||
              {
                  "x": [
                      1,
                      2,
                      3,
                      true,
                      false,
                      null,
                      "string\nstring"
                  ],
                  "y": {
                      "a": 1,
                      "b": 2,
                      "c": [
                          1,
                          2
                      ]
                  }
              }
            |||),
          ]
        },
        {
          name: 'manifestYamlDoc',
          params: ['value', 'indent_array_in_object=false'],
          description: [
              html.p({}, |||
                  Convert the given value to a YAML form. Note that <code>std.manifestJson</code> could also
                  be used for this purpose, because any JSON is also valid YAML. But this function will
                  produce more canonical-looking YAML.
              |||),
              html.pre({}, |||
                std.manifestYamlDoc(
                  {
                      x: [1, 2, 3, true, false, null,
                          "string\nstring\n"],
                      y: { a: 1, b: 2, c: [1, 2] },
                  },
                  indent_array_in_object=false)
              |||),
              html.p({}, |||
                Yields a string containing this YAML:
              |||),
              html.pre({}, |||
                  "x":
                    - 1
                    - 2
                    - 3
                    - true
                    - false
                    - null
                    - |
                        string
                        string
                  "y":
                    "a": 1
                    "b": 2
                    "c":
                        - 1
                        - 2
              |||),
              html.p({}, |||
                The <code>indent_array_in_object</code> param adds additional indentation which some people
                may find easier to read.
              |||),
          ],
        },
        {
          name: 'manifestYamlStream',
          params: ['value', 'indent_array_in_object=false', 'c_document_end=false'],
          description: [
            html.p({}, |||
                Given an array of values, emit a YAML "stream", which is a sequence of documents separated
                by <code>---</code> and ending with <code>...</code>.
            |||),

            html.pre({}, |||
              std.manifestYamlStream(
                ['a', 1, []],
                indent_array_in_object=false,
                c_document_end=true)
            |||),

            html.p({}, |||
              Yields this string:
            |||),

            html.pre({}, |||
              ---
              "a"
              ---
              1
              ---
              []
              ...
            |||),

            html.p({}, |||
              The <code>indent_array_in_object</code> param is the same as in <code>manifestYamlDoc</code>.
            |||),
            html.p({}, |||
              The <code>c_document_end</code> param adds the optional terminating <code>...</code>.
            |||),
          ],
        },
        {
          name: 'manifestXmlJsonml',
          params: ['value'],
          description: [
            html.p({}, |||
                Convert the given <a href="http://www.jsonml.org/">JsonML</a>-encoded value to a string
                containing the XML.
            |||),

            html.pre({}, |||
              std.manifestXmlJsonml([
                  'svg', { height: 100, width: 100 },
                  [
                      'circle', {
                      cx: 50, cy: 50, r: 40,
                      stroke: 'black', 'stroke-width': 3,
                      fill: 'red',
                      }
                  ],
              ])
            |||),

            html.p({}, |||
                Yields a string containing this XML (all on one line):
            |||),

            html.pre({}, html.escape(|||
              <svg height="100" width="100">
                  <circle cx="50" cy="50" fill="red" r="40"
                  stroke="black" stroke-width="3"></circle>;
              </svg>;
            |||)),

            html.p({}, |||
                Which represents the following image:
            |||),

            |||
              <svg height="100" width="100">
                  <circle cx="50" cy="50" r="40" stroke="black" stroke-width="3" fill="red" />
                  Sorry, your browser does not support inline SVG.
              </svg>
            |||,

            html.p({}, |||
                JsonML is designed to preserve "mixed-mode content" (i.e., textual data outside of or next
                to elements). This includes the whitespace needed to avoid having all the XML on one line,
                which is meaningful in XML. In order to have whitespace in the XML output, it must be
                present in the JsonML input:
            |||),

            html.pre({}, |||
              std.manifestXmlJsonml([
                  'svg',
                  { height: 100, width: 100 },
                  '\n  ',
                  [
                      'circle',
                      {
                      cx: 50, cy: 50, r: 40, stroke: 'black',
                      'stroke-width': 3, fill: 'red',
                      }
                  ],
                  '\n',
              ])
            |||),
          ],
        },
      ],
    },
    {
      name: 'Arrays',
      id: 'arrays',
      fields: [
        {
          name: 'makeArray',
          params: ['sz', 'func'],
          description: |||
            Create a new array of <code>sz</code> elements by calling <code>func(i)</code> to initialize
            each element. Func is expected to be a function that takes a single parameter, the index of
            the element it should initialize.
          |||,
          examples: [
            {
              input: 'std.makeArray(3,function(x) x * x)',
              output: std.makeArray(3, function(x) x * x),
            },
          ],
        },
        {
          name: 'member',
          params: ['arr', 'x'],
          availableSince: '0.15.0',
          description: |||
            Returns whether <code>x</code> occurs in <code>arr</code>.
            Argument <code>arr</code> may be an array or a string.
          |||,
        },
        {
          name: 'count',
          params: ['arr', 'x'],
          description: |||
            Return the number of times that <code>x</code> occurs in <code>arr</code>.
          |||,
        },
        {
          name: 'find',
          params: ['value', 'arr'],
          description: |||
            Returns an array that contains the indexes of all occurances of <code>value</code> in
            <code>arr</code>.
          |||,
        },
        {
          name: 'map',
          params: ['func', 'arr'],
          description: |||
            Apply the given function to every element of the array to form a new array.
          |||,
        },
        {
          name: 'mapWithIndex',
          params: ['func', 'arr'],
          description: |||
            Similar to <a href="#map">map</a> above, but it also passes to the function the element's
            index in the array. The function <code>func</code> is expected to take the index as the
            first parameter and the element as the second.
          |||,
        },
        {
          name: 'filterMap',
          params: ['filter_func', 'map_func', 'arr'],
          description: |||
            It first filters, then maps the given array, using the two functions provided.
          |||,
        },
        {
          name: 'flatMap',
          params: ['func', 'arr'],
          description: |||
            Apply the given function to every element of the array to form a new array then flatten the result.
            It can be thought of as a generalized map, where each element can get mapped to 0, 1 or more elements.
          |||,
          examples: [
            {
              input: 'std.flatMap(function(x) [x, x], [1, 2, 3])',
              output: std.flatMap(function(x) [x, x], [1, 2, 3]),
            },
            {
              input: 'std.flatMap(function(x) if x == 2 then [] else [x], [1, 2, 3])',
              output: std.flatMap(function(x) if x == 2 then [] else [x], [1, 2, 3]),
            },
            {
              input: 'std.flatMap(function(x) if x == 2 then [] else [x * 3, x * 2], [1, 2, 3])',
              output: std.flatMap(function(x) if x == 2 then [] else [x * 3, x * 2], [1, 2, 3]),
            },
          ],
        },
        {
          name: 'filter',
          params: ['func', 'arr'],
          description: |||
            Return a new array containing all the elements of <code>arr</code> for which the
            <code>func</code> function returns true.
          |||,
        },
        {
          name: 'foldl',
          params: ['func', 'arr', 'init'],
          description: |||
            Classic foldl function. Calls the function on the result of the previous function call and
            each array element, or <code>init</code> in the case of the initial element. Traverses the
            array from left to right.
          |||,
        },
        {
          name: 'foldr',
          params: ['func', 'arr', 'init'],
          description: |||
            Classic foldr function. Calls the function on the result of the previous function call and
            each array element, or <code>init</code> in the case of the initial element. Traverses the
            array from right to left.
          |||,
        },
        {
          name: 'range',
          params: ['from', 'to'],
          description: |||
            Return an array of ascending numbers between the two limits, inclusively.
          |||,
        },
        {
          name: 'repeat',
          params: ['what', 'count'],
          availableSince: '0.15.0',
          description: |||
            Repeats an array or a string <code>what</code> a number of times specified by an integer <code>count</code>.
          |||,
          examples: [
            {
              input: 'std.repeat([1, 2, 3], 3)',
              output: std.repeat([1, 2, 3], 3),
            },
            {
              input: 'std.repeat("blah", 2)',
              output: std.repeat('blah', 2),
            },
          ],
        },
        {
          name: 'join',
          params: ['sep', 'arr'],
          description: |||
            If <code>sep</code> is a string, then <code>arr</code> must be an array of strings, in which
            case they are concatenated with <code>sep</code> used as a delimiter. If <code>sep</code>
            is an array, then <code>arr</code> must be an array of arrays, in which case the arrays are
            concatenated in the same way, to produce a single array.
          |||,
          examples: [
            {
              input: 'std.join(".", ["www", "google", "com"])',
              output: std.join('.', ['www', 'google', 'com']),
            },
            {
              input: 'std.join([9, 9], [[1], [2, 3]])',
              output: std.join([9, 9], [[1], [2, 3]]),
            },
          ],
        },
        {
          name: 'lines',
          params: ['arr'],
          description: |||
            Concatenate an array of strings into a text file with newline characters after each string.
            This is suitable for constructing bash scripts and the like.
          |||,
        },
        {
          name: 'flattenArrays',
          params: ['arr'],
          description: |||
            Concatenate an array of arrays into a single array.
          |||,
          examples: [
            {
              input: 'std.flattenArrays([[1, 2], [3, 4], [[5, 6], [7, 8]]])',
              output: std.flattenArrays([[1, 2], [3, 4], [[5, 6], [7, 8]]]),
            },
          ],
        },
        {
          name: 'reverse',
          params: ['arrs'],
          availableSince: '0.13.0',
          description: |||
            Reverses an array.
          |||,
        },
        {
          name: 'sort',
          params: ['arr', 'keyF=id'],
          description: html.paragraphs([
            |||
              Sorts the array using the <= operator.
            |||,
            |||
              Optional argument <code>keyF</code> is a single argument function used to extract comparison key from each array element.
              Default value is identity function <code>keyF=function(x) x</code>.
            |||,
          ]),
        },
        {
          name: 'uniq',
          params: ['arr', 'keyF=id'],
          description: html.paragraphs([
            |||
              Removes successive duplicates. When given a sorted array, removes all duplicates.
            |||,
            |||
              Optional argument <code>keyF</code> is a single argument function used to extract comparison key from each array element.
              Default value is identity function <code>keyF=function(x) x</code>.
            |||,
          ]),
        },
      ],
    },
    {
      name: 'Sets',
      id: 'sets',
      intro: html.paragraphs([
        |||
          Sets are represented as ordered arrays without duplicates.
        |||,
        |||
          Note that the <code>std.set*</code> functions rely on the uniqueness and ordering
          on arrays passed to them to work. This can be guarenteed by using <code>std.set(arr)</code>.
          If that is not the case, the functions will quietly return non-meaningful results.
        |||,
        |||
          All <code>set.set*</code> functions accept <code>keyF</code> function of one argument, which can be
          used to extract key to use from each element. All Set operations then use extracted key for the purpose
          of identifying uniqueness. Default value is identity function <code>local id = function(x) x</code>.
        |||,
      ]),
      fields: [
        {
          name: 'set',
          params: ['arr', 'keyF=id'],
          description: |||
            Shortcut for std.uniq(std.sort(arr)).
          |||,
        },
        {
          name: 'setInter',
          params: ['a', 'b', 'keyF=id'],
          description: |||
            Set intersection operation (values in both a and b).
          |||,
        },
        {
          name: 'setUnion',
          params: ['a', 'b', 'keyF=id'],
          description: |||
            Set union operation (values in any of <code>a</code> or <code>b</code>). Note that + on sets will simply
            concatenate
            the arrays, possibly forming an array that is not a set (due to not being ordered without
            duplicates).
          |||,
          examples: [
            {
              input: 'std.setUnion([1, 2], [2, 3])',
              output: std.setUnion([1, 2], [2, 3]),
            },
            {
              input: 'std.setUnion([{n:"A", v:1}, {n:"B"}], [{n:"A", v: 9999}, {n:"C"}], keyF=function(x) x.n)',
              output: std.setUnion([{ n: 'A', v: 1 }, { n: 'B' }], [{ n: 'A', v: 9999 }, { n: 'C' }], keyF=function(x) x.n),
            },
          ],
        },
        {
          name: 'setDiff',
          params: ['a', 'b', 'keyF=id'],
          description: |||
            Set difference operation (values in a but not b).
          |||,
        },
        {
          name: 'setMember',
          params: ['x', 'arr', 'keyF=id'],
          description: |||
            Returns <code>true</code> if x is a member of array, otherwise <code>false</code>.
          |||,
        },
      ],
    },
    {
      name: 'Encoding',
      id: 'encoding',
      fields: [
        {
          name: 'base64',
          params: ['input'],
          description: |||
            Encodes the given value into a base64 string. The encoding sequence is <code>A-Za-z0-9+/</code> with
            <code>=</code>
            to pad the output to a multiple of 4 characters. The value can be a string or an array of
            numbers, but the codepoints / numbers must be in the 0 to 255 range. The resulting string
            has no line breaks.
          |||,
        },
        {
          name: 'base64DecodeBytes',
          params: ['str'],
          description: |||
            Decodes the given base64 string into an array of bytes (number values). Currently assumes
            the input string has no linebreaks and is padded to a multiple of 4 (with the = character).
            In other words, it consumes the output of std.base64().
          |||,
        },
        {
          name: 'base64Decode',
          params: ['str'],
          description: html.paragraphs([
            |||
              <em>Deprecated, use <code>std.base64DecodeBytes</code> and decode the string explicitly (e.g. with <code>std.decodeUTF8</code>) instead.</code></em>
            |||,
            |||
              Behaves like std.base64DecodeBytes() except returns a naively encoded string instead of an array of bytes.
            |||,
          ]),
        },
        {
          name: 'md5',
          params: ['s'],
          description: |||
            Encodes the given value into an MD5 string.
          |||,
        },
      ],
    },
    {
      name: 'JSON Merge Patch',
      id: 'json_merge_patch',
      fields: [
        {
          name: 'mergePatch',
          params: ['target', 'patch'],
          description: |||
            Applies <code>patch</code> to <code>target</code>
            according to <a href="https://tools.ietf.org/html/rfc7396">RFC7396</a>
          |||,
        },
      ],
    },
    {
      name: 'Debugging',
      id: 'debugging',
      fields: [
        {
          name: 'trace',
          params: ['target', 'patch'],
          availableSince: '0.11.0',
          description: [
            html.p({}, |||
                Outputs the given string <code>str</code> to stderr and
                returns <code>rest</code> as the result.
            |||),
            html.p({}, |||
              Example:
            |||),
            html.p({},
              html.pre({}, |||
                local conditionalReturn(cond, in1, in2) =
                  if (cond) then
                      std.trace('cond is true returning '
                              + std.toString(in1), in1)
                  else
                      std.trace('cond is false returning '
                              + std.toString(in2), in2);

                {
                    a: conditionalReturn(true, { b: true }, { c: false }),
                }
              |||),
            ),
            html.p({}, |||
              Prints:
            |||),
            html.p({},
              html.pre({}, |||
                TRACE: test.jsonnet:3 cond is true returning {"b": true}
                {
                    "a": {
                        "b": true
                    }
                }
              |||),
            ),
          ],
        },
      ],
    },
  ],
}
