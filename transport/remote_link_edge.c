#include "remote_link_edge.h"

void remote_link_edge_initialise(RemoteLinkEdge* edge) {
    edge->port_open = false;
}

RemoteLinkEdgeOutcome remote_link_edge_observe(RemoteLinkEdge* edge, bool cable_present, bool host_opened) {
    bool open_now = cable_present && host_opened;
    if(open_now == edge->port_open) {
        return RemoteLinkEdgeNone;
    }
    edge->port_open = open_now;
    return open_now ? RemoteLinkEdgeOpened : RemoteLinkEdgeClosed;
}

bool remote_link_edge_is_open(const RemoteLinkEdge* edge) {
    return edge->port_open;
}
