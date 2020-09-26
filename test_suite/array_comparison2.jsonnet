local arrs = [
  [[]],
  [[1, 2]],
  [[1], [2]],
  [[1, 2], []],
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
