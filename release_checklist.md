# Jsonnet Release Checklist

## Preparation

Before doing the release, make sure that the project is in good state:

- Triage issues – make sure that all issues are labelled.
- Check for release-blocking bugs. All bugs which results in a wrong result of the evaluation are release-blocking.
- Go through the open PRs – consider merging any outstanding ones. Do not merge big changes right before the release.
- Sync google/go-jsonnet and google/jsonnet
  1. Check out master from both
  1. Use the `update_cpp_jsonnet.sh` script in the go-jsonnet repository to sync it to jsonnet master.
  1. Run tests: `./tests.sh`
  1. Fix any failing tests.
  1. Send a PR with the updated version, so that all the CI tests are run.
- Check that CI is green in both C++ and Go jsonnet: Checks should run automatically on pushes to `master`,
  so you can check the status on the latest master commit.
- Optionally, run tests locally to double-check.
  - C++:
    1. `make test`
    1. `bazel test ...:all`
    1. `mkdir build ; cd build ; cmake .. ; make ; make test`
  - Go:
    1. `./tests.sh`
    2. `bazel test //:go_default_test`
- Optionally, make sure the Python bindings build and work correctly locally.
- Go through the commits since the last release and prepare the release notes.
  The release notes should have separate sections for the language changes and
  changes specific to each implementation.
- Make sure that the stdlib documentation is complete. Check the git history for any
  changes to stdlib (including builtins) and make sure they are reflected in the documentation.
- Make sure that you can build the website locally.
  - Download or build libjsonnet.wasm (see README.md for instructions)
  - `jekyll serve -s doc/` (you need Jekyll 4.3.0 or later) to serve the site locally to check it.
  - Check that it works in two different browsers. Make sure that live evaluation
    in the tutorial works.

All the above points apply to both google/jsonnet and google/go-jsonnet.

## Make a final decision to release

A this point you should be confident that the project is ready for the release, and know what
will be included in that release.

## Release the C++ version

1. Checkout the `prepare-release` branch.
1. Fast-forward merge it to match `master` (it should fast-forward cleanly!):
   `git merge --ff-only upstream/master`
   (`upstream` might have a different name depending on your local git checkout)
1. Modify `include/libjsonnet.h` to the version number for the new release. If you are making a
   release candidate and not a full release, include a suffix like `-rc1`.
1. Modify `MODULE.bazel` to match the new release version number.
1. Update `test_cmd` golden file version numbers:

   ```
   ./tools/scripts/replace_test_cmd_version.sh <NEW_VERSION>
   ```

1. In `doc/_stdlib_gen/stdlib-content.jsonnet` replace any `availableSince: "upcoming"` with NEW_VERSION".

1. Commit the version number changes. Typically I use a commit message like:
   `git commit -m "release: prepare to release v0.21.0-rc2"`

1. Push the `prepare-release` up to github. CI checks should automatically be triggered on the branch.
   Wait for them to be green, in: https://github.com/google/jsonnet/actions
   If CI checks don't pass, fix the problems.
1. Optionally: Perform a test Python build & publish to `testpypi`. This is important to do if the Python
   build/publish process has changed, but less important otherwise. To do this, on the GitHub Actions page,
   manually trigger the "Build and Publish Python Package" workflow. Set the options appropriately
   (check the "Upload generate package files to PyPI" checkbox, select "testpypi" as the instance to
   publish to). You will need to explicitly approve the publish step after the build step is completed.
   This lets you check the whole workflow end to end and check that all the steps succeeded.
   If you just want to verify that building the Python packages works (without checking the publishing
   process) you can run the workflow but leave "Upload generate package files to PyPI" unchecked.
1. On the GitHub Actions page, manually trigger the "Release" workflow.
   This will create a new _draft_ release in GitHub, and attach a tarball archive of the repository content.
   The version number for the release is extracted directly from the `LIB_JSONNET_VERSION` in the jsonnet header.
1. On the GitHub Actions page, manually trigger the "Build and Publish Python Package" workflow. In the
   workflow settings, check "Upload generate package files to PyPI" and select "pypi" as the instance to
   publish to.
   This will build Python packages for a variety of systems. It will then _pause_ and wait for an approval
   before publishing them to PyPI. I generally wait until I'm really ready to hit the publish button on the
   release in GitHub before approving the PyPI publish step (but I let PyPI publishing complete before hitting
   the GitHub button so that I can confirm that the packages get uploaded successfully).
1. Do any last minute checks you want to do!
   If you want to manually check any of the built Python packages before they're published, you can download
   them GitHub Actions - they are available as "Artifacts" on the workflow. If you want to download and check
   the source tarball you can download it from the draft release in GitHub.
1. Finalise the release notes in the GitHub release UI.
1. Approve PyPI publishing and let it complete.
1. Check that the new/proposed version tag specified in the GitHub release is correct and hit the Publish button
   (you do not have to manually create the tag - GitHub creates tag when you publish the release).
1. Fast-forward merge the `prepare-release` branch to `master` and push `master` so that the repo master state
   matches the release that was just published.
1. TO BE WRITTEN: Update the Bazel module at https://registry.bazel.build/modules/jsonnet.

## Release the Go version

This should be done _after_ the C++ version release, because the Go version depends on the C++ version: it uses
the standard library from the C++ version, and some header files.

1. In `go-jsonnet`, switch to the `prepare-release` branch and bring it up to date with `master`:
   Fast-forward merge with `git merge --ff-only`.
1. In `go-jsonnet` you can run the `update_cpp_jsonnet.sh` script and pass in the published version tag
   for the C++ version; this will update go-jsonnet to pin it to that specific release (instead of pinning
   to a commit hash). Pinning to the release is better, because it will use the stable jsonnet release
   source tarball (which has a stable hash that can be used) instead of the unstable GitHub automatic
   tarball for the commit.
1. Modify `vm.go` to update the version number.
1. Modify `MODULE.bazel` to update the version number.
1. Run tests locally, commit, and push to the `go-jsonnet` `prepare-release` branch. CI checks should run
   automatically on this branch - check the results in https://github.com/google/go-jsonnet/actions
1. Optionally: Perform a preflight Python build & publish by manually triggering the workflow in GitHub actions.
1. Start a release Python build & publish by manually triggering the workflow in GitHub Actions.
   It will wait for an approval before publishing (don't approve it yet).
   Check that the Python packages are all built successfully.
1. Manually trigger the Release workflow in GitHub Actions to create a new _draft_ release with all the
   built packages attached to it.
1. Fill in the release notes in the GitHub draft release.
1. Do any final release checks (including downloading built packages and checking them locally if you want).
1. Approve the Python publish step and wait for that to complete successfully.
1. Check that the new tag number is correct and click Publish in the GitHub draft release.
1. Fast-forward merge the `prepare-release` branch to `master` and push `master` so that the repo master state
   matches the release that was just published.
1. TO BE WRITTEN: Update the Bazel module at https://registry.bazel.build/modules/jsonnet_go.

## Update the website

In google/jsonnet:

- Build or download libjsonnet.wasm (see README.md)
- `jekyll serve -s docs/`
- Check that the local version works in two different browsers. Make sure that live evaluation in the tutorial works.
- `tools/scripts/push_docs.sh`
- Check that the public works in two different browsers. Make sure that you are getting the new version (and not an old cached version). Make sure that live evaluation in the tutorial works.

## Make the announcement

- Send an email to the mailing list.
- Announce the new release on Slack.

## After the release

It's a good time to merge big PRs.
