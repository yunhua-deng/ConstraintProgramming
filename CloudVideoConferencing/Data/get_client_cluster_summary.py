# -*- coding: utf-8 -*-
"""
Created on Mon Aug 22 19:52:12 2016

@author: yhdeng
"""

infile_name = 'Client_Clustering.txt'
outfile_name = 'Client_clustering_Summary.txt'

with open(infile_name, 'r') as infile:    
    with open(outfile_name, 'w') as outfile:
        for line in infile:
            outfile.write(line.split(',')[0] + ',' + str(len(line.split(',')) - 1) + '\n')