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

def parse_param_value(value):
    if value.find('\'') == 0:
        return value[1:-1]
    
    elif value.find('{') == 0:
        parts = value[1:-1].split(' ');
        value = []
        for part in parts:
            value.append(int(part.strip()))
            
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
    
def parse_event(event_str):
    parts = event_str.split('.')
    if (len(parts) < 3):
        raise Exception("invalid event: '%s'" % (event_str))

    node_name = parts[0]
    layer = parts[1]
    parts = parts[2]
    event_name = parts[0:parts.find('(')]
    params = parse_params(parts[parts.find('(') + 1:-1])
    
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

def parse_log_file(log_file_name):
    f = open(log_file_name)
    
    list = []
    
    for line in f.readlines():
        try:
            list.append(parse_line(line))

        except Exception as e:
            print("an error occurred: " + str(e))

    f.close()
    
    return list

if (__name__ == "__main__"):
    if (len(sys.argv) > 1):
        list = parse_log_file(sys.argv[1])
