import os
import sys
import argparse
import requests
import flask
import json
import re
import yaml
import shutil
import mmh3

from munch import Munch, munchify
from flask import render_template, redirect, url_for, send_from_directory
from markupsafe import escape


def log(*args, **kwargs):
    print(*args, **kwargs, flush=True)


def myhash_combine(curr, value):
    return curr ^ (value + 0x9e3779b9 + (curr<<6) + (curr>>2))


def optionals(obj, *attrs):
    ret = []
    for attr in attrs:
        if not hasattr(obj, attr):
            log("attr not present:", attr)
            continue
        ret.append(getattr(obj, attr))
    return ret


def myhash(*args):
    h = 137597
    for a in args:
        if isinstance(a, str):
            if a == "":
                continue
            b = bytes(a, "utf8")
        else:
            b = bytes(a)
        hb = mmh3.hash(b, signed=False)
        h = myhash_combine(h, hb)
    s = hex(h)
    return s[2:min(10, len(s))]


def copy_file_to_dir(file, dir):
    dir = os.path.abspath(dir)
    src = os.path.abspath(file)
    dst = f"{dir}/{os.path.basename(src)}"
    if not os.path.exists(dir):
        os.makedirs(dir)
    if os.path.exists(dst):
        os.remove(dst)
    log("copy:", src, "-->", dst)
    shutil.copy(src, dst)
    return dst


def chk(f):
    log(f"looking for file:", f)
    assert os.path.exists(f), f
    return f


def load_yml_file(filename):
    if not os.path.exists(filename):
        raise Exception(f"not found: {filename}")
    with open(filename) as f:
        return load_yml(f.read())


def dump_yml(data, filename):
    with open(filename, "w") as f:
        yaml.safe_dump(data, f)


def load_yml(yml):
    return munchify(yaml.safe_load(yml))


def dump_json(data, filename):
    with open(filename, "w") as f:
        f.write(json.dumps(data, indent=2, sort_keys=True))


def main():
    #
    parser = argparse.ArgumentParser(description="Browse benchmark results", prog="bm")
    parser.add_argument("--debug", action="store_true", help="enable debug mode")
    subparsers = parser.add_subparsers()
    #
    sp = subparsers.add_parser("create", help="create benchmark collection")
    sp.set_defaults(func=BenchmarkCollection.create_new)
    sp.add_argument("--debug", action="store_true", help="enable debug mode")
    sp.add_argument("filename", type=str, help="the YAML file with the benchmark specs")
    sp.add_argument("target", type=str, help="the directory to store the results")
    #
    sp = subparsers.add_parser("meta", help="get the required meta-information: cpu info, commit data")
    sp.set_defaults(func=add_meta)
    sp.add_argument("--debug", action="store_true", help="enable debug mode")
    sp.add_argument("results", type=str, help="the directory with the results")
    sp.add_argument("cmakecache", type=str, help="the path to the CMakeCache.txt file used to build the benchmark binaries")
    sp.add_argument("build_type", type=str, help="the build type, eg Release Debug MinSizeRel RelWithDebInfo")
    #
    sp = subparsers.add_parser("add", help="add benchmark results")
    sp.set_defaults(func=add_results)
    sp.add_argument("--debug", action="store_true", help="enable debug mode")
    sp.add_argument("results", type=str, help="the directory with the results")
    sp.add_argument("target", type=str, help="the directory to store the results")
    #
    sp = subparsers.add_parser("serve", help="serve benchmark results")
    sp.set_defaults(func=serve)
    sp.add_argument("--debug", action="store_true", help="enable debug mode")
    sp.add_argument("bmdir", type=os.path.abspath, default=os.getcwd(), help="the directory with the results. default=.")
    sp.add_argument("-H", "--host", type=str, default="localhost", help="host. default=%(default)s")
    sp.add_argument("-p", "--port", type=int, default=8000, help="port. default=%(default)s")
    #
    sp = subparsers.add_parser("export", help="export static html")
    sp.set_defaults(func=freeze)
    sp.add_argument("--debug", action="store_true", help="enable debug mode")
    sp.add_argument("bmdir", type=os.path.abspath, default=os.getcwd(), help="the directory with the results. default=.")
    #
    sp = subparsers.add_parser("deps", help="install server dependencies")
    sp.set_defaults(func=lambda _: download_deps())
    sp.add_argument("--debug", action="store_true", help="enable debug mode")
    #
    args = parser.parse_args(sys.argv[1:] if len(sys.argv) > 1 else ["serve"])
    if args.debug:
        log(args)
    args.func(args)


def get_manifest(args):
    bmdir = os.path.abspath(args.bmdir)
    manif_yml = os.path.join(bmdir, "manifest.yml")
    manif_json = os.path.join(bmdir, "manifest.json")
    manif = load_yml_file(manif_yml)
    dump_json(manif, manif_json)
    return manif


# ------------------------------------------------------------------------------
# ------------------------------------------------------------------------------
# ------------------------------------------------------------------------------

app = flask.Flask(__name__,
                  template_folder='template')


def _setup_app(args):
    def _s(prop, val):
        assert not hasattr(app, prop), prop
        setattr(app, prop, val)
    _s('args', args)
    _s('manifest', get_manifest(args))
    if args.debug:
        app.config["DEBUG"] = True


def freeze(args):
    "https://pythonhosted.org/Frozen-Flask/"
    from flask_frozen import Freezer
    _setup_app(args)
    freezer = Freezer(app)
    freezer.freeze(debug=args.debug)


def serve(args):
    _setup_app(args)
    app.run(host=args.host, port=args.port, debug=args.debug)


@app.route("/")
def home():
    log("requested home")
    return render_template("index.html")


@app.route("/<path>")
def other_(path):
    path = escape(path)
    d = app.args.bmdir
    log("requested other path:", path, "---", os.path.join(d, path))
    return send_from_directory(d, path)


@app.route("/static/<path>")
def static_(path):
    path = escape(path)
    d = os.path.join(app.args.bmdir, "static")
    log("requested static path:", path, "---", os.path.join(d, path))
    return send_from_directory(d, path, cache_timeout=1)  # timeout in seconds


@app.route("/bm/<commit>/<run>/<resultjson>")
def bm_(commit, run, resultjson):
    commit = escape(commit)
    run = escape(run)
    resultjson = escape(resultjson)
    d = os.path.join(app.args.bmdir, "runs", commit, run)
    log("requested result:", os.path.join(d, resultjson))
    return send_from_directory(d, resultjson, cache_timeout=1)  # timeout in seconds


# ------------------------------------------------------------------------------
# ------------------------------------------------------------------------------
# ------------------------------------------------------------------------------

def download_deps():
    deps = [
        "https://code.jquery.com/jquery-3.3.1.js",
        "https://code.jquery.com/jquery-3.3.1.js",
        "https://code.jquery.com/ui/1.12.1/jquery-ui.js",
        "https://cdn.datatables.net/1.10.20/js/jquery.dataTables.js",
        "https://cdn.datatables.net/1.10.20/js/jquery.dataTables.min.js",
        "https://cdn.datatables.net/1.10.20/css/jquery.dataTables.css",
        "https://cdn.datatables.net/1.10.20/css/jquery.dataTables.min.css",
        "https://www.chartjs.org/dist/2.9.1/Chart.min.js",
        #("https://cdnjs.cloudflare.com/ajax/libs/highlight.js/10.3.2/styles/github.css", "highlight.github.css"),
        ("https://cdnjs.cloudflare.com/ajax/libs/highlight.js/10.3.2/styles/github.min.css", "highlight.github.min.css"),
        #"https://cdnjs.cloudflare.com/ajax/libs/highlight.js/10.3.2/highlight.js",
        "https://cdnjs.cloudflare.com/ajax/libs/highlight.js/10.3.2/highlight.min.js",
    ]
    for src in deps:
        if type(src) == str:
            base = os.path.basename(src)
        else:
            src, base = src
        dst = f"{os.getcwd()}/static/{base}"
        download_url(src, dst)


def download_url(url, dst):
    log("download url:", url, "--->", dst)
    req = requests.get(url, stream=True)
    if req.status_code == 200:
        sz = 0
        with open(dst, 'wb') as f:
            for chunk in req:
                f.write(chunk)
                sz += len(chunk)
        log(f"........ finished: {sz}B")
    else:
        log(f"         error:", req.status_code, url)


# ------------------------------------------------------------------------------
# ------------------------------------------------------------------------------
# ------------------------------------------------------------------------------

class BenchmarkCollection:

    @staticmethod
    def create_new(args):
        dir = args.target
        filename = os.path.join(dir, "bm.yml")
        manifest = os.path.join(dir, "manifest.yml")
        if not os.path.exists(dir):
            os.makedirs(dir)
        shutil.copyfile(args.filename, filename)
        dump_yml(load_yml("""{runs: {}, bm: {}}"""), manifest)
        return __class__(dir)

    def __init__(self, dir):
        if not os.path.exists(dir):
            raise Exception(f"not found: {dir}")
        self.dir = os.path.abspath(dir)
        self.runs_dir = os.path.join(self.dir, "runs")
        self.manifest = os.path.join(self.dir, "manifest.yml")
        self.filename = os.path.join(self.dir, "bm.yml")
        self.specs = munchify(load_yml_file(self.filename))
        self.manif = munchify(load_yml_file(self.manifest))

    def add(self, results_dir):
        results_dir = os.path.abspath(results_dir)
        dst_dir, meta = self._read_run(results_dir)
        self._add_run(results_dir, dst_dir, meta)
        dump_yml(self.manif, self.manifest)

    def _read_run(self, results_dir):
        log("adding run...")
        id = f"{len(self.manif.runs.keys()):05d}"
        log(f"adding run: id={id}")
        meta = ResultMeta.load(results_dir)
        dst_dir = os.path.join(self.runs_dir, meta.name)
        return dst_dir, meta

    def _add_run(self, results_dir, dst_dir, meta):
        cats = self._add_meta_categories(meta)
        for filename in ("meta.yml",
                         "CMakeCCompiler.cmake",
                         "CMakeCXXCompiler.cmake",
                         "CMakeSystem.cmake",
                         "compile_commands.json"):
            filename = os.path.join(results_dir, filename)
            if os.path.exists(filename):
                copy_file_to_dir(filename, dst_dir)
            else:
                if not filename.endswith("compile_commands.json"):
                    raise Exception(f"wtf???? {filename}")
        for name, specs in self.specs.bm.items():
            if not hasattr(specs, 'variants'):
                filename = chk(f"{results_dir}/{name}.json")
                dst = copy_file_to_dir(filename, dst_dir)
                self._add_bm_run(name, specs, meta)
            else:
                for t in specs.variants:
                    tname = f"{name}-{t}"
                    filename = chk(f"{results_dir}/{tname}.json")
                    dst = copy_file_to_dir(filename, dst_dir)
                    self._add_bm_run(tname, specs, meta)

    def _add_bm_run(self, name, specs, meta):
        if name not in self.manif.bm.keys():
            self.manif.bm[name] = Munch(specs=specs, entries=[])
        entry = self.manif.bm[name]
        entry.specs = specs
        if meta.name not in entry.entries:
            entry.entries.append(meta.name)

    def _add_meta_categories(self, meta):
        run = Munch()
        for catname in ('commit', 'cpu', 'system', 'build'):
            meta_item = getattr(meta, catname)
            self._add_item_to_category(meta.name, catname, meta_item)
            run[catname] = meta_item.storage_id
        # build specs are too verbose; remove them
        self.manif.build[meta.build.storage_id].specs = Munch()
        self.manif.runs[meta.name] = run

    def _add_item_to_category(self, run, category_name, item):
        if not hasattr(self.manif, category_name):
            setattr(self.manif, category_name, Munch())
        category = getattr(self.manif, category_name)
        if item.storage_id not in category.keys():
            category[item.storage_id] = Munch(specs=item, entries=[])
        entry = category[item.storage_id]
        entry.specs = item
        if run not in entry.entries:
            entry.entries.append(run)


# -----------------------------------------------------------------------------
# -----------------------------------------------------------------------------
# -----------------------------------------------------------------------------

class ResultMeta(Munch):

    def __init__(self, results_dir, cmakecache, build_type):
        super().__init__(self)
        self.date = __class__.get_date()
        self.commit = __class__.get_commit(results_dir)
        self.cpu = __class__.get_cpu_info()
        self.system = __class__.get_sys_info()
        self.build = __class__.get_build_info(cmakecache, build_type)
        self.name = self._get_name()

    @staticmethod
    def load(results_dir):
        results_dir = os.path.join(os.path.abspath(results_dir), "meta.yml")
        data = load_yml_file(results_dir)
        return munchify(data)

    def save(self, results_dir):
        out = os.path.join(results_dir, "meta.yml")
        log("saving meta:", out)
        dump_yml(self, out)
        self.build.save(results_dir)

    @staticmethod
    def get_date():
        import datetime
        now = datetime.datetime.now()
        return now.strftime("%Y%m%d-%H%M%S")

    def _get_name(self):
        commit = self.commit.storage_name
        cpu = self.cpu.storage_name
        sys = self.system.storage_name
        build = self.build.storage_name
        name = f"{commit}/{cpu}-{sys}-{build}"
        return name

    @staticmethod
    def get_commit(results_dir):
        import git
        repo = git.Repo(results_dir, search_parent_directories=True)
        commit = repo.head.commit
        commit = {p: str(getattr(commit, p))
                  for p in ('message', 'summary', 'name_rev',
                            'author',
                            'authored_datetime',
                            'committer',
                            'committed_datetime',)}
        commit = Munch(commit)
        commit.message = commit.message.strip()
        commit.sha1 = commit.name_rev[:7]
        spl = commit.authored_datetime.split(" ")
        date = re.sub(r'-', '', spl[0])
        time = re.sub(r'(\d+):(\d+):(\d+).*', r'\1\2\3', spl[1])
        commit.storage_id = commit.sha1
        commit.storage_name = f"git{date}_{time}-{commit.sha1}"
        return commit

    @staticmethod
    def get_cpu_info():
        import cpuinfo
        nfo = cpuinfo.get_cpu_info()
        nfo = Munch(nfo)
        for a in ('cpu_version', 'cpu_version_string', 'python_version'):
            if hasattr(nfo, a):
                delattr(nfo, a)
        for a in ('arch_string_raw', 'brand_raw', 'hardware_raw', 'vendor_id_raw'):
            if not hasattr(nfo, a):
                setattr(nfo, a, '')
        nfo.storage_id = myhash(
            nfo.arch_string_raw, nfo.brand_raw, nfo.hardware_raw, nfo.vendor_id_raw,
            nfo.arch, nfo.bits, nfo.count, nfo.family, nfo.model, nfo.stepping,
            ",".join(nfo.flags), nfo.hz_advertised_friendly,
            nfo.l2_cache_associativity,
            nfo.l2_cache_line_size,
            nfo.l2_cache_size,
            nfo.l3_cache_size,
            *optionals('l1_data_cache_size', 'l1_instruction_cache_size')
        )
        nfo.storage_name = f"{nfo.arch.lower()}_{nfo.storage_id}"
        return nfo

    @staticmethod
    def get_sys_info():
        import platform
        uname = platform.uname()
        nfo = Munch(
            sys_platform=sys.platform,
            sys=platform.system(),
            uname=Munch(
                machine=uname.machine,
                node=uname.node,
                release=uname.release,
                system=uname.system,
                version=uname.version,
            )
        )
        nfo.storage_id = myhash(
            nfo.sys_platform,
            nfo.uname.machine,
        )
        nfo.storage_name = f"{nfo.sys_platform}_{nfo.storage_id}"
        return nfo

    @staticmethod
    def get_build_info(cmakecache_txt, buildtype):
        nfo = CMakeCache(cmakecache_txt)
        def _btflags(name):
            return (getattr(nfo, name), getattr(nfo, f"{name}_{buildtype.upper()}"))
        nfo.storage_id = myhash(
            buildtype,
            nfo.CMAKE_CXX_COMPILER_ID,
            nfo.CMAKE_CXX_COMPILER_VERSION,
            nfo.CMAKE_CXX_COMPILER_VERSION_INTERNAL,
            nfo.CMAKE_CXX_COMPILER_ABI,
            nfo.CMAKE_CXX_SIZEOF_DATA_PTR,
            nfo.CMAKE_C_COMPILER_ID,
            nfo.CMAKE_C_COMPILER_VERSION,
            nfo.CMAKE_C_COMPILER_VERSION_INTERNAL,
            nfo.CMAKE_C_COMPILER_ABI,
            nfo.CMAKE_C_SIZEOF_DATA_PTR,
            *_btflags("CMAKE_CXX_FLAGS"),
            *_btflags("CMAKE_C_FLAGS"),
            *_btflags("CMAKE_STATIC_LINKER_FLAGS"),
            *_btflags("CMAKE_SHARED_LINKER_FLAGS"),
        )
        #
        ccname = nfo.CMAKE_CXX_COMPILER_ID.lower()
        if ccname == "gnu":
            ccname = "gcc"
        ccname += nfo.CMAKE_CXX_COMPILER_VERSION.lower()
        #
        if nfo.CMAKE_C_SIZEOF_DATA_PTR == "4":
            bits = "32bit"
        elif nfo.CMAKE_C_SIZEOF_DATA_PTR == "8":
            bits = "64bit"
        else:
            raise Exception("unknown architecture")
        #
        nfo.storage_name = f"{bits}_{buildtype}_{ccname}_{nfo.storage_id}"
        return nfo


# -----------------------------------------------------------------------------
# -----------------------------------------------------------------------------
# -----------------------------------------------------------------------------

class CMakeCache(Munch):

    def __init__(self, cmakecache_txt):
        import glob
        for line in iter_cmake_lines(cmakecache_txt):
            spl = line.split("=")
            if len(spl) < 2:
                continue
            k, ty = spl[0].split(":")
            v = "=".join(spl[1:]).strip()
            setattr(self, k, v)
        bdir = os.path.dirname(os.path.abspath(cmakecache_txt))
        self._c_compiler_file = sorted(glob.glob(f"{bdir}/CMakeFiles/*/CMakeCCompiler.cmake"))[-1]  # get the last
        self._cxx_compiler_file = sorted(glob.glob(f"{bdir}/CMakeFiles/*/CMakeCXXCompiler.cmake"))[-1]  # get the last
        self._system_file = sorted(glob.glob(f"{bdir}/CMakeFiles/*/CMakeSystem.cmake"))[-1]  # get the last
        self._load_cmake_file(self._c_compiler_file)
        self._load_cmake_file(self._cxx_compiler_file)
        ccomfile = f"{bdir}/compile_commands.json"
        self._compile_commands_file = ccomfile if os.path.exists(ccomfile) else None

    def _load_cmake_file(self, filename):
        for line in iter_cmake_lines(filename):
            if not line.startswith("set("):
                continue
            k = re.sub(r"set\((.*)\ +(.*)\)", r"\1", line)
            v = re.sub(r"set\((.*)\ +(.*)\)", r"\2", line)
            v = v.strip('"').strip("'").strip()
            setattr(self, k, v)

    def save(self, results_dir):
        copy_file_to_dir(self._c_compiler_file, results_dir)
        copy_file_to_dir(self._cxx_compiler_file, results_dir)
        copy_file_to_dir(self._system_file, results_dir)
        if self._compile_commands_file is not None:
            copy_file_to_dir(self._compile_commands_file, results_dir)


def iter_cmake_lines(filename):
    with open(filename) as f:
        for line in f.readlines():
            line = line.strip()
            if line.startswith("#") or line.startswith("//") or len(line) == 0:
                continue
            yield line


# ------------------------------------------------------------------------------
# ------------------------------------------------------------------------------
# ------------------------------------------------------------------------------

def add_results(args):
    log("adding results:", args.results)
    col = BenchmarkCollection(args.target)
    col.add(args.results)


def add_meta(args):
    log("adding bm run metadata to results dir:", args.results)
    meta = ResultMeta(results_dir=args.results,
                      cmakecache=args.cmakecache,
                      build_type=args.build_type)
    meta.save(args.results)
    log("adding bm run metadata to results dir: success!")


# ------------------------------------------------------------------------------
# ------------------------------------------------------------------------------
# ------------------------------------------------------------------------------

if __name__ == '__main__':
    main()
