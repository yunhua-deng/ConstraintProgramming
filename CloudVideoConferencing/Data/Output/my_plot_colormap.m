%% dc_dc_rtt_matrix
ss = get(0, 'ScreenSize');
set(gcf, 'Position', [ss(1) ss(2) ss(3)/4 ss(4)/3]);

% region_name_list = {
%     'ec2-ap-northeast-1'
%     'ec2-ap-northeast-2'
%     'ec2-ap-south-1'
%     'ec2-ap-southeast-1'
%     'ec2-ap-southeast-2'
%     'ec2-eu-central-1'
%     'ec2-eu-west-1'
%     'ec2-sa-east-1'
%     'ec2-us-east-1'
%     'ec2-us-west-1'
%     'ec2-us-west-2'
% };

region_name_list = {
    'Tokyo' %'ec2-ap-northeast-1'
    'Seoul' %'ec2-ap-northeast-2'
    'Mumbai' %'ec2-ap-south-1'
    'Singapore' %'ec2-ap-southeast-1'
    'Sydney' %'ec2-ap-southeast-2'
    'Frankfurt' %'ec2-eu-central-1'
    'Ireland' %'ec2-eu-west-1'
    'Sao Paulo' %'ec2-sa-east-1'
    'US.Virginia' %'ec2-us-east-1'
    'US.California' %'ec2-us-west-1'
    'US.Oregon' %'ec2-us-west-2'
};

rtt_table = importdata('ping_to_dc_median_matrix.csv');

imagesc((rtt_table.data)./2); % one-way latency
c_h = colorbar;
set(c_h, 'YTick', [0 45 90 135 180]);
colormap(flipud(gray(4)));
%colormap('hot');

set(gca, 'fontsize', 10);
set(gca,'XTick',1:11);
set(gca,'YTick',1:11);
set(gca, 'XTickLabel', region_name_list, 'XTickLabelRotation', 45);
set(gca, 'YTickLabel', region_name_list, 'YTickLabelRotation', 0);
xlabel('', 'FontSize', 14);
ylabel('', 'FontSize', 14);
title('Pairwise latencies of EC2 datacenters [msec]');

export_fig dc_to_dc_latency.pdf -transparent

%% farthestClientPair
ss = get(0, 'ScreenSize');
set(gcf, 'Position', [ss(1) ss(2) ss(3) ss(4)/4]);

conf_size_list = zeros(1, 5);
conf_size_list(1) = 8;
for i = 2:length(conf_size_list)
    conf_size_list(i) = conf_size_list(i - 1) + 2;
end

region_name_list = {
    'Tokyo' %'ec2-ap-northeast-1'
    'Seoul' %'ec2-ap-northeast-2'
    'Mumbai' %'ec2-ap-south-1'
    'Singapore' %'ec2-ap-southeast-1'
    'Sydney' %'ec2-ap-southeast-2'
    'Frankfurt' %'ec2-eu-central-1'
    'Ireland' %'ec2-eu-west-1'
    'SaoPaulo' %'ec2-sa-east-1'
    'Virginia' %'ec2-us-east-1'
    'California' %'ec2-us-west-1'
    'Oregon' %'ec2-us-west-2'
};

pos = 0;
for i = 1:length(conf_size_list)
    pos = pos + 1;
    subplot(1, length(conf_size_list), pos);
    
    data_all = importdata(sprintf('%d_allClientPair_location_dist.csv', conf_size_list(i)));
    data_alg = importdata(sprintf('%d_farthestClientPair_location_dist_CP.csv', conf_size_list(i)));
    data = data_alg.data./data_all.data;
        
    imagesc(data);
    colorbar;
    colormap(flipud(gray(10)));
    %colormap('hot');
    
    set(gca, 'fontsize', 10);
    set(gca,'XTick',1:11);
    set(gca,'YTick',1:11);
    set(gca, 'XTickLabel', region_name_list, 'XTickLabelRotation', 45);
    set(gca, 'YTickLabel', region_name_list, 'YTickLabelRotation', 0);   
    title(sprintf('Conference size: %d', conf_size_list(i)));
end
export_fig FarthestClientPair_CP.pdf -transparent