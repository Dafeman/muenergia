function [Device_7cd, Device_7ef, Device_8d4, range_7cd, range_7ef, range_8d4] ...
         = get_device_data(data_dir)
    d7cd = strcat(data_dir, '/', '7cd');
    d7ef = strcat(data_dir, '/', '7ef');
    d8d4 = strcat(data_dir, '/', '8d4');

    Device_7cd = load(d7cd);
    Device_7ef = load(d7ef);
    Device_8d4 = load(d8d4);

    % 
    Device_7cd(:,2) = Device_7cd(:,2) / 100; % mbar
    Device_7ef(:,2) = Device_7ef(:,2) / 100; % mbar
    Device_8d4(:,2) = Device_8d4(:,2) / 100; % mbar

    range_7cd = (1:size(Device_7cd, 1))';
    range_7ef = (1:size(Device_7ef, 1))';
    range_8d4 = (1:size(Device_8d4, 1))';
    
    
    size(Device_7cd);
    size(Device_7ef);
    size(Device_8d4);
    
    size(range_7cd);
    size(range_7ef);
    size(range_8d4);
end