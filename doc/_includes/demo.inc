<div class="inverse hgroup">
  <div class=hgroup-inline>
    <div class="tab-window-input" id="{{ include.name }}-input">
      <div class="tab-header">
      </div>
      <textarea id={{ include.name }}-jsonnet>
{% include examples/{{ include.name }}.jsonnet %}
      </textarea>
    </div>
    <div class="bigarrow">➡</div>
    <div class="tab-window-output" id="{{ include.name }}-output">
      <div class="tab-header">
        <div class=selected onclick="tab_click(this, '{{ include.name }}-json-output')">output.json</div>
      </div>
      <textarea readonly class="selected code-json" id="{{ include.name }}-json-output">
{% include examples/{{ include.name }}.jsonnet.golden %}
      </textarea>
    </div>
    <script>
      demo(
        '{{ include.name }}-input',
        {
          '{{ include.name }}-jsonnet': '{{ include.name }}.jsonnet',
        },
        '{{ include.name }}.jsonnet',
        '{{ include.name }}-output',
        false,
        false
      );
    </script>
    <div style="clear: both"></div>
  </div>
</div>

