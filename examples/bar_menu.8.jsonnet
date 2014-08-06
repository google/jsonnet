// bar_menu.8.jsonnet
{
    cocktails: {
        Negroni: {
            local neg = self,
            ingredients: [
                { kind: "Farmers Gin", qty: 1 },
                { kind: "Sweet Red Vermouth",
                  qty: neg.ingredients[0].qty },
                { kind: "Campari",
                  qty: neg.ingredients[0].qty },
            ],
            garnish: "Orange Peel",
            served: "On The Rocks",
        },
    }
}           
