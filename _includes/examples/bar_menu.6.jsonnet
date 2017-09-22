// bar_menu.6.jsonnet
{
    cocktails: (import "martinis.libsonnet") + {
        Manhattan: {
            ingredients: [
                { kind: "Rye", qty: 2.5 },
                { kind: "Sweet Red Vermouth", qty: 1 },
                { kind: "Angostura", qty: "dash" },
            ],
            garnish: importstr "bar_menu.6.manhattan_garnish.txt",
            served: "Straight Up",
        },
        Cosmopolitan: {
            ingredients: [
                { kind: "Vodka", qty: 1.5 },
                { kind: "Cointreau", qty: 1 },
                { kind: "Cranberry Juice", qty: 2 },
                { kind: "Lime Juice", qty: 1 },
            ],
            garnish: "Lime Wheel",
            served: "Straight Up",
        },
    },
}
