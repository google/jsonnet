// bar_menu.2.jsonnet
{
    cocktails: {
        "Tom Collins": {
            ingredients: [
                { kind: "Farmers Gin", qty: 1.5 },
                { kind: "Lemon", qty: 1 },
                { kind: "Simple Syrup", qty: 0.5 },
                { kind: "Soda", qty: 2 },
                { kind: "Angostura", qty: "dash" },
            ],  
            garnish: "Maraschino Cherry",
            served: "Tall",
        },  
        Martini: {
            ingredients: [
                {
                    // Evaluate a path to get the first ingredient of the Tom Collins.
                    kind: $.cocktails["Tom Collins"].ingredients[0].kind, 
                    // or $["cocktails"]["Tom Collins"]["ingredients"][0]["kind"],
                    qty: 1
                },
                { kind: "Dry White Vermouth", qty: 1 },
            ],
            garnish: "Olive",
            served: "Straight Up",
        },
        "Gin Martini": self.Martini,
    }   
}   

