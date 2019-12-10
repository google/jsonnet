{

  // The base class of all Micromanage services.  While it is not necessary to use this, it does
  // provide the following useful features:
  // - Utilities for generating unique names and easy referencing via those names.
  // - Defaults for required fields.
  // The function takes the outer service (or null) and a name that must be unique amongst its
  // siblings.
  Service(outer, name): {
    // The final component of the full name.
    name:: std.toString(name),

    // The sequence of names leading from the outermost service to here.
    nameArray:: (if outer == null then [] else outer.nameArray) + [self.name],

    // The globally unique name of the service.
    fullName:: std.join('-', self.nameArray),

    // Generate a new name prefixed by the service's name.
    prefixName(name):: std.join('-', self.nameArray + [std.toString(name)]),

    // The environment in which this service will be deployed.
    environment: 'default',

    // The Terraform resources that will be deployed.
    infrastructure: {},

    // Any Terraform outputs that cannot be known until after deployment, such as generated
    // addresses.
    outputs: {},
  } + {

    // Prevent overriding the fundamental identity characteristics.
    assert self.name == super.name,
    assert self.nameArray == super.nameArray,
    assert self.fullName == super.fullName,
    assert self.prefixName('foo') == super.prefixName('foo'),
  },
}
