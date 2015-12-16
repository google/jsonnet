// bar_menu.9.jsonnet
{
    cocktails: {
        "Whiskey Sour": {
            ingredients: [
                { kind: "Bourbon", qty: 1.5 },
                { kind: "Lemon Juice", qty: 1 },
                { kind: "Gomme Syrup", qty: 0.5 },
            ],
            garnish: "Lemon Peel",
            served: "Straight Up",
        },
        "Whiskey Sour With Egg": self["Whiskey Sour"] + {
            ingredients: super.ingredients
                         + [ { kind: "Egg White", qty: 0.5 } ],
        },
    }
}





