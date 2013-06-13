#!/usr/bin/python

"""
Build a simple network from scratch, using mininet primitives.
This is more complicated than using the higher-level classes,
but it exposes the configuration details and allows customization.

For most tasks, the higher-level API will be preferable.
"""

from mininet.net import Mininet
from mininet.node import Node
from mininet.link import Link
from mininet.log import setLogLevel, info
from mininet.util import quietRun

from time import sleep

def scratchNet( cname='controller', cargs='-v ptcp:' ):
    "Create network from scratch using Open vSwitch."

    info( "*** Creating nodes\n" )
    controller = Node( 'c0', inNamespace=False )
    switch = Node( 's0', inNamespace=False )
    switch1 = Node( 's1', inNamespace=False )
    h0 = Node( 'h0' )
    h1 = Node( 'h1' )
    h2 = Node( 'h2' )
    h3 = Node( 'h3' )
    h4 = Node( 'h4' )
    h5 = Node( 'h5' )
    h6 = Node( 'h6' )
    h7 = Node( 'h7' )

    info( "*** Creating links\n" )
    Link( h0, switch )
    Link( h1, switch )
    Link( h2, switch )
    Link( h3, switch )
    Link( h4, switch1 )
    Link( h5, switch1 )
    Link( h6, switch1 )
    Link( h7, switch1 )
    info( "*** Configuring hosts\n" )
    h0.setIP( '192.168.123.1/24' )
    h1.setIP( '192.168.123.2/24' )
    h2.setIP( '192.168.123.3/24' )
    h3.setIP( '192.168.123.4/24' )
    h4.setIP( '192.168.123.5/24' )
    h5.setIP( '192.168.123.6/24' )
    h6.setIP( '192.168.123.7/24' )
    h7.setIP( '192.168.123.8/24' )
    info( str( h0 ) + '\n' )
    info( str( h1 ) + '\n' )
    info( str( h2 ) + '\n' )
    info( str( h3 ) + '\n' )
    info( str( h4 ) + '\n' )
    info( str( h5 ) + '\n' )
    info( str( h6 ) + '\n' )
    info( str( h7 ) + '\n' )

    info( "*** Starting network using Open vSwitch\n" )
    controller.cmd( cname + ' ' + cargs + '&' )
    switch.cmd( 'ovs-vsctl del-br dp0' )
    switch.cmd( 'ovs-vsctl add-br dp0' )
    switch1.cmd( 'ovs-vsctl del-br dp0' )
    switch1.cmd( 'ovs-vsctl add-br dp0' )
    for intf in switch.intfs.values():
        print switch.cmd( 'ovs-vsctl add-port dp0 %s' % intf )
    for intf in switch1.intfs.values():
        print switch1.cmd( 'ovs-vsctl add-port dp0 %s' % intf )

    # Note: controller and switch are in root namespace, and we
    # can connect via loopback interface
    switch.cmd( 'ovs-vsctl set-controller dp0 tcp:127.0.0.1:6633' )
    switch1.cmd( 'ovs-vsctl set-controller dp0 tcp:127.0.0.1:6633' )

    info( '*** Waiting for switch to connect to controller' )
    while 'is_connected' not in quietRun( 'ovs-vsctl show' ):
        sleep( 1 )
        info( '.' )
    info( '\n' )

    info( "*** Running test\n" )
    h0.cmdPrint( 'ping -c1 ' + h1.IP() )

    info( "*** Stopping network\n" )
    #controller.cmd( 'kill %' + cname )
    #switch.cmd( 'ovs-vsctl del-br dp0' )
    #switch.deleteIntfs()
    #switch1.cmd( 'ovs-vsctl del-br dp0' )
    #switch1.deleteIntfs()
    info( '\n' )

if __name__ == '__main__':
    setLogLevel( 'info' )
    info( '*** Scratch network demo (kernel datapath)\n' )
    Mininet.init()
    scratchNet()
