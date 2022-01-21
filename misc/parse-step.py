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
node_sizes[0] = 1000
node_colors[0] = 'red'

def generate_nodes(pair_nodes):
    G.add_edges_from([pair_nodes])
    plt.clf()
    plt.title("Anchor Demo")
    nx.draw(G, pos, with_labels=True,node_size=node_sizes,edgecolors='black',node_color=node_colors,connectionstyle='arc3, rad = 0.1')
    plt.show(block=False)
    plt.pause(1)  

def readIn():
    with open(inputFile, 'r+', encoding = "ISO-8859-1") as fd:
        for line in fd:
            if "On Interest" or "Flooded Interest" in line:
                strings = line.split()
                node_ip = strings[2]
                node_num = ipDict[node_ip]
                for i in range(len(strings)):
                    if strings[i] == "Interest:":
                        string_value = int(strings[i+1])
                        if node_num not in firstInterest:
                            firstInterest[node_num] = string_value
                        if (string_value, node_num) not in input_ancmt_list:
                            input_ancmt_list.append((string_value, node_num))
                            generate_nodes((string_value, node_num))
                    if strings[i] == "Flooded":
                        if node_num != 1:
                            if (node_num, firstInterest[node_num]) not in input_layer2_list:
                                input_layer2_list.append((node_num, firstInterest[node_num]))
                                generate_nodes((node_num, firstInterest[node_num]))

readIn()

plt.clf()
plt.title("Anchor Demo")
nx.draw(G, pos, with_labels=True,node_size=node_sizes,edgecolors='black',node_color=node_colors,connectionstyle='arc3, rad = 0.1')
plt.show(block=False)
plt.pause(200) 