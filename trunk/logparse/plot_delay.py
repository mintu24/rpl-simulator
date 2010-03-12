#!/usr/bin/env python

import sys
import os

import rslog

def load_rank_delay(log_file_name):
    log_lines = rslog.parse_log_file(log_file_name, ["ping_request", "ping_reply", "ping_timeout"])
    
    rank_delay_list = []
    ping_dict = {}
    
    line_no = 0
    for log_line in log_lines:
        line_no += 1

        time = log_line[0]
        event_name = log_line[3]
        src = log_line[4][0][1][4]
        dst = log_line[4][2][1]
        seq_num = log_line[4][3][1]
        rank = log_line[4][0][1][6]

        if event_name == "ping_request":
            ping_dict[src + dst + str(seq_num)] = (time, rank)
        
        elif event_name == "ping_reply":
            if ping_dict.has_key(dst + src + str(seq_num)):
                (req_time, rank) = ping_dict[dst + src + str(seq_num)]
                delay = time - req_time
                rank_delay_list.append((rank, delay))
                print("line_no = %d, rank = %d, delay = %d" % (line_no, rank, delay))
                
                del(ping_dict[dst + src + str(seq_num)])
            
            else: # reply came too late
                pass
                #print("unexpected ping reply encountered (@%d : %s -> %s)" % (time, src, dst))
        
        elif event_name == "ping_timeout":
            if ping_dict.has_key(dst + src + str(seq_num)):
                del(ping_dict[dst + src + str(seq_num)])
            
            else: # 
                pass
            
    return rank_delay_list


def compose_graph1(rank_delay_list, rank_width):
    rank_dict = {}
    point_list = []
    
    for (rank, delay) in rank_delay_list:
        if rank_dict.has_key(rank):
            rank_dict[rank].append(delay)
        else:
            rank_dict[rank] = [delay]

    for rank in rank_dict:
        delay_list = rank_dict[rank]
        delay_list.sort()
        
        for i in xrange(len(delay_list)):
            x = rank * rank_width + i * rank_width / len(delay_list)
            y = delay_list[i]
            
            point_list.append((x, y))
            
        point_list.append((-1, -1))
            
    return point_list


def compose_graph2(rank_delay_list, rank_width):
    rank_dict = {}
    point_list = []
    
    for (rank, delay) in rank_delay_list:
        if rank_dict.has_key(rank):
            rank_dict[rank].append(delay)
        else:
            rank_dict[rank] = [delay]

    
    for rank in xrange(1, max(rank_dict) + 1):
        if rank_dict.has_key(rank):
            delay_list = rank_dict[rank]
            
            y = float(sum(delay_list)) / len(delay_list)
            
            x = rank * rank_width
            point_list.append((x, y))
            
            x = (rank + 1) * rank_width
            point_list.append((x, y))
        
        else:
            #x = rank * rank_width
            #y = 0
            
            #point_list.append((x, y))
            point_list.append((-1, -1))
            
    return point_list


def write_temp_file(file_name, point_list):
    f = open(file_name, "w")
    
    for (x, y) in point_list:
        if (x == -1) and (y == -1):
            f.write("\n")
        else:
            f.write(str(x) + " " + str(y) + "\n")
    
    f.close()

def compose_plot_command(temp_file_name1, temp_file_name2):
    gnuplot_subcmd_list = []
    
    #gnuplot_subcmd_list.append("set yrange[0:%d]" % (dat_info_list[len(dat_info_list) - 1][0] + 2))
    gnuplot_subcmd_list.append("set title 'Delay vs. Rank'")
    #gnuplot_subcmd_list.append("set grid xtics 1000")
    gnuplot_subcmd_list.append("set xlabel 'Rank'")
    gnuplot_subcmd_list.append("set ylabel 'Delay [ms]'")
    
    gnuplot_subcmd_list.append("plot '%s' with points, '%s' with lines lt 3 lw 1" % (temp_file_name1, temp_file_name2))
    
    gnuplot_subcmds = ""
    for subcmd in gnuplot_subcmd_list:
        gnuplot_subcmds += "; " + subcmd
    
    gnuplot_cmd = "gnuplot -persist -e \"%s\"" % (gnuplot_subcmds)
    
    return gnuplot_cmd


if __name__ == '__main__':
    if (len(sys.argv) < 2):
        print("usage: %s <file.log>\n" % (sys.argv[0]))
        sys.exit()
    
    rank_delay_list = load_rank_delay(sys.argv[1])
    point_list1 = compose_graph1(rank_delay_list, 1000)
    point_list2 = compose_graph2(rank_delay_list, 1000)
    write_temp_file("temp1.dat", point_list1)
    write_temp_file("temp2.dat", point_list2)
    os.system(compose_plot_command("temp1.dat", "temp2.dat"))
