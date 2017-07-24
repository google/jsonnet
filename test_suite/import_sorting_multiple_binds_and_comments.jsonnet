local b = import "b.jsonnet",  # B comment.
      a = import "a.jsonnet";  # A comment.

local /* bbb */ b = import "b.jsonnet",  # B comment.
      /* aaa */ a = import "a.jsonnet";  # A comment.
true
