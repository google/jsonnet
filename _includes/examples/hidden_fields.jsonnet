// hidden_fields.jsonnet
{
    local Base = { w: 1, x: 2, y:: 3, z:: 4 },
    foo: Base { w: super.w, x:: super.x, y: super.y, z::: super.z },
}
