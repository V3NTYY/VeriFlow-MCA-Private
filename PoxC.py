from pox.core import core
import pox.openflow.libopenflow_01 as of

log = core.getLogger()

class CCPDNHandler:
    def __init__(self):
        core.openflow.addListeners(self)

    def _handle_ConnectionUp(self, event):
        log.info("Switch %s has connected", event.connection.dpid)
        # Add listeners for our custom events to the connection
        event.connection.addListener(of.ofp_stats_request, self._handle_ofp_stats_request)
        event.connection.addListener(of.ofp_packet_in, self._handle_ofp_packet_in)

    def _handle_ConnectionDown(self, event):
        log.info("Switch %s has disconnected", event.connection.dpid)

    def _handle_ofp_stats_request(self, event):
        msg = event.ofp  # The received stats request message
        log.debug("Received stats request from %s: %s", event.dpid, msg)

    def _handle_ofp_packet_in(self, event):
        # Intercept all OpenFlow messages
        raw_data = event.data
        log.debug("Received OpenFlow message: %s", raw_data)

        # Parse the msg
        msg = event.ofp

        # Check if the message is a STATS_REQUEST
        if msg.type == of.OFPT_STATS_REQUEST:
            log.info("Received STATS_REQUEST from switch %s", event.connection.dpid)

def launch():
    # Register the custom module
    core.registerNew(CCPDNHandler)