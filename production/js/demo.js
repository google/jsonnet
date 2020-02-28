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

function textarea_content_height(textarea) {
  // This trick only works if display is not 'none'.
  let old_display = textarea.style.display;
  textarea.style.display = 'block';
  // The trick is to measure the height of the text in the text area.
  let old_height = textarea.style.height;
  textarea.style.height = '0px';
  let height = textarea.scrollHeight;
  // Reset it.
  textarea.style.display = old_display;
  textarea.style.height = old_height;
  return height;
}

/** Make the text area large enough for its content. */
function resize_textarea(textarea) {
  let height = textarea_content_height(textarea);
  textarea.style.height = height + 'px';
}

/** Make the text area large enough for its content (but not too big). */
function resize_textarea_max(max_height, output_textarea) {
  let height = textarea_content_height(output_textarea);
  height = Math.min(height, max_height);
  output_textarea.style.height = height + 'px';
}

/** Fix the content of a textarea.
 *
 * In HTML, if the </textarea> is not on the final line of the text, or if the
 * content of the textarea is indented, extra whitespace appears on the page.
 * Defensively written HTML can avoid this, but it is hard to read / maintain.
 * Therefore, we use some javascript to correct it instead.
 */
function fix_textarea(output_textarea) {
  let lines = output_textarea.value.split(/\r\n|\r|\n/);
  // Assume that no hard tabs are used.
  let max_ws = 10000;
  for (let line of lines) {
    let match = /[^ ]/.exec(line);
    if (match) {
      if (max_ws == null || max_ws > match.index)
        max_ws = match.index;
    }
  }
  for(let i = 0, len = lines.length; i < len; i++) {
      lines[i] = lines[i].substring(max_ws).trimRight();
  }
  for (let line of lines) {
    let match = /[^ ]/.exec(line);
    if (match) {
      if (max_ws == null || max_ws > match.index)
        max_ws = match.index;
    }
  }
  while (lines.length > 0 && lines[lines.length - 1].length == 0) lines.pop();
  output_textarea.value = lines.join('\n');
}

let counter = 0;

function aux_demo(input_id, input_files, main_file, output_id, process_func) {
  // input is a div.tab-window-input, it contains tab header and textareas.
  let input = document.getElementById(input_id);
  // input is a div.tab-window-output, it also contains tab header and textareas.
  let output = document.getElementById(output_id);
  // Will populate this below in the function.
  let editor_by_textarea_id = {};

  // Create a bunch of editors, assign ids, and populate max_height and editor_by_textarea_id.
  let selected = 'selected';
  for (let input_child of input.childNodes) {
    if (input_child.tagName == 'TEXTAREA') {
      // Fix whitespace from HTML 
      fix_textarea(input_child);
      // Resize it to match its content
      resize_textarea(input_child);
      let language = input_files[input_child.id].split('.').pop();
      if (language == 'libsonnet')
        language = 'jsonnet';
      let editor = CodeMirror.fromTextArea(input_child, {
        mode: {name: language},
        lineNumbers: true,
        indentUnit: 2,
        tabSize: 2,
        matchBrackets: true,
        scrollbarStyle: null,
        extraKeys: {
          Tab: (cm) => {
              if (cm.somethingSelected()) {
                cm.indentSelection('add');
              } else {
                cm.execCommand('insertSoftTab');
              }
            },
            'Shift-Tab': (cm) => {
              cm.indentSelection('subtract');
            }
        }
      });
      let editor_div = editor.getWrapperElement()
      let id = 'editor-' + counter++;
      editor_div.classList.add(selected);
      selected = 'unselected';
      editor_div.setAttribute('id', id);
      editor_by_textarea_id[input_child.id] = editor;
    }
  }
  for (let input_child of input.childNodes) {
    if (input_child.className == 'tab-header') {
      // Should be only one tab-header.
      let selected = 'selected';
      for (let id in input_files) {
        let file = input_files[id];
        let editor_div = editor_by_textarea_id[id].getWrapperElement();
        input_child.innerHTML +=
          "<div class=" + selected + " onclick='tab_input_click(this, \"" + editor_div.id + "\")'>"
          + file + "</div>";
        selected = 'unselected';
      }
    }
  }

  for (let textarea_id in editor_by_textarea_id) {
    let editor = editor_by_textarea_id[textarea_id];
    editor.on('change', function() {
      // Clean out the existing textareas and tabs.
      // There should be only one.
      let tab_header = output.getElementsByClassName('tab-header')[0];
      let last_selected;
      while (tab_header.firstChild) {
        let child = tab_header.firstChild;
        if (child.className == 'selected') {
          last_selected = child.innerHTML;
        }
        tab_header.removeChild(child);
      }

      let condemned_textareas = output.getElementsByTagName('TEXTAREA');
      let last_scroll = 0;
      while (condemned_textareas[0]) {
        if (condemned_textareas[0].classList.contains('selected')) {
          last_scroll = condemned_textareas[0].scrollTop;
        }
        output.removeChild(condemned_textareas[0]);
      }


      let input_files_content = {};
      for (let id in input_files) {
        let file = input_files[id];
        let file_editor = editor_by_textarea_id[id];
        input_files_content[file] = file_editor.getValue();
      }
      function add_textarea_and_tab(filename, selected, scroll, output_content, output_class) {
        let output_textarea = document.createElement('TEXTAREA');
        let id = 'output-' + counter++;
        output_textarea.setAttribute('id', id);
        output_textarea.value = output_content;
        output.appendChild(output_textarea);

        let selected_class = 'unselected';
        if (filename == selected) {
          selected_class = 'selected';
          output_textarea.scrollTop = scroll;
        }
        output_textarea.className = selected_class + ' ' + output_class;
        let max_height = 300;
        for (let id in editor_by_textarea_id) {
          let editor_div = editor_by_textarea_id[id].getWrapperElement();
          max_height = Math.max(max_height, editor_div.offsetHeight);
        }
        resize_textarea_max(max_height, output_textarea);

        let output_tab = document.createElement('DIV');
        tab_header.appendChild(output_tab);
        output_tab.className = selected_class;
        output_tab.onclick = function() { tab_output_click(this, output_textarea.id); };
        output_tab.innerHTML = filename;
      }
      try {
        process_func(
            main_file, input_files_content, last_selected, last_scroll, add_textarea_and_tab);
      } catch (e) {
        if (typeof e === 'string') {
          // Remove last \n
          e = e.replace(/\n$/, '');
        }
        add_textarea_and_tab(last_selected, last_selected, last_scroll, e, 'code-error');
      }
    });
  }
  
  for (let output_child of output.childNodes) {
    if (output_child.tagName == 'TEXTAREA') {
      fix_textarea(output_child);
      let max_height = 300;
      for (let id in editor_by_textarea_id) {
        let editor_div = editor_by_textarea_id[id].getWrapperElement();
        max_height = Math.max(max_height, editor_div.offsetHeight);
      }
      resize_textarea_max(max_height, output_child);
    }
  }
}

function tab_input_click(tab, editor_div_id) {
  let output = document.getElementById(editor_div_id);
  for (let sibling of output.parentElement.childNodes) {
    if (!sibling.classList || !sibling.classList.contains('CodeMirror')) continue;
    sibling.classList.remove('selected');
    sibling.classList.add('unselected');
  }
  output.classList.remove('unselected');
  output.classList.add('selected');
  for (let sibling of tab.parentElement.childNodes) {
    if (sibling.nodeName == "#text") continue;
    sibling.classList.remove('selected');
    sibling.classList.add('unselected');
  }
  tab.classList.remove('unselected');
  tab.classList.add('selected');
}

function tab_output_click(tab, output_id) {
  let output = document.getElementById(output_id);
  for (let sibling of output.parentElement.childNodes) {
    if (sibling.tagName != 'TEXTAREA') continue;
    sibling.classList.remove('selected');
    sibling.classList.add('unselected');
  }
  output.classList.remove('unselected');
  output.classList.add('selected');
  for (let sibling of tab.parentElement.childNodes) {
    if (sibling.nodeName == "#text") continue;
    sibling.classList.remove('selected');
    sibling.classList.add('unselected');
  }
  tab.classList.remove('unselected');
  tab.classList.add('selected');
}

function handle_parsed_content(parsed_content, string_out) {
    if (string_out) {
      if (typeof(parsed_content) != 'string') {
        throw 'RUNTIME ERROR: expected string result.';
      }
      // Remove last \n
      return parsed_content.replace(/\n$/, '');
    } else {
      // Original JSON is sorted, ordering is preserved here.
      return JSON.stringify(parsed_content, null, 2);
    }
}

/** Take a bunch of existing HTML and make it into an active Jsonnet demo.
 *
 * The best way to understand how to use this function is to read where it is used.
 *
 * input_id: The id of the tab-window-input div.
 * input_files: A dict mapping input filenames to the ids of the corresponding textarea elements.
 * main_file: The one that Jsonnet is executed on (it imports the others).
 * output_id: The id of the tab-window-output div.
 * multi: True to enable multi-output mode.
 * top_level: A dict to configure top-level parameterization (tla / ext var).
 */
function demo(input_id, input_files, main_file, output_id, multi, string_out, top_level) {
  top_level = top_level || {};
  let ext_str = top_level['ext_str'] || {};
  let ext_code = top_level['ext_code'] || {};
  let tla_str = top_level['tla_str'] || {};
  let tla_code = top_level['tla_code'] || {};

  aux_demo(
      input_id, input_files, main_file, output_id,
      function(main_file, input_files_content, last_selected, last_scroll, add_textarea_and_tab) {
    let json_str = jsonnet_evaluate_snippet_wrapped(
        main_file, input_files_content[main_file], input_files_content,
        ext_str, ext_code, tla_str, tla_code);
    let parsed_output = JSON.parse(json_str);
    if (multi == true) {
      if (typeof parsed_output != 'object') {
        throw 'RUNTIME ERROR: multi mode: top-level object was a ' + typeof parsed_output
              + ', should be an object whose keys are filenames and values hold the JSON for'
              + ' that file.';
      }
      if (Object.keys(parsed_output).length == 0) {
        throw 'RUNTIME ERROR: multi mode: top-level object had no keys.';
      }
      let selected = Object.keys(parsed_output)[0];
      let scroll = 0;
      if (typeof parsed_output === 'object' && last_selected in parsed_output) {
        selected = last_selected;
        scroll = last_scroll;
      }
      for (let filename in parsed_output) {
        output_content = handle_parsed_content(parsed_output[filename], string_out);
        add_textarea_and_tab(filename, selected, scroll, output_content, 'code-json');
      }
    } else {
      // Original JSON is sorted, ordering is preserved here.
      output_content = handle_parsed_content(parsed_output, string_out);
      add_textarea_and_tab(last_selected, last_selected, last_scroll, output_content, 'code-json');
    }
  });
}

/** Take a bunch of existing HTML and make it into an active Jsonnet formatter demo.
 *
 * textarea_id: The id of the textarea containing the Jsonnet code.
 * fliename: The filename Jsonnet is formatting.
 * output_id: The id of the tab-window-output div.
 */
function fmt_demo(input_id, textarea_id, filename, output_id) {
  let process_func = function(
        main_file, input_files_content, last_selected, last_scroll, add_textarea_and_tab) {
    let content = input_files_content[main_file];
    let jsonnet_str = jsonnet_fmt_snippet_wrapped(filename, content)
    jsonnet_str = jsonnet_str.replace(/\n$/, '');
    add_textarea_and_tab(
        last_selected, last_selected, last_scroll, jsonnet_str, 'code-json');
  }
  let map = {};
  map[textarea_id] = filename;
  aux_demo(input_id, map, filename, output_id, process_func);
}

/** Take a bunch of existing HTML and make it into an active YAML conversion demo.
 *
 * textarea_id: The id of the textarea containing the YAML code.
 * fliename: The filename Jsonnet is formatting.
 * output_id: The id of the tab-window-output div.
 */
function yaml_conv_demo(input_id, textarea_id, filename, output_id) {
  let process_func = function(
        main_file, input_files_content, last_selected, last_scroll, add_textarea_and_tab) {
    let content = input_files_content[main_file];
    let parsed_yaml = jsyaml.load(content);
    let yaml_json = JSON.stringify(parsed_yaml, null, 2);
    let jsonnet_str = jsonnet_fmt_snippet_wrapped(filename, yaml_json)
    jsonnet_str = jsonnet_str.replace(/\n$/, '');
    add_textarea_and_tab(
        last_selected, last_selected, last_scroll, jsonnet_str, 'code-json');
  }
  let map = {};
  map[textarea_id] = filename;
  aux_demo(input_id, map, filename, output_id, process_func);
}
