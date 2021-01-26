# Contributing

Thanks for your contribution!

* Make sure to clone the project with `git clone --recursive` so that
  the submodules are initialized correctly.
* To enable both tests and benchmarks, configure ryml with `-DRYML_DEV=ON`
  when calling cmake. To enable only tests, use `-DRYML_BUILD_TESTS=ON`; to
  enable only benchmarks use `-DRYML_BUILD_BENCHMARKS=ON`. All these flags
  are disabled by default.
* Submitted pull requests should target the `dev` branch. (The `master`
  branch is the stable branch, and merges from `dev` to `master` are done
  only if the tests succeed.)
* Code style for pull requests should respect the existing code style:
    ```c++
    if(foo)  // no space before parens
    {   // curly brackets on next line
        // no tabs; indent with 4 spaces
        bar();
    }
    ```
