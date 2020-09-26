local arrs = [
  [],
  [1, 2],
  [1, 2, 3],
  [2, 3],
];
[
  {
    ['%s < %s' % [x, y]]: x < y,
    ['%s > %s' % [x, y]]: x > y,
    ['%s <= %s' % [x, y]]: x <= y,
    ['%s >= %s' % [x, y]]: x >= y,
  }
  for x in arrs
  for y in arrs
]
