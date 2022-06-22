import networkx as nx
import time
import matplotlib.pyplot as plt
import random
from IPython import display
import operator
from functools import reduce
import numpy as np
import copy

p = 0.3 #The probability p in Erdos-Renyi random graph model
N = 20  # number of nodes
M = 10000  # A large number represents the maximum message limit
INF = 999  # distance upper bound

#The first function used to generate the graph
def BuildGraph_Matrix_1(n):
    Graph = [[0] * n for row in range(n)]#Initialize the graph

    for i in range(0, n):
        for j in range(0, n):
            if i == j:
                Graph[i][j] = 0
            else:
                Graph[i][j] = INF #Set disconnected nodes to INF(999)
    # randomly set edges base on p
    for i in range(0, n):
        for j in range(i, n):
            if i != j and random.random() <= p:
                Graph[i][j] = 1
                Graph[j][i] = 1
    return Graph

#Calculate the shortest distance between two nodes
def Floyd(vertex_total):
    distance = [[0] * vertex_total for row in range(vertex_total)]
    # initialize
    for i in range(0, vertex_total):
        for j in range(0, vertex_total):
            distance[i][j] = Graph_Matrix[i][j]
    # Use the Floyd algorithm to find the shortest distance between two vertices of all vertices
    for k in range(0, vertex_total):
        for i in range(0, vertex_total):
            for j in range(i, vertex_total):
                if distance[i][k] + distance[k][j] < distance[i][j]:
                    distance[i][j] = distance[i][k] + distance[k][j]
                    distance[j][i] = distance[i][k] + distance[k][j]
    return distance

Graph_Matrix = BuildGraph_Matrix_1(14)
distance = Floyd(14)

Graph_Matrix
distance

G = nx.MultiDiGraph()
G.add_node(1, pos=(2, 10))
G.add_node(2, pos=(4, 14))
G.add_node(3, pos=(4, 6))
G.add_node(4, pos=(6, 10))
G.add_node(5, pos=(8, 14))
G.add_node(6, pos=(8, 6))
G.add_node(7, pos=(10, 10))
G.add_node(8, pos=(12, 14))
G.add_node(9, pos=(12, 6))
G.add_node(10, pos=(14, 10))
G.add_node(11, pos=(6, 18))
G.add_node(12, pos=(10, 18))
G.add_node(13, pos=(6, 2))
G.add_node(14, pos=(10, 2))

# pos = nx.get_node_attributes(G,'pos')
pos = nx.circular_layout(G)
node_sizes = [500]*14
node_colors = ['green']*14

for i in range(0,14):
    print("Node %d's neighbor is :" % (i+1))
    for j in range(0,14):
        if Graph_Matrix[i][j] == 1:
            print("%d " % (j+1))
            G.add_edge(i + 1,j + 1) 

plt.clf()
plt.title("Test")
plt.figure(1)
#nx.draw(G, pos, with_labels=True, node_size=node_sizes, edgecolors='black', node_color=node_colors, connectionstyle='arc3, rad = 0.1')
nx.draw(G, pos, with_labels=True, node_size=node_sizes, edgecolors='black', node_color=node_colors, arrows=False)
plt.show(block=False)
plt.pause(0)