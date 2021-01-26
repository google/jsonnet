import ryml
import ruamel.yaml
import yaml
import timeit
import time
import prettytable
from collections import OrderedDict as odict


class RunResults:

    __slots__ = ('name', 'count', 'time', 'avg', 'MBps', 'timeit')

    def __init__(self, name, time, count, MB, timeit):
        self.name = name
        self.time = time
        self.count = count
        self.avg = time / count
        self.MBps = MB / self.time / 1000.0
        self.timeit = timeit

    def __str__(self):
        fmt = "{}: count={} time={:.3f}ms avg={:.3f}ms MB/s={:.3f}"
        fmt = fmt.format(self.name, self.count, self.time, self.avg, self.MBps)
        return fmt


class BmCase:

    def __init__(self, filename):
        with open(filename, "r") as f:
            src = f.read()
        self.src_as_str = src
        self.src_as_bytes = bytes(src, "utf8")
        self.src_as_bytearray = bytearray(src, "utf8")

    def run(self, bm_name, cls):
        obj = cls()
        method = getattr(obj, bm_name)
        self.count = 0
        self.MB = 0
        def fn():
            method(self)
            self.count += 1
            self.MB += len(self.src_as_str)
        t = timeit.Timer(fn)
        delta = time.time()
        result = t.autorange()
        delta = 1000. * (time.time() - delta)
        name = bm_name + ":" + cls.__name__
        return RunResults(name, delta, self.count, self.MB, result)


class RymlRo:

    def parse(self, case):
        r = ryml.parse(case.src_as_bytearray)


class RymlRoReuse:

    def __init__(self):
        self.tree = ryml.Tree()

    def parse(self, case):
        ryml.parse(case.src_as_bytearray, tree=ryml.Tree())



class RymlInSitu:

    def parse(self, case):
        r = ryml.parse_in_situ(case.src_as_bytearray)


class RymlInSituReuse:

    def __init__(self):
        self.tree = ryml.Tree()

    def parse(self, case):
        self.tree.clear()
        self.tree.clear_arena()
        ryml.parse_in_situ(case.src_as_bytearray, tree=self.tree)


class RuamelYaml:

    def parse(self, case):
        r = ruamel.yaml.load(case.src_as_str, Loader=ruamel.yaml.Loader)


class PyYaml:

    def parse(self, case):
        r = yaml.safe_load(case.src_as_str)


def run(filename):
    case = BmCase(filename)
    approaches = (RuamelYaml,
                  PyYaml,
                  RymlRo,
                  RymlRoReuse,
                  RymlInSitu,
                  RymlInSituReuse)
    benchmarks = ('parse', )
    for bm in benchmarks:
        results = odict()
        for cls in approaches:
            r = case.run(bm, cls)
            results[r.name] = r
            print(r)
        table = prettytable.PrettyTable()
        table.field_names = ["case", "count", "time(ms)", "avg(ms)", "avg_read(MB/s)"]
        table.align["case"] = "l"
        def f(v): return "{:.3f}".format(v)
        for v in results.values():
            table.add_row([v.name, v.count, f(v.time), f(v.avg), f(v.MBps)])
        print(table)


if __name__ == "__main__":
    import sys
    if len(sys.argv) < 2:
        raise Exception("")
    filename = sys.argv[1]
    run(filename)
