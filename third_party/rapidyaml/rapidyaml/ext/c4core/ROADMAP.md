# ROADMAP

## New features

These changes will provide new features, and client code can be kept
unchanged.


## API changes

These changes will require client code to be updated.

* [breaking] drop use of C-style sprintf() formats in error messages and
  assertions. Change the implementation to use c4::format()
  ```c++
  C4_ASSERT_MSG(sz > s.size(), "sz=%zu s.size()=%zu", sz, s.size());
  // ... the above changes to:
  C4_ASSERT_MSG(sz > s.size(), "sz={} s.size()={}", sz, s.size());
  ```

## Implementation changes

* drop calls to sprintf() in charconv.hpp.
