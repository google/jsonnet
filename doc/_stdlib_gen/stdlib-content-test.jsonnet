local stdlib_content = import 'stdlib-content.jsonnet';

std.all([
  if !std.objectHas(field, 'availableSince') then
    error 'No availableSince field for std.%s' % field.name
  else if field.availableSince[0] == 'v' then
    error 'availableSince field for std.%s should not begin with a "v" (got %s)'
          % [field.name, field.availableSince]
  else
    true
  for group in stdlib_content.groups
  for field in group.fields
])
