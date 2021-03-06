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
set(gcf, 'Position', [ss(1) ss(2) ss(3)/2.5 ss(4)/2.5]);
%data = importdata('CDF_DelayToNearestDC.txt'); % importdata() is memory-intensive
data = dlmread('CDF_DelayToNearestDC.txt');
p_h = cdfplot(data);
set(gca, 'fontsize', 10);
%set(gca, 'XTick', [0 30 60 90 120 150]);
set(p_h, 'LineStyle', '-', 'LineWidth', 2, 'Color', 'b');
xlabel('Latency to closest EC2 datacenter (msec)', 'FontSize', 12);
ylabel('Cumulative fraction of prefixes', 'FontSize', 12);
title('');
grid on;
export_fig CDF_DelayToNearestDC.pdf -transparent

%% CDF_ShortestPathLength
ss = get(0, 'ScreenSize');
set(gcf, 'Position', [ss(1) ss(2) ss(3)/2.5 ss(4)/2.5]);
data = dlmread('CDF_ShortestPathLength.txt');
p_h = cdfplot(data);
set(gca, 'fontsize', 10);
set(p_h, 'LineStyle', '-', 'LineWidth', 2, 'Color', 'b');
xlabel('Latency of shortest path (msec)', 'FontSize', 12);
ylabel('Cumulative fraction of prefix pairs', 'FontSize', 12);
title('');
grid on;
export_fig CDF_ShortestPathLength.pdf -transparent

%% latency
ss = get(0, 'ScreenSize');
set(gcf, 'Position', [ss(1) ss(2) ss(3) ss(4)/1.5]);

conf_size_list = [4 8 12 16 20];

alg_name_list = {
    'CP'
    'CP-C'    
    'Single-DC'
    'Nearest-DC'
    };

pos = 0;

% latency_avg
pos = pos + 1;
subplot(2, 3, pos);

data = zeros(length(conf_size_list), length(alg_name_list));
for i = 1:length(conf_size_list)
    data(i, :) = csvread(sprintf('%d_latency_avg.csv', conf_size_list(i)));
end

alg_name_list = {
    'CP'
    'Single-DC'
    'Nearest-DC'
    };

plot(data(:, 2), '-o', 'LineWidth', 1, 'MarkerSize', 8);
hold on;
plot(data(:, 3), '-x', 'LineWidth', 1, 'MarkerSize', 8);
plot(data(:, 4), '-^', 'LineWidth', 1, 'MarkerSize', 8);

lh = legend(alg_name_list, 'Orientation', 'vertical', 'Location', 'best');
set(lh, 'FontSize', 12);

xlabel('Conference size (number of parties)', 'FontSize', 14);
ylabel('Latency (average) [msec]', 'FontSize', 14);
title('');

set(gca, 'fontsize', 12);
set(gca, 'XLim', [1-0.25 length(conf_size_list)+0.25]);
set(gca, 'XTick', 1:length(conf_size_list));
set(gca, 'XTickLabel', conf_size_list);

box on;
grid on;  
hold off;

% latency_CDF
for i = 1:length(conf_size_list)
    pos = pos + 1;
    subplot(2, 3, pos);
    
    data = dlmread(sprintf('%d_latency_CDF.csv', conf_size_list(i)));
    
    hold on;
    for j = 1:length(alg_name_list)
        p_h = cdfplot(data(j + 1, :));
        set(p_h, 'LineWidth', 2);
    end
    hold off;
    
    lh = legend(alg_name_list, 'Orientation', 'vertical', 'Location', 'best');
    set(lh, 'FontSize', 12);
    
    xlabel('Latency [msec]', 'FontSize', 14);
    ylabel(sprintf('CDF of conferences (%d-party)', conf_size_list(i)), 'FontSize', 14);
    title('');
    
    set(gca, 'fontsize', 12);
    set(gca, 'XLim', [100 300]);
    box on;
    grid on;  
    hold off;
end
export_fig latency.pdf %-transparent

%% cardinality_CDF
ss = get(0, 'ScreenSize');
set(gcf, 'Position', [ss(1) ss(2) ss(3) ss(4)/3]);

conf_size_list = [5 10 15 20];

alg_name_list = {
    'CP'
    'CP-C'
    'CP-P'
    'CP-C-1'
    'CP-P-1'
    };

pos = 0;
for i = 1:length(conf_size_list)
    pos = pos + 1;
    subplot(1, length(conf_size_list), pos);
    
    data = dlmread(sprintf('%d_cardinality_CDF.csv', conf_size_list(i)));
    
    hold on;
    for j = 1:length(alg_name_list)
        p_h = cdfplot(data(j, :));
        set(p_h, 'LineWidth', 2);
    end
    hold off;
    
    lh = legend(alg_name_list, 'Orientation', 'vertical', 'Location', 'best');
    set(lh, 'FontSize', 10);
    
    xlabel('Cardinality', 'FontSize', 12);
    ylabel(sprintf('CDF of %d-party conferences', conf_size_list(i)), 'FontSize', 12);
    title('');
    
    set(gca, 'fontsize', 10);
    set(gca, 'XLim', [1 11]);
    set(gca, 'XTick', 1:11);
    
    box on;
    grid on;  
    hold off;
end
%export_fig cardinality_CDF.pdf -transparent

%% proximity_CDF
ss = get(0, 'ScreenSize');
set(gcf, 'Position', [ss(1) ss(2) ss(3) ss(4)/3]);

conf_size_list = [5 10 15 20];

alg_name_list = {
    'CP'
    'CP-C'
    'CP-P'
    'CP-C-1'
    'CP-P-1'
    };

pos = 0;
for i = 1:length(conf_size_list)
    pos = pos + 1;
    subplot(1, length(conf_size_list), pos);    
    
    data = dlmread(sprintf('%d_proximity_CDF.csv', conf_size_list(i)));
    
    hold on;
    for j = 1:length(alg_name_list)
        p_h = cdfplot(data(j, :));
        set(p_h, 'LineWidth', 2);
    end
    hold off;
    
    lh = legend(alg_name_list, 'Orientation', 'vertical', 'Location', 'southeast');
    set(lh, 'FontSize', 10);
    
    xlabel('Proximity', 'FontSize', 12);
    ylabel(sprintf('CDF of users of %d-party conferences', conf_size_list(i)), 'FontSize', 12);
    title('');
    
    set(gca, 'fontsize', 10);
    set(gca, 'XLim', [1 11]);
    set(gca, 'XTick', 1:11);
    
    box on;
    grid on;   
end
%export_fig proximity_CDF.pdf -transparent