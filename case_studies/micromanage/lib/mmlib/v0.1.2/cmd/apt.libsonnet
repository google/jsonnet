local pip = import 'pip.libsonnet';

{
  local libos = self,

  // A mixin on top of an image or instance that allows high level installation of apt packages.
  Mixin:: {
    aptSourcesListDir:: '/etc/apt/sources.list.d',

    aptKeyUrls:: [],
    // { foo: "..." } will add a foo.list containing the given content.
    aptRepoLines:: {},
    // { foo: "..." } will add a foo.list fetched from the given URL.
    aptRepoUrls:: {},

    // Add the packages you want to install here.
    aptPackages:: [],

    local repoLineCommands = [
      'echo "%s" > %s/%s.list' % [self.aptRepoLines[k], self.aptSourcesListDir, k]
      for k in std.objectFields(self.aptRepoLines)
    ],
    local repoUrlCommands = [
      'curl -o %s/%s.list %s' % [self.aptSourcesListDir, k, self.aptRepoUrls[k]]
      for k in std.objectFields(self.aptRepoUrls)
    ],
    local keyCommands = ['curl --silent %s | apt-key add -' % [url] for url in self.aptKeyUrls],

    local env = 'DEBIAN_FRONTEND=noninteractive',
    local opts = '-o Dpkg::Options::=--force-confdef -o Dpkg::Options::=--force-confold',

    local updateCommands = ['apt-get -qq -y update'],
    local installCommands = [
      '%s apt-get -qq -y %s install %s' % [env, opts, std.join(' ', self.aptPackages)],
    ],

    cmds+: repoLineCommands + repoUrlCommands + keyCommands
           + updateCommands + installCommands,
  },

  // TODO: This would work better as an AptPipGlue mixin instead of extending Mixin.
  // Similar to `Mixin` above but also installs pip if you have any pip packages.
  PipMixin:: pip.Mixin {
    aptPackages+: if std.length(self.pipPackages) == 0 then [] else ['python-pip'],
  },
}
