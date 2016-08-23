# -*- coding: utf-8 -*-
"""
Created on Fri Aug 12 15:58:12 2016

@author: yhdeng
"""

import os
import csv
import re
import matplotlib
import matplotlib.pyplot as plt
import numpy
from collections import OrderedDict

session_size_list = ['8', '12', '16']
session_count_list = ['1000', '1000', '100']

alg_name_list = ['CP-1', 'CP-2', 'CP-3', 'CP-4', 'CP-4-fast', 'NA-all', 'NA-sub']
dc_list = ['ec2-ap-northeast-1', 'ec2-ap-northeast-2', 'ec2-ap-south-1', 'ec2-ap-southeast-1', 'ec2-ap-southeast-2', 'ec2-eu-central-1', 'ec2-eu-west-1', 'ec2-sa-east-1', 'ec2-us-east-1', 'ec2-us-west-1', 'ec2-us-west-2']
file_name_list = os.listdir()

for session_size, session_count in zip(session_size_list, session_count_list):
	
	matplotlib.rc('xtick', labelsize=18) 
	matplotlib.rc('ytick', labelsize=14)	
	fig, axes = plt.subplots(nrows=1, ncols=7, figsize=(40, 5))
		
	for i, alg_name in enumerate(alg_name_list):		
		appearance_count_dc_dict = OrderedDict.fromkeys(dc_list)
		for key in appearance_count_dc_dict:
			appearance_count_dc_dict[key] = 0		 
		for file_name in file_name_list:
			chosen_dc_set = []
			if re.match(('SessionSize-' + session_size + '_.*_' + alg_name), file_name) != None:
				with open(file_name, 'r') as csvfile:
					reader = csv.DictReader(csvfile)
					for row in reader:
						#dc = re.search('ec2-(.{2})-', row[reader.fieldnames[-1]]).group(1)
						dc = row[reader.fieldnames[-1]]
						if dc not in chosen_dc_set:							
							appearance_count_dc_dict[dc] += 1
							chosen_dc_set.append(dc)
 
		for key, value in appearance_count_dc_dict.items():
			print(key, value)
		appearance_count_dc_list = [value for key, value in appearance_count_dc_dict.items()]		 
		ind = numpy.arange(len(dc_list))
		width = 0.5
		axes[i].bar(ind, appearance_count_dc_list, width)
		axes[i].set_xlim(-width, len(ind) + width/2)
		axes[i].set_xticks(ind + width/2)
		axes[i].set_xticklabels(dc_list)
		#axes[i].set_ylim(0, ylim_value)
		axes[i].set_ylabel('Number of sessions', fontsize=18)
		axes[i].set_xlabel('Datacenter', fontsize=18)
		axes[i].set_title(alg_name, fontsize=18)
		axes[i].yaxis.grid(True)
		plt.subplots_adjust(wspace=0.33)
		plt.sca(axes[i])
		plt.xticks(rotation=90)
	plt.suptitle('Number of sessions choosing each datacenter for session_size = ' + session_size, fontsize=20, y=1.05);
	plt.savefig('DatacenterSelection_for_SessionSize-' + session_size + '.pdf', bbox_inches='tight')