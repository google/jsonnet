// Compiler template
local CCompiler = {
  cFlags: [],
  out: 'a.out',
  local flags_str = std.join(' ', self.cFlags),
  local files_str = std.join(' ', self.files),
  cmd: '%s %s %s -o %s' % [self.compiler, flags_str, files_str, self.out],
};

// GCC specialization
local Gcc = CCompiler { compiler: 'gcc' };

// Another specialization
local Clang = CCompiler { compiler: 'clang' };

// Mixins - append flags
local Opt = { cFlags: super.cFlags + ['-O3', '-DNDEBUG'] };
local Dbg = { cFlags: super.cFlags + ['-g'] };

// Output:
{
  targets: [
    Gcc { files: ['a.c', 'b.c'] },
    Clang { files: ['test.c'], out: 'test' },
    Clang + Opt { files: ['test2.c'], out: 'test2' },
    Gcc + Opt + Dbg { files: ['foo.c', 'bar.c'], out: 'baz' },
  ],
}
