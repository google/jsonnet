
## CPU info
```
i7-6800K CPU @ 3.40GHz
CPU Caches:
  L1 Data 32K (x6)
  L1 Instruction 32K (x6)
  L2 Unified 256K (x6)
  L3 Unified 15360K (x1)
Load Average: 0.99, 0.49, 0.29
```


## JSON file (linux)

```
----------------------------------------------------------------------------------------------------
Case    Benchmark             Compilation       Time             CPU      Iterations UserCounters...
----------------------------------------------------------------------------------------------------
json    yamlcpp        clangxx7.0-Release    2951978 ns      2949570 ns          237 bytes_per_second=15.1398M/s
json    ryml_ro        clangxx7.0-Release     132882 ns       132821 ns         5273 bytes_per_second=336.21M/s  items_per_second=2.17586M/s
json    ryml_rw        clangxx7.0-Release     132670 ns       132613 ns         5303 bytes_per_second=336.738M/s items_per_second=2.17928M/s
json    ryml_ro_reuse  clangxx7.0-Release     115183 ns       115123 ns         6215 bytes_per_second=387.898M/s items_per_second=2.51037M/s
json    ryml_rw_reuse  clangxx7.0-Release      98509 ns        98464 ns         7098 bytes_per_second=453.526M/s items_per_second=2.9351M/s
----------------------------------------------------------------------------------------------------
json    yamlcpp            gxx8.2-Release    2744383 ns      2741505 ns          259 bytes_per_second=16.2888M/s
json    ryml_ro            gxx8.2-Release     115434 ns       115369 ns         5912 bytes_per_second=387.068M/s items_per_second=2.505M/s
json    ryml_rw            gxx8.2-Release     114051 ns       113990 ns         6056 bytes_per_second=391.752M/s items_per_second=2.53531M/s
json    ryml_ro_reuse      gxx8.2-Release     105970 ns       105914 ns         6596 bytes_per_second=421.625M/s items_per_second=2.72864M/s
json    ryml_rw_reuse      gxx8.2-Release     103779 ns       103732 ns         6742 bytes_per_second=430.494M/s items_per_second=2.78604M/s
----------------------------------------------------------------------------------------------------
json    yamlcpp          clangxx7.0-Debug   27435004 ns     27415168 ns           26 bytes_per_second=1.62887M/s
json    ryml_ro          clangxx7.0-Debug    1655051 ns      1653997 ns          424 bytes_per_second=26.9987M/s items_per_second=174.728k/s
json    ryml_rw          clangxx7.0-Debug    1649918 ns      1648922 ns          424 bytes_per_second=27.0818M/s items_per_second=175.266k/s
json    ryml_ro_reuse    clangxx7.0-Debug     724446 ns       724002 ns          961 bytes_per_second=61.6791M/s items_per_second=399.17k/s
json    ryml_rw_reuse    clangxx7.0-Debug     721836 ns       721423 ns          970 bytes_per_second=61.8996M/s items_per_second=400.597k/s
----------------------------------------------------------------------------------------------------
json    yamlcpp              gxx8.2-Debug   29133193 ns     29113788 ns           24 bytes_per_second=1.53384M/s
json    ryml_ro              gxx8.2-Debug    1199038 ns      1198311 ns          578 bytes_per_second=37.2656M/s items_per_second=241.173k/s
json    ryml_rw              gxx8.2-Debug    1194125 ns      1193474 ns          585 bytes_per_second=37.4166M/s items_per_second=242.15k/s
json    ryml_ro_reuse        gxx8.2-Debug     613111 ns       612730 ns         1130 bytes_per_second=72.8801M/s items_per_second=471.66k/s
json    ryml_rw_reuse        gxx8.2-Debug     615212 ns       614905 ns         1124 bytes_per_second=72.6223M/s items_per_second=469.991k/s
```


## YAML file (linux)

```
----------------------------------------------------------------------------------------------------
Case    Benchmark             Compilation       Time             CPU      Iterations UserCounters...
----------------------------------------------------------------------------------------------------
travis  yamlcpp        clangxx7.0-Release     480382 ns       480037 ns         1460 bytes_per_second=8.07978M/s
travis  ryml_ro        clangxx7.0-Release      34300 ns        34285 ns        20435 bytes_per_second=113.127M/s items_per_second=3.2667M/s
travis  ryml_rw        clangxx7.0-Release      34299 ns        34282 ns        20559 bytes_per_second=113.136M/s items_per_second=3.26698M/s
travis  ryml_ro_reuse  clangxx7.0-Release      29431 ns        29419 ns        23805 bytes_per_second=131.838M/s items_per_second=3.80702M/s
travis  ryml_rw_reuse  clangxx7.0-Release      29490 ns        29478 ns        23849 bytes_per_second=131.577M/s items_per_second=3.79948M/s
----------------------------------------------------------------------------------------------------
travis  yamlcpp            gxx8.2-Release     471533 ns       471181 ns         1447 bytes_per_second=8.23164M/s
travis  ryml_ro            gxx8.2-Release      25020 ns        25009 ns        27616 bytes_per_second=155.086M/s items_per_second=4.47835M/s
travis  ryml_rw            gxx8.2-Release      24851 ns        24839 ns        27953 bytes_per_second=156.151M/s items_per_second=4.50908M/s
travis  ryml_ro_reuse      gxx8.2-Release      22101 ns        22092 ns        31662 bytes_per_second=175.569M/s items_per_second=5.0698M/s
travis  ryml_rw_reuse      gxx8.2-Release      21995 ns        21986 ns        31771 bytes_per_second=176.413M/s items_per_second=5.09417M/s
----------------------------------------------------------------------------------------------------
travis  yamlcpp          clangxx7.0-Debug    3687829 ns      3684240 ns          190 bytes_per_second=1078.02k/s
travis  ryml_ro          clangxx7.0-Debug     470504 ns       470249 ns         1492 bytes_per_second=8.24795M/s items_per_second=238.171k/s
travis  ryml_rw          clangxx7.0-Debug     468722 ns       468495 ns         1494 bytes_per_second=8.27884M/s items_per_second=239.063k/s
travis  ryml_ro_reuse    clangxx7.0-Debug     383510 ns       383335 ns         1826 bytes_per_second=10.118M/s  items_per_second=292.172k/s
travis  ryml_rw_reuse    clangxx7.0-Debug     382344 ns       382179 ns         1820 bytes_per_second=10.1486M/s items_per_second=293.057k/s
----------------------------------------------------------------------------------------------------
travis  yamlcpp              gxx8.2-Debug    3917888 ns      3913856 ns          180 bytes_per_second=1014.77k/s
travis  ryml_ro              gxx8.2-Debug     367717 ns       367522 ns         1904 bytes_per_second=10.5534M/s items_per_second=304.743k/s
travis  ryml_rw              gxx8.2-Debug     367291 ns       367101 ns         1908 bytes_per_second=10.5655M/s items_per_second=305.093k/s
travis  ryml_ro_reuse        gxx8.2-Debug     311178 ns       311031 ns         2251 bytes_per_second=12.4701M/s items_per_second=360.093k/s
travis  ryml_rw_reuse        gxx8.2-Debug     311510 ns       311362 ns         2248 bytes_per_second=12.4569M/s items_per_second=359.71k/s
----------------------------------------------------------------------------------------------------
```


## YAML file (windows)

```
----------------------------------------------------------------------------------------------------
Case      Benchmark          Compilation       Time             CPU      Iterations UserCounters...
----------------------------------------------------------------------------------------------------
appveyor  yamlcpp           vs2017-Debug   25423992 ns     25669643 ns           28 bytes_per_second=84.4185k/s
appveyor  ryml_rw_reuse     vs2017-Debug     328672 ns       329641 ns         2133 bytes_per_second=6.41971M/s items_per_second=218.419k/s
appveyor  yamlcpp         vs2017-Release     394764 ns       399013 ns         1723 bytes_per_second=5.30359M/s
appveyor  ryml_rw_reuse   vs2017-Release      20737 ns        20856 ns        34462 bytes_per_second=101.466M/s items_per_second=3.45219M/s
----------------------------------------------------------------------------------------------------
```
