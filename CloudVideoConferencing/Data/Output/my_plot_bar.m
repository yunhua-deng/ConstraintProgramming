%%
ss = get(0, 'ScreenSize');
set(gcf, 'Position', [ss(1) ss(2) ss(3)/1.2 ss(4)/3]);
pos = 0;

% latency
pos = pos + 1;
subplot(1, 3, pos);
data_8 = csvread(sprintf('sessionSize[%d]_latencyMeasure.csv', 8));
data_12 = csvread(sprintf('sessionSize[%d]_latencyMeasure.csv', 12));
data_16 = csvread(sprintf('sessionSize[%d]_latencyMeasure.csv', 16));
data = [
    data_8;
    data_12;
    data_16
    ];
bh = bar(data);
bh(1).FaceColor = rgb('LightBlue');
bh(2).FaceColor = rgb('SandyBrown');

set(gca, 'XTickLabel', [8 12 16]);
set(gca, 'YLim', [150 230]);
set(gca, 'fontsize', 10);

lh = legend('NA', 'CP', 'Orientation', 'horizontal', 'Location', 'northwest');
set(lh, 'FontSize', 12);

xlabel('Session size (# of clients)', 'FontSize', 12);
ylabel('Average latency [ms]', 'FontSize', 12);
title('');

box on;
grid on;

% cost
pos = pos + 1;
subplot(1, 3, pos);
data_8 = csvread(sprintf('sessionSize[%d]_costMeasure.csv', 8));
data_12 = csvread(sprintf('sessionSize[%d]_costMeasure.csv', 12));
data_16 = csvread(sprintf('sessionSize[%d]_costMeasure.csv', 16));
data = [
    data_8;
    data_12;
    data_16
    ];
bh = bar(data);
bh(1).FaceColor = rgb('LightBlue');
bh(2).FaceColor = rgb('SandyBrown');

set(gca, 'XTickLabel', [8 12 16]);
set(gca, 'fontsize', 10);

lh = legend('NA', 'CP', 'Orientation', 'horizontal', 'Location', 'northwest');
set(lh, 'FontSize', 12);

xlabel('Session size (# of clients)', 'FontSize', 12);
ylabel('Average cost [USD]', 'FontSize', 12);
title('');

box on;
grid on;

% time
pos = pos + 1;
subplot(1, 3, pos);
data_8 = csvread(sprintf('sessionSize[%d]_computationalTime.csv', 8));
data_12 = csvread(sprintf('sessionSize[%d]_computationalTime.csv', 12));
data_16 = csvread(sprintf('sessionSize[%d]_computationalTime.csv', 16));
data = [
    data_8./1000;
    data_12./1000;
    data_16./1000
    ];
bh = bar(data);
bh(1).FaceColor = rgb('SandyBrown');

set(gca, 'yscale',  'log');
set(gca, 'XTickLabel', [8 12 16]);
set(gca, 'fontsize', 10);

xlabel('Session size (# of clients)', 'FontSize', 12);
ylabel('Average comp. time of CP [s]', 'FontSize', 12);
title('');

box on;
grid on;

export_fig OptimizingLatencyFirst.pdf -transparent