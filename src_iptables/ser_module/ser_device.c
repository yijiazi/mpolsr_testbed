/******************************************************************
* Copyright (C) 2010                                                     
*     by      Pascal Lesage (Keosys)                                       
*             Xavier Lecourtier (Keosys) xavier.lecourtier@keosys.com                    
*             Sylvain David (IRCCyN, University of Nantes, France) sylvain.david@polytech.univ-nantes.fr          
*             Jiazi Yi (IRCCyN, University of Nantes, France) jiazi.yi@.univ-nantes.fr 
*      Benoit Parrein (IRCCyN, University of Nantes, France) benoit.parrein@polytech.univ-nantes.fr
*
*     Members of SEREADMO (French Research Grant ANR-05-RNRT-02803)                                                         
*                                                                                                                                     
*     This program is distributed in the hope that it will be useful,                                                          
*     but WITHOUT ANY WARRANTY; without even the implied warranty of
*     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
****************************************************************
* Used and modified 2015
*   by      Benjamin Mollé (engineering student, Polytech Nantes, University of Nantes, France) benjamin.molle@gmail.com
*           Denis Souron (engineering student, Polytech Nantes, Université of Nantes, France) denis.souron@laposte.net

*
* Creates char device
*
*************************************************************************
* Changes :
* --------
*
*************************************************************************/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include "common.h"
#include "ser_device.h"
#include "ser_path.h"



static int          g_major_dec_num = 0;            /* Major number assigned to the device driver */
static int          g_device_counter = 0;
static rwlock_t     g_ser_device_lock = __RW_LOCK_UNLOCKED(g_ser_device_lock);



//***********************************************************************
//***********************************************************************


//  Called when a process writes to dev file: echo "hi" > /dev/sereadmo
static ssize_t dev_device_write(struct file *filp, const char *buffer, size_t length, loff_t *off)
{

    struct dev_tc_token     token;
    int                     token_size = sizeof(struct dev_tc_token);
    int                     size = length;
    const char*             buff = buffer;

    if((length % token_size) != 0)
    {
        // invalid buffer size
        MSG_FAILED("SEREADMO invalid TC data sent to char device\n");
        return length;
    }

    while(size > 0)
    {
        if(copy_from_user(&token,buff,token_size) != 0)
        {
            MSG_FAILED("SEREADEMO copy_from_user failed\n");
            return length;
        }
        else
        {
            DBG_DEV(1,"Received token type : %ld\n",token.type);

            switch(token.type)
            {
            case DEV_CMD_RESET_TC :
                tc_free_data(0);
                break;

            case DEV_CMD_ADD_NODE :
                tc_add_node(token.addr_1);
                break;

            case DEV_CMD_REMOVE_NODE :
                tc_remove_node(token.addr_1);
                break;

            case DEV_CMD_ADD_LINK :
                tc_add_link(token.addr_1,token.addr_2,token.value);
                break;

            case DEV_CMD_REMOVE_LINK :
                tc_remove_link(token.addr_1,token.addr_2);
                break;

            case DEV_CMD_ADD_ALIAS :
                tc_add_alias(token.addr_1,token.addr_2);
                break;

            case DEV_CMD_REMOVE_ALIAS :
                tc_remove_alias(token.addr_1,token.addr_2);
                break;

            case DEV_CMD_SET_LOCAL_ADDR :
                tc_set_local_addr(token.addr_1,token.addr_2);
                break;

            case DEV_CMD_SET_ALIAS_ADDR :
                tc_set_local_alias(token.addr_1,token.addr_2);
                break;

            default:
                MSG_FAILED("SEREADEMO invalid cmd type : %ld\n",token.type);

            }
        }

        size -= token_size;
        buff += token_size;

    }

    tc_print_data(1,2);

    return length;

}


//***********************************************************************
//***********************************************************************


// Called when a process, which already opened the dev file, attempts to read from it
static ssize_t dev_device_read(struct file *filp, char *buffer, size_t length, loff_t *offset)
{
    char msg[100];
    int bytes_read = 0;

    WRITE_LOCK(g_ser_device_lock);
        sprintf(msg,"SEREADMO char device registred and open %d times", g_device_counter);
    WRITE_UNLOCK(g_ser_device_lock);


    bytes_read = strlen(msg);
    if(*offset >= bytes_read)
        // no more bytes to read
        return 0;

    bytes_read -= *offset; // bytes left to read

    if(bytes_read > length)
        bytes_read = length;


    if(copy_to_user(buffer,msg+(*offset),bytes_read) != 0)
    {
        MSG_FAILED("SEREADMO copy_to_user failed\n");
        bytes_read = 0;
    }

    *offset += bytes_read;
    return bytes_read;
}


//***********************************************************************
//***********************************************************************


// Called when a process tries to open the device file, like "cat /dev/sereadmo"
static int dev_device_open(struct inode *inode, struct file *file)
{
    WRITE_LOCK(g_ser_device_lock);
    g_device_counter++;
    WRITE_UNLOCK(g_ser_device_lock);

    return 0;
}


//***********************************************************************
//***********************************************************************


// Called when a process closes the device file
static int dev_device_release(struct inode *inode, struct file *file)
{
    WRITE_LOCK(g_ser_device_lock);
    g_device_counter--;
    WRITE_UNLOCK(g_ser_device_lock);

    return 0;
}


//***********************************************************************
//***********************************************************************


static struct file_operations g_fops = 
{
  .owner = THIS_MODULE,
  .read = dev_device_read, 
  .write = dev_device_write,
  .open = dev_device_open,
  .release = dev_device_release
};


int dev_init_device(void)
{

    DBG_MOD(1,"Init sereadmo char device\n" );

    g_major_dec_num = register_chrdev(0, DEVICE_NAME, &g_fops);

    DBG_MOD(2,"sereadmo char device major number : %d\n", g_major_dec_num );

    if (g_major_dec_num < 0) 
    {
        // Failed to init device driver
        MSG_FAILED("Failed to register SEREADMO char device : %d\n", g_major_dec_num);
        g_major_dec_num = 0;
        return -ENODEV;
    }

    return 0;
}


//***********************************************************************
//***********************************************************************


void dev_cleanup_device(void)
{
    DBG_MOD(1,"stop sereadmo char device\n" );

    /* Unregister the device */
    if(g_major_dec_num > 0)
    {
        unregister_chrdev(g_major_dec_num, DEVICE_NAME); 
//            MSG_FAILED("Failed to unregister SEREADMO char device\n");
    }
}


//***********************************************************************
//***********************************************************************


