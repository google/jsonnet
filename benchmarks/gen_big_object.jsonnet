local n = 2000;

local objLocal(name, body) = 'local ' + name + ' = ' + body + ',';
local objField(name, body) = name + ': ' + body + ',';

local allLocals =
  std.makeArray(n, function(i) objLocal('l' + i, '1'));

local allFields =
  std.makeArray(n, function(i) objField('f' + i, '2'));

local indent = '  ';
local indentAndSeparate(s) = indent + s + '\n';

local objContents = std.map(indentAndSeparate, allLocals + allFields);

local objectBody = std.join('', objContents);
'{\n' + objectBody + '}\n'
