% Sensor journal data visualization

clear; close all; clc;

% spec
data_dir = {'03_02_2015',  '03_23_2015', '03_25_2015', '03_30_2015'};
meta_dir = {'One', 'Five', 'Six', 'Seven'};
titles = {'BMP180 Tem', 'BMP180 Pre', 'BMP180 Alt', ...
              'ISL29023 Lig', 'SHT21 Hum', ...
              'SHT21 Tem', 'TMP006 Tem'};
print_order = zeros(length(titles), 1);
% temperature
print_order(1) = 1;
print_order(2) = 6;
print_order(3) = 7;
% Pressure
print_order(4) = 2;
print_order(5) = 3;
% Humidity
print_order(6) = 5;
% Light
print_order(7) = 4;

        
assert(length(data_dir) == length(meta_dir), ...
       'data_dir and meta_dir mismatch');        

subplot_rows = length(data_dir);
subplot_cols = length(print_order);
subplot_index = 1;
   
figure;
hold on;
for dir_idx = 1 : length(data_dir)    
    d7cd = strcat(data_dir{dir_idx}, '/', '7cd');
    d7ef = strcat(data_dir{dir_idx}, '/', '7ef');
    d8d4 = strcat(data_dir{dir_idx}, '/', '8d4');

    Device_7cd = load(d7cd);
    Device_7ed = load(d7ef);
    Device_8d4 = load(d8d4);

    % 
    Device_7cd(:,2) = Device_7cd(:,2) / 100; % mbar
    Device_7ed(:,2) = Device_7ed(:,2) / 100; % mbar
    Device_8d4(:,2) = Device_8d4(:,2) / 100; % mbar

    range_7cd = (1:size(Device_7cd, 1))';
    range_7ed = (1:size(Device_7ed, 1))';
    range_8d4 = (1:size(Device_8d4, 1))';
    
    if dir_idx == 1,
       Device_7cd_tmp = Device_7cd;
       Device_7ed_tmp = Device_7ed;
       Device_8d4_tmp = Device_8d4;
       
       range_7cd_tmp = range_7cd;
       range_7ed_tmp = range_7ed;
       range_8d4_tmp = range_8d4;
       
       
       Device_7cd = [];
       Device_7ed = [];
       Device_8d4 = [];
       
       range_7cd = [];
       range_7ed = [];
       range_8d4 = [];
       
             
       offset = 14;
       
       for j = 1 : size(Device_7cd_tmp, 2)
           column_values = [];
           for i = 1 : offset : size(Device_7cd_tmp, 1)
                sta_i = i;
                end_i = min(size(Device_7cd_tmp, 1), i + offset - 1);
                mean_value = mean(Device_7cd_tmp(sta_i:end_i,j));
                column_values = [column_values; mean_value(:)];
           end
           Device_7cd = [Device_7cd, column_values];
       end
       
       for j = 1 : size(Device_7ed_tmp, 2)
           column_values = [];
           for i = 1 : offset : size(Device_7ed_tmp, 1)
                sta_i = i;
                end_i = min(size(Device_7ed_tmp, 1), i + offset - 1);
                mean_value = mean(Device_7ed_tmp(sta_i:end_i,j));
                column_values = [column_values; mean_value(:)];
           end
           Device_7ed = [Device_7ed, column_values];
       end
       
       for j = 1 : size(Device_8d4_tmp, 2)
           column_values = [];
           for i = 1 : offset : size(Device_8d4_tmp, 1)
                sta_i = i;
                end_i = min(size(Device_8d4_tmp, 1), i + offset - 1);
                mean_value = mean(Device_8d4_tmp(sta_i:end_i,j));
                column_values = [column_values; mean_value(:)];
           end
           Device_8d4 = [Device_8d4, column_values];
       end
       
       
        range_7cd = (1:size(Device_7cd, 1))';
        range_7ed = (1:size(Device_7ed, 1))';
        range_8d4 = (1:size(Device_8d4, 1))';   
       
       
    end
    
    size(Device_7cd)
    size(Device_7ed)
    size(Device_8d4)
    
    size(range_7cd)
    size(range_7ed)
    size(range_8d4)
    
    
    for j = 1 : length(print_order)
        subplot(subplot_rows, subplot_cols, subplot_index);
        print_index = print_order(j);
        hold on;
        plot(range_7cd, Device_7cd(:, print_index));
        plot(range_7ed, Device_7ed(:, print_index));
        plot(range_8d4, Device_8d4(:, print_index));
        if dir_idx == 1,
            title(titles{print_index});
        end
        if j == 1 && subplot_index == 1,
            legend('Door', 'Back' ,'Roof', 'Location', 'southeast');
        end
        hold off;
        subplot_index = subplot_index + 1;
    end
 end
hold off;
%suptitle([meta_dir{dir_idx}]);