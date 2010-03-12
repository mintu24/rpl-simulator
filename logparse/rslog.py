#!/usr/bin/env python

import sys

#def compose_plot_command(dat_info_list):
#    gnuplot_subcmd_list = []
#    
#    gnuplot_subcmd_list.append("set yrange[0:%d]" % (dat_info_list[len(dat_info_list) - 1][0] + 2))
#    gnuplot_subcmd_list.append("set title 'Connectivity'")
#    gnuplot_subcmd_list.append("set xlabel 'time [us]'")
#    gnuplot_subcmd_list.append("set ylabel 'connected'")
#    
#    subsub = ""
#    for i in xrange(len(dat_info_list)):
#        subsub += "'%s' with filledcurve" % (dat_info_list[i][2])
#        if (i < len(dat_info_list) - 1):
#            subsub += ", "
#        
#    gnuplot_subcmd_list.append("plot " + subsub)
#    
#    gnuplot_subcmds = ""
#    for subcmd in gnuplot_subcmd_list:
#        gnuplot_subcmds += "; " + subcmd
#    
#    gnuplot_cmd = "gnuplot -persist -e \"%s\"" % (gnuplot_subcmds)
#    
#    return gnuplot_cmd

def parse_time(time_str):
    parts = time_str.split(':')
    if (len(parts) == 3): # hours
        return int(int(parts[0]) * 3600000 + int(parts[1]) * 60000 + float(parts[2]) * 1000)

    elif (len(parts) == 2): # minutes
        return int(int(parts[0]) * 60000 + float(parts[1]) * 1000)

    elif (len(parts) == 1): # seconds or/and milliseconds
        return int((float(parts[0])) * 1000)

    else:
        raise Exception("invalid time: '%s'" % (time_str))

def parse_param_value(value):
    if value.find('\'') == 0:
        return value[1:-1]
    
    elif value.find('{') == 0:
        parts = value[1:-1].split(' ');
        value = []
        for part in parts:
            value.append(parse_param_value(part.strip()))
            
        return value
    
    elif (value.find('.') != -1):
        return float(value)
    
    else:
        return int(value)

def parse_params(params):
    parts = params.split(',');
    
    params = []
    
    for part in parts:
        name = part[:part.index('=')].strip()
        value = part[part.index('=') + 1:].strip()
        
        params.append((name, parse_param_value(value)))
        
    return params
    
def parse_event(event_str, event_name_filter_list = None):
    parts = event_str.split('(')
    if (len(parts) < 2):
        raise Exception("invalid event: '%s'" % (event_str))

    subparts = parts[0].split('.')
    if (len(subparts) < 3):
        raise Exception("invalid event: '%s'" % (event_str))

    node_name = subparts[0]
    layer = subparts[1]
    event_name = subparts[2]
    
    if (event_name_filter_list == None) or (event_name in event_name_filter_list):
        params = parse_params(parts[1][:-1])

        return (node_name, layer, event_name, params)
    
    else:
        return None

def parse_line(line, event_name_filter_list = None):
    parts = line.split(' : ')
    if (len(parts) < 2):
        raise Exception("invalid line: '%s'" % (line))      

    time_str = parts[0].strip()
    event_str = parts[1].strip()
    
    tuple = parse_event(event_str, event_name_filter_list)
    if tuple != None:
        time = parse_time(time_str)
        (node_name, layer, event_name, params) = tuple
        
        return (time, node_name, layer, event_name, params)
    
    else:
        return None

def parse_log_file(log_file_name, event_name_filter_list = None):
    f = open(log_file_name)
    
    list = []
    
    for line in f.readlines():
        try:
            tuple = parse_line(line, event_name_filter_list)
            
            if tuple != None:
                list.append(tuple)

        except Exception as e:
            print("an error occurred: " + str(e))

    f.close()
    
    return list

if (__name__ == "__main__"):
    if (len(sys.argv) > 1):
        list = parse_log_file(sys.argv[1])
