// bar_menu.7.jsonnet
local utils = import "bar_menu_utils.libsonnet";
{
    local my_gin = "Farmers Gin",
    cocktails: {
        "Bee's Knees": {
            // Divide 4oz among the 3 ingredients.
            ingredients: utils.equal_parts(4, [
                "Honey Syrup",
                "Lemon Juice",
                my_gin,
            ]),
            garnish: "Lemon Twist",
            served: "Straight Up",
        },
        Negroni: {
            // Divide 3oz among the 3 ingredients.
            ingredients: utils.equal_parts(3, [
                my_gin,
                "Sweet Red Vermouth",
                "Campari",
            ]),
            garnish: "Orange Peel",
            served: "On The Rocks",
        },
    },
}
