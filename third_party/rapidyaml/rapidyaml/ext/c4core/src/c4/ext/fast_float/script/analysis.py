from math import floor

def log2(x):
   """returns ceil(log2(x)))"""
   y = 0
   while((1<<y) < x):
     y = y + 1
   return y


for q in range(1,17+1):
    d = 5**q
    b = 127 + log2(d)
    t = 2** b
    c = t//d + 1
    assert c < 2**128
    assert c >= 2**127
    K = 2**127
    if(not(c * K * d<=( K + 1) * t)):
      print(q)
      top = floor(t/(c  * d - t))
      sys.exit(-1)

for q in range(18, 344+1):
    d = 5**q
    b = 64 + 2*log2(d)
    t = 2**b
    c = t//d + 1
    assert c > 2**(64 +log2(d))
    K = 2**64
    if(not(c * K * d<=( K + 1) * t)):
      print(q)
      top = floor(t/(c  * d - t))
      sys.exit(-1)

print("all good")