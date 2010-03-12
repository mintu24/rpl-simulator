#!/usr/bin/env python

import sys
import os
import os.path

import rslog

def load_convergence(log_file_name):
    log_lines = rslog.parse_log_file(log_file_name, ["trickle_i_timeout"])
    
    converg_list = []
    
    line_no = 0
    for log_line in log_lines:
        line_no += 1

        time = log_line[0]
        converg = log_line[4][1][1][12]
        
        converg_list.append((time, converg))

    return converg_list


def compose_graph(converg_list):
    point_list = []
    
    time_dict = {}

    for (time, converg) in converg_list:
        if (time_dict.has_key(time)):
            time_dict[time].append(converg)
        else:
            time_dict[time] = [converg]
    
    time_list = time_dict.keys()
    time_list.sort()
    
    for time in time_list:
        converg_list = time_dict[time]
        converg = sum(converg_list) / len(converg_list)
        
        x = time
        y = converg
        point_list.append((x, y))

    return point_list

def write_temp_file(file_name, point_list):
    f = open(file_name, "w")
    
    for (x, y) in point_list:
        if (x == -1) and (y == -1):
            f.write("\n")
        else:
            f.write(str(x) + " " + str(y) + "\n")
    
    f.close()

def compose_plot_command(temp_file_name_list):
    gnuplot_subcmd_list = []
    
    #gnuplot_subcmd_list.append("set yrange[0:%d]" % (dat_info_list[len(dat_info_list) - 1][0] + 2))
    gnuplot_subcmd_list.append("set title 'Convergence vs. Time'")
    gnuplot_subcmd_list.append("set xlabel 'Time [ms]'")
    gnuplot_subcmd_list.append("set ylabel 'Number of stable nodes'")

    subsubcmd = ""
    for i in xrange(len(temp_file_name_list)):
        temp_file_name = temp_file_name_list[i]
        subsubcmd += "'%s' with lines smooth bezier" % (temp_file_name)
        
        if (i < len(temp_file_name_list) - 1):
            subsubcmd += ", "
    
    gnuplot_subcmd_list.append("plot " + subsubcmd)
    
    gnuplot_subcmds = ""
    for subcmd in gnuplot_subcmd_list:
        gnuplot_subcmds += "; " + subcmd
    
    gnuplot_cmd = "gnuplot -persist -e \"%s\"" % (gnuplot_subcmds)
    
    return gnuplot_cmd


if __name__ == '__main__':
    if (len(sys.argv) < 2):
        print("usage: %s <file1.log> [...fileN.log]\n" % (sys.argv[0]))
        sys.exit()
    
    file_name_list = sys.argv[1:]
    temp_file_name_list = []
    converg_list_list = []
    
    for file_name in file_name_list:
        converg_list = load_convergence(file_name)
        point_list = compose_graph(converg_list)
        
        temp_file_name = "temp_%s.dat" % (os.path.splitext(os.path.basename(file_name))[0])
        temp_file_name_list.append(temp_file_name)
        
        write_temp_file(temp_file_name, point_list)

    os.system(compose_plot_command(temp_file_name_list))
