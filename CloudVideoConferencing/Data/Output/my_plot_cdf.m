%% CDF_DelayToDC_90thOver50th
ss = get(0, 'ScreenSize');
set(gcf, 'Position', [ss(1) ss(2) ss(3)/3 ss(4)/3]);

hold on;

data = csvread('ap-southeast-1.ping_to_prefix_ratio_sorted.csv', 0, 1);
p_h = cdfplot(data);
set(p_h, 'LineStyle', '-', 'LineWidth', 2, 'Color', 'b');
data = csvread('us-east-1.ping_to_prefix_ratio_sorted.csv', 0, 1);
p_h = cdfplot(data);
set(p_h, 'LineStyle', '-.', 'LineWidth', 2, 'Color', 'r');

l_h = legend({'Singapore EC2', 'US.Virginia EC2'}, 'Orientation', 'vertical', 'Location', 'best');
set(l_h, 'fontsize', 12);

set(gca, 'XLim', [1 2]);
xlabel('Ratio of 90th-percentile latency to median latency', 'FontSize', 14);
ylabel('Cumulative fraction of prefixes', 'FontSize', 14);
title('');
box on;
export_fig CDF_DelayToDC_90thOver50th.pdf -transparent

%% CDF_DelayToNearestDC
ss = get(0, 'ScreenSize');
set(gcf, 'Position', [ss(1) ss(2) ss(3)/3 ss(4)/3]);
%data = importdata('CDF_DelayToNearestDC.txt'); % importdata() is memory-intensive
data = dlmread('CDF_DelayToNearestDC.txt');
p_h = cdfplot(data);
set(gca, 'fontsize', 10);
%set(gca, 'XTick', [0 30 60 90 120 150]);
set(p_h, 'LineStyle', '-', 'LineWidth', 2, 'Color', 'b');
xlabel('Latency to closest EC2 datacenter (msec)', 'FontSize', 14);
ylabel('Cumulative fraction of prefixes', 'FontSize', 14);
title('');
grid on;
export_fig CDF_DelayToNearestDC.pdf -transparent

%% CDF_ShortestPathLength
ss = get(0, 'ScreenSize');
set(gcf, 'Position', [ss(1) ss(2) ss(3)/3 ss(4)/3]);
data = dlmread('CDF_ShortestPathLength.txt');
p_h = cdfplot(data);
set(gca, 'fontsize', 10);
set(p_h, 'LineStyle', '-', 'LineWidth', 2, 'Color', 'b');
xlabel('Latency of shortest path (msec)', 'FontSize', 14);
ylabel('Cumulative fraction of prefix pairs', 'FontSize', 14);
title('');
grid on;
export_fig CDF_ShortestPathLength.pdf -transparent

%% CDF_Cardinality
ss = get(0, 'ScreenSize');
set(gcf, 'Position', [ss(1) ss(2) ss(3) ss(4)/3]);

conf_size_list = zeros(1, 5);
conf_size_list(1) = 4;
for i = 2:length(conf_size_list)
    conf_size_list(i) = conf_size_list(i - 1) + 4;
end

alg_name_list = {'CP(1,0)' 'CP(0,2)' 'CP(0,3)' 'CP(0,4)'};
line_style_list = {'-' '--' ':' '-.'};
line_color_list = {'r' 'g' 'b' 'c'};

pos = 0;
for i = 1:length(conf_size_list)
    pos = pos + 1;
    subplot(1, length(conf_size_list), pos);
    
    data = dlmread(sprintf('%d_cardinality_CDF.csv', conf_size_list(i)));
    
    hold on;
    for j = 1:length(alg_name_list)
        p_h = cdfplot(data(j, :));
        set(p_h, 'LineStyle', line_style_list(j), 'LineWidth', 2, 'Color', line_color_list(j));
    end
    hold off;
    
    lh = legend(alg_name_list, 'Orientation', 'vertical', 'Location', 'best');
    set(lh, 'FontSize', 10);
    
    xlabel('Number of selected datacenters', 'FontSize', 10);
    ylabel(sprintf('CDF of size-%d conferences', conf_size_list(i)), 'FontSize', 10);
    title('');
    
    set(gca, 'fontsize', 10);
    set(gca, 'XLim', [1 11]);
    set(gca, 'XTick', 1:11);
    
    box on;
    grid on;  
    hold off;
end
export_fig CDF_Cardinality.pdf -transparent

%% CDF_Ranking
ss = get(0, 'ScreenSize');
set(gcf, 'Position', [ss(1) ss(2) ss(3) ss(4)/4]);

conf_size_list = zeros(1, 5);
conf_size_list(1) = 4;
for i = 2:length(conf_size_list)
    conf_size_list(i) = conf_size_list(i - 1) + 4;
end

alg_name_list = {'CP(0,1)', 'CP(0,2)', 'CP(0,3)', 'CP(0,4)'};
line_style_list = {'-', '--', ':', '-.'};
line_color_list = {'r', 'g', 'b', 'c'};

pos = 0;
for i = 1:length(conf_size_list)
    pos = pos + 1;
    subplot(1, length(conf_size_list), pos);    
    
    data = dlmread(sprintf('%d_ranking_CDF.csv', conf_size_list(i)));
    
    hold on;
    for j = 1:length(alg_name_list)
        p_h = cdfplot(data(j, :));
        set(p_h, 'LineStyle', line_style_list(j), 'LineWidth', 2, 'Color', line_color_list(j));
    end
    hold off;
    
    lh = legend(alg_name_list, 'Orientation', 'vertical', 'Location', 'southeast');
    set(lh, 'FontSize', 10);
    
    xlabel('Assigned datacenter''s proximity ranking', 'FontSize', 10);
    ylabel(sprintf('CDF of users in size-%d conferences', conf_size_list(i)), 'FontSize', 10);
    title('');
    
    set(gca, 'fontsize', 10);
    set(gca, 'XLim', [1 11]);
    set(gca, 'XTick', 1:11);
    
    box on;
    grid on;   
end
export_fig CDF_Ranking.pdf -transparent