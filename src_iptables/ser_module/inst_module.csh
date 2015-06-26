#!/bin/sh
# *************************************************************************
# *             Projet SEREADMO - N° ANR-05-RNRT-028-01
# *              Securite des Reseaux Ad hoc & Mojette
# *************************************************************************
# *
# * AUTHORS    : P.Lesage <pascal.lesage@keosys.com>
# * VERSION    : 1.0
# *
# =========================================================================
# *
# * 
# *
# *************************************************************************
# * Changes :
# * --------
# *
# **************************************************************************
# * Used and modified 2015
# *   by      Benjamin Mollé (engineering student, Polytech Nantes, University of Nantes, France) benjamin.molle@gmail.com
# *           Denis Souron (engineering student, Polytech Nantes, Université of Nantes, France) denis.souron@laposte.net

PROC_NAME="seriptables"
DEV_NAME="sereadmo"
MAJOR_NUM=0

cp ser_iptables.ko /lib/modules/2.6.21.5/

depmod 
modprobe ser_iptables

if [ $? -eq 0 ] 
then
    MAJOR_NUM=`grep ${PROC_NAME} /proc/devices | cut -d' ' -f1`

    if [ -z ${MAJOR_NUM} ] 
    then
        echo "Failed to get major number for device : /dev/${DEV_NAME}"
    else
        if [ ${MAJOR_NUM} -ne 0 ] 
        then
            mknod /dev/${DEV_NAME} c ${MAJOR_NUM} 0
            
            if [ $? -ne 0 ]
            then
                echo "Failed to install device : /dev/${DEV_NAME}"
            fi
        else
            echo "Failed to get major number for device : /dev/${DEV_NAME}"
        fi
    fi
else
    echo "Failed to install sereadmo iptables module"
fi


