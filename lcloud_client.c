////////////////////////////////////////////////////////////////////////////////
//
//  File          : lcloud_client.c
//  Description   : This is the client side of the Lion Clound network
//                  communication protocol.
//
//  Author        : Patrick McDaniel
//  Last Modified : Sat 28 Mar 2020 09:43:05 AM EDT
//

// Include Files
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>

// Project Include Files
#include <lcloud_network.h>
#include <cmpsc311_log.h>
#include <cmpsc311_util.h>;
//
// Functions

////////////////////////////////////////////////////////////////////////////////
//
// Function     : client_lcloud_bus_request
// Description  : This the client regstateeration that sends a request to the 
//                lion client server.   It will:
//
//                1) if INIT make a connection to the server
//                2) send any request to the server, returning results
//                3) if CLOSE, will close the connection
//
// Inputs       : reg - the request reqisters for the command
//                buf - the block to be read/written from (READ/WRITE)
// Outputs      : the response structure encoded as needed



void extract_lcloud_registers_client(LCloudRegisterFrame T_pack,uint64_t *B0,uint64_t *B1,uint64_t *C0,uint64_t *C1,uint64_t *C2,uint64_t *D0,uint64_t *D1)
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


////////////////////////////////////////////////////////////////////////////////
//
// Function     : client_lcloud_bus_request
// Description  : communincation between the server and the client
//
// Inputs       : reg - the packet of registers
//                buf - pointer to data to write
// Outputs      : commute with the server to apply operations and -1, if failure occurs. 

uint64_t *B0, *B1, *C0, *C1, *C2, *D0, *D1; 
int socket_handle = -1;
LCloudRegisterFrame client_lcloud_bus_request( LCloudRegisterFrame reg, void *buf ) 
{
    if (socket_handle == -1)
    {
        struct sockaddr_in caddr;
        caddr.sin_family = AF_INET;
        caddr.sin_port = htons(LCLOUD_DEFAULT_PORT);

        if ( inet_aton(LCLOUD_DEFAULT_IP, &caddr.sin_addr) == 0 )   //setups the address
        {
            return( -1 );
        }

        socket_handle = socket(PF_INET, SOCK_STREAM, 0);        //setups the socket
        if (socket_handle == -1) 
        {
            printf( "Error on socket creation [%s]\n", strerror(errno) );
            return( -1 );
        }

        if (connect(socket_handle, (const struct sockaddr *)&caddr, sizeof(caddr)) == -1 )  //setups the connection 
        {
            printf( "Error on socket connect [%s]\n", strerror(errno) );
            return( -1 );
        }  
    }

    extract_lcloud_registers_client(reg, &B0, &B1, &C0, &C1, &C2, &D0, &D1);    //unfolding the registers got from the server
    uint64_t *networ_value;
    reg = htonll64(reg);
    networ_value = &reg;
    buf = (char *)buf;
    
    if (C0 == LC_BLOCK_XFER)    //checks the operation 
    {   
        // CASE 1: write operation (look at the c0 and c2 fields)
        // SEND: (reg) <- Network format : send the register reg to the network 
        // after converting the register to 'network format'.
        //       buf 256-byte block (Data to write to that frame)
        //
        // RECEIVE: (reg) -> Host format

        if (C2 == LC_XFER_WRITE)    //write
        {
            write(socket_handle, networ_value, sizeof(uint64_t));
            write(socket_handle, buf, LC_DEVICE_BLOCK_SIZE);
            read(socket_handle, networ_value, sizeof(uint64_t));

            return(ntohll64(*networ_value));
        }

        // CASE 2: read operation (look at the c0 and c2 fields)
        // SEND: (reg) <- Network format : send the register reg to the network
        // after converting the register to 'network format'.
        //
        // RECEIVE: (reg) -> Host format
        //          256-byte block (Data read from that frame)

        else    //read
        {
            write(socket_handle, networ_value, sizeof(uint64_t));
            read(socket_handle, networ_value, sizeof(uint64_t));
            read(socket_handle, buf, LC_DEVICE_BLOCK_SIZE);

            return(ntohll64(*networ_value));
        }   
    }

    // CASE 3: power off operation, devprobe and devinit
    // SEND: (reg) <- Network format : send the register reg to the network 
    // after converting the register to 'network format'.
    //
    // RECEIVE: (reg) -> Host format
    //
    // Close the socket when finished : reset socket_handle to initial value of -1.
    // close(socket_handle)

    else    //takes care of the power off and power on, devinit and devprobe. 
    {
        if (write(socket_handle, networ_value, sizeof(uint64_t)) != sizeof(uint64_t)) 
        {
            printf( "Error writing network data [%s]\n", strerror(errno) );
        }

        if (read(socket_handle, networ_value, sizeof(uint64_t)) != sizeof(uint64_t)) 
        {
            printf( "Error reading network data [%s]\n", strerror(errno));   
        }
        
        if (C0 == LC_POWER_OFF)
        {
            close(socket_handle);
            socket_handle = -1;
        }
    
        return(ntohll64(*networ_value));
    }
}

