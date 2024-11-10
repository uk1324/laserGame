arrays = ['mirror', 'portal']
# arrays = ['a', 'b', 'c', 'd']
output = ''

for i in range(0, len(arrays) - 1):
    a = arrays[i]
    output += f'for (const auto& {a} : {a}s) {{\n'
    for j in range(i + 1, len(arrays)):
        b = arrays[j]
        output += f'\tfor (const auto& {b} : {b}s) {{\n'
        output += f'\t\tif ({a}Vs{b.capitalize()}Collision({a}, {b})) {{  }}\n'
        output += '\t}\n'
    output += '}\n'

print(output)