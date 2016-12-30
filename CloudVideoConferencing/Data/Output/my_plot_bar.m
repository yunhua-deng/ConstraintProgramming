%%
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

bar_color_list = {    
    'LightYellow'
    'LightGreen'
    'LightBlue'
    'Gold'
    'SandyBrown'
    'MediumPurple'
    'DarkBlue'
    'Black'};

data = zeros(length(conf_size_list), length(alg_name_list));
data_err = zeros(length(conf_size_list), length(alg_name_list), 2);

pos = 0;

% % latency_avg and latency_max
% pos = pos + 1;
% subplot(1, 4, pos);
% 
% for i = 1:length(conf_size_list)
%     data(i, :) = csvread(sprintf('%d_latency_avg.csv', conf_size_list(i)));
%     data_err(i, :, 2) = csvread(sprintf('%d_latency_max.csv', conf_size_list(i)));
% end
% 
% data_err(:, :, 2) = data_err(:, :, 2) - data(:, :);
% 
% bh = barwitherr(data_err, data);
% %bh = bar(data);
% 
% for i = 1:length(alg_name_list)
%     bh(i).FaceColor = rgb(bar_color_list(i));
% end
% 
% set(gca, 'XLim', [0.5 (length(conf_size_list) + 0.5)]);
% set(gca, 'XTickLabel', conf_size_list);
% set(gca, 'YLim', [100 inf]);
% set(gca, 'fontsize', 10);
% 
% lh = legend(alg_name_list, 'Orientation', 'vertical', 'Location', 'best');
% set(lh, 'FontSize', 10);
% 
% xlabel('Conference Size', 'FontSize', 12);
% ylabel('Latency [msec]', 'FontSize', 12);
% title('');
% 
% box on;
% grid on;

% cardinality_avg
pos = pos + 1;
subplot(1, 3, pos);

for i = 1:length(conf_size_list)
    data(i, :) = csvread(sprintf('%d_cardinality_avg.csv', conf_size_list(i)));
end

bh = bar(data);
for i = 1:(length(alg_name_list))
    bh(i).FaceColor = rgb(bar_color_list(i));
end

set(gca, 'XLim', [0.5 (length(conf_size_list) + 0.5)]);
set(gca, 'XTickLabel', conf_size_list);
%set(gca, 'YLim', [0.5 1]);
set(gca, 'fontsize', 10);

lh = legend(alg_name_list, 'Orientation', 'vertical', 'Location', 'best');
set(lh, 'FontSize', 10);

xlabel('Conference Size', 'FontSize', 12);
ylabel('Cardinality', 'FontSize', 12);
title('');

box on;
grid on;

% proximity_avg
pos = pos + 1;
subplot(1, 3, pos);

for i = 1:length(conf_size_list)
    data(i, :) = csvread(sprintf('%d_proximity_avg.csv', conf_size_list(i)));
end

bh = bar(data);
for i = 1:(length(alg_name_list))
    bh(i).FaceColor = rgb(bar_color_list(i));
end

set(gca, 'XLim', [0.5 (length(conf_size_list) + 0.5)]);
set(gca, 'XTickLabel', conf_size_list);
%set(gca, 'YLim', [0.5 1]);
set(gca, 'fontsize', 10);

lh = legend(alg_name_list, 'Orientation', 'vertical', 'Location', 'best');
set(lh, 'FontSize', 10);

xlabel('Conference Size', 'FontSize', 12);
ylabel('Proximity', 'FontSize', 12);
title('');

box on;
grid on;

% time_avg and time_max
pos = pos + 1;
subplot(1, 3, pos);

for i = 1:length(conf_size_list)
    data(i, :) = csvread(sprintf('%d_time_avg.csv', conf_size_list(i)));
    data_err(i, :, 2) = csvread(sprintf('%d_time_max.csv', conf_size_list(i)));
end

data_err(:, :, 2) = data_err(:, :, 2) - data(:, :);

bh = barwitherr(data_err, data);

for i = 1:length(alg_name_list)
    bh(i).FaceColor = rgb(bar_color_list(i));
end

set(gca, 'XLim', [0.5 (length(conf_size_list) + 0.5)]);
set(gca, 'XTickLabel', conf_size_list);
%set(gca, 'YLim', [100 300]);
set(gca, 'fontsize', 10);

lh = legend(alg_name_list, 'Orientation', 'vertical', 'Location', 'best');
set(lh, 'FontSize', 10);

xlabel('Conference Size', 'FontSize', 12);
ylabel('Time [msec]', 'FontSize', 12);
title('');

box on;
grid on;

%% time
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