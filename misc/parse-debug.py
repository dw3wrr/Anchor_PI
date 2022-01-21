import fileinput
import time
import os
import networkx as nx
import matplotlib.pyplot as plt
import pprint
from IPython import display

inputFile = "/home/debug/Documents/anchor_debug/build/debug-output.txt"
outputFile = "/home/debug/Documents/anchor_debug/build/parsed-debug.txt"

NODE1 = "155.246.44.142"
NODE2 = "155.246.215.101"
NODE3 = "155.246.202.145"
NODE4 = "155.246.216.113"
NODE5 = "155.246.203.173"
NODE6 = "155.246.216.39"
NODE7 = "155.246.202.111"
NODE8 = "155.246.212.111"
NODE9 = "155.246.213.83"
NODE10 = "155.246.210.98"

#ip lookup dictionary
ipDict = {
    NODE1 : 1,
    NODE2 : 2,
    NODE3 : 3,
    NODE4 : 4,
    NODE5 : 5,
    NODE6 : 6,
    NODE7 : 7,
    NODE8 : 8,
    NODE9 : 9,
    NODE10 : 10
}

#dictionary of nodes that have flooded
firstInterest = {}

#empty dict
nodeDict = {}
input_dynamic_links = []

input_ancmt_list = []
input_layer2_list = []
prev_ancmt_list = []
prev_layer2_list = []
combined_list = []

node8_list = []
node9_list = []
combined_data_list = []
prev_node8 = 0
prev_node9 = 0

data_gen_start = 0

G = nx.MultiDiGraph()
G.add_node(1,pos=(2,6))
G.add_node(2,pos=(4,10))
G.add_node(3,pos=(4,2))
G.add_node(4,pos=(6,6))
G.add_node(5,pos=(8,10))
G.add_node(6,pos=(8,2))
G.add_node(7,pos=(10,6))
G.add_node(8,pos=(12,10))
G.add_node(9,pos=(12,2))
G.add_node(10,pos=(14,6))
pos = nx.get_node_attributes(G,'pos')
node_sizes = [500]*10
node_colors = ['green']*10

#this is for real time video
def generate_continuous_nodes():
    combined_list = input_ancmt_list + input_layer2_list
    G.add_edges_from(combined_list)
    plt.clf()
    plt.title("Anchor Demo")
    nx.draw(G, pos, with_labels=True,node_size=node_sizes,edgecolors='black',node_color=node_colors,connectionstyle='arc3, rad = 0.1')
    plt.show(block=False)
    plt.pause(0.002)

def generate_data_nodes():
    combined_data_list = node8_list + node9_list
    G.remove_edges_from(input_ancmt_list)
    G.remove_edges_from(input_layer2_list)
    G.add_edges_from(combined_data_list)
    plt.clf()
    plt.title("Data Path")
    nx.draw(G, pos, with_labels=True,node_size=node_sizes,edgecolors='black',node_color=node_colors,connectionstyle='arc3, rad = 0.1')
    plt.show(block=False)
    plt.pause(0.3)

def readIn():
    with open(inputFile, 'r+', encoding = "ISO-8859-1") as fd:
        for line in fd:
            
            if "Is Anchor" in line:
                node_sizes[0] = 1000
                node_colors[0] = 'red'
            if "On Interest" or "Flooded Interest" in line:
                strings = line.split()
                node_ip = strings[2]
                node_num = ipDict[node_ip]
                global input_ancmt_list
                global input_layer2_list
                for i in range(len(strings)):
                    if strings[i] == "Interest:":
                        string_value = int(strings[i+1])
                        if node_num not in firstInterest:
                            firstInterest[node_num] = string_value
                        if (string_value, node_num) not in input_ancmt_list:
                            input_ancmt_list.append((string_value, node_num))
                    if strings[i] == "Flooded":
                        if node_num != 1:
                            if (node_num, firstInterest[node_num]) not in input_layer2_list:
                                input_layer2_list.append((node_num, firstInterest[node_num]))
            if "Data" in line:
                G.remove_edges_from(input_ancmt_list)
                G.remove_edges_from(input_layer2_list)
                plt.clf()
                plt.title("Data Path")
                nx.draw(G, pos, with_labels=True,node_size=node_sizes,edgecolors='black',node_color=node_colors,connectionstyle='arc3, rad = 0.1')
                plt.show(block=False)
                plt.pause(0.1)
                global data_gen_start
                data_gen_start = 1

#G.remove_edges_from(

def parseData():
    with open(inputFile, 'r+', encoding = "ISO-8859-1") as fd:
        for line in fd:
            if "Data Sent" in line:
                strings = line.split()
                node_ip = strings[2]
                node_num = ipDict[node_ip]
                node_sizes[node_num-1] = 1000
                node_colors[node_num-1] = 'yellow'
                #clear graph
                if node_num == 8:
                    global node8_list
                    G.remove_edges_from(node8_list)
                    global prev_node8
                    prev_node8 = 8
                    node8_list = []
                if node_num == 9:
                    global node9_list
                    G.remove_edges_from(node9_list)
                    global prev_node9
                    prev_node9 = 9
                    node9_list = []
            if "On Data: 8" in line:
                strings = line.split()
                node_ip = strings[2]
                node_num = ipDict[node_ip]
                if (prev_node8, node_num) > (1,1) :
                    node8_list.append((prev_node8, node_num))
                prev_node8 = node_num
            if "On Data: 9" in line:
                strings = line.split()
                node_ip = strings[2]
                node_num = ipDict[node_ip]
                if (prev_node9, node_num) > (1,1) :
                    node9_list.append((prev_node9, node_num))
                prev_node9 = node_num

# readIn()
# print(input_ancmt_list)
# print(input_layer2_list)
# generate_continuous_nodes(0)


while data_gen_start == 0:
    readIn()
    generate_continuous_nodes()
    prev_ancmt_list = input_ancmt_list
    prev_layer2_list = input_layer2_list
    time.sleep(0.001)

while True:
    parseData()
    generate_data_nodes()
    time.sleep(0.2)

# def remove_suffix(input_string, suffix):
#     if suffix and input_string.endswith(suffix):
#         return input_string[:-len(suffix)]
#     return input_string

# def generate_static_nodes():
#     input_links = []

#     for keys in nodeDict:
#         for values in nodeDict[keys]:
#             if isinstance(values, int):
#                 input_links.append((values,keys))
#     print(input_links)

#     G.add_edges_from(input_links)
#     plt.clf()
#     plt.title("Demo")
#     nx.draw(G, pos, with_labels=True,node_size=node_sizes,edgecolors='black',node_color=node_colors,connectionstyle='arc3, rad = 0.1')
#     plt.show()

# def generate_dynamic_nodes(source, destination):
#     input_dynamic_links.append((source, destination))
#     G.add_edges_from(input_dynamic_links)
#     plt.clf()
#     plt.title("Demo")
#     nx.draw(G, pos, with_labels=True,node_size=node_sizes,edgecolors='black',node_color=node_colors,connectionstyle='arc3, rad = 0.1')
#     plt.show()
#     time.sleep(5)

# def generate_layer_2():
#     G.add_edges_from(input_dynamic_links)
#     plt.clf()
#     plt.title("Layer 2")
#     nx.draw(G, pos, with_labels=True,node_size=node_sizes,edgecolors='black',node_color=node_colors,connectionstyle='arc3, rad = 0.1')
#     plt.show()

# def generate_node_interrupt(time):
#     #if list has changed from last time, then generate new image and save
#     if input_ancmt_list != prev_ancmt_list or input_layer2_list != prev_layer2_list
#         combined_list = input_ancmt_list + input_layer2_list
#         G.add_edges_from(combined_list)
#         plt.clf()
#         plt.title('Time = {} s'.format(time))
#         nx.draw(G, pos, with_labels=True,node_size=node_sizes,edgecolors='black',node_color=node_colors,connectionstyle='arc3, rad = 0.1')
#         #fig = plt.figure()
#         #fig.savefig("topology.png")
#         plt.savefig("topology-{}.png".format(pic_num))
#         pic_num += 1
#         plt.show(block=False)
#         plt.pause(0.01)
#     prev_ancmt_list = input_ancmt_list
#     prev_layer2_list = input_layer2_list

# ----------------------------------
# def readIn():
#     with open(inputFile, 'r+', encoding = "ISO-8859-1") as fd:

#         for line in fd:
#             # if "Is Anchor" in line:
#             #     strings = line.split()
#             #     node_ip = strings[2]
#             #     node_num = ipDict[node_ip]
#             #     nodeDict[node_num] = ["anchor"]

#             if "On Interest" or "Flooded Interest" in line:
#                 strings = line.split()
#                 #ip number
#                 node_ip = strings[2]
#                 for i in range(len(strings)):
#                     if strings[i] == "Interest:":
#                         #strings[i+1] = 80n 
#                         strings[i + 1] = remove_suffix(strings[i + 1], "On")
#                         # also account for remove suffix flooded
#                         string_value = int(strings[i + 1])
#                         if node_num in nodeDict:
#                             if string_value not in nodeDict[node_num]:
#                                 nodeDict[node_num].append(string_value)
#                                 #generate_dynamic_nodes(string_value, node_num)
#                         else:
#                             #this will only add the first node every time
#                             nodeDict[node_num] = [string_value]
#                             #generate_dynamic_nodes(string_value, node_num)
#                             input_dynamic_links.append((node_num, string_value))
#                     elif strings[i] == "Flooded":

# add_edges_from() takes in a list of tuples

# def node():
#     G=nx.MultiDiGraph()
#     G.add_node(1,pos=(2,6))
#     G.add_node(2,pos=(4,10))
#     G.add_node(3,pos=(4,2))
#     G.add_node(4,pos=(6,6))
#     G.add_node(5,pos=(8,10))
#     G.add_node(6,pos=(8,2))
#     G.add_node(7,pos=(10,6))
#     G.add_node(8,pos=(12,10))
#     G.add_node(9,pos=(12,2))
#     G.add_node(10,pos=(14,6))
#     pos=nx.get_node_attributes(G,'pos')

#     node_sizes = [500]*10
#     node_colors = ['green']*10

#     for epoch in range(1, 18):
#         plt.clf()
#         if epoch == 2:
#             G.add_edges_from([(1, 2),(1, 3)])
#             node_sizes[0]= 1200
#             node_colors[0] = 'red'
#         if epoch == 3:
#             G.add_edges_from([(2,1),(3,1)])
#         if epoch == 4:
#             G.add_edges_from([(2, 4),(3, 4),(2, 5), (3, 6)])
#         if epoch == 5:
#             G.add_edges_from([(4,2)])
#         if epoch == 6:
#             G.add_edges_from([(4, 5),(4, 7), (4, 6),(5,7),(5, 8),(6,7),(6, 9)])
#         if epoch == 7:
#             G.add_edges_from([(7, 4),(5, 2), (6, 4)])
#         if epoch == 8:
#             G.add_edges_from([(7, 8),(7, 9)])   
#         if epoch == 9:
#             G.add_edges_from([(8, 7),(9, 6)]) 
#         if epoch == 10:
#             G.add_edges_from([(8, 10),(9, 10)]) 
#         if epoch == 11:
#             G.add_edges_from([(10, 8)]) 
#         if epoch == 12:
#             G.remove_edges_from([(1, 2),(1, 3),(2, 4),(3, 4),(2, 5), (3, 6),(4, 5),(4, 7), (4, 6),(5,7),(5, 8),(6,7),(6, 9),(7, 8),(7, 9),(8, 10),(9, 10)])
        
#         if epoch == 13:
#             node_sizes[5]= 800
#             node_colors[5] = 'yellow'
#             G.add_edge(6,4,weight=6)
#         if epoch == 14:
#             node_colors[3] = 'yellow'
#         if epoch == 15:
#             node_colors[1] = 'yellow'  
#         if epoch == 16:
#             node_colors[0] = 'yellow'
#         if epoch == 17:
#             node_colors[1] = 'green'
#             node_colors[3] = 'green'
#             node_colors[0] = 'red'
             
#         #create a matplot figure in order to automatically change the figure
#         plt.clf()
#         plt.title('Time {}s'.format(epoch))
#         nx.draw(G, pos, with_labels=True,node_size=node_sizes,edgecolors='black',node_color=node_colors,connectionstyle='arc3, rad = 0.1')        
#         #display.clear_output(wait=True)
#         #display.display(plt.gcf()) 
#         plt.show(block=False)
#         plt.pause(0.02)
#         time.sleep(1)

#     time.sleep(0.003)

# readIn()
# print(nodeDict)
# generate_layer_2()

# --------------------------------
# generate_static_nodes()

# for synchrony over time 
# while True:
#     fd = open(inputFile, 'r+')
#     for line in fd:
#         if()
#     fd.close()
#     time.sleep(0.003)


# with open(inputFile, 'r+') as fd:
#     for line in fd:
#         print(line)
#         if "" in line:
#             strings = line.split()
#             del strings[5]
#             #print(type(strings))
#             print(strings)

# 2 versions
# one with real time simulations
# one where we generate a new picture whenever the structure of the topology changes\
# counter for sleep cycle for 2nd version
# change ip for debug server when vm closes