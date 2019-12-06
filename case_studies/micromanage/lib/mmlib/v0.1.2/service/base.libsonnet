{
    Service(parent, name): {
        parent:: parent,
        name:: std.toString(name),

        nameArray:: (if parent == null then [] else parent.nameArray) + [self.name],
        fullName:: std.join('-', self.nameArray),
        prefixName(name):: std.join('-', self.nameArray + [std.toString(name)]),

        environment: "default",
        infrastructure: {},
        outputs: {},
    } + {
        // Prevent overriding the fundamental identity characteristics.
        assert self.name == super.name,
        assert self.nameArray == super.nameArray,
        assert self.fullName == super.fullName,
        assert self.prefixName('foo') == super.prefixName('foo'),
    },
}
