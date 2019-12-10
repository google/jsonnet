{
  // A mixin on top of an image or instance that allows high level installation of pip packages.
  Mixin:: {

    // Add pip packages here.
    pipPackages:: [],

    cmds+:
      if std.length(self.pipPackages) == 0 then
        []
      else
        ['pip install ' + std.join(' ', self.pipPackages)],
  },

}
