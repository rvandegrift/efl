#define EFL_NET_SERVER_SSL_PROTECTED 1
#define EFL_NET_SERVER_FD_PROTECTED 1
#define EFL_NET_SERVER_PROTECTED 1
#define EFL_LOOP_FD_PROTECTED 1

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Ecore.h"
#include "Ecore_Con.h"
#include "ecore_con_private.h"

#define MY_CLASS EFL_NET_SERVER_SSL_CLASS

typedef struct _Efl_Net_Server_Ssl_Data
{
   Eo *server;
   Eo *ssl_ctx;
} Efl_Net_Server_Ssl_Data;

static void
_efl_net_server_ssl_client_event_closed(void *data, const Efl_Event *event)
{
   Eo *server = data;
   Eo *client = event->object;

   efl_event_callback_del(client, EFL_IO_CLOSER_EVENT_CLOSED, _efl_net_server_ssl_client_event_closed, server);
   if (efl_parent_get(client) == server)
     efl_parent_set(client, NULL);

   /* do NOT change count as we're using the underlying server's count */
   //efl_net_server_clients_count_set(server, efl_net_server_clients_count_get(server) - 1);
}

static Eina_Bool
_efl_net_server_ssl_efl_net_server_client_announce(Eo *o, Efl_Net_Server_Ssl_Data *pd EINA_UNUSED, Eo *client)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(client, EINA_FALSE);
   EINA_SAFETY_ON_FALSE_GOTO(efl_isa(client, EFL_NET_SOCKET_SSL_CLASS), wrong_type);
   EINA_SAFETY_ON_FALSE_GOTO(efl_parent_get(client) == o, wrong_parent);

   efl_event_callback_call(o, EFL_NET_SERVER_EVENT_CLIENT_ADD, client);

   if (efl_parent_get(client) != o)
     {
        DBG("client %s was reparented! Ignoring it...",
            efl_net_socket_address_remote_get(client));
        return EINA_TRUE;
     }

   if (efl_ref_get(client) == 1) /* users must take a reference themselves */
     {
        DBG("client %s was not handled, closing it...",
            efl_net_socket_address_remote_get(client));
        efl_del(client);
        return EINA_FALSE;
     }
   else if (efl_io_closer_closed_get(client))
     {
        DBG("client %s was closed from 'client,add', delete it...",
            efl_net_socket_address_remote_get(client));
        efl_del(client);
        return EINA_FALSE;
     }

   /* do NOT change count as we're using the underlying server's count */
   //efl_net_server_clients_count_set(o, efl_net_server_clients_count_get(o) + 1);
   efl_event_callback_add(client, EFL_IO_CLOSER_EVENT_CLOSED, _efl_net_server_ssl_client_event_closed, o);
   return EINA_TRUE;

 wrong_type:
   ERR("%p client %p (%s) doesn't implement Efl.Net.Socket.Ssl class, deleting it.", o, client, efl_class_name_get(efl_class_get(client)));
   efl_del(client);
   return EINA_FALSE;

 wrong_parent:
   ERR("%p client %p (%s) parent=%p is not our child, deleting it.", o, client, efl_class_name_get(efl_class_get(client)), efl_parent_get(client));
   efl_del(client);
   return EINA_FALSE;
}

static void
_efl_net_server_ssl_tcp_client_add(void *data, const Efl_Event *event)
{
   Eo *o = data;
   Efl_Net_Server_Ssl_Data *pd = efl_data_scope_get(o, MY_CLASS);
   Eo *client_ssl;
   Eo *client_tcp = event->info;
   const char *addr = efl_net_socket_address_remote_get(client_tcp);

   if (!pd->ssl_ctx)
     {
        ERR("ssl server %p rejecting client %p '%s' since no SSL context was set!", o, client_tcp, addr);
        efl_event_callback_call(o, EFL_NET_SERVER_EVENT_CLIENT_REJECTED, (void *)addr);
        return;
     }

   client_ssl = efl_add(EFL_NET_SOCKET_SSL_CLASS, o,
                        efl_net_socket_ssl_adopt(efl_added, client_tcp, pd->ssl_ctx));
   if (!client_ssl)
     {
        ERR("ssl server %p could not wrap client %p '%s' using context=%p", o, client_tcp, addr, pd->ssl_ctx);
        efl_event_callback_call(o, EFL_NET_SERVER_EVENT_CLIENT_REJECTED, (void *)addr);
        return;
     }

   efl_net_server_client_announce(o, client_ssl);
}

static void
_efl_net_server_ssl_tcp_client_rejected(void *data, const Efl_Event *event)
{
   Eo *o = data;
   char *client = event->info;
   efl_event_callback_call(o, EFL_NET_SERVER_EVENT_CLIENT_REJECTED, client);
}

static void
_efl_net_server_ssl_tcp_error(void *data, const Efl_Event *event)
{
   Eo *o = data;
   Eina_Error *perr = event->info;
   efl_event_callback_call(o, EFL_NET_SERVER_EVENT_ERROR, perr);
}

static void
_efl_net_server_ssl_tcp_serving(void *data, const Efl_Event *event EINA_UNUSED)
{
   Eo *o = data;
   efl_event_callback_call(o, EFL_NET_SERVER_EVENT_SERVING, NULL);
}

EFL_CALLBACKS_ARRAY_DEFINE(_efl_net_server_ssl_tcp_cbs,
                           { EFL_NET_SERVER_EVENT_CLIENT_ADD, _efl_net_server_ssl_tcp_client_add },
                           { EFL_NET_SERVER_EVENT_CLIENT_REJECTED, _efl_net_server_ssl_tcp_client_rejected },
                           { EFL_NET_SERVER_EVENT_ERROR, _efl_net_server_ssl_tcp_error },
                           { EFL_NET_SERVER_EVENT_SERVING, _efl_net_server_ssl_tcp_serving });

EOLIAN Efl_Object *
_efl_net_server_ssl_efl_object_constructor(Eo *o, Efl_Net_Server_Ssl_Data *pd)
{
   o = efl_constructor(efl_super(o, MY_CLASS));
   if (!o) return NULL;

   pd->server = efl_add(EFL_NET_SERVER_TCP_CLASS, o,
                      efl_event_callback_array_add(efl_added, _efl_net_server_ssl_tcp_cbs(), o));
   EINA_SAFETY_ON_NULL_RETURN_VAL(pd->server, NULL);

   return o;
}

static void
_efl_net_server_ssl_ctx_del(void *data, const Efl_Event *event EINA_UNUSED)
{
   Eo *o = data;
   Efl_Net_Server_Ssl_Data *pd = efl_data_scope_get(o, MY_CLASS);
   pd->ssl_ctx = NULL;
}

EOLIAN void
_efl_net_server_ssl_efl_object_destructor(Eo *o, Efl_Net_Server_Ssl_Data *pd)
{
   if (pd->server)
     {
        efl_del(pd->server);
        pd->server = NULL;
     }

   if (pd->ssl_ctx)
     {
        efl_event_callback_del(pd->ssl_ctx, EFL_EVENT_DEL, _efl_net_server_ssl_ctx_del, o);
        efl_unref(pd->ssl_ctx);
        pd->ssl_ctx = NULL;
     }

   efl_destructor(efl_super(o, MY_CLASS));
}

EOLIAN static void
_efl_net_server_ssl_ssl_context_set(Eo *o EINA_UNUSED, Efl_Net_Server_Ssl_Data *pd, Eo *ssl_ctx)
{
   EINA_SAFETY_ON_FALSE_RETURN(efl_isa(ssl_ctx, EFL_NET_SSL_CONTEXT_CLASS));

   if (pd->ssl_ctx == ssl_ctx) return;
   efl_unref(pd->ssl_ctx);
   pd->ssl_ctx = efl_ref(ssl_ctx);
   if (ssl_ctx)
     efl_event_callback_add(ssl_ctx, EFL_EVENT_DEL, _efl_net_server_ssl_ctx_del, o);
}

EOLIAN static Eo *
_efl_net_server_ssl_ssl_context_get(Eo *o EINA_UNUSED, Efl_Net_Server_Ssl_Data *pd)
{
   return pd->ssl_ctx;
}

EOLIAN static Eina_Error
_efl_net_server_ssl_socket_activate(Eo *o, Efl_Net_Server_Ssl_Data *pd, const char *address)
{
   EINA_SAFETY_ON_TRUE_RETURN_VAL(efl_net_server_serving_get(o), EALREADY);
   EINA_SAFETY_ON_NULL_RETURN_VAL(address, EINVAL);
   return efl_net_server_fd_socket_activate(pd->server, address);
}

EOLIAN static Eina_Error
_efl_net_server_ssl_efl_net_server_serve(Eo *o EINA_UNUSED, Efl_Net_Server_Ssl_Data *pd, const char *address)
{
   return efl_net_server_serve(pd->server, address);
}

EOLIAN static const char *
_efl_net_server_ssl_efl_net_server_address_get(Eo *o EINA_UNUSED, Efl_Net_Server_Ssl_Data *pd)
{
   return efl_net_server_address_get(pd->server);
}

EOLIAN static unsigned int
_efl_net_server_ssl_efl_net_server_clients_count_get(Eo *o EINA_UNUSED, Efl_Net_Server_Ssl_Data *pd)
{
   return efl_net_server_clients_count_get(pd->server);
}

EOLIAN static void
_efl_net_server_ssl_efl_net_server_clients_limit_set(Eo *o EINA_UNUSED, Efl_Net_Server_Ssl_Data *pd, unsigned int limit, Eina_Bool reject_excess)
{
   efl_net_server_clients_limit_set(pd->server, limit, reject_excess);
}

EOLIAN static void
_efl_net_server_ssl_efl_net_server_clients_limit_get(Eo *o EINA_UNUSED, Efl_Net_Server_Ssl_Data *pd, unsigned int *limit, Eina_Bool *reject_excess)
{
   efl_net_server_clients_limit_get(pd->server, limit, reject_excess);
}

EOLIAN static Eina_Bool
_efl_net_server_ssl_efl_net_server_serving_get(Eo *o EINA_UNUSED, Efl_Net_Server_Ssl_Data *pd)
{
   return efl_net_server_serving_get(pd->server);
}

EOLIAN int
_efl_net_server_ssl_family_get(Eo *o EINA_UNUSED, Efl_Net_Server_Ssl_Data *pd)
{
   return efl_net_server_fd_family_get(pd->server);
}

EOLIAN Eina_Bool
_efl_net_server_ssl_close_on_exec_set(Eo *o EINA_UNUSED, Efl_Net_Server_Ssl_Data *pd, Eina_Bool close_on_exec)
{
   return efl_net_server_fd_close_on_exec_set(pd->server, close_on_exec);
}

EOLIAN Eina_Bool
_efl_net_server_ssl_close_on_exec_get(Eo *o EINA_UNUSED, Efl_Net_Server_Ssl_Data *pd)
{
   return efl_net_server_fd_close_on_exec_get(pd->server);
}

EOLIAN Eina_Bool
_efl_net_server_ssl_reuse_address_set(Eo *o EINA_UNUSED, Efl_Net_Server_Ssl_Data *pd, Eina_Bool reuse_address)
{
   return efl_net_server_fd_reuse_address_set(pd->server, reuse_address);
}

EOLIAN Eina_Bool
_efl_net_server_ssl_reuse_address_get(Eo *o EINA_UNUSED, Efl_Net_Server_Ssl_Data *pd)
{
   return efl_net_server_fd_reuse_address_get(pd->server);
}

EOLIAN Eina_Bool
_efl_net_server_ssl_reuse_port_set(Eo *o EINA_UNUSED, Efl_Net_Server_Ssl_Data *pd, Eina_Bool reuse_port)
{
   return efl_net_server_fd_reuse_port_set(pd->server, reuse_port);
}

EOLIAN Eina_Bool
_efl_net_server_ssl_reuse_port_get(Eo *o EINA_UNUSED, Efl_Net_Server_Ssl_Data *pd)
{
   return efl_net_server_fd_reuse_port_get(pd->server);
}

EOLIAN void
_efl_net_server_ssl_ipv6_only_set(Eo *o EINA_UNUSED, Efl_Net_Server_Ssl_Data *pd, Eina_Bool ipv6_only)
{
   efl_net_server_tcp_ipv6_only_set(pd->server, ipv6_only);
}

EOLIAN Eina_Bool
_efl_net_server_ssl_ipv6_only_get(Eo *o EINA_UNUSED, Efl_Net_Server_Ssl_Data *pd)
{
   return efl_net_server_tcp_ipv6_only_get(pd->server);
}


#include "efl_net_server_ssl.eo.c"
