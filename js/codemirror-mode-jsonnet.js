/** @license
 *
 * Copyright 2017 Google Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/


(function(mod) {
  if (typeof exports == "object" && typeof module == "object") // CommonJS
    mod(require("../../lib/codemirror"), require("../../addon/mode/simple"));
  else if (typeof define == "function" && define.amd) // AMD
    define(["../../lib/codemirror", "../../addon/mode/simple"], mod);
  else // Plain browser env
    mod(CodeMirror);
})(function(CodeMirror) {
  "use strict";

  let keywords = {
      "local": "keyword",
      "self": "keyword",
      "super": "keyword",
      "assert": "keyword",
      "function": "keyword",
      "if": "keyword",
      "then": "keyword",
      "else": "keyword",
      "for": "keyword",
      "in": "keyword",
      "tailstrict": "keyword",
      "error": "keyword",
      "true": "atom",
      "false": "atom",
      "null": "atom",
  };

  CodeMirror.defineMode("jsonnet", function() {
    return {
      token: function(stream, state) {
        // Handle special states:

        // In a C-style comment
        if (state.cComment) {
          if (stream.match(/\*\//)) {
            state.cComment = false;
            return "comment";
          }
          stream.next();
          return "comment";
        }

        // In a text block (|||)
        if (state.textBlock) {
          if (state.textBlockIndent == null) {
            state.textBlockIndent = stream.indentation();
          }
          if (stream.indentation() >= state.textBlockIndent) {
            stream.skipToEnd();
            return "string";
          }
          if (stream.match(/\s*\|\|\|/)) {
            state.textBlock = false;
            return "string";
          }
          stream.next();
          return "error";
        }

        // In a string (all 4 variants)
        if (state.string || state.importString) {
          let mode = state.string ? "string" : "meta";
          if (state.stringRaw) {
            if (stream.match(state.stringSingle ? /''/ : /""/)) {
              return "string-2";
            }
          } else {
            if (stream.match(/\\[\\"'bfnrt0]/)) {
              return "string-2";
            }
            if (stream.match(/\\u[0-9a-fA-F]{4}/)) {
              return "string-2";
            }
            if (stream.match(/\\/)) {
              return "error";
            }
          }
          if (stream.match(state.stringSingle ? /'/ : /"/)) {
            state.string = false;
            state.importString = false;
            state.stringRaw = false;
            state.stringDouble = false;
            return mode;
          }
          stream.next();
          return mode;
        }

        // Regular (whole token at a time) processing:

        // Comments.
        if (stream.match(/\/\//) || stream.match(/#/)) {
          stream.skipToEnd();
          return "comment";
        }
        if (stream.match(/\/\*/)) {
          state.cComment = true;
          return "comment";
        }

        // Imports (including the strings after them).
        if (stream.match(/import(?:str)?\s*"/)) {
          state.importString = true;
          state.stringSingle = false;
          state.stringRaw = false;
          return "meta";
        }

        if (stream.match(/import(?:str)?\s*'/)) {
          state.importString = true;
          state.stringSingle = true;
          state.stringRaw = false;
          return "meta";
        }

        if (stream.match(/import(?:str)?\s*@"/)) {
          state.importString = true;
          state.stringSingle = false;
          state.stringRaw = true;
          return "meta";
        }

        if (stream.match(/import(?:str)?\s*@'/)) {
          state.importString = true;
          state.stringSingle = true;
          state.stringRaw = true;
          return "meta";
        }

        // Strings (without imports)
        if (stream.match(/"/)) {
          state.string = true;
          state.stringSingle = false;
          state.stringRaw = false;
          return "string";
        }

        if (stream.match(/'/)) {
          state.string = true;
          state.stringSingle = true;
          state.stringRaw = false;
          return "string";
        }

        if (stream.match(/@"/)) {
          state.string = true;
          state.stringSingle = false;
          state.stringRaw = true;
          return "string";
        }

        if (stream.match(/@'/)) {
          state.string = true;
          state.stringSingle = true;
          state.stringRaw = true;
          return "string";
        }

        // Enter text block.
        if (stream.match(/\|\|\|/)) {
          state.textBlock = true;
          state.textBlockCol = null;
          return "string";
        }

        if (stream.match(/\$/)) return "keyword";
        if (stream.match(/(?:\.\d+|\d+\.?\d*)(?:e[-+]?\d+)?/i)) return "number";
        if (stream.match(/[-+\/*=<>!&~^|$%]+/)) return "operator";

        // Identifiers and keywords that look like identifiers.
        let identifier = stream.match(/[a-zA-Z_][a-zA-Z0-9_]*/);
        if (identifier) {
          identifier = identifier[0];
          // If it's not in the dict, we return null to indicate no special
          // syntax highlighting.
          return keywords.hasOwnProperty(identifier) ? keywords[identifier] : undefined;
        }

        stream.next();

        return null;
      },
      startState: function() {
        return {
          cComment: false,
          textBlock: false,
          importString: false,
          string: false,
          stringSingle: false,
          stringRaw: false,
        };
      },
    };
  });
});

CodeMirror.defineMIME("text/x-jsonnet", "jsonnet");
