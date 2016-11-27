%% CDF_DelayToNearestDC
ss = get(0, 'ScreenSize');
set(gcf, 'Position', [ss(1) ss(2) ss(3)/4 ss(4)/3]);
%data = importdata('CDF_DelayToNearestDC.txt'); % importdata() is memory-intensive
data = dlmread('CDF_DelayToNearestDC.txt');
p_h = cdfplot(data);
set(gca, 'fontsize', 10);
set(p_h, 'LineStyle', '-', 'LineWidth', 2, 'Color', 'b');
xlabel('Latency between the prefix and its nearest datacenter [ms]', 'FontSize', 12);
ylabel('CDF of IP address prefixes', 'FontSize', 12);
title('');
grid on;
export_fig CDF_DelayToNearestDC.pdf -transparent

%% CDF_ShortestPathLength
ss = get(0, 'ScreenSize');
set(gcf, 'Position', [ss(1) ss(2) ss(3)/4 ss(4)/3]);
data = dlmread('CDF_ShortestPathLength.txt');
p_h = cdfplot(data);
set(gca, 'fontsize', 10);
set(p_h, 'LineStyle', '-', 'LineWidth', 2, 'Color', 'b');
xlabel('Latency (one-way) of the shortest path [ms]', 'FontSize', 12);
ylabel('CDF of shortest paths', 'FontSize', 12);
title('');
grid on;
export_fig CDF_ShortestPathLength.pdf -transparent
pdf -transparent

%% CDF_Cardinality
ss = get(0, 'ScreenSize');
set(gcf, 'Position', [ss(1) ss(2) ss(3) ss(4)/3]);
pos = 0;
for size = [4 8 12 16]
    pos = pos + 1;
    subplot(1, 4, pos);
    
    data = dlmread(sprintf('sessionSize[%d]_cardinality_CDF.csv', size));
    
    p_h = cdfplot(data(1, :));    
    set(p_h, 'LineStyle', '-', 'LineWidth', 2, 'Color', 'm');
    hold on;
    p_h = cdfplot(data(2, :));
    set(p_h, 'LineStyle', '-.', 'LineWidth', 2, 'Color', 'b');
%     hold on;
%     p_h = cdfplot(data(3, :));
%     set(p_h, 'LineStyle', '-.', 'LineWidth', 2, 'Color', 'r');
%     hold on;
%     p_h = cdfplot(data(4, :));
%     set(p_h, 'LineStyle', '-.', 'LineWidth', 2, 'Color', 'k');
    
    lh = legend('NA', 'CP', 'Orientation', 'vertical', 'Location', 'southeast');
    set(lh, 'FontSize', 12);
    
    xlabel('Number of unique datacenters selected', 'FontSize', 12);
    ylabel(sprintf('CDF of conferences of size %d', size), 'FontSize', 12);
    title('');
    
    set(gca, 'fontsize', 10);
    set(gca, 'XLim', [1 11]);
    set(gca, 'XTick', [1 2 3 4 5 6 7 8 9 10 11]);
    
    box on;
    grid on;  
    hold off;
end
export_fig CDF_Cardinality.pdf -transparent

%% CDF_Ranking
ss = get(0, 'ScreenSize');
set(gcf, 'Position', [ss(1) ss(2) ss(3) ss(4)/3]);
pos = 0;
for size = [4 8 12 16]
    pos = pos + 1;
    subplot(1, 4, pos);    
    
    data = dlmread(sprintf('sessionSize[%d]_ranking_CDF.csv', size));
    
     p_h = cdfplot(data(1, :));    
    set(p_h, 'LineStyle', '-', 'LineWidth', 2, 'Color', 'm');
    hold on;
    p_h = cdfplot(data(2, :));
    set(p_h, 'LineStyle', '-.', 'LineWidth', 2, 'Color', 'b');
%     hold on;
%     p_h = cdfplot(data(3, :));
%     set(p_h, 'LineStyle', '-.', 'LineWidth', 2, 'Color', 'r');
%     hold on;
%     p_h = cdfplot(data(4, :));
%     set(p_h, 'LineStyle', '-.', 'LineWidth', 2, 'Color', 'k');
    
    lh = legend('SD', 'CP', 'Orientation', 'vertical', 'Location', 'southeast');
    set(lh, 'FontSize', 12);
    
    xlabel('Ranking of its assigned datacenter', 'FontSize', 12);
    ylabel(sprintf('CDF of clients in conferences of size %d', size), 'FontSize', 12);
    title('');
    
    set(gca, 'fontsize', 10);
    set(gca, 'XLim', [1 11]);
    set(gca, 'XTick', [1 2 3 4 5 6 7 8 9 10 11]);
    
    box on;
    grid on;   
end
export_fig CDF_Ranking.pdf -transparent