# Jsonnet - The data templating language

For an introduction to Jsonnet and documentation,
[visit our website](https://jsonnet.org).

This is the Python module for the original C++ implementation of Jsonnet.

**Security notes:** The C++ implementation is not suitable for processing *untrusted inputs* without significant external effort to sandbox it. It is not hardened and may have unknown exploitable bugs. It is intended for use to evaluate Jsonnet code that you trust not to be malicious (e.g., code written by you/your organisation). Even ignoring the risk of exploitable bugs in the implementation, the `import`, `importstr`, and `importbin` language constructs can potentially be used to exfiltrate sensitive data unless you take extra steps to restrict these or sandbox the jsonnet interpreter. By default, these constructs can import from any path accessible to the interpreter process.

## Using the Jsonnet Python module

You can install from PyPI with `uv add jsonnet` or with your preferred Python package management tool.

The two main functions in the `_jsonnet` package are `evaluate_file` and `evaluate_snippet`:

- `evaluate_file(filename, ...)` reads and evalutes the file at the given path, returning an output as a JSON string.
- `evaluate_snippet(filename, src, ...)` evaluates the given source code (`src`), returning an output as a JSON string. The provided filename is used in error messages.

The functions support keyword arguments:

- `jpathdir`: A string (path to a directory) or list of strings (list of directories) to be added to the import search path.
- `ext_vars`, `ext_codes`: Dictionaries of variables to set; these can be used from within the evaluated code through the `std.extVar` Jsonnet function. `ext_codes` values are Jsonnet expressions, `ext_vars` values are provided to Jsonnet as plain strings.
- `tla_vars`, `tla_codes`: Dictionaries of Top-Level arguments. If the provided Jsonnet code to evaluate represents a function, that function is called with these values as (named) arguments.
- `import_callback`: A function which will be called when `import` (or `importstr`, `importbin`) expressions are evaluated. See below.
- `native_callbacks`: A dictionary of functions which can be called from Jsonnet using the Jsonnet `std.native` function. See below.

And some configuration for internal implementation details:

- `max_stack`
- `max_trace`
- `gc_min_objects`
- `gc_growth_trigger`

### `import_callback`

Usage example:

```python
import _jsonnet

FILES = {
    "the_message.txt": b"hello, world",
}

def import_callback_memfile(dir, rel):
    """Custom import function which only returns files from some in-memory storage.
    
    Args:
      dir: The directory part of the file in which the `import` occurred.
      rel: The path string passed to the `import` expression.
    """
    if rel in FILES:
        # Note that the returned file _content_ should be a bytes value, not a string.
        return rel, FILES[rel]
    raise RuntimeError('File not found')

result = _jsonnet.evaluate_snippet(
    'example',
    'local the_message = importstr "the_message.txt"; ' +
    '{ msg: the_message }',
    import_callback=import_callback_memfile)

assert result == '{\n   "msg": "hello, world"\n}\n';
```

### `native_callbacks`

Usage example:

```python
import _jsonnet

def concat(a, b):
    return a + b
    
native_callbacks = {
    'concat': (('a', 'b'), concat),
}

result = _jsonnet.evaluate_snippet(
    'example',
    'local concat = std.native("concat"); ' +
    'concat("hello, ", "world")',
    native_callbacks=native_callbacks)

assert result == '"hello, world"\n';
```
