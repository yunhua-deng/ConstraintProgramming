%%
ss = get(0, 'ScreenSize');
set(gcf, 'Position', [ss(1) ss(2) ss(3) ss(4)/3]);

conf_size_list = [4 8 12 16 20];

alg_name_list = {
    'CP-Proximity'
    'CP-Cardinality'
    'Single-DC'
    'Nearest-DC'
    };

line_marker_list = {    
    '-o'
    '-*'    
    '-x'
    '-^'};

metric_name_list = {
    'cardinality'
    'proximity'
    'time'
    };

y_label_list = {
    'Cardinality (average)'
    'Proximity (average)'
    'Time (slowest) [msec]'
    };

data = zeros(length(conf_size_list), length(alg_name_list));

for metric_index = 1:length(metric_name_list)    
    subplot(1, length(metric_name_list), metric_index);
    
    for i = 1:length(conf_size_list)         
        data(i, :) = csvread(sprintf('%d_%s_avg.csv', conf_size_list(i), char(metric_name_list(metric_index))));
        if metric_index == 3
            data(i, :) = csvread(sprintf('%d_%s_max.csv', conf_size_list(i), char(metric_name_list(metric_index))));
        end
    end
    
    if metric_index == 3
        for i = 1:length(alg_name_list(1:2))
            plot(data(:, i), char(line_marker_list(i)), 'MarkerSize', 8);
            hold on;
        end
    else
        for i = 1:length(alg_name_list)
            plot(data(:, i), char(line_marker_list(i)), 'MarkerSize', 8);
            hold on;
        end
    end    
    
    set(gca, 'XLim', [1-0.25 length(conf_size_list)+0.25]);
    set(gca, 'XTick', 1:length(conf_size_list));
    set(gca, 'XTickLabel', conf_size_list);

    if metric_index == 2
        set(gca, 'YTick', 1:y_max_list(metric_index));
    end
    set(gca, 'fontsize', 12);    
    
    if metric_index == 1
        lh = legend(alg_name_list, 'Orientation', 'vertical', 'Location', 'southeast');
        set(lh, 'FontSize', 12);
    end
    
    if metric_index == 2
        lh = legend(alg_name_list, 'Orientation', 'vertical', 'Location', 'east');
        set(lh, 'FontSize', 12);
    end
    
    if metric_index == 3
        lh = legend(alg_name_list(1:2), 'Orientation', 'vertical', 'Location', 'best');
        set(lh, 'FontSize', 12);
    end

    xlabel('Conference size (number of parties)', 'FontSize', 14);
    ylabel(char(y_label_list(metric_index)), 'FontSize', 14);
    title('');

    box on;
    grid on;
end

export_fig cardinality_proximity_time.pdf %-transparent