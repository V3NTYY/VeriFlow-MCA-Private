from pox.core import core
import pox.openflow.libopenflow_01 as of
from pox.lib.util import dpid_to_str
from pox.lib.revent import EventMixin

log = core.getLogger()

class _OpenFlowMessageHandler(object):
    def __init__(self):
        core.openflow.addListeners(self)
        core.openflow.addListenerByName("ConnectionUp", self._handle_ConnectionUp)
        core.openflow.addListenerByName("StatsRequest", self._handle_StatsRequest)
        log.debug("OpenFlowMessageHandler initialized")

    def _handle_ConnectionUp (self, event):
        log.debug("ConnectionUp event received from possible CCPDN: {}".format(event.connection))

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