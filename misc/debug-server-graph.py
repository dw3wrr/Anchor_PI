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

#update node ips for new node nums
NODE1 = "10.156.82.252"
NODE2 = "10.156.82.254"
NODE3 = "10.156.82.255"
NODE4 = "10.156.83.0"
NODE5 = "10.156.83.1"
NODE6 = "10.156.83.2"
NODE7 = "10.156.83.3"
NODE8 = "10.156.83.5"
NODE9 = "10.156.83.4"
NODE10 = "10.156.83.6"
NODE11 = "10.156.83.7"
NODE12 = "10.156.83.8"
NODE13 = "10.156.82.253"
NODE14 = "10.156.83.9"

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
    NODE10 : 10,
    NODE11 : 11,
    NODE12 : 12,
    NODE13 : 13,
    NODE14 : 14
}

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
pos = nx.circular_layout(G)
#change [500]*(number of nodes)
node_sizes = [500]*14
node_colors = ['green']*14

H = nx.MultiDiGraph()
H.add_node(1, pos=(2, 10))
H.add_node(2, pos=(4, 14))
H.add_node(3, pos=(4, 6))
H.add_node(4, pos=(6, 10))
H.add_node(5, pos=(8, 14))
H.add_node(6, pos=(8, 6))
H.add_node(7, pos=(10, 10))
H.add_node(8, pos=(12, 14))
H.add_node(9, pos=(12, 6))
H.add_node(10, pos=(14, 10))
H.add_node(11, pos=(6, 18))
H.add_node(12, pos=(10, 18))
H.add_node(13, pos=(6, 2))
H.add_node(14, pos=(10, 2))
# pos = nx.get_node_attributes(H,'pos')
pos = nx.circular_layout(H)
node_sizes = [500]*14
node_colors = ['green']*14

#python3
graph_title = "Anchor Demo"
input_ancmt_list = []
input_l2interest_list = []

firstInterest = {}
data_received_bool = 0
colors = 0

lat_dict = {}
total_ban_ancmt = 0
total_ban_l2int = 0
total_ban_l1dat = 0
total_ban_l2dat = 0
total_ban_vec = 0
#create a global module laters
draw_graph_count = 0

def split_chars(word):
    return list(word)

def calc_average():
    global lat_dict
    global total_ban_ancmt
    global total_ban_l2int
    global total_ban_l1dat
    global total_ban_l2dat
    print("-------------------------------------")
    overall_avg = overall_throughput = overall_largest_avg = overall_largest_lat = overall_cost = 0
    counter = 0
    if len(lat_dict) > 0:
        # iterating through dictionary
        for i in lat_dict:
            if (len(lat_dict[i]) > 1) and lat_dict[i][0] != 0:
                counter += 1
                total_hour = total_minute = total_sec = total_milsec = total_throughput = largest_lat = total_cost = 0

                # iterating through list of lat_dict[i]
                for j in (lat_dict[i])[1:]:
                    total_hour += ((j.hour) - (lat_dict[i][0].hour))
                    total_minute += ((j.minute) - (lat_dict[i][0].minute))
                    total_sec += ((j.sec) - (lat_dict[i][0].sec))
                    total_milsec += ((j.milsec) - (lat_dict[i][0].milsec))
                    total_latency = (((j.hour) - (lat_dict[i][0].hour)) * 3600) + (((j.minute) - (lat_dict[i][0].minute)) * 60) + ((j.sec) - (lat_dict[i][0].sec)) + (((j.milsec) - (lat_dict[i][0].milsec)) * (0.000001))
                    total_throughput += (j.pkt_size / total_latency)
                    total_cost += (j.pkt_size)
                    if total_latency > largest_lat:
                        largest_lat = total_latency
                
                if largest_lat > overall_largest_lat:
                    overall_largest_lat = largest_lat

                avg_hour = (total_hour / (len(lat_dict[i]) - 1)) * 3600
                avg_minute = (total_minute / (len(lat_dict[i]) - 1)) * 60
                avg_sec = (total_sec / (len(lat_dict[i]) - 1))
                avg_milsec = (total_milsec / (len(lat_dict[i]) - 1)) * (0.000001)
                avg_throughput = (total_throughput / (len(lat_dict[i]) - 1))

                total_avg = avg_hour + avg_minute + avg_sec + avg_milsec
                overall_avg += total_avg
                overall_throughput += avg_throughput
                overall_largest_avg += largest_lat
                overall_cost += total_cost

                print(i + ":" , lat_dict[i], "Packet Latency: "+ str(round(total_avg,6)) + " seconds, Packet Throughput: " + str(round(avg_throughput,6)) + " bytes/sec, Largest Latency: " + str(round(largest_lat,6)) + " seconds(large), Bandwidth Cost (Data): " + str(round(total_cost,6)) + " bytes")
        if(counter > 0):
            print("Overall Average Latency: " + str(round((overall_avg / counter),6)) + " seconds")
            print("Overall Largest Average Latency: " + str(round((overall_largest_avg / counter),6)) + " seconds, Largest Overall: " + str(round(overall_largest_lat,6)) + " seconds")
            print("Overall Average Bandwidth Cost (Data): " + str(round(overall_cost / counter,6)) +" bytes")
            print("Overall Average Throughput: " + str(round((overall_throughput / counter),6)) + " bytes/second")
            print("Overall Bandwidth Cost (Ancmt): " + str(total_ban_ancmt) + " bytes")
            print("Overall Bandwidth Cost (L2Interest): " + str(total_ban_l2int) + " bytes")
            print("Overall Bandwidth Cost (L1Data): " + str(total_ban_l1dat) + " bytes")
            print("Overall Bandwidth Cost (L2Data): " + str(total_ban_l2dat) + " bytes")
            print("Overall Bandwidth Cost (Vector): " + str(total_ban_vec) + " bytes")
    print("-------------------------------------")

class time_struct:
    def __init__(self, hour, minute, sec, milsec, pkt_size):
        self.hour = hour
        self.minute = minute
        self.sec = sec
        self.milsec = milsec
        self.pkt_size = pkt_size

    def __repr__(self):
        return str(self.hour) + "." + str(self.minute) + "." + str(self.sec) + "." + str(self.milsec) + ">" + str(self.pkt_size)

class EchoServerProtocol(asyncio.Protocol):
    def connection_made(self, transport):
        peername = transport.get_extra_info('peername')
        node_ip = peername[0]
        node_num = ipDict[node_ip]
        print('Connection from {} (Node {})'.format(peername, node_num))
        self.transport = transport

    def data_received(self, data):
        #time stamp, ip, node num, data
        global input_ancmt_list
        global input_l2interest_list
        global G
        global H
        global lat_dict
        global total_ban_ancmt
        global total_ban_l2int
        global total_ban_l1dat
        global total_ban_l2dat
        global total_ban_vec

        message = data.decode("ISO-8859-1")
        node_info = self.transport.get_extra_info('peername')
        node_ip = node_info[0]
        now = datetime.now()
        strings = message.split()
        node_num = ipDict[node_ip]
        print('{} FROM: Node {!r} MESSAGE: {!r}'.format(now, node_num, message))

        if "Clear Graph" in message:
            H.remove_edges_from(list(H.edges()))

        for i in range(len(strings)):
            chars = split_chars(strings[i])

            if "1:" in strings[i]:
                result = (re.search("1:(.*)>",strings[i])).group(1).split(".")
                data = (re.search(">(.*)_",strings[i])).group(1)
                sizep = (re.search("_(.*)",strings[i])).group(1)
                time_entry = time_struct(int(result[0]),int(result[1]),int(result[2]),int(result[3]),int(sizep))
                if data not in lat_dict:
                    lat_dict[data] = [time_entry]
                else:
                    lat_dict[data][0] = time_entry
        
            if "2:" in strings[i]:
                result = (re.search("2:(.*)>",strings[i])).group(1).split(".")
                data = (re.search(">(.*)_",strings[i])).group(1)
                sizep = (re.search("_(.*)",strings[i])).group(1)
                time_entry = time_struct(int(result[0]),int(result[1]),int(result[2]),int(result[3]),int(sizep))
                if data not in lat_dict:
                    lat_dict[data] = [0]
                lat_dict[data].append(time_entry)

            if "i1size:" in strings[i]:
                sizep = (re.search(":(.*)",strings[i])).group(1)
                total_ban_ancmt += int(sizep)

            if "i2size:" in strings[i]:
                sizep = (re.search(":(.*)",strings[i])).group(1)
                total_ban_l2int += int(sizep)

            if "d1size:" in strings[i]:
                sizep = (re.search(":(.*)",strings[i])).group(1)
                total_ban_l1dat += int(sizep)

            if "d2size:" in strings[i]:
                sizep = (re.search(":(.*)",strings[i])).group(1)
                total_ban_l2dat += int(sizep)

            if "vecsize:" in strings[i]:
                sizep = (re.search(":(.*)",strings[i])).group(1)
                total_ban_vec += int(sizep)

            if "l2interest" in strings[i]:
                dash_counter = 0
                num_buffer = []
                for e in range(len(chars)):
                    if dash_counter == 3:
                        num_buffer.append(chars[e])
                    if chars[e] == "/":
                        dash_counter += 1
                selector = int(''.join(num_buffer))
                G.add_edges_from([(selector, node_num)], color='red', weight = 2)

            if "l2data" in strings[i]:
                dash_counter = 0
                num_buffer = []
                for e in range(len(chars)):
                    if dash_counter == 3:
                        num_buffer.append(chars[e])
                    if chars[e] == "/":
                        dash_counter += 1
                selector = int(''.join(num_buffer))
                H.add_edges_from([(selector, node_num)], color='blue', weight = 2)

            if "vector" in strings[i]:
                dash_counter = 0
                num_buffer = []
                for e in range(len(chars)):
                    if dash_counter == 3:
                        num_buffer.append(chars[e])
                    if chars[e] == "/":
                        dash_counter += 1
                selector = int(''.join(num_buffer))
                H.add_edges_from([(selector, node_num)], color='yellow', weight = 2)

        calc_average()

        colors = list(nx.get_edge_attributes(G,'color').values())
        weights = list(nx.get_edge_attributes(G,'weight').values())
        colors_data = list(nx.get_edge_attributes(H,'color').values())
        weights_data = list(nx.get_edge_attributes(H,'weight').values())
        plt.clf()
        plt.title(graph_title)
        plt.figure(1)
        nx.draw(G, pos, with_labels=True,node_size=node_sizes,edgecolors='black', edge_color = colors, width = weights,node_color=node_colors,connectionstyle='arc3, rad = 0.1')
        plt.figure(2)
        nx.draw(H, pos, with_labels=True,node_size=node_sizes,edgecolors='black', edge_color = colors_data, width = weights_data,node_color=node_colors,connectionstyle='arc3, rad = 0.1')
        plt.show(block=False)
        plt.pause(0.000001)

async def main():
    # Get a reference to the event loop as we plan to use low-level APIs
    loop = asyncio.get_running_loop()

    server = await loop.create_server(
        lambda: EchoServerProtocol(),
        '0.0.0.0', 8887)

    async with server:
        await server.serve_forever()

print("Server Started.")
asyncio.run(main())