import fileinput
import os

insert_string = " -pthread"
# print(os.getcwd())
#change pi to debug

file = "/home/debug/Documents/anchor_debug/build/CMakeFiles/normal-node.dir/link.txt"
with open(file, 'r+') as fd:
    contents = fd.readlines()
    print(contents)
    string = contents[0].split()
    string.insert(8, insert_string)
    string = ' '.join(string)
    print(string)
    fd.seek(0)
    fd.writelines(string)
    print("Written to normal-node.dir")

file = "/home/debug/Documents/anchor_debug/build/CMakeFiles/normal-node-demo1.dir/link.txt"
with open(file, 'r+') as fd:
    contents = fd.readlines()
    print(contents)
    string = contents[0].split()
    string.insert(8, insert_string)
    string = ' '.join(string)
    print(string)
    fd.seek(0)
    fd.writelines(string)
    print("Written to normal-node-demo1.dir")

file = "/home/debug/Documents/anchor_debug/build/CMakeFiles/normal-node-demo2.dir/link.txt"
with open(file, 'r+') as fd:
    contents = fd.readlines()
    print(contents)
    string = contents[0].split()
    string.insert(8, insert_string)
    string = ' '.join(string)
    print(string)
    fd.seek(0)
    fd.writelines(string)
    print("Written to normal-node-demo2.dir")

file = "/home/debug/Documents/anchor_debug/build/CMakeFiles/normal-node-demo3.dir/link.txt"
with open(file, 'r+') as fd:
    contents = fd.readlines()
    print(contents)
    string = contents[0].split()
    string.insert(8, insert_string)
    string = ' '.join(string)
    print(string)
    fd.seek(0)
    fd.writelines(string)
    print("Written to normal-node-demo3.dir")

file = "/home/debug/Documents/anchor_debug/build/CMakeFiles/normal-node-demo4.dir/link.txt"
with open(file, 'r+') as fd:
    contents = fd.readlines()
    print(contents)
    string = contents[0].split()
    string.insert(8, insert_string)
    string = ' '.join(string)
    print(string)
    fd.seek(0)
    fd.writelines(string)
    print("Written to normal-node-demo4.dir")

file = "/home/debug/Documents/anchor_debug/build/CMakeFiles/normal-node-demo5.dir/link.txt"
with open(file, 'r+') as fd:
    contents = fd.readlines()
    print(contents)
    string = contents[0].split()
    string.insert(8, insert_string)
    string = ' '.join(string)
    print(string)
    fd.seek(0)
    fd.writelines(string)
    print("Written to normal-node-demo5.dir")

file = "/home/debug/Documents/anchor_debug/build/CMakeFiles/normal-node-demo6.dir/link.txt"
with open(file, 'r+') as fd:
    contents = fd.readlines()
    print(contents)
    string = contents[0].split()
    string.insert(8, insert_string)
    string = ' '.join(string)
    print(string)
    fd.seek(0)
    fd.writelines(string)
    print("Written to normal-node-demo6.dir")

file = "/home/debug/Documents/anchor_debug/build/CMakeFiles/normal-node-demo7.dir/link.txt"
with open(file, 'r+') as fd:
    contents = fd.readlines()
    print(contents)
    string = contents[0].split()
    string.insert(8, insert_string)
    string = ' '.join(string)
    print(string)
    fd.seek(0)
    fd.writelines(string)
    print("Written to normal-node-demo7.dir")

file = "/home/debug/Documents/anchor_debug/build/CMakeFiles/normal-node-demo8.dir/link.txt"
with open(file, 'r+') as fd:
    contents = fd.readlines()
    print(contents)
    string = contents[0].split()
    string.insert(8, insert_string)
    string = ' '.join(string)
    print(string)
    fd.seek(0)
    fd.writelines(string)
    print("Written to normal-node-demo8.dir")

file = "/home/debug/Documents/anchor_debug/build/CMakeFiles/normal-node-demo9.dir/link.txt"
with open(file, 'r+') as fd:
    contents = fd.readlines()
    print(contents)
    string = contents[0].split()
    string.insert(8, insert_string)
    string = ' '.join(string)
    print(string)
    fd.seek(0)
    fd.writelines(string)
    print("Written to normal-node-demo9.dir")

file = "/home/debug/Documents/anchor_debug/build/CMakeFiles/normal-node-demo10.dir/link.txt"
with open(file, 'r+') as fd:
    contents = fd.readlines()
    print(contents)
    string = contents[0].split()
    string.insert(8, insert_string)
    string = ' '.join(string)
    print(string)
    fd.seek(0)
    fd.writelines(string)
    print("Written to normal-node-demo10.dir")