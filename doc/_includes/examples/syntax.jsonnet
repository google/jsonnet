/* A C-style comment. */
# A Python-style comment.
{
  cocktails: {
    // Ingredient quantities are in fl oz.
    Manhattan: {
      ingredients: [
        { kind: 'Rye', qty: 2.5 },
        { kind: 'Sweet Red Vermouth', qty: 1 },
        { kind: 'Angostura', qty: 'dash' },
      ],
      garnish: 'Maraschino Cherry',
      served: 'Straight Up',
      description: @'A clear \ red drink.',
    },
    'Trinidad Sour': {
      ingredients: [
        { kind: 'Angostura bitters', qty: 1.333_333 },
        { kind: 'Rye whiskey', qty: 0.5 },
        { kind: 'Fresh lemon juice', qty: 0.75 },
        { kind: 'Orgeat syrup', qty: 1 },
      ],
      garnish: 'Lemon twist',
      served: 'chilled Nick & Nora glass',
      description: |||
        Boldly balanced: 1 1/3 oz Angostura
        transforms bitters into the star spirit.
      |||,
    },
  },
}
