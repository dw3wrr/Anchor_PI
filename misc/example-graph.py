import socket
import asyncio
import sys
import time
from datetime import datetime
import os
import networkx as nx
import matplotlib.pyplot as plt
import pprint
from IPython import display
import re

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

pos = nx.get_node_attributes(G,'pos')
#make 
node_sizes = [500]*14
node_colors = ['green']*14

# def node():
    #fig = plt.figure()
    #ax = fig.add_subplot(1, 1, 1)
    # for epoch in range(1,18):
    #     plt.clf()
        # if epoch == 2:
        #     G.add_edges_from([(1, 2), (1, 3)])
        #     node_sizes[0] = 1200
        #     node_colors[0] = 'red'
        # if epoch == 3:
        #     G.add_edges_from([(2, 1), (3, 1)])
        # if epoch == 4:
        #     G.add_edges_from([(2, 4), (3, 4), (2, 5), (3, 6)])
        # if epoch == 5:
        #     G.add_edges_from([(4, 2)])
        # if epoch == 6:
        #     G.add_edges_from([(4, 5), (4, 7), (4, 6), (5, 7), (5, 8), (6, 7), (6, 9)])
        # if epoch == 7:
        #     G.add_edges_from([(7, 4), (5, 2), (6, 4)])
        # if epoch == 8:
        #     G.add_edges_from([(7, 8), (7, 9)])
        # if epoch == 9:
        #     G.add_edges_from([(8, 7), (9, 6)])
        # if epoch == 10:
        #     G.add_edges_from([(8, 10), (9, 10)])
        # if epoch == 11:
        #     G.add_edges_from([(10, 8)])
        # if epoch == 12:
        #     G.remove_edges_from(
        #         [(1, 2), (1, 3), (2, 4), (3, 4), (2, 5), (3, 6), (4, 5), (4, 7), (4, 6), (5, 7), (5, 8), (6, 7), (6, 9),
        #          (7, 8), (7, 9), (8, 10), (9, 10)])

        # if epoch == 13:
        #     node_sizes[5] = 800
        #     node_colors[5] = 'yellow'
        #     G.add_edge(6, 4, weight=6)
        # if epoch == 14:
        #     node_colors[3] = 'yellow'
        # if epoch == 15:
        #     node_colors[1] = 'yellow'
        # if epoch == 16:
        #     node_colors[0] = 'yellow'
        # if epoch == 17:
        #     node_colors[1] = 'green'
        #     node_colors[3] = 'green'
        #     node_colors[0] = 'red'

        # plt.clf()
        # plt.title('Time {}s'.format(epoch))
        # nx.draw(G, pos, with_labels=True, node_size=node_sizes, edgecolors='black', node_color=node_colors, connectionstyle='arc3, rad = 0.1')
        # #display.clear_output(wait=True)
        # #display.display(plt.gcf())
        # #plt.show()
        # plt.show(block=False)
        # plt.pause(0.001)
        #plt.close()
        # time.sleep(0.002)

# node()

# G.add_edges_from([(4, 5), (4, 7), (4, 6), (5, 7), (5, 8), (6, 7), (6, 9)])
plt.clf()
plt.title("POS TEST")
plt.figure(1)
nx.draw(G, pos, with_labels=True, node_size=node_sizes, edgecolors='black', node_color=node_colors, connectionstyle='arc3, rad = 0.1')
plt.show(block=False)
plt.pause(0)