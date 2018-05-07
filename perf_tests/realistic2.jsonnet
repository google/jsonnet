local rfc3339(timestamp) = '1970-01-01T00:00:00Z';

local name1(a, b, c) =
  'AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA%sBBBBBBB%sCCCCCCCCCCC%s'
  % [a, b, c];

local name2(x) =
  'XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX%s' % x;

local T1 = 'PPPPPPPPPPPPPPPPPPPPPPP';
local T2 = 'QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ';

local LOCATIONS = [
  'europe-west1-b',
  'europe-west1-c',
  'europe-west1-d',
  'europe-west2-a',
  'europe-west2-b',
  'europe-west2-c',
  'europe-west3-a',
  'europe-west3-b',
  'europe-west3-c',
  'europe-west4-a',
  'europe-west4-b',
  'europe-west4-c',
  'us-central1-a',
  'us-central1-b',
  'us-central1-c',
  'us-central1-f',
  'us-east1-b',
  'us-east1-c',
  'us-east1-d',
  'us-east4-a',
  'us-east4-b',
  'us-east4-c',
  'us-west1-a',
  'us-west1-b',
  'us-west1-c',
];


// The return value is a function to allow it to be parameterized.
function(
  timestamp=0,
  x='xxxxxxxxxxxxxxxxxxx',
  prefix='prefix',
  num1=50,
  count=25,
  offset=0,
)
  local rfc_timestamp = rfc3339(timestamp);

  local func1(i) =
    local location = LOCATIONS[i % std.length(LOCATIONS)];
    [
      local name = '%s-%000d-%000d' % [prefix, i, j];
      {
        field_zz1: rfc_timestamp,
        fie_z2: {
          field_z3: name1(x, location, name),
          field_zzzzzzzzzz4: name2(x),
          field_zzz5: [name],
          field_z6: T1,
          field_z7: location,
          fi_8: '???',
          fiel_z9: '99',
        },
      }
      for j in std.range(0, num1 - 1)
    ];

  local func2(i) =
    local location = LOCATIONS[i % std.length(LOCATIONS)];
    local all = [
      name1(x, location, '%s-%000d-%000d' % [prefix, i, j])
      for j in std.range(0, num1 - 1)
    ];
    [
      {
        field_yy1: rfc_timestamp,
        field_yyyyy2: 'EEEE',
        field_yyyyy3: {
          field_y4: T2,
          field_yyyyyyy5: p,
          field_yyyyyyy6: q,
        },
      }
      for p in all
      for q in all
      if p != q
    ];


  {
    field_x1: '-----',
    field_xxxxxxxxxxxxxxxx2: std.join([], [
      func1(offset * count + i)
      for i in std.range(0, count - 1)
    ]),
    field_xxxxxxxxxxxxxxxxxxxxxx3: std.join([], [
      func2(offset * count + i)
      for i in std.range(0, count - 1)
    ]),
  }

