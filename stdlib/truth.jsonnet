{


    expect()::
        self,

    that(subject)::
        if std.objectHas(self, "subject") then
            error("subject cannot be set twice")
        else
            self + { subject: subject },


    // returns the first string representing a failed expecatation or returns
    // null if no assertions failed.
    expectations(arr)::
        std.foldl(
            function(cur, result)
                if cur != null then
                    cur
                else
                    if result != null then
                        result,
            arr,
            null,
        ),

    fatalFailures: false,

    fatal()::
        self + { fatalFailures: true},

    fail(message)::
        if self.fatalFailures then
            error(message)
        else
            message,

    local fail = self.fail,

    named(name)::
        if std.objectHas(self, "name") then
            error("name cannot be set twice")
        else
            self + { name: name },


    // propositions

    hasType(object)::
        if std.type(self.subject) != object then
            fail("expected '%s' to be of type '%s' but was of type '%s'" % [ self.subject, object, std.type(self.subject) ]),

    isEqualTo(object)::
        if self.subject != object then
            fail("expected '%s' and '%s' to be equal" % [ self.subject, object ]),

    isNotEqualTo(object)::
        if self.subject == object then
            fail("expected '%s' and '%s' not to be equal" % [ self.subject, object ]),

    isTrue()::
        self.expectations([
            self.hasType("boolean"),
            if !self.subject then
                fail("expected true"),
        ]),

    isFalse()::
        self.expectaions([
            self.hasType("boolean"),
            if self.subject then
                fail("expected false"),
        ]),


}
