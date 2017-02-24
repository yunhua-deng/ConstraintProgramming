ss = get(0, 'ScreenSize');
set(gcf, 'Position', [ss(1) ss(2) ss(3) ss(4)]);

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
subplot(3, 3, pos);

data = zeros(length(conf_size_list), length(alg_name_list));
for i = 1:length(conf_size_list)
    data(i, :) = csvread(sprintf('%d_latency_avg.csv', conf_size_list(i)));
end

alg_name_list = {
    'CP'
    'Single-DC'
    'Nearest-DC'
    };

plot(data(:, 2), '-o', 'LineWidth', 1);
hold on;
plot(data(:, 3), '-*', 'LineWidth', 1);
plot(data(:, 4), '-^', 'LineWidth', 1);

lh = legend(alg_name_list, 'Orientation', 'vertical', 'Location', 'best');
set(lh, 'FontSize', 10);

xlabel('Conference size (number of parties)', 'FontSize', 12);
ylabel('Latency [msec]', 'FontSize', 12);
title('');

set(gca, 'fontsize', 10);
set(gca, 'XLim', [1-0.1 length(conf_size_list)+0.1]);
set(gca, 'XTick', 1:length(conf_size_list));
set(gca, 'XTickLabel', conf_size_list);

box on;
grid on;  
hold off;

% latency_CDF
for i = 1:length(conf_size_list)
    pos = pos + 1;
    subplot(3, 3, pos);
    
    data = dlmread(sprintf('%d_latency_CDF.csv', conf_size_list(i)));
    
    hold on;
    for j = 1:length(alg_name_list)
        p_h = cdfplot(data(j + 1, :));
        set(p_h, 'LineWidth', 2);
    end
    hold off;
    
    lh = legend(alg_name_list, 'Orientation', 'vertical', 'Location', 'best');
    set(lh, 'FontSize', 10);
    
    xlabel('Latency [msec]', 'FontSize', 12);
    ylabel(sprintf('CDF of conferences (%d-party)', conf_size_list(i)), 'FontSize', 12);
    title('');
    
    set(gca, 'fontsize', 10);
    set(gca, 'XLim', [100 300]);
    box on;
    grid on;  
    hold off;
end

% other metrics

alg_name_list = {
    'CP-Proximity'
    'CP-Cardinality'
    'Single-DC'
    'Nearest-DC'
    };

line_marker_list = {    
    '-o'
    '-p'    
    '-*'
    '-^'};

metric_name_list = {
    'cardinality'
    %'proximity'    
    %'proximityLocal'
    'ratioNearest'
    'ratioNearestLocal'
    %'time'
    };

y_label_list = {
    'Cardinality'    
    'Ratio of Nearest (global)'
    'Ratio of Nearest (local)'
    %'Proximity (global)'
    %'Proximity (local)'
    %'Time (slowest) [msec]'
    };

data = zeros(length(conf_size_list), length(alg_name_list));

for metric_index = 1:length(metric_name_list)    
    subplot(3, length(metric_name_list), metric_index + 6);
    
%     for i = 1:length(conf_size_list)         
%         data(i, :) = csvread(sprintf('%d_%s_avg.csv', conf_size_list(i), char(metric_name_list(metric_index))));
%         if metric_index == 3
%             data(i, :) = csvread(sprintf('%d_%s_max.csv', conf_size_list(i), char(metric_name_list(metric_index))));
%         end
%     end
%     
%     if metric_index == 3
%         for i = 1:length(alg_name_list(1:2))
%             plot(data(:, i), char(line_marker_list(i)));
%             hold on;
%         end
%     else
%         for i = 1:length(alg_name_list)
%             plot(data(:, i), char(line_marker_list(i)));
%             hold on;
%         end
%     end   

    for i = 1:length(conf_size_list)         
        data(i, :) = csvread(sprintf('%d_%s_avg.csv', conf_size_list(i), char(metric_name_list(metric_index))));
    end

    for i = 1:length(alg_name_list)
        plot(data(:, i), char(line_marker_list(i)));
        hold on;
    end
    
    set(gca, 'fontsize', 10);
    set(gca, 'XLim', [1-0.05 length(conf_size_list)+0.05]);
    set(gca, 'XTick', 1:length(conf_size_list));
    set(gca, 'XTickLabel', conf_size_list);
    
    lh = legend(alg_name_list, 'Orientation', 'vertical', 'Location', 'best');
    set(lh, 'FontSize', 10);

    xlabel('Conference size (number of parties)', 'FontSize', 12);
    ylabel(char(y_label_list(metric_index)), 'FontSize', 12);
    title('');

    box on;
    grid on;
end

export_fig all.pdf %-transparent