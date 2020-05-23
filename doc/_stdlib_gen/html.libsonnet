// A little helper library for building HTML documents with Jsonnet.
//
// Parts of the HTML document (elements, comments, etc.) are represented
// as Jsonnet objects and arrays.
//
// The following types are available:
// * element – HTML element
// * comment – HTML comment
// * doctype – HTML doctype (e.g. <!DOCTYPE html>)
// * verbatim – verbatim HTML code,
// * array – HTML parts seprated by whitespace
// * spaceless – HTML parts concatenated without any separator.
//
// The library predefines functions for common elements, but
// it is easy to define your own.
//
// The library takes a somewhat opinionated approach to HTML formatting.
// It is possible to get a specific formatting with `verbatim` and `spaceless`
// types, but normally the `render` function automatically handles indentations
// and newlines. The outputs are intended to be diff friendly and readable.

local element(elementType, bodyFormatting='indented') = function(attrs, body) {
  type: 'element',
  elementType: elementType,
  attrs: attrs,
  body: body,
  bodyFormatting: bodyFormatting,
};
local comment(text) = {
  type: 'comment',
  text: text,
};
local doctype(doctype) = {
  type: 'doctype',
  doctype: doctype,
};
local verbatim(html) = {
  type: 'verbatim',
  html: html,
};
local spaceless(parts) = {
  type: 'spaceless',
  parts: parts,
};

local renderAttrs(attrs) =
  std.join(' ', [
    attr + '="' + attrs[attr] + '"'
    for attr in std.objectFields(attrs)
  ]);

local renderOpeningTag(type, attrs) =
  local attrsStr = renderAttrs(attrs);
  '<' +
  type +
  (
    if attrsStr != '' then
      ' ' + attrsStr
    else
      ''
  )
  +
  '>';

local renderClosingTag(type) =
  '</' +
  type +
  '>';


local paragraphs(ps) = [element('p')({}, p) for p in ps];
local newline = verbatim('\n');

local indentString(s, indent) =
  local lines = std.split(s, '\n');
  // If the string ends with a newline, drop it.
  local lines2 =
    if lines[std.length(lines) - 1] == '' then
      lines[:std.length(lines) - 1]
    else
      lines
  ;
  std.join('\n' + indent, lines2)
;

local
  render(content) = renderAux(content, indent='  ', curIndent=''),
  renderAux(content, indent, curIndent) =
    local renderIndented(c) =
      '\n' +
      curIndent +
      indent +
      renderAux(c, indent, curIndent + indent) +
      '\n' +
      curIndent
    ;
    if std.isObject(content) then
      if content.type == 'element' then
        renderOpeningTag(content.elementType, content.attrs) +
        (
          if content.bodyFormatting == 'indented' then
            renderIndented(content.body)
          else if content.bodyFormatting == 'compact' then
            renderAux(content.body, indent, curIndent)
          else if content.bodyFormatting == 'unindented' then
            renderAux(content.body, indent, '')
          else
            error ("Invalid bodyFormatting: " + content.bodyFormatting)
        ) +
        renderClosingTag(content.elementType)
      else if content.type == 'comment' then
        '<!-- ' + content.text + ' -->'
      else if content.type == 'doctype' then
        '<!DOCTYPE ' + content.doctype + ' />'
      else if content.type == 'verbatim' then
        content.html
      else if content.type == 'spaceless' then
        std.join('', [renderAux(c, indent, curIndent) for c in content.parts if c != null])
      else error 'unrecognized type'
    else if std.isArray(content) then
      std.join('\n' + curIndent, [renderAux(c, indent, curIndent) for c in content if c != null])
    else if std.isString(content) then
      indentString(content, curIndent)
    else
      'content must be an object, an array or a string';

// Escape HTML characters (e.g. for including HTML in pre)
// TODO(sbarzowski) consider adding a variant which also removes quotation marks (like html.escape in Python)
local escape(str) =
  std.strReplace(
    std.strReplace(
      std.strReplace(str, "&", "&amp;"),
      "<", "&lt;"),
    ">", "&gt;");

{
  // Basics:
  element: element,
  doctype: doctype,
  comment: comment,
  verbatim: verbatim,
  spaceless: spaceless,
  render: render,
  escape: escape,

  // Elements:
  a: element('a'),
  body: element('body'),
  html: element('html'),
  head: element('head'),
  h1: element('h1'),
  h2: element('h2'),
  h3: element('h3'),
  h4: element('h4'),
  h5: element('h5'),
  h6: element('h6'),
  p: element('p'),
  div: element('div'),
  code: element('code', bodyFormatting='compact'),
  pre: element('pre', bodyFormatting='unindented'),
  em: element('em'),
  svg: element('svg'),

  // Batteries:
  paragraphs: paragraphs,
  newline: newline,
}
