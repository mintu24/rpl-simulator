#!/usr/bin/env python

import sys
import os
import os.path

import rslog

def load_overhead(log_file_name):
    log_lines = rslog.parse_log_file(log_file_name, ["pdu_send", "pdu_receive"])
    
    send_count = 0
    receive_count = 0
    node_dict = {}
    min_time = 100000000
    max_time = 0
    
    line_no = 0
    for log_line in log_lines:
        line_no += 1
        
        time = log_line[0]
        event_name = log_line[3]
        node_name = log_line[1]
        dis_count = log_line[4][1][1][5]
        dio_count = log_line[4][1][1][6]
        dao_count = log_line[4][1][1][7]

        if event_name == "pdu_send":
            send_count += 1
            
        elif event_name == "pdu_receive":
            receive_count += 1
            
        if (time < min_time):
            min_time = time
        
        if (time > max_time):
            max_time = time
            
        node_dict[node_name] = (dis_count, dio_count, dao_count)
    
    dis_count = 0
    dio_count = 0
    dao_count = 0
    for node in node_dict:
        dis_count += node_dict[node][0]
        dio_count += node_dict[node][1]
        dao_count += node_dict[node][2]
    
    return (len(node_dict.keys()), max_time - min_time + 1, send_count, dis_count, dio_count, dao_count)

if __name__ == '__main__':
    if (len(sys.argv) < 2):
        print("usage: %s <file.log>\n" % (sys.argv[0]))
        sys.exit()
    
    (node_count, total_time, send_count, dis_count, dio_count, dao_count) = load_overhead(sys.argv[1])
    print("nodes = %d, total time = %dms, packets sent = %d, dis packets = %d, dio packets = %d, dao packets = %d" % \
          (node_count, total_time, send_count, dis_count, dio_count, dao_count))
    
    dis = float(dis_count) / send_count
    dio = float(dio_count) / send_count
    dao = float(dao_count) / send_count
    traffic = 1 - dis - dio - dao
    
    print("dis = %.02f%%, dio = %.02f%%, dao = %.02f%%, traffic = %.02f%%" % \
          (dis * 100, dio * 100, dao * 100, traffic * 100))
