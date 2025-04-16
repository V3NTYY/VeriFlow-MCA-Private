from pox.core import core
import pox.openflow.libopenflow_01 as of
from pox.lib.util import dpid_to_str
from pox.lib.addresses import EthAddr

log = core.getLogger()

def _handle_ConnectionUp(event):
    log.info("Connection established with switch: {}".format(dpid_to_str(event.dpid)))

def _handle_PacketIn(event):
    packet = event.parsed  # The parsed packet data
    log.info("Packet received: {}".format(packet))
    if isinstance(packet, EthAddr):
        log.info("Ethernet packet: {} -> {}, type: {}".format(packet.src, packet.dst, packet.type))
    else:
        log.info("Non-Ethernet packet received")

def launch():
    core.openflow.addListenerByName("ConnectionUp", _handle_ConnectionUp)
    core.openflow.addListenerByName("PacketIn", _handle_PacketIn)
    log.info("POX/CCPDN controller script loaded and listening for packets...")