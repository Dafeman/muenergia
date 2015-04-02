% Pressure calibration

clear; close all; clc;

% spec
data_dir_1h = {'03_26_2015_1f1h', '03_26_2015_2f1h', ...
               '03_26_2015_3f1h', '03_26_2015_4f1h', ...
               '03_26_2015_5f1h'};
        
data_dir_2h = {'03_26_2015_1f2h', '03_26_2015_2f2h', ...
               '03_26_2015_3f2h', '03_26_2015_4f2h', ...
               '03_26_2015_5f2h'};
        
subplot_index = 1;
   
figure;
hold on;

% pressure
for dir_idx = 1 : length(data_dir_1h)    
    [Device_7cd, Device_7ef, Device_8d4, range_7cd, range_7ef, range_8d4] ...
         = get_device_data(data_dir_1h{dir_idx});
    subplot(4, 5, subplot_index);
    hold on;
    plot(range_7cd, Device_7cd(:, 2));
    plot(range_7ef, Device_7ef(:, 2));
    plot(range_8d4, Device_8d4(:, 2));
    hold off;
    subplot_index = subplot_index + 1;
end
 
% Alt
for dir_idx = 1 : length(data_dir_1h)    
    [Device_7cd, Device_7ef, Device_8d4, range_7cd, range_7ef, range_8d4] ...
         = get_device_data(data_dir_1h{dir_idx});
    subplot(4, 5, subplot_index);
    hold on;
    plot(range_7cd, Device_7cd(:, 3));
    plot(range_7ef, Device_7ef(:, 3));
    plot(range_8d4, Device_8d4(:, 3));
    ylim([-20, 20]);
    hold off;
    subplot_index = subplot_index + 1;
end

legend('7cd', '7ef' ,'8d4', 'Location', 'southeast')

%%%%

% pressure
for dir_idx = 1 : length(data_dir_2h)    
    [Device_7cd, Device_7ef, Device_8d4, range_7cd, range_7ef, range_8d4] ...
         = get_device_data(data_dir_2h{dir_idx});
    subplot(4, 5, subplot_index);
    hold on;
    plot(range_7cd, Device_7cd(:, 2));
    plot(range_7ef, Device_7ef(:, 2));
    plot(range_8d4, Device_8d4(:, 2));
    hold off;
    subplot_index = subplot_index + 1;
end
 
% Alt
for dir_idx = 1 : length(data_dir_2h)    
    [Device_7cd, Device_7ef, Device_8d4, range_7cd, range_7ef, range_8d4] ...
         = get_device_data(data_dir_2h{dir_idx});
    subplot(4, 5, subplot_index);
    hold on;
    plot(range_7cd, Device_7cd(:, 3));
    plot(range_7ef, Device_7ef(:, 3));
    plot(range_8d4, Device_8d4(:, 3));
    ylim([-20, 20]);
    hold off;
    subplot_index = subplot_index + 1;
end


hold off;