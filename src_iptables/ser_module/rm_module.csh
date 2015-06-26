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

DEV_NAME="sereadmo"

if [ -e /dev/${DEV_NAME} ]
then
    rm /dev/${DEV_NAME}
fi

rmmod ser_iptables

if [ $? -ne 0 ] 
then
    echo "Failed to uninstall sereadmo iptables module"
fi

