syntax match Float "\<\d*\([Ee][+-]\?\d\+\)\?\>"
syntax match Float "\<\d\+[.]\d*\([Ee][+-]\?\d\+\)\?\>"
syntax match Float "\<[.]\d\+\([Ee][+-]\?\d\+\)\?\>"

syn match Type "std.assertEqual"
syn match Type "std.pow"
syn match Type "std.floor"
syn match Type "std.ceil"
syn match Type "std.sqrt"
syn match Type "std.sin"
syn match Type "std.cos"
syn match Type "std.tan"
syn match Type "std.asin"
syn match Type "std.acos"
syn match Type "std.atan"
syn match Type "std.toString"
syn match Type "std.map"
syn match Type "std.filter"
syn match Type "std.foldl"
syn match Type "std.foldr"
syn match Type "std.range"
syn match Type "std.objectHas"
syn match Type "std.length"
syn match Type "std.join"
syn match Type "std.objectFields"
syn match Type "std.filterMap"

syn match Type "\$"

syn region String start='L\="' skip='\\\\\|\\"' end='"'

syn region Comment start="/[*]" end="[*]/"
syn match Comment "//.*$"

syn match Keyword "\<[a-zA-Z_][a-z0-9A-Z_]*\w*:"

syntax keyword Special import
syntax keyword Type local function self super if then else for in
syntax keyword Constant true false null
syntax keyword Error error


