%%
ss = get(0, 'ScreenSize');
set(gcf, 'Position', [ss(1) ss(2) ss(3) ss(4)/3]);

conf_size_list = zeros(1, 7);
conf_size_list(1) = 4;
for i = 2:length(conf_size_list)
    conf_size_list(i) = conf_size_list(i - 1) + 2;
end
alg_name_list = {
    'CP(1,0)'
    'CP(0,1)'
    'CP(0,2)'
    'CP(0,3)'
    'CP(0,4)'
    'CP(0,5)'};
bar_color_list = {    
    'LightGreen'
    'LightBlue'
    'Khaki'
    'SandyBrown'
    'MediumPurple'
    'DarkBlue'};

data = zeros(length(conf_size_list), length(alg_name_list));
data_std = zeros(length(conf_size_list), length(alg_name_list), 2);

pos = 0;

% latency_avg
pos = pos + 1;
subplot(1, 4, pos);

for i = 1:length(conf_size_list)
    data(i, :) = csvread(sprintf('%d_latency.csv', conf_size_list(i)));   
end

bh = bar(data);
for i = 1:length(alg_name_list)
    bh(i).FaceColor = rgb(bar_color_list(i));
end

set(gca, 'XLim', [0 (length(conf_size_list) + 1)]);
set(gca, 'XTickLabel', conf_size_list);
set(gca, 'YLim', [100 300]);
set(gca, 'fontsize', 10);

lh = legend(alg_name_list, 'Orientation', 'vertical', 'Location', 'best');
set(lh, 'FontSize', 10);

xlabel('Conference Size', 'FontSize', 12);
ylabel('Average Latency [msec]', 'FontSize', 12);
title('');

box on;
grid on;

% latency_99th
pos = pos + 1;
subplot(1, 4, pos);

for i = 1:length(conf_size_list)
    data(i, :) = csvread(sprintf('%d_latency_max.csv', conf_size_list(i)));   
end

bh = bar(data);
for i = 1:length(alg_name_list)
    bh(i).FaceColor = rgb(bar_color_list(i));
end

set(gca, 'XLim', [0 (length(conf_size_list) + 1)]);
set(gca, 'XTickLabel', conf_size_list);
set(gca, 'YLim', [100 300]);
set(gca, 'fontsize', 10);

lh = legend(alg_name_list, 'Orientation', 'vertical', 'Location', 'best');
set(lh, 'FontSize', 10);

xlabel('Conference Size', 'FontSize', 12);
ylabel('Maximum Latency [msec]', 'FontSize', 12);
title('');

box on;
grid on;

% cost
pos = pos + 1;
subplot(1, 4, pos);

data_normalized = [];
for i = 1:length(conf_size_list)
    data(i, :) = csvread(sprintf('%d_cost.csv', conf_size_list(i)));
    data(i, :) = data(i, 1:end) / data(i, 1);  
    data_normalized = [data_normalized; data(i, 2:end)];
end

bh = bar(data_normalized);
for i = 1:(length(alg_name_list) - 1)
    bh(i).FaceColor = rgb(bar_color_list(i + 1));
end

set(gca, 'XLim', [0 (length(conf_size_list) + 1)]);
set(gca, 'XTickLabel', conf_size_list);
%set(gca, 'YLim', [0.5 1]);
set(gca, 'fontsize', 10);

lh = legend(alg_name_list(2:end), 'Orientation', 'vertical', 'Location', 'best');
set(lh, 'FontSize', 10);

xlabel('Conference Size', 'FontSize', 12);
ylabel('Average Cost, normalized by CP(1,0)', 'FontSize', 12);
title('');

box on;
grid on;

% time
pos = pos + 1;
subplot(1, 4, pos);

conf_size_list = zeros(1, 4);
conf_size_list(1) = 10;
for i = 2:length(conf_size_list)
    conf_size_list(i) = conf_size_list(i - 1) + 2;
end
alg_name_list = alg_name_list(3:end);
data = zeros(length(conf_size_list), length(alg_name_list));
data_std = zeros(length(conf_size_list), length(alg_name_list), 2); % time_max

for i = 1:length(conf_size_list)
    data(i, :) = csvread(sprintf('%d_time.csv', conf_size_list(i)));
    data_std(i, :, 2) = csvread(sprintf('%d_time_max.csv', conf_size_list(i)));
end
data_std(:, :, 2) = data_std(:, :, 2) - data;

bh = barwitherr(data_std./1000, data./1000);
for i=1:length(alg_name_list)
    bh(i).FaceColor = rgb(bar_color_list(i+2));
end

set(gca, 'yscale',  'log');
set(gca, 'XTickLabel', conf_size_list);
set(gca, 'fontsize', 10);

lh = legend(alg_name_list, 'Orientation', 'vertical', 'Location', 'best');
set(lh, 'FontSize', 10);

xlabel('Conference Size', 'FontSize', 12);
ylabel('Average & Maximum Runtime [sec]', 'FontSize', 12);
title('');

box on;
grid on;

export_fig OptimizingLatencyFirst.pdf -transparent