from pox.core import core
import pox.openflow.libopenflow_01 as of
from pox.lib.util import dpid_to_str

log = core.getLogger()

class _OpenFlowMessageHandler(object):
    def __init__(self):
        core.openflow.addListeners(self)
        log.debug("OpenFlowMessageHandler initialized")

    def _handle_ConnectionUp (self, event):
        log.debug("ConnectionUp event received from possible CCPDN: {}".format(event.connection))

    def _inspect_raw_message(self, event):
        log.debug("Raw message received from CCPDN: {}".format(event.raw))

    def _handle_OpenFlow_Packet(self, event):
        log.debug("OpenFlow Packet event received from CCPDN: {}".format(event.ofp))

    def _handle_PacketIn(self, event):
        log.debug("PacketIn event received from CCPDN: {}".format(event.parsed))

    def _handle_FlowMod(self, event):
        log.debug("FlowMod event received from CCPDN: {}".format(event.ofp))

    def _handle_FlowRemoved(self, event):
        log.debug("FlowRemoved event received from CCPDN: {}".format(event.ofp))

    def _handle_StatsRequest(self, event):
        log.debug("StatsRequest event received from CCPDN: {}".format(event.ofp))

    def _handle_FeaturesReply(self, event):
        log.debug("FeaturesReply event received from CCPDN: {}".format(event.ofp))

    def _handle_Unknown(self, event):
        log.debug("Unknown event received from CCPDN: {}".format(event.ofp))

def launch():
    core.registerNew(_OpenFlowMessageHandler)
    log.info("POX/CCPDN controller script loaded and listening for packets...")