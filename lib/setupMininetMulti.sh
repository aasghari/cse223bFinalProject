#!/bin/bash

EXEC="route add -net 224.0.0.0 netmask 240.0.0.0 `ifconfig | grep eth | cut -f1 -d' '`"
echo $EXEC
$EXEC
