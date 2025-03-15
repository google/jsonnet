import sys
import _jsonnet

result = _jsonnet.evaluate_file("example.jsonnet")
sys.stdout.write(result)
