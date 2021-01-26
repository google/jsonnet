def format(number):
    upper = number // (1<<64)
    lower = number % (1<<64)
    print(""+hex(upper)+","+hex(lower)+",")

for q in range(-342,0):
    power5 = 5 ** -q
    z = 0
    while( (1<<z) < power5) :
        z += 1
    if(q >= -27):
        b = z + 127
        c = 2 ** b // power5 + 1
        format(c)
    else:
        b = 2 * z + 2 * 64
        c = 2 ** b // power5 + 1
        # truncate
        while(c >= (1<<128)):
          c //= 2
        format(c)

for q in range(0,308+1):
    power5 = 5 ** q
    # move the most significant bit in position
    while(power5 < (1<<127)):
        power5 *= 2
    # *truncate*
    while(power5 >= (1<<128)):
        power5 //= 2
    format(power5)
