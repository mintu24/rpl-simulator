#!/usr/bin/env python

import sys
import os
import os.path

import rslog

def load_idleness(log_file_name):
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

        if event_name == "pdu_send":
            send_count += 1
            
        elif event_name == "pdu_receive":
            receive_count += 1
            
        if (time < min_time):
            min_time = time
        
        if (time > max_time):
            max_time = time
            
        node_dict[node_name] = True
    
    return (len(node_dict.keys()), max_time - min_time + 1, send_count, receive_count)

if __name__ == '__main__':
    if (len(sys.argv) < 2):
        print("usage: %s <file.log>\n" % (sys.argv[0]))
        sys.exit()
    
    (node_count, total_time, send_count, receive_count) = load_idleness(sys.argv[1])
    print("nodes = %d, total time = %dms, packets sent = %d, packets received = %d" % (node_count, total_time, send_count, receive_count))
    
    send = float(send_count) / node_count * 20 / total_time
    receive = float(receive_count) / node_count * 20 / total_time
    idle = 1 - send - receive
    print("send = %.02f%%, receive = %.02f%%, idle = %.02f%%" % (send * 100, receive * 100, idle * 100))
