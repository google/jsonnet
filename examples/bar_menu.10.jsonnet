// bar_menu.10.jsonnet

import "bar_menu.9.jsonnet" {
    cocktails: super.cocktails {
        "Whiskey Sour": {
            ingredients: [
                { kind: "Scotch", qty: 1.5 },
                { kind: "Lemon Juice", qty: 0.75 },
            ],
            garnish: "Lemon Peel",
            served: "On The Rocks",
        }
    }
}







