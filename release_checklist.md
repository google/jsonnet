# Checklist for building Jsonnet releases

## Are jsonnet & go-jsonnet in sync?
1) Check out master from both
1) Check go-jsonnet/cpp-jsonnet submodule is HEAD of jsonnet tree, if not:
1) Inside cpp-jsonnet:
1) `git checkout master`
1) `git pull`
1) `cd ..`
1) `git checkout -b release`
1) `git commit -a -m 'update cpp-jsonnet'`
1) `./tests.sh`  # Have to commit before running this, or it resets the submodule

## Is HEAD ready?
1) Checkout master
1) `make test`
1) `bazel test ...:all`
1) `mkdir build ; cd build ; cmake .. ; make ; make test`
1) modify `include/libjsonnet.h` to bump the version number but add `-pre1`
1)
```
find test_cmd -name '*.cpp' -o -name '*.golang' -o -name '*.stdout' -o -name '*.stderr' -o -name 'stdout' -o -name 'stderr' | \
  xargs sed -i 's/ v0[.][0-9.]*/ NEW_VERSION_GOES_HERE/g'
```
1) Check if any changes to the documentation are necessary by checking commits since previous release (especially stdlib additions).
1) In stdlib documentation replace any "Available in upcoming release." with "Available since NEW_VERSION_GOES_HERE".
1) `python setup.py build sdist`
1) `twine upload dist/whatever.tar.gz`  (Needs credentials on pypi)
1) ON ANOTHER MACHINE AND CHECK THAT THE VERSION IS CORRECT AND IT ACTUALLY IS BUILDING THINGS IN THE LOG: `sudo pip install jsonnet --pre --upgrade`
1) `python`
1) `import _jsonnet`
1) `_jsonnet.evaluate_snippet('foo', '1+1')`
1) Checkout go-jsonnet master
1) update cpp-jsonnet commit
1) `./tests`

## Do release
1) Remove -pre from version in `include/libjsonnet.h`
1) Update version in `CMakeLists.txt`
1) Update `test_cmd` golden file version numbers
1) run test again!
1) commit and push
1) Make release in github, write release notes by checking commits since previous release
1) `python setup.py build sdist`
1) `twine upload dist/whatever.tar.gz` (the version without the -pre)
1) Post to Jsonnet Google Group
1) Update the live version of the website
