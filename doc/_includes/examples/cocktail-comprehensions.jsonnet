{
  cocktails: {
    "Bee's Knees": {
      // Construct the ingredients by using
      // 4/3 oz of each element in the given
      // list.
      ingredients: [  // Array comprehension.
        { kind: kind, qty: 4 / 3 }
        for kind in [
          'Honey Syrup',
          'Lemon Juice',
          'Farmers Gin',
        ]
      ],
      garnish: 'Lemon Twist',
      served: 'Straight Up',
    },
  } + {  // Object comprehension.
    [sd.name + 'Screwdriver']: {
      ingredients: [
        { kind: 'Vodka', qty: 1.5 },
        { kind: sd.fruit, qty: 3 },
      ],
      served: 'On The Rocks',
    }
    for sd in [
      { name: 'Yellow ', fruit: 'Lemonade' },
      { name: '', fruit: 'Orange Juice' },
    ]
  },
}
