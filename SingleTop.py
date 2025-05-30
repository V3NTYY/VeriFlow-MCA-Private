#!/usr/bin/env python

import sys
from mininet.net import Mininet
from mininet.node import Controller, RemoteController, OVSController
from mininet.node import CPULimitedHost, Host, Node
from mininet.node import OVSKernelSwitch, UserSwitch
from mininet.node import IVSSwitch
from mininet.cli import CLI
from mininet.log import setLogLevel, info
from mininet.link import TCLink, Intf
from subprocess import call

def getPorts():
    print("Please enter the ports used for both controllers in the format: <port1> <port2>")
    ports = input("Ports: ")
    ports = ports.split()
    if len(ports) != 2:
        print("Invalid input. Please enter two ports.")
        return getPorts()
    try:
        port1 = int(ports[0])
        port2 = int(ports[1])
    except ValueError:
        print("Invalid input. Please enter two valid port numbers.")
        return getPorts()
    if port1 < 1024 or port2 < 1024:
        print("Invalid input. Ports must be greater than 1024.")
        return getPorts()
    if port1 == port2:
        print("Invalid input. Ports must be different.")
        return getPorts()
    print("Please enter an offset (any number from 1-25)")
    offset = input("Offset: ")
    try:
        offset = int(offset)
    except ValueError:
        print("Invalid input. Please enter a valid number.")
        return getPorts()
    if offset < 1 or offset > 25:
        print("Invalid input. Offset must be between 1 and 25.")
        return getPorts()

    return [port1, port2, offset]

def myNetwork(port1, port2, offset):

    port1 = int(port1)
    port2 = int(port2)
    offset = int(offset)

    net = Mininet( topo=None,
                   build=False,
                   ipBase='10.0.0.0/8')

    info( '*** Adding controller\n' )
    c0=net.addController(name='c0',
                      controller=RemoteController,
                      ip='127.0.0.1',
                      protocol='tcp',
                      port=port1)

    info( '*** Add switches\n')
    s1 = net.addSwitch('s1', cls=OVSKernelSwitch, dpid='1')
    s2 = net.addSwitch('s2', cls=OVSKernelSwitch, dpid='2')

    info( '*** Add hosts\n')

    # Dynamically calculate IPs based on offset
    h1_ip = "10.0.0.{}".format(1 * offset)
    h2_ip = "10.0.0.{}".format(2 * offset)
    h3_ip = "10.0.0.{}".format(3 * offset)

    h2 = net.addHost('h2', cls=Host, ip=h1_ip, defaultRoute=None)
    h1 = net.addHost('h1', cls=Host, ip=h2_ip, defaultRoute=None)
    h3 = net.addHost('h3', cls=Host, ip=h3_ip, defaultRoute=None)

    info( '*** Add links\n')
    net.addLink(h1, s1)
    net.addLink(h2, s1)
    net.addLink(s2, h3)

    info( '*** Starting network\n')
    net.build()
    info( '*** Starting controllers\n')
    for controller in net.controllers:
        controller.start()

    info( '*** Starting switches\n')
    net.get('s1').start([c0])
    net.get('s2').start([c0])

    info( '*** Post configure switches and hosts\n')
    s1.cmd('ifconfig s1 10.0.0.{}'.format(5*offset))
    s2.cmd('ifconfig s2 10.0.0.{}'.format(6*offset))

    CLI(net)
    net.stop()

if __name__ == '__main__':
    # results = getPorts()
    results = [sys.argv[1], sys.argv[2], sys.argv[3]]
    setLogLevel( 'info' )
    myNetwork(results[0], results[1], results[2])

