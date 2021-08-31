/** Bind the functions as variables in Javascript. */
let jsonnet_make = Module.cwrap(
    'jsonnet_make', 'number', [])
let jsonnet_import_callback = Module.cwrap(
    'jsonnet_import_callback', 'number', ['number', 'number', 'number'])
let jsonnet_ext_var = Module.cwrap(
    'jsonnet_ext_var', 'number', ['number', 'string', 'string'])
let jsonnet_ext_code = Module.cwrap(
    'jsonnet_ext_code', 'number', ['number', 'string', 'string'])
let jsonnet_tla_var = Module.cwrap(
    'jsonnet_tla_var', 'number', ['number', 'string', 'string'])
let jsonnet_tla_code = Module.cwrap(
    'jsonnet_tla_code', 'number', ['number', 'string', 'string'])
let jsonnet_realloc = Module.cwrap(
    'jsonnet_realloc', 'number', ['number', 'number', 'number'])
let jsonnet_evaluate_snippet = Module.cwrap(
    'jsonnet_evaluate_snippet', 'number', ['number', 'string', 'string', 'number'])
let jsonnet_fmt_snippet = Module.cwrap(
    'jsonnet_fmt_snippet', 'number', ['number', 'string', 'string', 'number'])
let jsonnet_destroy = Module.cwrap(
    'jsonnet_destroy', 'number', ['number'])

function string_to_jsonnet(vm, str) {
  let size = lengthBytesUTF8(str);
  let result_buf = jsonnet_realloc(vm, 0, size + 1);
  stringToUTF8(str, result_buf, size + 1);
  return result_buf;
}

// Pass an int through C, which is an index into this array.
let ctx_mapping = { };
let ctx_counter = 1;

// We don't use ctx, instead just rely on Javascript's cloure environment.
let import_callback = addFunction(function (ctx_, base_, rel_, found_here_, success_) {
  let vm = ctx_mapping[ctx_].vm
  let files = ctx_mapping[ctx_].files
  let base = UTF8ToString(base_);
  let rel = UTF8ToString(rel_);
  let abs_path;
  // It is possible that rel is actually absolute.
  if (rel[0] == '/') {
      abs_path = rel;
  } else {
      abs_path = base + rel;
  }
  if (abs_path in files) {
    setValue(success_, 1, 'i32*');
    setValue(found_here_, string_to_jsonnet(vm, abs_path), 'i8*');
    return string_to_jsonnet(vm, files[abs_path]);
  } else {
    setValue(success_, 0, 'i32*');
    return string_to_jsonnet(vm, 'file not found');
  }
});

/** Wrap the raw C-level function in something Javascript-friendly. */
function jsonnet_evaluate_snippet_wrapped(
    filename, code, files, ext_str, ext_code, tla_str, tla_code) {
  let vm = jsonnet_make();
  let ctx_ptr = counter++;
  ctx_mapping[ctx_ptr] = { vm: vm, files: files };
  jsonnet_import_callback(vm, import_callback, ctx_ptr);
  for (let key in ext_str) {
    jsonnet_ext_var(vm, key, ext_str[key]);
  }
  for (let key in ext_code) {
    jsonnet_ext_code(vm, key, ext_code[key]);
  }
  for (let key in tla_str) {
    jsonnet_tla_var(vm, key, tla_str[key]);
  }
  for (let key in tla_code) {
    jsonnet_tla_code(vm, key, tla_code[key]);
  }
  let error_ptr = _malloc(4);
  let output_ptr = jsonnet_evaluate_snippet(vm, filename, code, error_ptr);
  let error = getValue(error_ptr, 'i32*');
  _free(error_ptr);
  let result = UTF8ToString(output_ptr);
  jsonnet_realloc(vm, output_ptr, 0);
  jsonnet_destroy(vm);
  delete ctx_mapping[ctx_ptr];
  if (error) {
    throw result;
  }
  return result;
}

/** Wrap the raw C-level function in something Javascript-friendly. */
function jsonnet_fmt_snippet_wrapped(filename, code) {
  let vm = jsonnet_make();
  let error_ptr = _malloc(4);
  let output_ptr = jsonnet_fmt_snippet(vm, filename, code, error_ptr);
  let error = getValue(error_ptr, 'i32*');
  _free(error_ptr);
  let result = UTF8ToString(output_ptr);
  jsonnet_realloc(vm, output_ptr, 0);
  jsonnet_destroy(vm);
  if (error) {
    throw result;
  }
  return result;
}
