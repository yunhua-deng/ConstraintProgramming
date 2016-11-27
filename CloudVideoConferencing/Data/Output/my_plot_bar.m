%%
ss = get(0, 'ScreenSize');
set(gcf, 'Position', [ss(1) ss(2) ss(3)/1 ss(4)/3]);
pos = 0;

% latency
pos = pos + 1;
subplot(1, 3, pos);
data_4 = csvread(sprintf('sessionSize[%d]_latency.csv', 4));
data_8 = csvread(sprintf('sessionSize[%d]_latency.csv', 8));
data_12 = csvread(sprintf('sessionSize[%d]_latency.csv', 12));
data_16 = csvread(sprintf('sessionSize[%d]_latency.csv', 16));
data = [
    data_4;
    data_8;
    data_12;
    data_16
    ];

bh = bar(data);
bh(1).FaceColor = rgb('LightGreen');
bh(2).FaceColor = rgb('LightBlue');
bh(3).FaceColor = rgb('Khaki');
bh(4).FaceColor = rgb('SandyBrown');
bh(5).FaceColor = rgb('MediumPurple');

set(gca, 'XTickLabel', [4 8 12 16]);
set(gca, 'YLim', [100 240]);
set(gca, 'fontsize', 10);

lh = legend('SD', 'NA', 'CP', 'CP-L', 'CP-G', 'Orientation', 'vertical', 'Location', 'northwest');
%set(lh, 'FontSize', 12);

xlabel('Conference size', 'FontSize', 12);
ylabel('Latency of longest path [msec]', 'FontSize', 12);
title('');

box on;
grid on;

% cost
pos = pos + 1;
subplot(1, 3, pos);
data_4 = csvread(sprintf('sessionSize[%d]_cost.csv', 4));
data_8 = csvread(sprintf('sessionSize[%d]_cost.csv', 8));
data_12 = csvread(sprintf('sessionSize[%d]_cost.csv', 12));
data_16 = csvread(sprintf('sessionSize[%d]_cost.csv', 16));
data = [
    data_4;
    data_8;
    data_12;
    data_16
    ];

bh = bar(data);
bh(1).FaceColor = rgb('LightGreen');
bh(2).FaceColor = rgb('LightBlue');
bh(3).FaceColor = rgb('Khaki');
bh(4).FaceColor = rgb('SandyBrown');
bh(5).FaceColor = rgb('MediumPurple');

set(gca, 'XLim', [0.5 4.5]);
set(gca, 'XTick', [1 2 3 4]);
set(gca, 'XTickLabel', [4 8 12 16]);
set(gca, 'fontsize', 10);

lh = legend('SD', 'NA', 'CP', 'CP-L', 'CP-G', 'Orientation', 'vertical', 'Location', 'northwest');
%set(lh, 'FontSize', 12);

xlabel('Conference size', 'FontSize', 12);
ylabel('Traffic cost [dollar]', 'FontSize', 12);
title('');

box on;
grid on;

% time
pos = pos + 1;
subplot(1, 3, pos);
data_4 = csvread(sprintf('sessionSize[%d]_time.csv', 4));
data_8 = csvread(sprintf('sessionSize[%d]_time.csv', 8));
data_12 = csvread(sprintf('sessionSize[%d]_time.csv', 12));
data_16 = csvread(sprintf('sessionSize[%d]_time.csv', 16));
data = [
    data_4./1000;
    data_8./1000;
    data_12./1000;
    data_16./1000
    ];
bh = bar(data);
bh(1).FaceColor = rgb('Khaki');
bh(2).FaceColor = rgb('SandyBrown');
bh(3).FaceColor = rgb('MediumPurple');

set(gca, 'yscale',  'log');
set(gca, 'XTickLabel', [4 8 12 16]);
set(gca, 'fontsize', 10);

lh = legend('CP', 'CP-L', 'CP-G', 'Orientation', 'vertical', 'Location', 'northwest');
%set(lh, 'FontSize', 12);

xlabel('Conference size', 'FontSize', 12);
ylabel('Max. comp. time [sec]', 'FontSize', 12);
title('');

box on;
grid on;

export_fig OptimizingLatencyFirst.pdf -transparent