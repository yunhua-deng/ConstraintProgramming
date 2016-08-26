# -*- coding: utf-8 -*-
"""
Created on Fri Aug 12 11:45:46 2016

@author: yhdeng
"""

import csv
import matplotlib
import matplotlib.pyplot as plt

csv_file_name_list = ['ping_to_pl_median_matrix', \
'ping_to_prefix_median_matrix', \
'ping_to_dc_median_matrix']

fig_title_list = ['Distribution of the latency (RTT) from each datacenter to 150+ PlanetLab nodes', \
'Distribution of the latency (RTT) from each datacenter to 15K+ IP prefixes', \
'Distribution of the latency (RTT) from each datacenter to other 10 datacenters']

skip_zero_option_list = [False, False, True]

matplotlib.rc('xtick', labelsize=16)
matplotlib.rc('ytick', labelsize=16)

fig, axes = plt.subplots(nrows=3, ncols=1, figsize=(20, 25))

subfig_counter = 0

for csv_file_name, fig_title, skip_zero_option in zip(csv_file_name_list, fig_title_list, skip_zero_option_list):
    with open(csv_file_name + '.csv', 'r') as csvfile:
        csvreader = csv.DictReader(csvfile)
        dc_list = csvreader.fieldnames
        dc_list.remove('')
       
        all_data = []
        for dc in dc_list:    
            with open(csv_file_name + '.csv', 'r') as csvfile:
                csvreader = csv.DictReader(csvfile)
                all_data.append([int(row[dc]) for row in csvreader if not skip_zero_option or (skip_zero_option and row[dc] != '0')]) 
        
        plt.sca(axes[subfig_counter])
        plt.xticks(rotation=15)
        axes[subfig_counter].boxplot(all_data, labels=dc_list, showmeans=True)
        axes[subfig_counter].set_ylabel('Latency (RTT) in msec', fontsize=20)
        axes[subfig_counter].set_xlabel('Datacenter', fontsize=20)
        axes[subfig_counter].set_title(fig_title, fontsize=20, y=1.02)
        axes[subfig_counter].yaxis.grid(True)
        subfig_counter += 1
plt.subplots_adjust(hspace=0.4)
plt.savefig('latency_dataset_summary.pdf', bbox_inches='tight')
        