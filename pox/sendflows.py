from pox.core import core
import pox.openflow.libopenflow_01 as of
from pox.lib.util import dpid_to_str
from pox.lib.revent import EventMixin

log = core.getLogger()

class _OpenFlowMessageHandler(object):
    def __init__(self):
        core.openflow.addListeners(self)
        log.debug("OpenFlowMessageHandler initialized")

    def _handle_ConnectionUp (self, event):
        log.debug("ConnectionUp event received from possible CCPDN: {}".format(event.connection))

    def _handle_FlowMod(self, event):
        log.debug("FlowMod event received from CCPDN: {}".format(event.ofp))

    def _handle_FlowRemoved(self, event):
        log.debug("FlowRemoved event received from CCPDN: {}".format(event.ofp))

    def _handle_StatsRequest(self, event):
        log.debug("StatsRequest event received from CCPDN: {}".format(event.ofp))

    def _handle_StatsReply(self, event):
        log.debug("StatsReply event received from CCPDN: {}".format(event.ofp))

    def _handle_FeaturesReply(self, event):
        log.debug("FeaturesReply event received from CCPDN: {}".format(event.ofp))

    def _handle_Unknown(self, event):
        log.debug("Unknown event received from CCPDN: {}".format(event.ofp))

    def _handle(self, connection, msg):
        log.debug("Received message from CCPDN: {}".format(msg))
        if msg.type == of.OFP_TYPE_FLOW_MOD:
            self._handle_FlowMod(msg)
        elif msg.type == of.OFP_TYPE_FLOW_REMOVED:
            self._handle_FlowRemoved(msg)
        elif msg.type == of.OFP_TYPE_STATS_REQUEST:
            self._handle_StatsRequest(msg)
        elif msg.type == of.OFP_TYPE_STATS_REPLY:
            self._handle_StatsReply(msg)
        elif msg.type == of.OFP_TYPE_FEATURES_REPLY:
            self._handle_FeaturesReply(msg)
        else:
            self._handle_Unknown(msg)

def launch():
    core.registerNew(_OpenFlowMessageHandler)
    log.info("POX/CCPDN controller script loaded and listening for packets...")