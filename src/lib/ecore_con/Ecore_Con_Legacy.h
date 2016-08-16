#ifndef _ECORE_CON_LEGACY_H
#define _ECORE_CON_LEGACY_H
#include <Eina.h>
#include <Eo.h>

#include "efl_network.eo.legacy.h"
#include "efl_network_server.eo.legacy.h"
#include "efl_network_connector.eo.legacy.h"
#include "efl_network_client.eo.legacy.h"


/********************************************************************
 * ecore_con_base.eo.h
 *******************************************************************/
typedef Eo Ecore_Con_Base;


/********************************************************************
 * ecore_con_client.eo.h
 *******************************************************************/
typedef Eo Ecore_Con_Client;


/********************************************************************
 * ecore_con_server.eo.h
 *******************************************************************/
typedef Eo Ecore_Con_Server;


/********************************************************************
 * ecore_con_url.eo.h
 *******************************************************************/
typedef Eo Ecore_Con_Url;


/********************************************************************
 * ecore_con_url.eo.legacy.h
 *******************************************************************/
/**
 * * Controls the URL to send the request to.
 * @param[in] url The URL
 */
EAPI Eina_Bool ecore_con_url_url_set(Ecore_Con_Url *obj, const char *url);

/**
 * * Controls the URL to send the request to.
 */
EAPI const char *ecore_con_url_url_get(const Ecore_Con_Url *obj);

#endif
