% Pressure calibration

clear; close all; clc;

% spec
data_dir_1h = {'03_26_2015_1f1h', '03_26_2015_2f1h', ...
               '03_26_2015_3f1h', '03_26_2015_4f1h', ...
               '03_26_2015_5f1h'};
        
data_dir_2h = {'03_26_2015_1f2h', '03_26_2015_2f2h', ...
               '03_26_2015_3f2h', '03_26_2015_4f2h', ...
               '03_26_2015_5f2h'};
           
meta_floor = {'Floor #1', 'Floor #2', 'Floor #3', 'Floor #4', 'Floor #5'};           
        
subplot_index = 1;

mean_pre_1h = [];
mean_alt_1h = [];

mean_pre_2h = [];
mean_alt_2h = [];
   
figure;
hold on;

% pressure
for dir_idx = 1 : length(data_dir_1h)    
    [Device_7cd, Device_7ef, Device_8d4, range_7cd, range_7ef, range_8d4] ...
         = get_device_data(data_dir_1h{dir_idx});
    subplot(4, 5, subplot_index);
    hold on;
    plot(range_7cd, Device_7cd(:, 5));
    plot(range_7ef, Device_7ef(:, 5));
    plot(range_8d4, Device_8d4(:, 5));
    
    
    fprintf('p: %d \n', dir_idx)
    mean(Device_7cd(:,2))
    mean(Device_7ef(:,2))
    mean(Device_8d4(:,2))
    
    std(Device_7cd(:,2))
    std(Device_7ef(:,2))
    std(Device_8d4(:,2))
    
               
    title(meta_floor{dir_idx});
    hold off;
    subplot_index = subplot_index + 1;
end

fprintf('=======\n')
 
% Alt
for dir_idx = 1 : length(data_dir_1h)    
    [Device_7cd, Device_7ef, Device_8d4, range_7cd, range_7ef, range_8d4] ...
         = get_device_data(data_dir_1h{dir_idx});
    subplot(4, 5, subplot_index);
    hold on;
    plot(range_7cd, Device_7cd(:, 3));
    plot(range_7ef, Device_7ef(:, 3));
    plot(range_8d4, Device_8d4(:, 3));
    
    fprintf('a: %d \n', dir_idx)
    ma = mean(Device_7cd(:,3))
    mb = mean(Device_7ef(:,3))
    mc = mean(Device_8d4(:,3))
    
    std(Device_7cd(:,3))
    std(Device_7ef(:,3))
    std(Device_8d4(:,3))
    
    mean_alt_1h = [mean_alt_1h, [ma; mb; mc]];
    
    ylim([-20, 20]);
    hold off;
    subplot_index = subplot_index + 1;
end

fprintf('=======\n')


%%%%

% pressure
for dir_idx = 1 : length(data_dir_2h)    
    [Device_7cd, Device_7ef, Device_8d4, range_7cd, range_7ef, range_8d4] ...
         = get_device_data(data_dir_2h{dir_idx});
    subplot(4, 5, subplot_index);
    hold on;
    plot(range_7cd, Device_7cd(:, 5));
    plot(range_7ef, Device_7ef(:, 5));
    plot(range_8d4, Device_8d4(:, 5));
    
    fprintf('p: %d \n', dir_idx)
    mean(Device_7cd(:,2))
    mean(Device_7ef(:,2))
    mean(Device_8d4(:,2))
    
    std(Device_7cd(:,2))
    std(Device_7ef(:,2))
    std(Device_8d4(:,2))
    
    
    
    hold off;
    subplot_index = subplot_index + 1;
end

fprintf('++++++++\n')
 
% Alt
for dir_idx = 1 : length(data_dir_2h)    
    [Device_7cd, Device_7ef, Device_8d4, range_7cd, range_7ef, range_8d4] ...
         = get_device_data(data_dir_2h{dir_idx});
    subplot(4, 5, subplot_index);
    hold on;
    plot(range_7cd, Device_7cd(:, 3));
    plot(range_7ef, Device_7ef(:, 3));
    plot(range_8d4, Device_8d4(:, 3));
    
    fprintf('a: %d \n', dir_idx)
    
    ma = mean(Device_7cd(:,3))
    mb = mean(Device_7ef(:,3))
    mc = mean(Device_8d4(:,3))
    
    std(Device_7cd(:,3))
    std(Device_7ef(:,3))
    std(Device_8d4(:,3))
    
    mean_alt_2h = [mean_alt_2h, [ma; mb; mc]];
    
    
    
    ylim([-20, 20]);
    hold off;
    subplot_index = subplot_index + 1;
end

fprintf('=======\n')

legend('7cd', '7ef' ,'8d4', 'Location', 'southeast')

size(mean_alt_1h)
size(mean_alt_2h)

mean_alt2_1h = zeros(3, 4);
for i = 1 : 3
    for j = 2 : 5
       mean_alt2_1h(i, j-1) = mean_alt_1h(i,j) - mean_alt_1h(i,j-1); 
    end
end

mean_alt2_2h = zeros(3, 4);
for i = 1 : 3
    for j = 2 : 5
       mean_alt2_2h(i, j-1) = mean_alt_2h(i,j) - mean_alt_2h(i,j-1); 
    end
end

color_s = {'-r', '-g', '-b'}

figure;
hold on;
for i = 1 : 3
   %plot(1:5, mean_alt_1h(i,:)); 
   %bar(mean_alt_1h(i,:));
   plot(1:4, mean_alt2_1h(i,:), color_s{i});
end
set(gca,'XTick',[1:5]);
hold off;

figure;
hold on;
for i = 1 : 3
   %plot(1:5, mean_alt_2h(i,:)); 
   %bar(mean_alt_2h(i,:));
   plot(1:4, mean_alt2_2h(i,:), color_s{i});
end
set(gca,'XTick',[1:5]);
hold off;


hold off;