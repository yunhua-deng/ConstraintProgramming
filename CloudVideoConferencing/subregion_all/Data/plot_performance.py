# -*- coding: utf-8 -*-
"""
Created on Thu Aug 11 17:47:16 2016

@author: yhdeng
"""

import os
import csv
import re
import matplotlib
from collections import defaultdict
import matplotlib.pyplot as plt

session_size_list = ['8', '12', '16']
session_count_list = ['1000', '1000', '1000']

for session_size, session_count in zip(session_size_list, session_count_list):
	all_data = defaultdict(list)
	alg_name_list = []
	metric_name_list = []
	
	for file_name in os.listdir():
		if re.match(('Details_SessionSize-' + session_size + '_.*\.csv'), file_name) != None:
			print(file_name)
			extracted_strings = re.search('.*_.*_(.*)\.csv', file_name)	 
			alg_name_list.append(extracted_strings.group(1))		
			with open(file_name, 'r') as csvfile:
				reader = csv.DictReader(csvfile)	 
				metric_name_list = reader.fieldnames
				metric_name_list.remove('session_id')
				metric_name_list.remove('interDC_cost_ratio')
				for metric_name in metric_name_list:				
					if metric_name != 'session_id':									  
						with open(file_name, 'r') as csvfile: # reopen the file in order to start a new iteration for each metric_name
							reader = csv.DictReader(csvfile)
							one_alg_data = [float(row[metric_name]) for row in reader]
							all_data[metric_name].append(one_alg_data)

	plt.figure()
	matplotlib.rc('xtick', labelsize=14) 
	matplotlib.rc('ytick', labelsize=14) 
	fig, axes = plt.subplots(nrows=1, ncols=4, figsize=(30, 5))
	counter = 0
	for metric_name in metric_name_list:   
		axes[counter].boxplot(all_data[metric_name], labels=alg_name_list, showmeans=True)		  
		axes[counter].set_xlabel('Algorithm', fontsize=18)
		if 'achieved_delay_bound' == metric_name:
			metric_name += ' (msec)'		
		elif 'data_transfer_cost' == metric_name:
			metric_name += ' (USD)'
		elif 'alg_running_time' == metric_name:
			metric_name += ' (msec)'
		axes[counter].set_ylabel(metric_name, fontsize=18)
		axes[counter].set_title(metric_name, fontsize=18)
		axes[counter].yaxis.grid(True)		
		counter+=1
	plt.suptitle('Performance comparison with session size = ' + session_size, fontsize=20, y=1.05);
	plt.savefig('Performance_for_SessionSize-' + session_size + '.pdf', bbox_inches='tight')
	
#for metric_name in all_data:
#	 plt.figure()	 
#	 plt.ylabel(metric_name)
#	 plt.xlabel('Algorithm')
#	 plt.boxplot(all_data[metric_name], labels=alg_name_list, showmeans=True)
#	 plt.show()
#	 #plt.savefig(metric_name + '.pdf')