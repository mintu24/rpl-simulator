#!/usr/bin/env python

import sys
import os
import os.path


def parse_time(time_str):
    parts = time_str.split(':')
    if (len(parts) == 3): # hours
        return int((int(parts[0]) * 3600000 + int(parts[1]) * 60000 + float(parts[2])) * 1000)

    elif (len(parts) == 2): # minutes
        return int((int(parts[0]) * 60000 + float(parts[1])) * 1000)

    elif (len(parts) == 1): # seconds or/and milliseconds
        return int((float(parts[0])) * 1000)

    else:
        raise Exception("invalid time: '%s'" % (time_str))

def parse_event(event_str):
    parts = event_str.split('.')
    if (len(parts) < 3):
        raise Exception("invalid event: '%s'" % (event_str))

    node_name = parts[0]
    layer = parts[1]
    parts = parts[2]
    event_name = parts[0:parts.find('(')]
    params = parts[parts.find('(') + 1:-1]
    
    return [node_name, layer, event_name, params]

def parse_line(line):
    parts = line.split(' : ')
    if (len(parts) < 2):
        raise Exception("invalid line: '%s'" % (line))      
          
    time_str = parts[0].strip()
    event_str = parts[1].strip()
    
    time = parse_time(time_str)
    [node_name, layer, event_name, params] = parse_event(event_str)
    
    return [time, node_name, layer, event_name, params]

def parse_log_file(log_file_name, dat_info_list):
    f = open(log_file_name)
    
    for dat_info in dat_info_list:
        dat_info.append(open(dat_info[2], "w"))
        dat_info.append(0)
        
    for line in f.readlines():
        try:
            [time, node_name, layer, event_name, params] = parse_line(line)

            if layer != "measure":
                continue
            
            dat_info = [dat_info for dat_info in dat_info_list if dat_info[1] == node_name]
            if (len(dat_info) == 0):
                continue;
            
            dat_info = dat_info[0]
            
            print("%d | %s | %s | %s | %s" % (time, node_name, layer, event_name, params))
            
            if event_name == "connect_established":
                dat_info[3].write("%d %d\n" % (time, dat_info[0]))
                dat_info[3].write("%d %d\n" % (time, dat_info[0] + 1))
            
            elif event_name == "connect_lost":
                dat_info[3].write("%d %d\n" % (time, dat_info[0] + 1))
                dat_info[3].write("%d %d\n" % (time, dat_info[0]))
                
            if (time > dat_info[4]):
                dat_info[4] = time

        except Exception as e:
            print("an error occured: " + str(e))
    
    for dat_info in dat_info_list:
        dat_info[3].write("%d %d\n" % (dat_info[4], dat_info[0])) # make sure we finish "disconnected"
        dat_info[3].close()

    f.close()
    
def compose_plot_command(dat_info_list):
    gnuplot_subcmd_list = []
    
    gnuplot_subcmd_list.append("set yrange[0:%d]" % (dat_info_list[len(dat_info_list) - 1][0] + 2))
    gnuplot_subcmd_list.append("set title 'Connectivity'")
    gnuplot_subcmd_list.append("set xlabel 'time [us]'")
    gnuplot_subcmd_list.append("set ylabel 'connected'")
    
    subsub = ""
    for i in xrange(len(dat_info_list)):
        subsub += "'%s' with filledcurve" % (dat_info_list[i][2])
        if (i < len(dat_info_list) - 1):
            subsub += ", "
        
    gnuplot_subcmd_list.append("plot " + subsub)
    
    gnuplot_subcmds = ""
    for subcmd in gnuplot_subcmd_list:
        gnuplot_subcmds += "; " + subcmd
    
    gnuplot_cmd = "gnuplot -persist -e \"%s\"" % (gnuplot_subcmds)
    
    return gnuplot_cmd

if len(sys.argv) < 3:
    print("usage: " + sys.argv[0] + " <log_file> <node1> [... nodeN]\n")
    sys.exit(1)

prog_name = sys.argv[0]
log_file_name = sys.argv[1]
node_names = sys.argv[2:]

dat_info_list = []
for i in xrange(len(sys.argv[2:])):
    dat_info_list.append([i, node_names[i], os.path.splitext(log_file_name)[0] + "_" + node_names[i] + ".dat"])

parse_log_file(log_file_name, dat_info_list)
os.system(compose_plot_command(dat_info_list))

