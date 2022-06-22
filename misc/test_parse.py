import socket
import asyncio
import sys
import time
from datetime import datetime
import os
import pprint
import re

#time_entry = time_struct(10, 12, 23, 132771)
#SHA encoding has bitstream output, if we convert bitstream to base64 then base64 does not include "_" char
#throughput and latency are factors for table

#lat_dict[foo][0] is the the time that data1 is generated

data1string = "1:18.6.5.134013>test1_71 ; 2:18.6.5.314977>test1_76 ; 2:18.7.10.777123>test2_75 ; "
data2string = "2:18.7.10.315132>test2_78 ; 1:18.7.10.115612>test2_70 ; 2:18.6.5.885130>test1_75 ; "
data3string = "1:0.0.10.10000>test3_80 ; 2:0.0.10.110000>test3_82 ; "

lat_dict = {}

def split_chars(word):
    return list(word)

def pop_lat(string_input):
    global lat_dict
    strings = string_input.split()
    for i in range(len(strings)):
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

    # for i in lat_dict:
    #     print(i + ":" , lat_dict[i])

def calc_average():
    print("-------------------------------------")
    overall_avg = overall_throughput = overall_largest_avg = overall_largest_lat = overall_cost = 0
    global lat_dict
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
                    #
                    total_latency = (((j.hour) - (lat_dict[i][0].hour)) * 3600) + (((j.minute) - (lat_dict[i][0].minute)) * 60) + ((j.sec) - (lat_dict[i][0].sec)) + (((j.milsec) - (lat_dict[i][0].milsec)) * (0.000001))
                    # throughput = j.pkt_size / total_latency # bytes / second
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
                print(i + ":" , lat_dict[i], "Packet Latency: "+ str(round(total_avg,6)) + " seconds, Packet Throughput: " + str(round(avg_throughput,6)) + " bytes/sec, Largest Latency: " + str(round(largest_lat,6)) + " seconds(large), Bandwidth Cost: " + str(round(total_cost,6)) + " bytes")
        if(counter > 0):
            # print("Overall Average:", round((overall_avg / len(lat_dict)),6))
            print("Overall Average Latency: " + str(round((overall_avg / counter),6)) + " seconds")
            print("Overall Largest Average Latency: " + str(round((overall_largest_avg / counter),6)) + " seconds, Largest Overall: " + str(round(overall_largest_lat,6)) + " seconds")
            print("Overall Average Bandwidth Cost: " + str(round(overall_cost / counter,6)) +" bytes")
            print("Overall Average Throughput: " + str(round((overall_throughput / counter),6)) + " bytes/second")
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

pop_lat(data1string)
calc_average()
pop_lat(data2string)
calc_average()
pop_lat(data3string)
calc_average()




#extra code

# node8_list = []
# node9_list = []
# prev_node8 = 0
# prev_node9 = 0
#------------------------------------------------------------------------

        # if "Is Anchor" in message:
        #     node_sizes[0] = 1000
        #     node_colors[0] = 'red'
        
        # if "Clear Graph" in message:
        #     node_sizes[node_num-1] = 1000
        #     node_colors[node_num-1] = 'yellow'
        #     H.remove_edges_from(list(H.edges()))

        # #have to account for 10
        # for i in range(len(strings)):
        #     chars = split_chars(strings[i])
        #     if "ancmt" in strings[i]:
        #         dash_counter = 0
        #         num_buffer = []
        #         for e in range(len(chars)):
        #             if dash_counter == 3:
        #                 num_buffer.append(chars[e])
        #             if chars[e] == "/":
        #                 dash_counter += 1
        #         selector = int(''.join(num_buffer))
        #         G.add_edges_from([(selector, node_num)], color='r', weight = 2)
                    
        #     if "l2interest" in strings[i]:
        #         dash_counter = 0
        #         num_buffer = []
        #         for e in range(len(chars)):
        #             if dash_counter == 3:
        #                 num_buffer.append(chars[e])
        #             if chars[e] == "/":
        #                 dash_counter += 1
        #         selector = int(''.join(num_buffer))
        #         G.add_edges_from([(selector, node_num)], color='b', weight = 2)

        #     if "l1data" in strings[i]:
        #         dash_counter = 0
        #         num_buffer = []
        #         for e in range(len(chars)):
        #             if dash_counter == 3:
        #                 num_buffer.append(chars[e])
        #             if chars[e] == "/":
        #                 dash_counter += 1
        #         selector = int(''.join(num_buffer))
        #         H.add_edges_from([(selector, node_num)], color='green', weight = 2)

        #     if "l2data" in strings[i]:
        #         dash_counter = 0
        #         num_buffer = []
        #         for e in range(len(chars)):
        #             if dash_counter == 3:
        #                 num_buffer.append(chars[e])
        #             if chars[e] == "/":
        #                 dash_counter += 1
        #         selector = int(''.join(num_buffer))
        #         H.add_edges_from([(selector, node_num)], color='purple', weight = 2)
                
        # #print(edges)
        # colors = list(nx.get_edge_attributes(G,'color').values())
        # weights = list(nx.get_edge_attributes(G,'weight').values())
        # colors_data = list(nx.get_edge_attributes(H,'color').values())
        # weights_data = list(nx.get_edge_attributes(H,'weight').values())
        # # print(colors)
        # # print(weights)
        # plt.clf()
        # plt.title(graph_title)
        # plt.figure(1)
        # nx.draw(G, pos, with_labels=True,node_size=node_sizes,edgecolors='black', edge_color = colors, width = weights,node_color=node_colors,connectionstyle='arc3, rad = 0.1')
        # plt.figure(2)
        # nx.draw(H, pos, with_labels=True,node_size=node_sizes,edgecolors='black', edge_color = colors_data, width = weights_data,node_color=node_colors,connectionstyle='arc3, rad = 0.1')
        # plt.show(block=False)
        # plt.pause(0.000001)

        #-------------------------------------------------------------------------
        
        # if "Is Anchor" in message:
        #     node_sizes[0] = 1000
        #     node_colors[0] = 'red'
        # if ("On Interest" in message) or ("Flooded Interest" in message):
        #     for i in range(len(strings)):
        #         global input_ancmt_list
        #         global input_layer2_list
        #         global firstInterest
        #         if strings[i] == "Interest:":
        #             string_value = int(strings[i+1])
        #             if node_num not in firstInterest:
        #                 firstInterest[node_num] = string_value
        #             if (string_value, node_num) not in input_ancmt_list:
        #                 input_ancmt_list.append((string_value, node_num))
        #             global G
        #             G.add_edges_from([(string_value, node_num)], color='r', weight = 2)
        #         if (strings[i] == "Flooded") and (node_num != 1) and ((node_num, firstInterest[node_num]) not in input_layer2_list):
        #             input_layer2_list.append((node_num, firstInterest[node_num]))
        #             G.add_edges_from([(node_num, firstInterest[node_num])], color='b', weight = 2)
        # if "Data" in message:
        #     global data_received_bool
        #     if data_received_bool == 0:
        #         G.remove_edges_from(input_ancmt_list)
        #         G.remove_edges_from(input_layer2_list)
        #         global graph_title
        #         graph_title = "Data Path"
        #         data_received_bool = 1
        #     if "Data Sent" in message:
        #         node_sizes[node_num-1] = 1000
        #         node_colors[node_num-1] = 'yellow'
        #         if node_num == 8:
        #             global node8_list
        #             G.remove_edges_from(node8_list)
        #             global prev_node8
        #             prev_node8 = 8
        #             node8_list = []
        #         if node_num == 9:
        #             global node9_list
        #             G.remove_edges_from(node9_list)
        #             global prev_node9
        #             prev_node9 = 9
        #             node9_list = []
        #     if "On Data: 8" in message:
        #         if (prev_node8, node_num) != (1,1) :
        #             G.add_edges_from([(prev_node8, node_num)], color='black', weight = 2)
        #         node8_list.append((prev_node8, node_num))
        #         prev_node8 = node_num
        #     if "On Data: 9" in message:
        #         if (prev_node9, node_num) != (1,1) :
        #             G.add_edges_from([(prev_node9, node_num)], color='black', weight = 2)
        #         node9_list.append((prev_node9, node_num))
        #         prev_node9 = node_num

        # edges = G.edges()
        # #print(edges)
        # colors = list(nx.get_edge_attributes(G,'color').values())
        # weights = list(nx.get_edge_attributes(G,'weight').values())
        # # print(colors)
        # # print(weights)
        # plt.clf()
        # plt.title(graph_title)
        # nx.draw(G, pos, with_labels=True,node_size=node_sizes,edgecolors='black', edge_color = colors, width = weights,node_color=node_colors,connectionstyle='arc3, rad = 0.1')
        # plt.show(block=False)
        # plt.pause(0.000001)

        # print('Send: {!r}'.format(message))
        # self.transport.write(data)
        # print('Close the client socket')
        #self.transport.close()