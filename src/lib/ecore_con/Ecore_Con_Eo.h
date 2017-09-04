#include "efl_net_types.eot.h"

#include "efl_net_ip_address.eo.h"

#include "efl_net_socket.eo.h"
#include "efl_net_dialer.eo.h"
#include "efl_net_server.eo.h"

#include "efl_net_socket_simple.eo.h"
#include "efl_net_dialer_simple.eo.h"
#include "efl_net_server_simple.eo.h"

#include "efl_net_socket_fd.eo.h"
#include "efl_net_server_fd.eo.h"

#include "efl_net_socket_tcp.eo.h"
#include "efl_net_dialer_tcp.eo.h"
#include "efl_net_server_tcp.eo.h"

#ifdef _WIN32
#include "efl_net_socket_windows.eo.h"
#include "efl_net_dialer_windows.eo.h"
#include "efl_net_server_windows.eo.h"
#else
#include "efl_net_socket_unix.eo.h"
#include "efl_net_dialer_unix.eo.h"
#include "efl_net_server_unix.eo.h"
#endif

#include "efl_net_socket_udp.eo.h"
#include "efl_net_dialer_udp.eo.h"
#include "efl_net_server_udp.eo.h"
#include "efl_net_server_udp_client.eo.h"

#include "efl_net_http_types.eot.h"

#include "efl_net_dialer_http.eo.h"
#include "efl_net_dialer_websocket.eo.h"

#include "efl_net_ssl_types.eot.h"

#include "efl_net_ssl_context.eo.h"
#include "efl_net_socket_ssl.eo.h"
#include "efl_net_dialer_ssl.eo.h"
#include "efl_net_server_ssl.eo.h"

#include "efl_net_control_technology.eo.h"
#include "efl_net_control_access_point.eo.h"
#include "efl_net_control.eo.h"
#include "efl_net_session.eo.h"
