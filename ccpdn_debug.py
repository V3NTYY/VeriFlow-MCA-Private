from pox.core import core
from pox.lib.util import dpid_to_str
from pox.lib.packet.ethernet import ethernet
from pox.openflow.of_01 import OFP_VERSIONS

log = core.getLogger()

def _handle_ConnectionUp(event):
    log.info(f"Connection established with switch: {dpid_to_str(event.dpid)}")

def _handle_PacketIn(event):
    packet = event.parsed  # The parsed packet data
    log.info(f"Packet received: {packet}")
    if isinstance(packet, ethernet):
        log.info(f"Ethernet packet: {packet.src} -> {packet.dst}, type: {packet.type}")
    else:
        log.info("Non-Ethernet packet received")

def launch():
    core.openflow.addListenerByName("ConnectionUp", _handle_ConnectionUp)
    core.openflow.addListenerByName("PacketIn", _handle_PacketIn)
    log.info("POX controller script loaded and listening for events")