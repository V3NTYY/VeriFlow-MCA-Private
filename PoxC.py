from pox.core import core
from pox.openflow import of

log = core.getLogger()

class StatsRequestHandler:
    def __init__(self):
        core.openflow.addListeners(self)

    def _handle_ConnectionUp(self, event):
        log.info("Switch %s has connected", event.connection.dpid)

    def _handle_ConnectionDown(self, event):
        log.info("Switch %s has disconnected", event.connection.dpid)

    def _handle_OpenFlow_PacketIn(self, event):
        # Intercept all OpenFlow messages
        msg = event.ofp
        log.debug("Received OpenFlow message: %s", msg)

        # Check if the message is a STATS_REQUEST
        if msg.type == of.OFPT_STATS_REQUEST:
            log.info("Received STATS_REQUEST from switch %s", event.connection.dpid)

            # Example: Respond with a STATS_REPLY (optional)
            reply = of.ofp_stats_reply()
            reply.type = of.OFPST_FLOW
            reply.body = []
            event.connection.send(reply)
            log.info("Sent STATS_REPLY to switch %s", event.connection.dpid)

def launch():
    # Register the custom module
    core.registerNew(StatsRequestHandler)