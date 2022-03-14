////////////////////////////////////////////////////////////////////////////////
//
//  File           : lcloud_cache.c
//  Description    : This is the cache implementation for the LionCloud
//                   assignment for CMPSC311.
//
//   Author        : Patrick McDaniel
//   Last Modified : Thu 19 Mar 2020 09:27:55 AM EDT
//

// Includes 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cmpsc311_log.h>
#include <lcloud_cache.h>



int time_tracker = 0;
int maximim_blks;
int hit_track = 0;
int miss_track = 0;
struct mycache_info
{
    int device_id;
    int sect;
    int blkt;
    int cache;
    int file_time; 
    char my_data[256];

}*cache_tracker; 

//
// Functions

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcloud_getcache
// Description  : Search the cache for a block 
//
// Inputs       : did - device number of block to find
//                sec - sector number of block to find
//                blk - block number of block to find
// Outputs      : cache block if found (pointer), NULL if not or failure

char * lcloud_getcache( LcDeviceId did, uint16_t sec, uint16_t blk ) {
    /* Return not found */

    for (int i = 0; i < maximim_blks; i++)
    {
        if (((cache_tracker+i)->device_id == did)  && ((cache_tracker+i)->blkt == blk) && ((cache_tracker+i)->sect == sec))
        { 
            hit_track = hit_track+1;
            return(cache_tracker+i)->my_data;
        }
    }
    
    miss_track = miss_track+1;
    return( NULL );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcloud_putcache
// Description  : Put a value in the cache 
//
// Inputs       : did - device number of block to insert
//                sec - sector number of block to insert
//                blk - block number of block to insert
// Outputs      : 0 if succesfully inserted, -1 if failure

int lcloud_putcache( LcDeviceId did, uint16_t sec, uint16_t blk, char *block ) {
    /* Return successfully */
    
   for (int i = 0; i < maximim_blks; i++)
    {   
        if ((cache_tracker+i)->device_id == did  && (cache_tracker+i)->blkt == blk && (cache_tracker+i)->sect == sec)
        {
            for (int j = 0; j < 255; j++)
            {
                (cache_tracker+i)->my_data[j] = block[j];  
            }
            
            (cache_tracker+i)->file_time = time_tracker;
            time_tracker++;
        }
        else    //else if does not match with the above condition then check if any struct is empty or not. if it is then store all the device info in it. 
        {
            if ((cache_tracker+i)->blkt == NULL)
            {
                (cache_tracker+i)->device_id = did;
                (cache_tracker+i)->sect = sec;
                (cache_tracker+i)->blkt = block;
                
                for (int j = 0; j < 255; j++)
                {
                    (cache_tracker+i)->my_data[j] = block[j];   
                }

                (cache_tracker+i)->file_time = time_tracker;
                time_tracker++;
            }   
            else    // else use the LRU to dynammically allocate a struct which was Least Recently Used. 
            {
                int least_RU = 9999999999;
                int final_LRU_used; 
                for (int k = 0; k < maximim_blks; k++)
                {
                    if ((cache_tracker+k)->file_time < least_RU )
                    {
                        least_RU = (cache_tracker+k)->file_time;
                        final_LRU_used = k;
                    }
                }
                
                (cache_tracker+final_LRU_used)->device_id = did;
                (cache_tracker+final_LRU_used)->sect = sec;
                (cache_tracker+final_LRU_used)->blkt = block;

                for (int j = 0; j < 255; j++)
                {
                    (cache_tracker+final_LRU_used)->my_data[j] = block[j];     
                }

                (cache_tracker+final_LRU_used)->file_time = time_tracker;
                time_tracker++;

            }   
        }
    }
    
    return( 0 );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcloud_initcache
// Description  : Initialze the cache by setting up metadata a cache elements.
//
// Inputs       : maxblocks - the max number number of blocks 
// Outputs      : 0 if successful, -1 if failure

int lcloud_initcache( int maxblocks ) {
    /* Return successfully */
    maximim_blks = maxblocks;
    cache_tracker = (struct mycache_info*)malloc(sizeof(struct mycache_info)*maxblocks);
    return( 0 );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcloud_closecache
// Description  : Clean up the cache when program is closing
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int lcloud_closecache( void ) {
    /* Return successfully */

    free(cache_tracker);
    
    float hit_ratio = (time_tracker- miss_track)/(float)(time_tracker)*100.00;
    float miss_ratio = miss_track/(float)(time_tracker)*100.00;
    logMessage(LOG_INFO_LEVEL, "number of Hits:  %d", time_tracker - (miss_track));
    logMessage(LOG_INFO_LEVEL, "number of Misses:  %d",miss_track);
    logMessage(LOG_INFO_LEVEL, "number of Hit Ratio:  %f",  hit_ratio);
    logMessage(LOG_INFO_LEVEL, "number of Miss Ratio:  %f", miss_ratio);
    return( 0 );
}