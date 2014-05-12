/*
     This file is part of GNUnet.
     (C) 

     GNUnet is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published
     by the Free Software Foundation; either version 3, or (at your
     option) any later version.

     GNUnet is distributed in the hope that it will be useful, but
     WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with GNUnet; see the file COPYING.  If not, write to the
     Free Software Foundation, Inc., 59 Temple Place - Suite 330,
     Boston, MA 02111-1307, USA.
*/

/**
 * @file peerstore/gnunet-service-peerstore.c
 * @brief peerstore service implementation
 * @author Omar Tarabai
 */
#include "platform.h"
#include "gnunet_util_lib.h"
#include "peerstore.h"
#include "gnunet_peerstore_plugin.h"

//TODO: GNUNET_SERVER_receive_done() ?
//TODO: implement value lifetime

/**
 * Our configuration.
 */
static const struct GNUNET_CONFIGURATION_Handle *cfg;

/**
 * Database plugin library name
 */
char *db_lib_name;

/**
 * Database handle
 */
static struct GNUNET_PEERSTORE_PluginFunctions *db;

/**
 * Task run during shutdown.
 *
 * @param cls unused
 * @param tc unused
 */
static void
shutdown_task (void *cls,
	       const struct GNUNET_SCHEDULER_TaskContext *tc)
{
  if(NULL != db_lib_name)
  {
    GNUNET_break (NULL == GNUNET_PLUGIN_unload (db_lib_name, db));
    GNUNET_free (db_lib_name);
    db_lib_name = NULL;
  }
}


/**
 * A client disconnected.  Remove all of its data structure entries.
 *
 * @param cls closure, NULL
 * @param client identification of the client
 */
static void
handle_client_disconnect (void *cls,
			  struct GNUNET_SERVER_Client
			  * client)
{
}

/**
 * Handle a store request from client
 *
 * @param cls unused
 * @param client identification of the client
 * @param message the actual message
 */
void handle_store (void *cls,
    struct GNUNET_SERVER_Client *client,
    const struct GNUNET_MessageHeader *message)
{
  struct StoreRequestMessage *req;
  uint16_t msg_size;
  uint16_t sub_system_size;
  uint16_t value_size;
  char *sub_system;
  void *value;
  struct GNUNET_SERVER_TransmitContext *tc;
  struct StoreResponseMessage *res;
  char *emsg;
  size_t emsg_size = 0;
  char *emsg_dest;

  msg_size = ntohs(message->size);
  if(msg_size < sizeof(struct StoreRequestMessage))
  {
    GNUNET_break(0);
    GNUNET_SERVER_receive_done(client, GNUNET_SYSERR);
    return;
  }
  req = (struct StoreRequestMessage *)message;
  sub_system_size = ntohs(req->sub_system_size);
  value_size = ntohs(req->value_size);
  if(sub_system_size + value_size + sizeof(struct StoreRequestMessage)
      != msg_size)
  {
    GNUNET_break(0);
    GNUNET_SERVER_receive_done(client, GNUNET_SYSERR);
    return;
  }
  sub_system = (char *)&req[1];
  value = sub_system + sub_system_size;
  GNUNET_log(GNUNET_ERROR_TYPE_INFO, "Received a store request (size: %lu) for sub system `%s' and peer `%s'\n",
      msg_size,
      sub_system,
      GNUNET_i2s (&req->peer));
  if(GNUNET_OK != db->store_record(db->cls,
      &req->peer,
     sub_system,
     value,
     value_size))
  {
    GNUNET_log(GNUNET_ERROR_TYPE_ERROR, "Failed to store requested value, sqlite database error.");
    emsg = _("Failed to store requested value, sqlite database error.");
    emsg_size = strlen(emsg) + 1;
  }
  res = GNUNET_malloc(sizeof(struct StoreResponseMessage) + emsg_size);
  res->emsg_size = htons(emsg_size);
  res->header.size = htons(sizeof(struct StoreResponseMessage) + emsg_size);
  res->header.type = htons(GNUNET_MESSAGE_TYPE_PEERSTORE_STORE_RESULT);
  if(emsg_size > 0)
  {
    res->success = htons(GNUNET_NO);
    emsg_dest = (char *)&res[1];
    memcpy(emsg_dest, emsg, emsg_size);
  }
  else
    res->success = htons(GNUNET_YES);
  tc = GNUNET_SERVER_transmit_context_create (client);
  GNUNET_SERVER_transmit_context_append_message(tc, (struct GNUNET_MessageHeader *)res);
  GNUNET_free(res);
  GNUNET_SERVER_transmit_context_run (tc, GNUNET_TIME_UNIT_FOREVER_REL);

}

/**
 * Peerstore service runner.
 *
 * @param cls closure
 * @param server the initialized server
 * @param c configuration to use
 */
static void
run (void *cls,
     struct GNUNET_SERVER_Handle *server,
     const struct GNUNET_CONFIGURATION_Handle *c)
{
  static const struct GNUNET_SERVER_MessageHandler handlers[] = {
      {&handle_store, NULL, GNUNET_MESSAGE_TYPE_PEERSTORE_STORE, 0},
      {NULL, NULL, 0, 0}
  };
  char *database;

  cfg = c;
  if (GNUNET_OK !=
        GNUNET_CONFIGURATION_get_value_string (cfg, "peerstore", "DATABASE",
                                               &database))
    GNUNET_log (GNUNET_ERROR_TYPE_ERROR, "No database backend configured\n");

  else
  {
    GNUNET_asprintf (&db_lib_name, "libgnunet_plugin_peerstore_%s", database);
    db = GNUNET_PLUGIN_load(db_lib_name, (void *) cfg);
    GNUNET_free(database);
  }
  if(NULL == db)
	  GNUNET_log(GNUNET_ERROR_TYPE_ERROR, "Could not load database backend `%s'\n", db_lib_name);
  else
  {
    GNUNET_SERVER_add_handlers (server, handlers);
    GNUNET_SERVER_disconnect_notify (server,
             &handle_client_disconnect,
             NULL);
  }
  GNUNET_SCHEDULER_add_delayed (GNUNET_TIME_UNIT_FOREVER_REL,
				&shutdown_task,
				NULL);
}


/**
 * The main function for the peerstore service.
 *
 * @param argc number of arguments from the command line
 * @param argv command line arguments
 * @return 0 ok, 1 on error
 */
int
main (int argc, char *const *argv)
{
  return (GNUNET_OK ==
          GNUNET_SERVICE_run (argc,
                              argv,
                              "peerstore",
			      GNUNET_SERVICE_OPTION_NONE,
			      &run, NULL)) ? 0 : 1;
}

/* end of gnunet-service-peerstore.c */