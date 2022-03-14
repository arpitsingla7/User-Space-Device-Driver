////////////////////////////////////////////////////////////////////////////////
//
//  File           : lcloud_filesys.c
//  Description    : This is the implementation of the Lion Cloud device 
//                   filesystem interfaces.
//
//   Author        : *** INSERT YOUR NAME ***
//   Last Modified : *** DATE ***
//

// Include files
#include <stdlib.h>
#include <string.h>
#include <cmpsc311_log.h>

// Project include files
#include <lcloud_filesys.h>
#include <lcloud_controller.h>
#include <lcloud_cache.h>

//
// File system interface implementation

LCloudRegisterFrame create_lcloud_registers(uint64_t B0, uint64_t B1, uint64_t C0, uint64_t C1, uint64_t C2, uint64_t D0, uint64_t D1)
{
    //unint64_t T_pack; 
    LCloudRegisterFrame T_pack; 

    D0 = D0 << 16;
    C2 = C2 << 32;
    C1 = C1 << 40;
    C0 = C0 << 48;
    B1 = B1 << 56;
    B0 = B0 << 60;

    T_pack = B0 | B1 | C0 | C1 | C2 | D0 | D1;      //oring all the registers to create one big registers 
    return T_pack; 
}

void extract_lcloud_registers(LCloudRegisterFrame T_pack,uint64_t *B0,uint64_t *B1,uint64_t *C0,uint64_t *C1,uint64_t *C2,uint64_t *D0,uint64_t *D1)
{ 

     LCloudRegisterFrame New_pack = T_pack;

     uint64_t temp_B0 = New_pack >> 60; //B0
     *B0 = temp_B0;   

     uint64_t temp_B1a = (temp_B0<<60) ^ (New_pack);
     *B1 = (temp_B1a >> 56); //B1
     
     uint64_t temp_C0a = (New_pack >> 56);
     uint64_t temp_C0b = (temp_C0a << 56) ^ (New_pack);
     *C0 = temp_C0b >> 48;  //C0

     uint64_t temp_C1a = (New_pack >> 48);
     uint64_t temp_C1b = (temp_C1a << 48) ^ (New_pack);
     *C1 = temp_C1b >> 40;  //C1

     uint64_t temp_C2a = (New_pack >> 40);
     uint64_t temp_C2b = (temp_C2a << 40) ^ (New_pack);
     *C2 = temp_C2b >> 32; //C2

     uint64_t temp_D0a = (New_pack >> 32);
     uint64_t temp_D0b = (temp_D0a << 32) ^ (New_pack);
     *D0 = temp_D0b >> 16;  //D0

     uint64_t temp_D1a = (New_pack >> 16);
     *D1 = (temp_D1a << 16) ^ (New_pack); //D1  
}

//uint64_t B0, B1, C0, C1, C2, D0, D1; 
uint64_t *B0, *B1, *C0, *C1, *C2, *D0, *D1; 
int power_status = 0;
int file_counter = 1;
int device_arr[16];
int pos = 0;
uint64_t tempand = 1;
int j = -1;

struct      //A struct to store the info about each device's total sectors and total blocks. 
{
    int tot_sec;
    int tot_blk;

}My_device[16]; 

struct device_info      //This struct stores information for each file such as in which sector, block and device the file has been wrote. 
{
    int secs_filled[1024];
    int blks_filled[1024];
    int devices[1024];
    int blk_tracker;

};

struct
{
    char file_name[300];
    int file_fh;
    int file_pointer;
    int file_open;
    int file_len;
    int last_sec;
    int last_blk;
    int remain_space;
    int device_id;
    struct device_info file_deviceinfo;

}file_info[1024]; 

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcopen
// Description  : Open the file for for reading and writing
//
// Inputs       : path - the path/filename of the file to be read
// Outputs      : file handle if successful test, -1 if failure

LcFHandle lcopen( const char *path ) {
    j = j +1;
    if(power_status == 0)
    {
        LCloudRegisterFrame register_pack =  create_lcloud_registers(0,0, LC_POWER_ON, 0, 0, 0, 0);
        LCloudRegisterFrame register_bus = client_lcloud_bus_request(register_pack, NULL);
        extract_lcloud_registers(register_bus, &B0, &B1, &C0, &C1, &C2, &D0, &D1);
        lcloud_initcache(LC_CACHE_MAXBLOCKS);
        power_status = 1;

        LCloudRegisterFrame register_Dpack =  create_lcloud_registers(0,0, LC_DEVPROBE, 0, 0, 0, 0);
        LCloudRegisterFrame register_Dbus = client_lcloud_bus_request(register_Dpack, NULL);
        extract_lcloud_registers(register_Dbus, &B0, &B1, &C0, &C1, &C2, &D0, &D1);
        
        for (int i = 0; i < 16; i++)
        {
            uint64_t T0 = D0;
            T0 = T0 & tempand; 
            T0 = T0>>i;
            tempand = tempand<<1; 

            if (T0 == 1)
            {
                device_arr[pos] = i; 
                pos = pos+1;   
            }
            else
            {
                continue;
            }
        }

        for(int times = 0; times < pos; times++)
        {
            LCloudRegisterFrame register_init =  create_lcloud_registers(0,0, LC_DEVINIT, device_arr[times], 0, 0, 0);
            LCloudRegisterFrame bus_init = client_lcloud_bus_request(register_init, NULL);
            extract_lcloud_registers(bus_init, &B0, &B1, &C0, &C1, &C2, &D0, &D1);    
            My_device[device_arr[times]].tot_sec = D0; 
            My_device[device_arr[times]].tot_blk = D1;
        }
    }
    else
    {
        power_status = 1;
    }

    if (2==1)
    {
        return -1;
    }
    else
    {   
        for (int i = j; i <file_counter ; i++)
        {   
            if (file_info[i].file_open==1)
            {  
                return '-1';
            }
            else
            {
                if(strcmp(file_info[i].file_name, path)==0)   
                {
                    file_info[i].file_open = 1;
                    file_info[i].file_pointer = 0;
                    
                    return file_info[i].file_fh;   
                }
                else
                {   strcpy(file_info[i].file_name,path);
                    file_info[i].file_open = 1;
                    file_info[i].file_len = 0;
                    file_info[i].file_pointer = 0;
                    file_info[i].file_fh = i;
                    file_counter++;  
                    
                    return file_info[i].file_fh;
                }
            } 
        }
    }
} 

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcread
// Description  : Read data from the file 
//
// Inputs       : fh - file handle for the file to read from
//                buf - place to put the data
//                len - the length of the read
// Outputs      : number of bytes read, -1 if failure

int sec = 0;
int blk = 0;
int device_no = 0;
int next_blk_secpos;    

int lcread( LcFHandle fh, char *buf, size_t len ) 
{
    int file_fh_pos = fh;
    
    if (file_info[file_fh_pos].file_fh == fh)            //check if file handle is not bad.
    {   
        if (file_info[file_fh_pos].file_open !=1)
    {
        return (-1);
    }
      else{
            if (file_info[file_fh_pos].file_pointer == file_info[file_fh_pos].file_len)   //if they are at the same position then their is nothing to overwrite.
            {
                return (0);
            }
            else
            {
                int sec_no = My_device[device_arr[device_no]].tot_sec;
                int blk_no = My_device[device_arr[device_no]].tot_blk;  

                if (file_info[file_fh_pos].file_pointer != file_info[file_fh_pos].file_len)   //if they are same, then their is nothing to read.
                {  
                    char read_buf[256];
                    char *cache_buf;
                    int temp_blks = file_info[file_fh_pos].file_pointer/256;
                    blk = file_info[file_fh_pos].file_deviceinfo.blks_filled[temp_blks];
                    sec = file_info[file_fh_pos].file_deviceinfo.secs_filled[temp_blks];
                    device_arr[device_no] = file_info[file_fh_pos].file_deviceinfo.devices[temp_blks];

                    if ((file_info[file_fh_pos].file_len - file_info[file_fh_pos].file_pointer) <= len)   //if the length to be read is greater then the space btw file_len and file_pointer.
                    {  
                        if ((file_info[file_fh_pos].file_pointer%256)+ len <= 256)   //sub condition 1. 
                        {
                            cache_buf = lcloud_getcache(device_arr[device_no], sec, blk);
                            if (cache_buf == NULL)
                            {
                                logMessage(LOG_INFO_LEVEL, "LioCLoud Cache ***** MISS *****");
                                logMessage(LOG_INFO_LEVEL, "Started to read the elemenets in %d/%d/%d", device_arr[device_no], sec, blk);
                                LCloudRegisterFrame frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, device_arr[device_no], LC_XFER_READ, sec, blk );     //specifying the sectors, blocks and operation to be performed by the bus.
                                client_lcloud_bus_request(frm, read_buf);       //takes our operation to be performed by the device.
                            }
                            else
                            {
                                logMessage(LOG_INFO_LEVEL, "LioCLoud Cache ***** HIT *****");
                                logMessage(LOG_INFO_LEVEL, "Started to read the elemenets in %d/%d/%d", device_arr[device_no], sec, blk);   
                                memcpy(read_buf, cache_buf, 256);
                            }
                            
                            int n = 0;
                            for (int i = (file_info[file_fh_pos].file_pointer%256); i < (file_info[file_fh_pos].file_len - file_info[file_fh_pos].file_pointer)+ file_info[file_fh_pos].file_pointer%256; i++)    //assign the values to be read from the provided buffer.
                            {
                                buf[n] = read_buf[i];   
                                n++;
                            }
                           
                            len=file_info[file_fh_pos].file_len-file_info[file_fh_pos].file_pointer;
                            file_info[file_fh_pos].file_pointer = file_info[file_fh_pos].file_len;
                            return (len);     
                        }
                        else    //if (file_info.file_pointer%256)+ len > 256
                        {   
                            int total_len_remain = file_info[file_fh_pos].file_len - file_info[file_fh_pos].file_pointer;

                            if (total_len_remain <=256) //because the total length to be read is greater then the file_len itself. therefore we calculated the total len
                            {                           //to be equal to the file_pointer - file_len

                                if (total_len_remain + file_info[file_fh_pos].file_pointer%256 < 256)  //read just inside one block
                                {
                                    cache_buf = lcloud_getcache(device_arr[device_no], sec, blk);
                                    if (cache_buf == NULL)
                                    {
                                        logMessage(LOG_INFO_LEVEL, "LioCLoud Cache ***** MISS *****");
                                        logMessage(LOG_INFO_LEVEL, "Started to read the elemenets in %d/%d/%d", device_arr[device_no], sec, blk);
                                        LCloudRegisterFrame frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, device_arr[device_no], LC_XFER_READ, sec, blk );     //specifying the sectors, blocks and operation to be performed by the bus.
                                        client_lcloud_bus_request(frm, read_buf);       //takes our operation to be performed by the device.
                                    }
                                    else
                                    {
                                        logMessage(LOG_INFO_LEVEL, "LioCLoud Cache ***** HIT *****");
                                        logMessage(LOG_INFO_LEVEL, "Started to read the elemenets in %d/%d/%d", device_arr[device_no], sec, blk);   
                                        memcpy(read_buf, cache_buf, 256);
                                    }

                                    int n = 0;
                                    for (int i = file_info[file_fh_pos].file_pointer%256 ; i < total_len_remain + file_info[file_fh_pos].file_pointer%256; i++)
                                    {
                                        buf[n] = read_buf[i]; 
                                        n++;
                                    }
                                    file_info[file_fh_pos].file_pointer = file_info[file_fh_pos].file_len;
                                    len = total_len_remain;
                                    return (len);   
                                }
                                else        
                                {
                                    cache_buf = lcloud_getcache(device_arr[device_no], sec, blk);
                                    if (cache_buf == NULL)
                                    {
                                        logMessage(LOG_INFO_LEVEL, "LioCLoud Cache ***** MISS *****");
                                        logMessage(LOG_INFO_LEVEL, "Started to read the elemenets in %d/%d/%d", device_arr[device_no], sec, blk);
                                        LCloudRegisterFrame frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, device_arr[device_no], LC_XFER_READ, sec, blk );     //specifying the sectors, blocks and operation to be performed by the bus.
                                        client_lcloud_bus_request(frm, read_buf);       //takes our operation to be performed by the device.
                                    }
                                    else
                                    {
                                        logMessage(LOG_INFO_LEVEL, "LioCLoud Cache ***** HIT *****");
                                        logMessage(LOG_INFO_LEVEL, "Started to read the elemenets in %d/%d/%d", device_arr[device_no], sec, blk);   
                                        memcpy(read_buf, cache_buf, 256);
                                    }
                                    
                                    int n = 0;
                                    for (int i = file_info[file_fh_pos].file_pointer%256 ; i < 256; i++)
                                    {
                                        buf[n] = read_buf[i];   
                                        n++;
                                    }
                                    
                                    int local_remain_len = total_len_remain - (n);
                                    next_blk_secpos = (file_info[file_fh_pos].file_pointer/256) + 1;
                                    blk = file_info[file_fh_pos].file_deviceinfo.blks_filled[next_blk_secpos];
                                    sec = file_info[file_fh_pos].file_deviceinfo.secs_filled[next_blk_secpos];
                                    device_arr[device_no] = file_info[file_fh_pos].file_deviceinfo.devices[next_blk_secpos];

                                    cache_buf = lcloud_getcache(device_arr[device_no], sec, blk);
                                    if (cache_buf == NULL)
                                    {
                                        logMessage(LOG_INFO_LEVEL, "LioCLoud Cache ***** MISS *****");
                                        logMessage(LOG_INFO_LEVEL, "Started to read the elemenets in %d/%d/%d", device_arr[device_no], sec, blk);
                                        LCloudRegisterFrame frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, device_arr[device_no], LC_XFER_READ, sec, blk );     //specifying the sectors, blocks and operation to be performed by the bus.
                                        client_lcloud_bus_request(frm, read_buf);       //takes our operation to be performed by the device.
                                    }
                                    else
                                    {
                                        logMessage(LOG_INFO_LEVEL, "LioCLoud Cache ***** HIT *****");
                                        logMessage(LOG_INFO_LEVEL, "Started to read the elemenets in %d/%d/%d", device_arr[device_no], sec, blk);   
                                        memcpy(read_buf, cache_buf, 256);
                                    }

                                    for (int i = 0 ; i < local_remain_len; i++)
                                    {   
                                        buf[(n)+i] = read_buf[i]; 
                                    }

                                    file_info[file_fh_pos].file_pointer = file_info[file_fh_pos].file_len;
                                    len = total_len_remain;
                                    return (len);  
                                }
                            }
                            else   //total_len_remain > 256     //if read more then one block and till the end of the file
                            {   
                                cache_buf = lcloud_getcache(device_arr[device_no], sec, blk);
                                if (cache_buf == NULL)
                                {
                                    logMessage(LOG_INFO_LEVEL, "LioCLoud Cache ***** MISS *****");
                                    logMessage(LOG_INFO_LEVEL, "Started to read the elemenets in %d/%d/%d", device_arr[device_no], sec, blk);
                                    LCloudRegisterFrame frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, device_arr[device_no], LC_XFER_READ, sec, blk );     //specifying the sectors, blocks and operation to be performed by the bus.
                                    client_lcloud_bus_request(frm, read_buf);       //takes our operation to be performed by the device.
                                }
                                else
                                {
                                    logMessage(LOG_INFO_LEVEL, "LioCLoud Cache ***** HIT *****");
                                    logMessage(LOG_INFO_LEVEL, "Started to read the elemenets in %d/%d/%d", device_arr[device_no], sec, blk);   
                                    memcpy(read_buf, cache_buf, 256);
                                }
                                
                                int n = 0;
                                for (int i = file_info[file_fh_pos].file_pointer%256 ; i < 256; i++)
                                {   
                                    buf[n] = read_buf[i];   
                                    n++;
                                }
                               
                                next_blk_secpos = (file_info[file_fh_pos].file_pointer/256) + 1;
                                blk = file_info[file_fh_pos].file_deviceinfo.blks_filled[next_blk_secpos];
                                sec = file_info[file_fh_pos].file_deviceinfo.secs_filled[next_blk_secpos];
                                device_arr[device_no] = file_info[file_fh_pos].file_deviceinfo.devices[next_blk_secpos];

                                int local_remain_len = total_len_remain - (n);
                                int times = local_remain_len/256;

                                for (int i = 0; i < times; i++)
                                {   
                                    cache_buf = lcloud_getcache(device_arr[device_no], sec, blk);
                                    if (cache_buf == NULL)
                                    {
                                        logMessage(LOG_INFO_LEVEL, "LioCLoud Cache ***** MISS *****");
                                        logMessage(LOG_INFO_LEVEL, "Started to read the elemenets in %d/%d/%d", device_arr[device_no], sec, blk);
                                        LCloudRegisterFrame frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, device_arr[device_no], LC_XFER_READ, sec, blk );     //specifying the sectors, blocks and operation to be performed by the bus.
                                        client_lcloud_bus_request(frm, read_buf);       //takes our operation to be performed by the device.
                                    }
                                    else
                                    {
                                        logMessage(LOG_INFO_LEVEL, "LioCLoud Cache ***** HIT *****");
                                        logMessage(LOG_INFO_LEVEL, "Started to read the elemenets in %d/%d/%d", device_arr[device_no], sec, blk);   
                                        memcpy(read_buf, cache_buf, 256);
                                    }
                                
                                    for (int j = 0; j < 256; j++)
                                    {  
                                        buf[(n)+j + i*256] = read_buf[j];       //note that, the given buffer is long as the given length
                                    }                                                                   //so, continuing assign the values from the position we left above. 
                                    next_blk_secpos++;
                                    blk = file_info[file_fh_pos].file_deviceinfo.blks_filled[next_blk_secpos];
                                    sec = file_info[file_fh_pos].file_deviceinfo.secs_filled[next_blk_secpos];
                                    device_arr[device_no] = file_info[file_fh_pos].file_deviceinfo.devices[next_blk_secpos]; 
                                }
                                
                                int end_len = total_len_remain - (n) - ((times)*256);

                                cache_buf = lcloud_getcache(device_arr[device_no], sec, blk);
                                if (cache_buf == NULL)
                                {
                                    logMessage(LOG_INFO_LEVEL, "LioCLoud Cache ***** MISS *****");
                                    logMessage(LOG_INFO_LEVEL, "Started to read the elemenets in %d/%d/%d", device_arr[device_no], sec, blk);
                                    LCloudRegisterFrame frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, device_arr[device_no], LC_XFER_READ, sec, blk );     //specifying the sectors, blocks and operation to be performed by the bus.
                                    client_lcloud_bus_request(frm, read_buf);       //takes our operation to be performed by the device.
                                }
                                else
                                {
                                    logMessage(LOG_INFO_LEVEL, "LioCLoud Cache ***** HIT *****");
                                    logMessage(LOG_INFO_LEVEL, "Started to read the elemenets in %d/%d/%d", device_arr[device_no], sec, blk);   
                                    memcpy(read_buf, cache_buf, 256);
                                }
                               
                                for (int i = 0; i < end_len; i++)
                                {
                                    buf[(n) + ((times)*256) + i] = read_buf[i];
                                }

                                file_info[file_fh_pos].file_pointer = file_info[file_fh_pos].file_len;
                                len = total_len_remain;
                                return (len); 
                            }
                        }
                    }   
                    else if (len < (file_info[file_fh_pos].file_len - file_info[file_fh_pos].file_pointer))   //when the number of bytes to be read are less then the space btw file_len and pointer. 
                    {  
                        if ((file_info[file_fh_pos].file_pointer%256)+ len <= 256)   
                        {  
                            cache_buf = lcloud_getcache(device_arr[device_no], sec, blk);
                            if (cache_buf == NULL)
                            {
                                logMessage(LOG_INFO_LEVEL, "LioCLoud Cache ***** MISS *****");
                                logMessage(LOG_INFO_LEVEL, "Started to read the elemenets in %d/%d/%d", device_arr[device_no], sec, blk);
                                LCloudRegisterFrame frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, device_arr[device_no], LC_XFER_READ, sec, blk );     //specifying the sectors, blocks and operation to be performed by the bus.
                                client_lcloud_bus_request(frm, read_buf);       //takes our operation to be performed by the device.
                            }
                            else
                            {
                                logMessage(LOG_INFO_LEVEL, "LioCLoud Cache ***** HIT *****");
                                logMessage(LOG_INFO_LEVEL, "Started to read the elemenets in %d/%d/%d", device_arr[device_no], sec, blk);   
                                memcpy(read_buf, cache_buf, 256);
                            }

                            int n = 0;
                            for (int i = (file_info[file_fh_pos].file_pointer%256); i < (file_info[file_fh_pos].file_pointer%256)+ len; i++)
                            {
                                buf[n] = read_buf[i];
                                n++;
                            }

                            file_info[file_fh_pos].file_pointer = file_info[file_fh_pos].file_pointer + len;  
                            return (len);
                        }
                        else    //if (file_info.file_pointer%256)+ len > 256    
                        {   
                            cache_buf = lcloud_getcache(device_arr[device_no], sec, blk);
                            if (cache_buf == NULL)
                            {
                                logMessage(LOG_INFO_LEVEL, "LioCLoud Cache ***** MISS *****");
                                logMessage(LOG_INFO_LEVEL, "Started to read the elemenets in %d/%d/%d", device_arr[device_no], sec, blk);
                                LCloudRegisterFrame frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, device_arr[device_no], LC_XFER_READ, sec, blk );     //specifying the sectors, blocks and operation to be performed by the bus.
                                client_lcloud_bus_request(frm, read_buf);       //takes our operation to be performed by the device.
                            }
                            else
                            {
                                logMessage(LOG_INFO_LEVEL, "LioCLoud Cache ***** HIT *****");
                                logMessage(LOG_INFO_LEVEL, "Started to read the elemenets in %d/%d/%d", device_arr[device_no], sec, blk);   
                                memcpy(read_buf, cache_buf, 256);
                            }

                            int n = 0;
                            for (int i = (file_info[file_fh_pos].file_pointer%256); i < 256; i++)
                            {
                                buf[n] = read_buf[i]; 
                                n++;
                            }

                            next_blk_secpos = (file_info[file_fh_pos].file_pointer/256) + 1;
                            blk = file_info[file_fh_pos].file_deviceinfo.blks_filled[next_blk_secpos];
                            sec = file_info[file_fh_pos].file_deviceinfo.secs_filled[next_blk_secpos];
                            device_arr[device_no] = file_info[file_fh_pos].file_deviceinfo.devices[next_blk_secpos];  
                            
                            int remainT_len = (len)-n;
                            int times = remainT_len/256;
                           
                            for (int i = 0; i < times; i++)
                            { 
                                cache_buf = lcloud_getcache(device_arr[device_no], sec, blk);
                                if (cache_buf == NULL)
                                {
                                    logMessage(LOG_INFO_LEVEL, "LioCLoud Cache ***** MISS *****");
                                    logMessage(LOG_INFO_LEVEL, "Started to read the elemenets in %d/%d/%d", device_arr[device_no], sec, blk);
                                    LCloudRegisterFrame frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, device_arr[device_no], LC_XFER_READ, sec, blk );     //specifying the sectors, blocks and operation to be performed by the bus.
                                    client_lcloud_bus_request(frm, read_buf);       //takes our operation to be performed by the device.
                                }
                                else
                                {
                                    logMessage(LOG_INFO_LEVEL, "LioCLoud Cache ***** HIT *****");
                                    logMessage(LOG_INFO_LEVEL, "Started to read the elemenets in %d/%d/%d", device_arr[device_no], sec, blk);   
                                    memcpy(read_buf, cache_buf, 256);
                                }

                                for (int j = 0; j < 256; j++)
                                {
                                    buf[n+j+(i*256)] = read_buf[j];
                                }

                                next_blk_secpos = next_blk_secpos+ 1;
                                blk = file_info[file_fh_pos].file_deviceinfo.blks_filled[next_blk_secpos];
                                sec = file_info[file_fh_pos].file_deviceinfo.secs_filled[next_blk_secpos];
                                device_arr[device_no] = file_info[file_fh_pos].file_deviceinfo.devices[next_blk_secpos];
                            }

                            int remain_len = (remainT_len - (times*256));
                            cache_buf = lcloud_getcache(device_arr[device_no], sec, blk);
                            if (cache_buf == NULL)
                            {
                                logMessage(LOG_INFO_LEVEL, "LioCLoud Cache ***** MISS *****");
                                logMessage(LOG_INFO_LEVEL, "Started to read the elemenets in %d/%d/%d", device_arr[device_no], sec, blk);
                                LCloudRegisterFrame frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, device_arr[device_no], LC_XFER_READ, sec, blk );     //specifying the sectors, blocks and operation to be performed by the bus.
                                client_lcloud_bus_request(frm, read_buf);       //takes our operation to be performed by the device.
                            }
                            else
                            {
                                logMessage(LOG_INFO_LEVEL, "LioCLoud Cache ***** HIT *****");
                                logMessage(LOG_INFO_LEVEL, "Started to read the elemenets in %d/%d/%d", device_arr[device_no], sec, blk);   
                                memcpy(read_buf, cache_buf, 256);
                            }
                            
                            for (int i = 0; i < remain_len; i++)
                            {  
                                 buf[n+(256*times)+i] = read_buf[i];
                            }
                            file_info[file_fh_pos].file_pointer = file_info[file_fh_pos].file_pointer + len; 
                            return (len); 
                        }
                    }
                }
                else
                {
                    return (-1);
                }
            }     
        }
    }
    else
    {
        return (-1);
    } 
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcwrite
// Description  : write data to the file
//
// Inputs       : fh - file handle for the file to write to
//                buf - pointer to data to write
//                len - the length of the write
// Outputs      : number of bytes written if successful test, -1 if failure

int blk_sec_pos;
int lcwrite( LcFHandle fh, char *buf, size_t len ) 
{    
    char temp_buf[256];
    int addblks;
   
    int file_fh_pos = fh;

    if (fh == file_info[file_fh_pos].file_fh)
    {  
        if (file_info[file_fh_pos].file_open != 1)
        {   
            return (-1);
        }
        else
        {
            int sec_no = My_device[device_arr[device_no]].tot_sec;
            int blk_no = My_device[device_arr[device_no]].tot_blk;

                if (file_info[file_fh_pos].file_pointer == file_info[file_fh_pos].file_len)   
                {  
                    if (file_info[file_fh_pos].file_deviceinfo.blk_tracker == 0)
                    {
                        blk_sec_pos = 0; 
                    }
                    else
                    {  
                        blk_sec_pos = file_info[file_fh_pos].file_deviceinfo.blk_tracker;
                    }

                    if ((file_info[file_fh_pos].file_pointer % 256) == 0)     //Super condition 1, if the pointer is always at the start of the block.
                    {   
                        if (len == 256)     //Sub condition 1, if exactly 256 bytes to be written.
                        {
                            char *cache_write_buf;
                            cache_write_buf = lcloud_getcache(device_arr[device_no], sec, blk);
                            if (cache_write_buf == NULL)
                            {   
                                lcloud_putcache(device_arr[device_no], sec, blk, buf);
                                logMessage(LOG_INFO_LEVEL,"LionCLoud Writing [%d/%d/%d]",device_arr[device_no], sec, blk);
                                LCloudRegisterFrame frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, device_arr[device_no], LC_XFER_WRITE, sec, blk );   
                                client_lcloud_bus_request(frm ,buf);
                            }
                            else
                            {
                                lcloud_putcache(device_arr[device_no], sec, blk, buf);
                                logMessage(LOG_INFO_LEVEL,"LionCLoud Writing [%d/%d/%d]",device_arr[device_no], sec, blk);
                                LCloudRegisterFrame frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, device_arr[device_no], LC_XFER_WRITE, sec, blk );   
                                client_lcloud_bus_request(frm ,buf);
                            }

                            file_info[file_fh_pos].last_sec = sec;
                            file_info[file_fh_pos].last_blk = blk;
                            file_info[file_fh_pos].file_pointer = file_info[file_fh_pos].file_pointer + len;      //absolute pointer position 
                            file_info[file_fh_pos].file_len = file_info[file_fh_pos].file_pointer;                //absolute file length
                            
                            file_info[file_fh_pos].file_deviceinfo.blks_filled[blk_sec_pos] = blk;
                            file_info[file_fh_pos].file_deviceinfo.secs_filled[blk_sec_pos] = sec;
                            file_info[file_fh_pos].file_deviceinfo.devices[blk_sec_pos] = device_arr[device_no];
                            file_info[file_fh_pos].file_deviceinfo.blk_tracker = blk_sec_pos + 1;
                            blk_sec_pos++;

                            if (blk<(blk_no-1) && sec < sec_no )
                            {
                                    blk = blk + 1;
                            }
                            else if (blk == (blk_no-1) && sec < sec_no-1)
                            {
                                blk = 0;
                                sec = sec+1;
                            }
                            else if (blk == (blk_no-1) && sec == (sec_no-1))        //incrementing the sectors, blocks and devices
                            {
                                sec = 0;
                                blk = 0;
                                device_no++;
                            }
                            
                            return (len);   
                        }
                        else if(len < 256)      //Sub condition 2, if len to be written is less then 256 bytes. 
                        {
                            char *cache_write_buf1;
                            cache_write_buf1 = lcloud_getcache(device_arr[device_no], sec, blk);
                            if (cache_write_buf1 == NULL)
                            {   
                                lcloud_putcache(device_arr[device_no], sec, blk, buf);
                                logMessage(LOG_INFO_LEVEL,"LionCLoud Writing [%d/%d/%d]",device_arr[device_no], sec, blk);
                                LCloudRegisterFrame frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, device_arr[device_no], LC_XFER_WRITE, sec, blk );   
                                client_lcloud_bus_request(frm ,buf);
                            }
                            else
                            {
                                lcloud_putcache(device_arr[device_no], sec, blk, buf);
                                logMessage(LOG_INFO_LEVEL,"LionCLoud Writing [%d/%d/%d]",device_arr[device_no], sec, blk);
                                LCloudRegisterFrame frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, device_arr[device_no], LC_XFER_WRITE, sec, blk );   
                                client_lcloud_bus_request(frm ,buf);
                            }

                            file_info[file_fh_pos].file_pointer = file_info[file_fh_pos].file_pointer + len; 
                            file_info[file_fh_pos].file_len = file_info[file_fh_pos].file_pointer;
                            file_info[file_fh_pos].last_sec = sec;   //updating the last block and sector
                            file_info[file_fh_pos].last_blk = blk;
                            
                            file_info[file_fh_pos].file_deviceinfo.blks_filled[blk_sec_pos] = blk;
                            file_info[file_fh_pos].file_deviceinfo.secs_filled[blk_sec_pos] = sec;
                            file_info[file_fh_pos].file_deviceinfo.devices[blk_sec_pos] = device_arr[device_no];
                            file_info[file_fh_pos].file_deviceinfo.blk_tracker = blk_sec_pos + 1;
                            blk_sec_pos++;

                            if (blk<(blk_no-1) && sec < sec_no )
                            {
                                    blk = blk + 1;
                            }
                            else if (blk == (blk_no-1) && sec < sec_no-1)
                            {
                                blk = 0;
                                sec = sec+1;
                            }
                            else if (blk == (blk_no-1) && sec == (sec_no-1))
                            {
                                sec = 0;
                                blk = 0;
                                device_no++;
                            }
                            
                            return (len);   
                        }

                        else if (len>256)       //Sub condition 3, if len to be written is greater then 256
                        {   
                            int extra_len = len%256;    //because the pointer is zero, extra length for the last block to be filled 
                            int times = len/256;        //number of blocks to be filled with 256 bytes 
                        
                            for (int  i = 0; i < times; i++)
                            {   
                                for (int j = 0; j < 256; j++)
                                {
                                    temp_buf[j] = buf[j+(i*256)]; 
                                   
                                }  

                            char *cache_write_buf2;
                            cache_write_buf2 = lcloud_getcache(device_arr[device_no], sec, blk);
                            if (cache_write_buf2 == NULL)
                            {   
                                lcloud_putcache(device_arr[device_no], sec, blk, temp_buf);
                                logMessage(LOG_INFO_LEVEL,"LionCLoud Writing [%d/%d/%d]",device_arr[device_no], sec, blk);
                                LCloudRegisterFrame frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, device_arr[device_no], LC_XFER_WRITE, sec, blk );   
                                client_lcloud_bus_request(frm ,temp_buf);
                            }
                            else
                            {
                                lcloud_putcache(device_arr[device_no], sec, blk, temp_buf);
                                logMessage(LOG_INFO_LEVEL,"LionCLoud Writing [%d/%d/%d]",device_arr[device_no], sec, blk);
                                LCloudRegisterFrame frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, device_arr[device_no], LC_XFER_WRITE, sec, blk );   
                                client_lcloud_bus_request(frm ,temp_buf);
                            }

                                file_info[file_fh_pos].file_deviceinfo.blks_filled[blk_sec_pos] = blk;
                                file_info[file_fh_pos].file_deviceinfo.secs_filled[blk_sec_pos] = sec;
                                file_info[file_fh_pos].file_deviceinfo.devices[blk_sec_pos] = device_arr[device_no];
                                file_info[file_fh_pos].file_deviceinfo.blk_tracker = blk_sec_pos+1;
                                blk_sec_pos++;
                                
                                //update the sectors and the blocks 
                                if (blk<(blk_no-1) && sec < sec_no )
                                {
                                        blk = blk + 1;
                                }
                                else if (blk == (blk_no-1) && sec < sec_no-1)
                                {
                                    blk = 0;
                                    sec = sec+1;
                                }
                                else if (blk == (blk_no-1) && sec == (sec_no-1))
                                {
                                    sec = 0;
                                    blk = 0;
                                    device_no++;
                                }
                            }

                            if (blk==0)         //updating the last/_sector and last_block based on the above code. 
                            {
                                if (sec!= 0)
                                {
                                    file_info[file_fh_pos].last_blk = 63;
                                    file_info[file_fh_pos].last_sec = sec-1;
                                }
                                else
                                {
                                    file_info[file_fh_pos].last_blk = 63;
                                    file_info[file_fh_pos].last_sec = sec;
                                }
                            }
                            else
                            {
                                file_info[file_fh_pos].last_sec = sec;
                                file_info[file_fh_pos].last_blk = blk-1;
                            }

                            int b = 0;
                            for (int  k = 0; k < extra_len; k++)    //writing  the extra length 
                            {
                                temp_buf[k] = buf[(len - extra_len) + k];    
                                b = b+1;
                            }
                            
                            if (b!=0)       //if their was extra length then it will go in the if loop else in the else condition
                            {   
                                char *cache_write_buf3;
                                cache_write_buf3 = lcloud_getcache(device_arr[device_no], sec, blk);
                                if (cache_write_buf3 == NULL)
                                {   
                                    lcloud_putcache(device_arr[device_no], sec, blk, temp_buf);
                                    logMessage(LOG_INFO_LEVEL,"LionCLoud Writing [%d/%d/%d]",device_arr[device_no], sec, blk);
                                    LCloudRegisterFrame frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, device_arr[device_no], LC_XFER_WRITE, sec, blk );   
                                    client_lcloud_bus_request(frm ,temp_buf);
                                }
                                else
                                {
                                    lcloud_putcache(device_arr[device_no], sec, blk, temp_buf);
                                    logMessage(LOG_INFO_LEVEL,"LionCLoud Writing [%d/%d/%d]",device_arr[device_no], sec, blk);
                                    LCloudRegisterFrame frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, device_arr[device_no], LC_XFER_WRITE, sec, blk );   
                                    client_lcloud_bus_request(frm ,temp_buf);
                                }

                                file_info[file_fh_pos].last_sec = sec;
                                file_info[file_fh_pos].last_blk = blk;  

                                file_info[file_fh_pos].file_deviceinfo.blks_filled[blk_sec_pos] = blk;
                                file_info[file_fh_pos].file_deviceinfo.secs_filled[blk_sec_pos] = sec;
                                file_info[file_fh_pos].file_deviceinfo.devices[blk_sec_pos] = device_arr[device_no];
                                file_info[file_fh_pos].file_deviceinfo.blk_tracker = blk_sec_pos+1;
                                blk_sec_pos++;

                                if (blk<(blk_no-1) && sec < sec_no )
                                {
                                        blk = blk + 1;
                                }
                                else if (blk == (blk_no-1) && sec < sec_no-1)
                                {
                                    blk = 0;
                                    sec = sec+1;
                                }
                                else if (blk == (blk_no-1) && sec == (sec_no-1))
                                {
                                    sec = 0;
                                    blk = 0;
                                    device_no++;
                                }
                            }
                            
                            file_info[file_fh_pos].file_pointer = file_info[file_fh_pos].file_pointer + len; 
                            file_info[file_fh_pos].file_len = file_info[file_fh_pos].file_pointer;    //updating the total file length.
                            return (len);   
                        }
                    }
             
                    else if((file_info[file_fh_pos].file_pointer%256) !=0)   //Super condition 2, if the file pointer is not zero and is somewhere in the middle of the block.
                    {   
                        int n = 0; 
                        char read_buf[256];
                        char temp_buf[256];
        
                        if (((file_info[file_fh_pos].file_pointer % 256) + len) <=256)   //Sub condition 1 
                        {
                            if (file_info[file_fh_pos].file_pointer % 256 == 0)
                            {
                                return '-1';
                            }
                            else
                            {  
                                char *cache_write_buf4;
                                cache_write_buf4 = lcloud_getcache(file_info[file_fh_pos].file_deviceinfo.devices[blk_sec_pos-1], file_info[file_fh_pos].file_deviceinfo.secs_filled[blk_sec_pos-1], file_info[file_fh_pos].file_deviceinfo.blks_filled[blk_sec_pos-1]);
                                if (cache_write_buf4 == NULL)
                                {   
                                    LCloudRegisterFrame frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, file_info[file_fh_pos].file_deviceinfo.devices[blk_sec_pos-1], LC_XFER_READ, file_info[file_fh_pos].file_deviceinfo.secs_filled[blk_sec_pos-1], file_info[file_fh_pos].file_deviceinfo.blks_filled[blk_sec_pos-1]);  
                                    client_lcloud_bus_request(frm, read_buf); 

                                    for (int i = (file_info[file_fh_pos].file_pointer%256); i < (len+(file_info[file_fh_pos].file_pointer%256)); i++)     //filling the current block
                                    {
                                        read_buf[i] = buf[n];
                                        n = n+1;
                                    }

                                    lcloud_putcache(file_info[file_fh_pos].file_deviceinfo.devices[blk_sec_pos-1], file_info[file_fh_pos].file_deviceinfo.secs_filled[blk_sec_pos-1], file_info[file_fh_pos].file_deviceinfo.blks_filled[blk_sec_pos-1], read_buf);
                                    frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, file_info[file_fh_pos].file_deviceinfo.devices[blk_sec_pos-1], LC_XFER_WRITE, file_info[file_fh_pos].file_deviceinfo.secs_filled[blk_sec_pos-1], file_info[file_fh_pos].file_deviceinfo.blks_filled[blk_sec_pos-1]); 
                                    client_lcloud_bus_request(frm, read_buf); 
                                }
                                else
                                {   
                                    memcpy(read_buf, cache_write_buf4, 256);
                                    for (int i = (file_info[file_fh_pos].file_pointer%256); i < (len+(file_info[file_fh_pos].file_pointer%256)); i++)     //filling the current block
                                    {
                                        read_buf[i] = buf[n];
                                        n = n+1;
                                    }

                                    lcloud_putcache(file_info[file_fh_pos].file_deviceinfo.devices[blk_sec_pos-1], file_info[file_fh_pos].file_deviceinfo.secs_filled[blk_sec_pos-1], file_info[file_fh_pos].file_deviceinfo.blks_filled[blk_sec_pos-1], read_buf);
                                    LCloudRegisterFrame frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, file_info[file_fh_pos].file_deviceinfo.devices[blk_sec_pos-1], LC_XFER_WRITE, file_info[file_fh_pos].file_deviceinfo.secs_filled[blk_sec_pos-1], file_info[file_fh_pos].file_deviceinfo.blks_filled[blk_sec_pos-1]); 
                                    client_lcloud_bus_request(frm, read_buf); 
                                }

                                file_info[file_fh_pos].last_sec = sec; 
                                file_info[file_fh_pos].last_blk = blk;
                                
                                file_info[file_fh_pos].file_pointer = file_info[file_fh_pos].file_pointer + len;
                                file_info[file_fh_pos].file_len = file_info[file_fh_pos].file_pointer;  
                                return len; 
                            }    
                        }

                        else if (((file_info[file_fh_pos].file_pointer % 256) + len)>256)        //Sub condition 2
                        {  
                            if (file_info[file_fh_pos].file_pointer % 256 == 0)//block is filled and start writing from current blk
                            {
                                return '-1';
                            }

                            else//start from the last block and go to the current blk
                            {   
                                int n = 0;
                                blk_sec_pos = file_info[file_fh_pos].file_deviceinfo.blk_tracker-1;

                                char *cache_write_buf5;
                                cache_write_buf5 = lcloud_getcache(file_info[file_fh_pos].file_deviceinfo.devices[blk_sec_pos], file_info[file_fh_pos].file_deviceinfo.secs_filled[blk_sec_pos], file_info[file_fh_pos].file_deviceinfo.blks_filled[blk_sec_pos]);
                                if (cache_write_buf5 == NULL)
                                {   
                                    LCloudRegisterFrame frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, file_info[file_fh_pos].file_deviceinfo.devices[blk_sec_pos], LC_XFER_READ, file_info[file_fh_pos].file_deviceinfo.secs_filled[blk_sec_pos], file_info[file_fh_pos].file_deviceinfo.blks_filled[blk_sec_pos]);  
                                    client_lcloud_bus_request(frm, read_buf); 

                                    for (int i = (file_info[file_fh_pos].file_pointer % 256); i < 256; i++)      //filling the current block first, note that because the conditon specifies the total length 
                                    {                                                             //to be written is greater then 256, we need to calculate the remaining space. 
                                        read_buf[i] = buf[n];
                                        n = n+1;    
                                    }

                                    lcloud_putcache(file_info[file_fh_pos].file_deviceinfo.devices[blk_sec_pos], file_info[file_fh_pos].file_deviceinfo.secs_filled[blk_sec_pos], file_info[file_fh_pos].file_deviceinfo.blks_filled[blk_sec_pos], read_buf);
                                    frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, file_info[file_fh_pos].file_deviceinfo.devices[blk_sec_pos], LC_XFER_WRITE, file_info[file_fh_pos].file_deviceinfo.secs_filled[blk_sec_pos], file_info[file_fh_pos].file_deviceinfo.blks_filled[blk_sec_pos]); 
                                    client_lcloud_bus_request(frm, read_buf); 
                                }
                                else
                                {   
                                    memcpy(read_buf, cache_write_buf5, 256);
                                    for (int i = (file_info[file_fh_pos].file_pointer % 256); i < 256; i++)      //filling the current block first, note that because the conditon specifies the total length 
                                    {                                                             //to be written is greater then 256, we need to calculate the remaining space. 
                                        read_buf[i] = buf[n];
                                        n = n+1;    
                                    }

                                    lcloud_putcache(file_info[file_fh_pos].file_deviceinfo.devices[blk_sec_pos], file_info[file_fh_pos].file_deviceinfo.secs_filled[blk_sec_pos], file_info[file_fh_pos].file_deviceinfo.blks_filled[blk_sec_pos], read_buf);
                                    LCloudRegisterFrame frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, file_info[file_fh_pos].file_deviceinfo.devices[blk_sec_pos], LC_XFER_WRITE, file_info[file_fh_pos].file_deviceinfo.secs_filled[blk_sec_pos], file_info[file_fh_pos].file_deviceinfo.blks_filled[blk_sec_pos]); 
                                    client_lcloud_bus_request(frm, read_buf); 
                                }

                                int temp_pointer = file_info[file_fh_pos].file_pointer % 256; 
                                int remain_len = len-(256 - (file_info[file_fh_pos].file_pointer % 256));
                                if (remain_len <= 256)      //condtion if the remaining length is less then 256   //doing the current block part
                                {   
                                    char temp_buf[256];

                                    char *cache_write_buf6;
                                    cache_write_buf6 = lcloud_getcache(device_arr[device_no], sec, blk);
                                    if (cache_write_buf6 == NULL)
                                    {   
                                        for (int i = 0; i < remain_len; i++)
                                        {
                                            temp_buf[i] = buf[(256-temp_pointer)+ i];
                                        }

                                        lcloud_putcache(device_arr[device_no], sec, blk, temp_buf);
                                        logMessage(LOG_INFO_LEVEL,"LionCLoud Writing [%d/%d/%d]",device_arr[device_no], sec, blk);
                                        LCloudRegisterFrame frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, device_arr[device_no], LC_XFER_WRITE, sec, blk );   
                                        client_lcloud_bus_request(frm ,temp_buf);
                                    }
                                    else
                                    {
                                        for (int i = 0; i < remain_len; i++)
                                        {
                                            temp_buf[i] = buf[(256-temp_pointer)+ i];
                                        }
                                        lcloud_putcache(device_arr[device_no], sec, blk, temp_buf);
                                        logMessage(LOG_INFO_LEVEL,"LionCLoud Writing [%d/%d/%d]",device_arr[device_no], sec, blk);
                                        LCloudRegisterFrame frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, device_arr[device_no], LC_XFER_WRITE, sec, blk );   
                                        client_lcloud_bus_request(frm ,temp_buf);
                                    }

                                    file_info[file_fh_pos].last_sec = sec;
                                    file_info[file_fh_pos].last_blk = blk;
                                    blk_sec_pos = blk_sec_pos + 1;
                                    file_info[file_fh_pos].file_deviceinfo.blks_filled[blk_sec_pos] = blk;
                                    file_info[file_fh_pos].file_deviceinfo.secs_filled[blk_sec_pos] = sec;
                                    file_info[file_fh_pos].file_deviceinfo.devices[blk_sec_pos] = device_arr[device_no];
                                    file_info[file_fh_pos].file_deviceinfo.blk_tracker = blk_sec_pos+1;
                                    blk_sec_pos++;

                                    if (blk<(blk_no-1) && sec < sec_no )
                                    {
                                            blk = blk + 1;
                                    }
                                    else if (blk == (blk_no-1) && sec < sec_no-1)
                                    {
                                        blk = 0;
                                        sec = sec+1;
                                    }
                                    else if (blk == (blk_no-1) && sec == (sec_no-1))
                                    {
                                        sec = 0;
                                        blk = 0;
                                        device_no++;
                                    } 
                                    
                                    file_info[file_fh_pos].file_pointer = file_info[file_fh_pos].file_pointer + len;
                                    file_info[file_fh_pos].file_len = file_info[file_fh_pos].file_pointer;  
                                    return (len);   
                                }

                                else if (remain_len>256)        //condtion if the remaining length to be written is greater then 256
                                {  
                                    int extra_len = remain_len%256;
                                    int times = remain_len/256;
                                  
                                    for (int  i = 0; i < times; i++)    //number of times the remaining bytes of 256 blocks are to be run. 
                                    {
                                        for (int j = 0; j < 256; j++)
                                        {   
                                            temp_buf[j] = buf[((len-remain_len) + j + 256*i)];
                                        }

                                        char *cache_write_buf7;
                                        cache_write_buf7 = lcloud_getcache(device_arr[device_no], sec, blk);
                                        if (cache_write_buf7 == NULL)
                                        {   
                                            lcloud_putcache(device_arr[device_no], sec, blk, temp_buf);
                                            logMessage(LOG_INFO_LEVEL,"LionCLoud Writing [%d/%d/%d]",device_arr[device_no], sec, blk);
                                            LCloudRegisterFrame frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, device_arr[device_no], LC_XFER_WRITE, sec, blk );   
                                            client_lcloud_bus_request(frm ,temp_buf);
                                        }
                                        else
                                        {
                                            lcloud_putcache(device_arr[device_no], sec, blk, temp_buf);
                                            logMessage(LOG_INFO_LEVEL,"LionCLoud Writing [%d/%d/%d]",device_arr[device_no], sec, blk);
                                            LCloudRegisterFrame frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, device_arr[device_no], LC_XFER_WRITE, sec, blk );   
                                            client_lcloud_bus_request(frm ,temp_buf);
                                        }  
                                        blk_sec_pos = blk_sec_pos + 1;
                                        file_info[file_fh_pos].file_deviceinfo.blks_filled[blk_sec_pos] = blk;
                                        file_info[file_fh_pos].file_deviceinfo.secs_filled[blk_sec_pos] = sec;
                                        file_info[file_fh_pos].file_deviceinfo.devices[blk_sec_pos] = device_arr[device_no];
                                        file_info[file_fh_pos].file_deviceinfo.blk_tracker = blk_sec_pos+1;
                                    

                                        if (blk<(blk_no-1) && sec < sec_no )
                                        {
                                                blk = blk + 1;
                                        }
                                        else if (blk == (blk_no-1) && sec < sec_no-1)
                                        {
                                            blk = 0;
                                            sec = sec+1;
                                        }
                                        else if (blk == (blk_no-1) && sec == (sec_no-1))
                                        {
                                            sec = 0;
                                            blk = 0;
                                            device_no++;
                                        } 
                                    }
                                
                                    int b = 0;          
                                    for (int  k = 0; k < extra_len; k++)    //then again writing the final set of bytes which are obviously now less then 256. 
                                    {
                                        temp_buf[k] = buf[(len - extra_len) + k];   
                                        b = b+1;
                                    }
                                   
                                    if (b!=0)
                                    {  
                                        char temp3_buf[256];
                                        char *cache_write_buf8;
                                        cache_write_buf8 = lcloud_getcache(device_arr[device_no], sec, blk);
                                        if (cache_write_buf8 == NULL)
                                        {   
                                            lcloud_putcache(device_arr[device_no], sec, blk, temp_buf);
                                            logMessage(LOG_INFO_LEVEL,"LionCLoud Writing [%d/%d/%d]",device_arr[device_no], sec, blk);
                                            LCloudRegisterFrame frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, device_arr[device_no], LC_XFER_WRITE, sec, blk );   
                                            client_lcloud_bus_request(frm ,temp_buf);
                                        }
                                        else
                                        {
                                            lcloud_putcache(device_arr[device_no], sec, blk, temp_buf);
                                            logMessage(LOG_INFO_LEVEL,"LionCLoud Writing [%d/%d/%d]",device_arr[device_no], sec, blk);
                                            LCloudRegisterFrame frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, device_arr[device_no], LC_XFER_WRITE, sec, blk );   
                                            client_lcloud_bus_request(frm ,temp_buf);
                                        }

                                        file_info[file_fh_pos].last_sec = sec;
                                        file_info[file_fh_pos].last_blk = blk; 
                                        blk_sec_pos = blk_sec_pos + 1;
                                        file_info[file_fh_pos].file_deviceinfo.blks_filled[blk_sec_pos] = blk;
                                        file_info[file_fh_pos].file_deviceinfo.secs_filled[blk_sec_pos] = sec;
                                        file_info[file_fh_pos].file_deviceinfo.devices[blk_sec_pos] = device_arr[device_no];
                                        file_info[file_fh_pos].file_deviceinfo.blk_tracker = blk_sec_pos+1;
                                        blk_sec_pos++;

                                        if (blk<(blk_no-1) && sec < sec_no )
                                        {
                                                blk = blk + 1;
                                        }
                                        else if (blk == (blk_no-1) && sec < sec_no-1)
                                        {
                                            blk = 0;
                                            sec = sec+1;
                                        }
                                        else if (blk == (blk_no-1) && sec == (sec_no-1))
                                        {
                                            sec = 0;
                                            blk = 0;
                                            device_no++;
                                        }

                                        file_info[file_fh_pos].file_pointer = file_info[file_fh_pos].file_pointer + len;
                                        file_info[file_fh_pos].file_len = file_info[file_fh_pos].file_pointer; 
                                           
                                    }
                                    else
                                    {
                                        file_info[file_fh_pos].file_pointer = file_info[file_fh_pos].file_pointer + len;
                                        file_info[file_fh_pos].file_len = file_info[file_fh_pos].file_pointer; 
                                    }
                                    
                                    file_info[file_fh_pos].last_sec = sec;
                                    file_info[file_fh_pos].last_blk = blk;  
                                    return (len);   
                                }
                              
                            }
                        }
                    }
                }   ///till there I have my write function
                //from here copy the overwrite part

                else
                {   
                    if (file_info[file_fh_pos].file_pointer + len <= file_info[file_fh_pos].file_len)
                    {
                        if (file_info[file_fh_pos].file_pointer%256 + len <= 256)
                        {
                            next_blk_secpos = (file_info[file_fh_pos].file_pointer/256);
                            int temp_blk = file_info[file_fh_pos].file_deviceinfo.blks_filled[next_blk_secpos];
                            int temp_sec = file_info[file_fh_pos].file_deviceinfo.secs_filled[next_blk_secpos];
                            int temp_device_arr_device_no = file_info[file_fh_pos].file_deviceinfo.devices[next_blk_secpos];
                            
                            int n = 0;
                            char temp_buf[256];
                            LCloudRegisterFrame frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, temp_device_arr_device_no, LC_XFER_READ, temp_sec,temp_blk); 
                            client_lcloud_bus_request(frm, temp_buf);
                            for (int i = file_info[file_fh_pos].file_pointer%256; i < file_info[file_fh_pos].file_pointer%256 + len; i++)
                            {   
                                temp_buf[i] = buf[n];
                                n++;    
                            }
                            frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, temp_device_arr_device_no, LC_XFER_WRITE, temp_sec,temp_blk); 
                            client_lcloud_bus_request(frm, temp_buf);
                            lcloud_putcache(temp_device_arr_device_no, temp_sec, temp_blk, temp_buf);

                            file_info[file_fh_pos].file_pointer = file_info[file_fh_pos].file_pointer + len;
                            return len;
                        }
                        else //(file_info[file_fh_pos].file_pointer + len > 256)
                        {
                            next_blk_secpos = (file_info[file_fh_pos].file_pointer/256);
                            int temp_blk = file_info[file_fh_pos].file_deviceinfo.blks_filled[next_blk_secpos];
                            int temp_sec = file_info[file_fh_pos].file_deviceinfo.secs_filled[next_blk_secpos];
                            int temp_device_arr_device_no = file_info[file_fh_pos].file_deviceinfo.devices[next_blk_secpos];
                            
                            int n = 0;
                            char temp_buf[256];
                            LCloudRegisterFrame frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, temp_device_arr_device_no, LC_XFER_READ, temp_sec,temp_blk); 
                            client_lcloud_bus_request(frm, temp_buf);
                            for (int i = file_info[file_fh_pos].file_pointer%256; i < 256; i++)
                            {
                                temp_buf[i] = buf[n];
                                n++;    
                            }
                            frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, temp_device_arr_device_no, LC_XFER_WRITE, temp_sec,temp_blk); 
                            client_lcloud_bus_request(frm, temp_buf);
                            lcloud_putcache(temp_device_arr_device_no, temp_sec, temp_blk, temp_buf);
                            next_blk_secpos++;
                            temp_blk = file_info[file_fh_pos].file_deviceinfo.blks_filled[next_blk_secpos];
                            temp_sec = file_info[file_fh_pos].file_deviceinfo.secs_filled[next_blk_secpos];
                            temp_device_arr_device_no = file_info[file_fh_pos].file_deviceinfo.devices[next_blk_secpos];

                            int remain_len = len-(n);
                            for (int i = 0; i < remain_len/256; i++)
                            {
                                LCloudRegisterFrame frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, temp_device_arr_device_no, LC_XFER_READ, temp_sec,temp_blk); 
                                client_lcloud_bus_request(frm, temp_buf);
                                for (int j = 0; j < 256; j++)
                                {
                                    temp_buf[j] = buf[(n)+j+(i*256)];   
                                }
                                frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, temp_device_arr_device_no, LC_XFER_WRITE, temp_sec,temp_blk); 
                                client_lcloud_bus_request(frm, temp_buf);
                                lcloud_putcache(temp_device_arr_device_no, temp_sec, temp_blk, temp_buf);
                                next_blk_secpos++;
                                temp_blk = file_info[file_fh_pos].file_deviceinfo.blks_filled[next_blk_secpos];
                                temp_sec = file_info[file_fh_pos].file_deviceinfo.secs_filled[next_blk_secpos];
                                temp_device_arr_device_no = file_info[file_fh_pos].file_deviceinfo.devices[next_blk_secpos];
                            }
                          

                            frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, temp_device_arr_device_no, LC_XFER_READ, temp_sec,temp_blk); 
                            client_lcloud_bus_request(frm, temp_buf);
                            int remain_final = remain_len - ((remain_len/256)*256);
                            for (int i = 0; i < remain_final-1; i++)
                            {
                                temp_buf[i] = buf[(n) + (256*(remain_len/256)) + i];
                            }
                            frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, temp_device_arr_device_no, LC_XFER_WRITE, temp_sec,temp_blk); 
                            client_lcloud_bus_request(frm, temp_buf);
                            lcloud_putcache(temp_device_arr_device_no, temp_sec, temp_blk, temp_buf);
                            file_info[file_fh_pos].file_pointer = file_info[file_fh_pos].file_pointer + len;
                            return len; 
                        }
                    }
                    else  //(file_info[file_fh_pos].file_pointer + len > file_info[file_fh_pos].file_len)
                    {   
                        next_blk_secpos = (file_info[file_fh_pos].file_pointer/256);
                        int temp_blk = file_info[file_fh_pos].file_deviceinfo.blks_filled[next_blk_secpos];
                        int temp_sec = file_info[file_fh_pos].file_deviceinfo.secs_filled[next_blk_secpos];
                        int temp_device_arr_device_no = file_info[file_fh_pos].file_deviceinfo.devices[next_blk_secpos];
                        
                        int n = 0;
                        char temp_buf[256];
                        LCloudRegisterFrame frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, temp_device_arr_device_no, LC_XFER_READ, temp_sec,temp_blk); 
                        client_lcloud_bus_request(frm, temp_buf);
            
                        for (int i = file_info[file_fh_pos].file_pointer%256; i < 256; i++)
                        {
                            temp_buf[i] = buf[n];
                            n++;    
                        }
                        frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, temp_device_arr_device_no, LC_XFER_WRITE, temp_sec,temp_blk); 
                        client_lcloud_bus_request(frm, temp_buf);
                        lcloud_putcache(temp_device_arr_device_no, temp_sec, temp_blk, temp_buf);

                        next_blk_secpos++;
                        int remain_len = len-(n);
                        for (int i = next_blk_secpos; i < file_info[file_fh_pos].file_deviceinfo.blk_tracker; i++)
                        {  
                            for (int j = 0; j < 256; j++)
                            {
                                temp_buf[j] = buf[(n)+j+(i*256)];   ////(n-1 changed to n)
                            }
                            
                            int temp_blk = file_info[file_fh_pos].file_deviceinfo.blks_filled[next_blk_secpos];
                            int temp_sec = file_info[file_fh_pos].file_deviceinfo.secs_filled[next_blk_secpos];
                            int temp_device_arr_device_no = file_info[file_fh_pos].file_deviceinfo.devices[next_blk_secpos];
                            
                            frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, temp_device_arr_device_no, LC_XFER_WRITE, temp_sec,temp_blk); 
                            client_lcloud_bus_request(frm, temp_buf);
                            lcloud_putcache(temp_device_arr_device_no, temp_sec, temp_blk, temp_buf);
                        }

                        int remain_final = remain_len - ((remain_len/256)*256);

                        if (remain_final<=256)
                        {
                            frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, device_arr[device_no], LC_XFER_READ, sec, blk );  
                            client_lcloud_bus_request(frm, temp_buf);
                             
                            for (int i = 0; i < remain_final; i++)
                            {
                                temp_buf[i] = buf[(n) + (256*(remain_len/256)) + i];  //(n-1 changed to n)
                            }
                            LCloudRegisterFrame frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, device_arr[device_no], LC_XFER_WRITE, sec, blk ); 
                            client_lcloud_bus_request(frm, temp_buf);
                            lcloud_putcache(device_arr[device_no], sec, blk, temp_buf );
                            
                            file_info[file_fh_pos].last_sec = sec;
                            file_info[file_fh_pos].last_blk = blk;
                            file_info[file_fh_pos].file_pointer = file_info[file_fh_pos].file_pointer + len;      //absolute pointer position 
                            file_info[file_fh_pos].file_len = file_info[file_fh_pos].file_pointer;                //absolute file length
                            
                            blk_sec_pos = file_info[file_fh_pos].file_deviceinfo.blk_tracker+1;
                            file_info[file_fh_pos].file_deviceinfo.blks_filled[blk_sec_pos] = blk;
                            file_info[file_fh_pos].file_deviceinfo.secs_filled[blk_sec_pos] = sec;
                            file_info[file_fh_pos].file_deviceinfo.devices[blk_sec_pos] = device_arr[device_no];
                            file_info[file_fh_pos].file_deviceinfo.blk_tracker = blk_sec_pos;
                            blk_sec_pos++;

                            if (blk<(blk_no-1) && sec < sec_no )
                            {
                                    blk = blk + 1;
                            }
                            else if (blk == (blk_no-1) && sec < sec_no-1)
                            {
                                blk = 0;
                                sec = sec+1;
                            }
                            else if (blk == (blk_no-1) && sec == (sec_no-1))
                            {
                                sec = 0;
                                blk = 0;
                                device_no++;
                            }
                            return len;
                        }

                        else //remain_final>256
                        {
                            int extra_len = remain_final%256;    //because the pointer is zero, extra length for the last block to be filled 
                            int times = remain_final/256;        //number of blocks to be filled with 256 bytes 
                        
                            for (int  i = 0; i < times; i++)
                            {   
                                LCloudRegisterFrame frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, device_arr[device_no], LC_XFER_READ, sec, blk );
                                client_lcloud_bus_request(frm ,temp_buf); 
                                for (int j = 0; j < 256; j++)
                                {
                                    temp_buf[j] = buf[j+(i*256)]; 
                                   
                                }
                                frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, device_arr[device_no], LC_XFER_WRITE, sec, blk ); 
                                client_lcloud_bus_request(frm ,temp_buf);    
                                lcloud_putcache(device_arr[device_no], sec, blk, temp_buf );
                                blk_sec_pos = file_info[file_fh_pos].file_deviceinfo.blk_tracker+1;
                                file_info[file_fh_pos].file_deviceinfo.blks_filled[blk_sec_pos] = blk;
                                file_info[file_fh_pos].file_deviceinfo.secs_filled[blk_sec_pos] = sec;
                                file_info[file_fh_pos].file_deviceinfo.devices[blk_sec_pos] = device_arr[device_no];
                                file_info[file_fh_pos].file_deviceinfo.blk_tracker = blk_sec_pos;
                                blk_sec_pos++;
                                
                                //update the sectors and the blocks 
                                if (blk<(blk_no-1) && sec < sec_no )
                                {
                                        blk = blk + 1;
                                }
                                else if (blk == (blk_no-1) && sec < sec_no-1)
                                {
                                    blk = 0;
                                    sec = sec+1;
                                }
                                else if (blk == (blk_no-1) && sec == (sec_no-1))
                                {
                                    sec = 0;
                                    blk = 0;
                                    device_no++;
                                }
                            }

                            if (blk==0)         //updating the last/_sector and last_block based on the above code. 
                            {
                                if (sec!= 0)
                                {
                                    file_info[file_fh_pos].last_blk = 63;
                                    file_info[file_fh_pos].last_sec = sec-1;
                                }
                                else
                                {
                                    file_info[file_fh_pos].last_blk = 63;
                                    file_info[file_fh_pos].last_sec = sec;
                                }
                            }
                            else
                            {
                                file_info[file_fh_pos].last_sec = sec;
                                file_info[file_fh_pos].last_blk = blk-1;
                            }

                            int b = 0;
                            frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, device_arr[device_no], LC_XFER_READ, sec, blk );
                            client_lcloud_bus_request(frm ,temp_buf); 
                            for (int  k = 0; k < extra_len; k++)    //writing  the extra length 
                            {
                                temp_buf[k] = buf[(remain_final - extra_len) + k];    
                                b = b+1;
                            }
                            
                            if (b!=0)       //if their was extra length then it will go in the if loop else in the else condition
                            {   
                                LCloudRegisterFrame frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, device_arr[device_no], LC_XFER_WRITE, sec, blk ); 
                                client_lcloud_bus_request(frm ,temp_buf);
                                lcloud_putcache(device_arr[device_no], sec, blk, temp_buf );
                                file_info[file_fh_pos].last_sec = sec;
                                file_info[file_fh_pos].last_blk = blk;  

                                file_info[file_fh_pos].file_deviceinfo.blks_filled[blk_sec_pos] = blk;
                                file_info[file_fh_pos].file_deviceinfo.secs_filled[blk_sec_pos] = sec;
                                file_info[file_fh_pos].file_deviceinfo.devices[blk_sec_pos] = device_arr[device_no];
                                file_info[file_fh_pos].file_deviceinfo.blk_tracker = blk_sec_pos;
                                blk_sec_pos++;

                                if (blk<(blk_no-1) && sec < sec_no )
                                {
                                        blk = blk + 1;
                                }
                                else if (blk == (blk_no-1) && sec < sec_no-1)
                                {
                                    blk = 0;
                                    sec = sec+1;
                                }
                                else if (blk == (blk_no-1) && sec == (sec_no-1))
                                {
                                    sec = 0;
                                    blk = 0;
                                    device_no++;
                                }
                            }
                            
                            file_info[file_fh_pos].file_pointer = file_info[file_fh_pos].file_pointer + len; 
                            file_info[file_fh_pos].file_len = file_info[file_fh_pos].file_pointer;    //updating the total file length.
                            return (len); 
                        }
                    }
                }       
            }   
        }   //if file handler is bad 
        else
        {
            return (-1); //the file handle is faulty, something wrong check 
        }
   // } 
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcseek
// Description  : Seek to a specific place in the file
//
// Inputs       : fh - the file handle of the file to seek in
//                off - offset within the file to seek to
// Outputs      : 0 if successful test, -1 if failure

int lcseek( LcFHandle fh, size_t off ) 
{
    int file_fh_pos = fh;
    int temp_blks = off/256;
    
    if (file_info[file_fh_pos].file_fh == fh)
    {
        if (file_info[file_fh_pos].file_open == 1)         //basically, updating the file_pointer to a new position. 
        {                                                  //allows us to over write our wriiten data. 
            if (off<=file_info[file_fh_pos].file_len)
            { 
                file_info[file_fh_pos].file_pointer = off;
                blk = file_info[file_fh_pos].file_deviceinfo.blks_filled[temp_blks];
                sec = file_info[file_fh_pos].file_deviceinfo.secs_filled[temp_blks];
                device_arr[device_no] = file_info[file_fh_pos].file_deviceinfo.devices[temp_blks];  
                
                return file_info[file_fh_pos].file_pointer;
            }
            else
            {
                return (-1);
            }
        }
        else
        {
            return (-1);
        }
    }
    else
    {
        return (-1);
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcclose
// Description  : Close the file
//
// Inputs       : fh - the file handle of the file to close
// Outputs      : 0 if successful test, -1 if failure

int lcclose( LcFHandle fh ) {

    if (file_info[fh].file_fh == fh && file_info[fh].file_open == 1)         //closing all the files based on the passed filehandle and file status 
    {
        file_info[fh].file_open = 0; 
        return (0);
    }
    else
    {
        return (-1);
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcshutdown
// Description  : Shut down the filesystem
//
// Inputs       : none
// Outputs      : 0 if successful test, -1 if failure

int lcshutdown( void ) {
    
    lcloud_closecache();
    LCloudRegisterFrame register_pack =  create_lcloud_registers(0,0, LC_POWER_OFF, 0, 0, 0, 0);    //shutting down the device based on the device id. 
    client_lcloud_bus_request(register_pack, NULL);
}
