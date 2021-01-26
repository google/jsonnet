# Roadmap

Roughly in order of priority:

  * Conformance of the parser to the YAML standard:
    * Increase success rate of YAML test suite parsing (as of May 2020,
      ~30/1300 tests are failing)
    * Cross-check the suite test correctness with the events specified in
      each test. (Currently the tests are only checking for successful
      parsing and idempotent emitting).
  * Cleanup:
    * Review & cleanup API surface.
    * turn calls to `C4_ASSERT()` into calls to `RYML_ASSERT()`
    * use `c4::MemoryResource` in place of `c4::yml::MemoryResource`, and
      remove `c4::yml::MemoryResource`
    * same for allocators and error callbacks: Use the facilities from c4core.
  * Review `parse()` API: add suffixes `_in_situ` and `_in_arena` to clarify
    intent. Ie:
    * rename `parse(substr)` to `parse_in_situ(substr)`
    * rename `parse(csubstr)` to `parse_in_arena(csubstr)`
  * Add emit formatting controls:
    * add single-line flow formatter
    * add multi-line flow formatters
      * indenting
      * non indenting
    * keep current block formatter
    * add customizable linebreak limits (number of columns) to every formatter
    * add per node format flags
    * (lesser priority) add auto formatter using reasonable heuristics to
      switch between other existing formatters
  * use `csubstr` instead of `csubstr const&` in return and parameter types, but
    quantify the performance effect.
  * Investigate possibility of comment-preserving roundtrips
