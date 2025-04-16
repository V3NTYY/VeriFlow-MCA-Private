from pox.core import core
import pox.openflow.libopenflow_01 as of
from pox.lib.util import dpid_to_str

log = core.getLogger()

def _handle_ConnectionUp(event):
    log.info("CONNECTION established with switch: {}".format(dpid_to_str(event.dpid)))

    # intercept flow messages
    event.connection.addListeners(_OpenFlowMessageHandler())

class _OpenFlowMessageHandler(object):
    def __init__(self):
        pass

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
    core.openflow.addListenerByName("ConnectionUp", _handle_ConnectionUp)
    log.info("POX/CCPDN controller script loaded and listening for packets...")