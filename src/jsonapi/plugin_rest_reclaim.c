/*
   This file is part of GNUnet.
   Copyright (C) 2012-2015 GNUnet e.V.

   GNUnet is free software: you can redistribute it and/or modify it
   under the terms of the GNU Affero General Public License as published
   by the Free Software Foundation, either version 3 of the License,
   or (at your option) any later version.

   GNUnet is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Affero General Public License for more details.
  
   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
   */
/**
 * @author Martin Schanzenbach
 * @author Philippe Buschmann
 * @file reclaim/plugin_rest_reclaim.c
 * @brief GNUnet reclaim REST plugin
 *
 */

#include "platform.h"
#include "gnunet_rest_plugin.h"
#include "gnunet_identity_service.h"
#include "gnunet_gns_service.h"
#include "gnunet_gnsrecord_lib.h"
#include "gnunet_namestore_service.h"
#include "gnunet_rest_lib.h"
#include "gnunet_jsonapi_lib.h"
#include "gnunet_jsonapi_util.h"
#include "microhttpd.h"
#include <jansson.h>
#include <inttypes.h>
#include "gnunet_signatures.h"
#include "gnunet_reclaim_attribute_lib.h"
#include "gnunet_reclaim_service.h"

/**
 * REST root namespace
 */
#define GNUNET_REST_API_NS_RECLAIM "/reclaim"

/**
 * Attribute namespace
 */
#define GNUNET_REST_API_NS_RECLAIM_ATTRIBUTES "/reclaim/attributes"

/**
 * Ticket namespace
 */
#define GNUNET_REST_API_NS_IDENTITY_TICKETS "/reclaim/tickets"

/**
 * Revoke namespace
 */
#define GNUNET_REST_API_NS_IDENTITY_REVOKE "/reclaim/revoke"

/**
 * Revoke namespace
 */
#define GNUNET_REST_API_NS_IDENTITY_CONSUME "/reclaim/consume"

/**
 * Attribute key
 */
#define GNUNET_REST_JSONAPI_RECLAIM_ATTRIBUTE "attribute"

/**
 * Ticket key
 */
#define GNUNET_REST_JSONAPI_IDENTITY_TICKET "ticket"


/**
 * Value key
 */
#define GNUNET_REST_JSONAPI_RECLAIM_ATTRIBUTE_VALUE "value"

/**
 * State while collecting all egos
 */
#define ID_REST_STATE_INIT 0

/**
 * Done collecting egos
 */
#define ID_REST_STATE_POST_INIT 1

/**
 * The configuration handle
 */
const struct GNUNET_CONFIGURATION_Handle *cfg;

/**
 * HTTP methods allows for this plugin
 */
static char* allow_methods;

/**
 * @brief struct returned by the initialization function of the plugin
 */
struct Plugin
{
  const struct GNUNET_CONFIGURATION_Handle *cfg;
};

/**
 * The ego list
 */
struct EgoEntry
{
  /**
   * DLL
   */
  struct EgoEntry *next;

  /**
   * DLL
   */
  struct EgoEntry *prev;

  /**
   * Ego Identifier
   */
  char *identifier;

  /**
   * Public key string
   */
  char *keystring;

  /**
   * The Ego
   */
  struct GNUNET_IDENTITY_Ego *ego;
};


struct RequestHandle
{
  /**
   * Ego list
   */
  struct EgoEntry *ego_head;

  /**
   * Ego list
   */
  struct EgoEntry *ego_tail;

  /**
   * Selected ego
   */
  struct EgoEntry *ego_entry;

  /**
   * Pointer to ego private key
   */
  struct GNUNET_CRYPTO_EcdsaPrivateKey priv_key;

  /**
   * The processing state
   */
  int state;

  /**
   * Handle to Identity service.
   */
  struct GNUNET_IDENTITY_Handle *identity_handle;

  /**
   * Rest connection
   */
  struct GNUNET_REST_RequestHandle *rest_handle;

  /**
   * Handle to NAMESTORE
   */
  struct GNUNET_NAMESTORE_Handle *namestore_handle;

  /**
   * Iterator for NAMESTORE
   */
  struct GNUNET_NAMESTORE_ZoneIterator *namestore_handle_it;

  /**
   * Attribute claim list
   */
  struct GNUNET_RECLAIM_ATTRIBUTE_ClaimList *attr_list;

  /**
   * IDENTITY Operation
   */
  struct GNUNET_IDENTITY_Operation *op;

  /**
   * Identity Provider
   */
  struct GNUNET_RECLAIM_Handle *idp;

  /**
   * Idp Operation
   */
  struct GNUNET_RECLAIM_Operation *idp_op;

  /**
   * Attribute iterator
   */
  struct GNUNET_RECLAIM_AttributeIterator *attr_it;

  /**
   * Ticket iterator
   */
  struct GNUNET_RECLAIM_TicketIterator *ticket_it;

  /**
   * A ticket
   */
  struct GNUNET_RECLAIM_Ticket ticket;

  /**
   * Desired timeout for the lookup (default is no timeout).
   */
  struct GNUNET_TIME_Relative timeout;

  /**
   * ID of a task associated with the resolution process.
   */
  struct GNUNET_SCHEDULER_Task *timeout_task;

  /**
   * The plugin result processor
   */
  GNUNET_REST_ResultProcessor proc;

  /**
   * The closure of the result processor
   */
  void *proc_cls;

  /**
   * The url
   */
  char *url;

  /**
   * Error response message
   */
  char *emsg;

  /**
   * Reponse code
   */
  int response_code;

  /**
   * Response object
   */
  struct GNUNET_JSONAPI_Document *resp_object;

};

/**
 * Cleanup lookup handle
 * @param handle Handle to clean up
 */
static void
cleanup_handle (struct RequestHandle *handle)
{
  struct GNUNET_RECLAIM_ATTRIBUTE_ClaimListEntry *claim_entry;
  struct GNUNET_RECLAIM_ATTRIBUTE_ClaimListEntry *claim_tmp;
  struct EgoEntry *ego_entry;
  struct EgoEntry *ego_tmp;
  GNUNET_log (GNUNET_ERROR_TYPE_DEBUG,
              "Cleaning up\n");
  if (NULL != handle->resp_object)
    GNUNET_JSONAPI_document_delete (handle->resp_object);
  if (NULL != handle->timeout_task)
    GNUNET_SCHEDULER_cancel (handle->timeout_task);
  if (NULL != handle->identity_handle)
    GNUNET_IDENTITY_disconnect (handle->identity_handle);
  if (NULL != handle->attr_it)
    GNUNET_RECLAIM_get_attributes_stop (handle->attr_it);
  if (NULL != handle->ticket_it)
    GNUNET_RECLAIM_ticket_iteration_stop (handle->ticket_it);
  if (NULL != handle->idp)
    GNUNET_RECLAIM_disconnect (handle->idp);
  if (NULL != handle->url)
    GNUNET_free (handle->url);
  if (NULL != handle->emsg)
    GNUNET_free (handle->emsg);
  if (NULL != handle->namestore_handle)
    GNUNET_NAMESTORE_disconnect (handle->namestore_handle);
  if ( NULL != handle->attr_list )
  {
    for (claim_entry = handle->attr_list->list_head;
    NULL != claim_entry;)
    {
      claim_tmp = claim_entry;
      claim_entry = claim_entry->next;
      GNUNET_free(claim_tmp->claim);
      GNUNET_free(claim_tmp);
    }
    GNUNET_free (handle->attr_list);
  }
  for (ego_entry = handle->ego_head;
       NULL != ego_entry;)
  {
    ego_tmp = ego_entry;
    ego_entry = ego_entry->next;
    GNUNET_free (ego_tmp->identifier);
    GNUNET_free (ego_tmp->keystring);
    GNUNET_free (ego_tmp);
  }
  if (NULL != handle->attr_it)
  {
    GNUNET_free(handle->attr_it);
  }
  GNUNET_free (handle);
}

static void
cleanup_handle_delayed (void *cls)
{
  cleanup_handle (cls);
}


/**
 * Task run on error, sends error message.  Cleans up everything.
 *
 * @param cls the `struct RequestHandle`
 */
static void
do_error (void *cls)
{
  struct RequestHandle *handle = cls;
  struct MHD_Response *resp;
  char *json_error;

  GNUNET_asprintf (&json_error, "{ \"error\" : \"%s\" }",
		   handle->emsg);
  if ( 0 == handle->response_code )
  {
    handle->response_code = MHD_HTTP_BAD_REQUEST;
  }
  resp = GNUNET_REST_create_response (json_error);
  MHD_add_response_header (resp, "Content-Type", "application/json");
  handle->proc (handle->proc_cls, resp, handle->response_code);
  GNUNET_SCHEDULER_add_now (&cleanup_handle_delayed, handle);
  GNUNET_free (json_error);
}


/**
 * Task run on timeout, sends error message.  Cleans up everything.
 *
 * @param cls the `struct RequestHandle`
 */
static void
do_timeout (void *cls)
{
  struct RequestHandle *handle = cls;

  handle->timeout_task = NULL;
  do_error (handle);
}


static void
collect_error_cb (void *cls)
{
  struct RequestHandle *handle = cls;

  do_error (handle);
}

static void
finished_cont (void *cls,
               int32_t success,
               const char *emsg)
{
  struct RequestHandle *handle = cls;
  struct MHD_Response *resp;

  resp = GNUNET_REST_create_response (emsg);
  if (GNUNET_OK != success)
  {
    GNUNET_SCHEDULER_add_now (&do_error, handle);
    return;
  }
  handle->proc (handle->proc_cls, resp, MHD_HTTP_OK);
  GNUNET_SCHEDULER_add_now (&cleanup_handle_delayed, handle);
}


/**
 * Return attributes for identity
 *
 * @param cls the request handle
 */
static void
return_response (void *cls)
{
  char* result_str;
  struct RequestHandle *handle = cls;
  struct MHD_Response *resp;

  GNUNET_JSONAPI_document_serialize (handle->resp_object, &result_str);
  GNUNET_log (GNUNET_ERROR_TYPE_DEBUG, "Result %s\n", result_str);
  resp = GNUNET_REST_create_response (result_str);
  handle->proc (handle->proc_cls, resp, MHD_HTTP_OK);
  GNUNET_free (result_str);
  cleanup_handle (handle);
}

static void
collect_finished_cb (void *cls)
{
  struct RequestHandle *handle = cls;
  //Done
  handle->attr_it = NULL;
  handle->ticket_it = NULL;
  GNUNET_SCHEDULER_add_now (&return_response, handle);
}


/**
 * Collect all attributes for an ego
 *
 */
static void
ticket_collect (void *cls,
                const struct GNUNET_RECLAIM_Ticket *ticket)
{
  struct GNUNET_JSONAPI_Resource *json_resource;
  struct RequestHandle *handle = cls;
  json_t *value;
  char* tmp;

  GNUNET_log (GNUNET_ERROR_TYPE_DEBUG, "Adding ticket\n");
  tmp = GNUNET_STRINGS_data_to_string_alloc (&ticket->rnd,
                                             sizeof (uint64_t));
  json_resource = GNUNET_JSONAPI_resource_new (GNUNET_REST_JSONAPI_IDENTITY_TICKET,
                                                       tmp);
  GNUNET_free (tmp);
  GNUNET_JSONAPI_document_resource_add (handle->resp_object, json_resource);

  tmp = GNUNET_STRINGS_data_to_string_alloc (&ticket->identity,
                                             sizeof (struct GNUNET_CRYPTO_EcdsaPublicKey));
  value = json_string (tmp);
  GNUNET_JSONAPI_resource_add_attr (json_resource,
                                    "issuer",
                                    value);
  GNUNET_free (tmp);
  json_decref (value);
  tmp = GNUNET_STRINGS_data_to_string_alloc (&ticket->audience,
                                             sizeof (struct GNUNET_CRYPTO_EcdsaPublicKey));
  value = json_string (tmp);
  GNUNET_JSONAPI_resource_add_attr (json_resource,
                                    "audience",
                                    value);
  GNUNET_free (tmp);
  json_decref (value);
  tmp = GNUNET_STRINGS_data_to_string_alloc (&ticket->rnd,
                                             sizeof (uint64_t));
  value = json_string (tmp);
  GNUNET_JSONAPI_resource_add_attr (json_resource,
                                    "rnd",
                                    value);
  GNUNET_free (tmp);
  json_decref (value);
  GNUNET_RECLAIM_ticket_iteration_next (handle->ticket_it);
}



/**
 * List tickets for identity request
 *
 * @param con_handle the connection handle
 * @param url the url
 * @param cls the RequestHandle
 */
static void
list_tickets_cont (struct GNUNET_REST_RequestHandle *con_handle,
                   const char* url,
                   void *cls)
{
  const struct GNUNET_CRYPTO_EcdsaPrivateKey *priv_key;
  struct RequestHandle *handle = cls;
  struct EgoEntry *ego_entry;
  char *identity;

  GNUNET_log (GNUNET_ERROR_TYPE_DEBUG, "Getting tickets for %s.\n",
              handle->url);
  if ( strlen (GNUNET_REST_API_NS_IDENTITY_TICKETS) >=
       strlen (handle->url))
  {
    GNUNET_log (GNUNET_ERROR_TYPE_ERROR, "No identity given.\n");
    GNUNET_SCHEDULER_add_now (&do_error, handle);
    return;
  }
  identity = handle->url + strlen (GNUNET_REST_API_NS_IDENTITY_TICKETS) + 1;

  for (ego_entry = handle->ego_head;
       NULL != ego_entry;
       ego_entry = ego_entry->next)
    if (0 == strcmp (identity, ego_entry->identifier))
      break;
  handle->resp_object = GNUNET_JSONAPI_document_new ();

  if (NULL == ego_entry)
  {
    //Done
    GNUNET_log (GNUNET_ERROR_TYPE_DEBUG, "Ego %s not found.\n",
                identity);
    GNUNET_SCHEDULER_add_now (&return_response, handle);
    return;
  }
  priv_key = GNUNET_IDENTITY_ego_get_private_key (ego_entry->ego);
  handle->idp = GNUNET_RECLAIM_connect (cfg);
  handle->ticket_it = GNUNET_RECLAIM_ticket_iteration_start (handle->idp,
                                                                       priv_key,
                                                                       &collect_error_cb,
                                                                       handle,
                                                                       &ticket_collect,
                                                                       handle,
                                                                       &collect_finished_cb,
                                                                       handle);
}


static void
add_attribute_cont (struct GNUNET_REST_RequestHandle *con_handle,
                    const char* url,
                    void *cls)
{
  const struct GNUNET_CRYPTO_EcdsaPrivateKey *identity_priv;
  const char* identity;
  const char* name_str;
  const char* value_str;
  const char* exp_str;

  struct RequestHandle *handle = cls;
  struct EgoEntry *ego_entry;
  struct MHD_Response *resp;
  struct GNUNET_RECLAIM_ATTRIBUTE_Claim *attribute;
  struct GNUNET_JSONAPI_Document *json_obj;
  struct GNUNET_JSONAPI_Resource *json_res;
  struct GNUNET_TIME_Relative exp;
  char term_data[handle->rest_handle->data_size+1];
  json_t *value_json;
  json_t *data_json;
  json_t *exp_json;
  json_error_t err;
  struct GNUNET_JSON_Specification docspec[] = {
    GNUNET_JSON_spec_jsonapi_document (&json_obj),
    GNUNET_JSON_spec_end()
  };

  GNUNET_log (GNUNET_ERROR_TYPE_DEBUG, "Adding an attribute for %s.\n",
              handle->url);
  if ( strlen (GNUNET_REST_API_NS_RECLAIM_ATTRIBUTES) >=
       strlen (handle->url))
  {
    GNUNET_log (GNUNET_ERROR_TYPE_ERROR, "No identity given.\n");
    GNUNET_SCHEDULER_add_now (&do_error, handle);
    return;
  }
  identity = handle->url + strlen (GNUNET_REST_API_NS_RECLAIM_ATTRIBUTES) + 1;

  for (ego_entry = handle->ego_head;
       NULL != ego_entry;
       ego_entry = ego_entry->next)
    if (0 == strcmp (identity, ego_entry->identifier))
      break;

  if (NULL == ego_entry)
  {
    GNUNET_log (GNUNET_ERROR_TYPE_ERROR,
                "Identity unknown (%s)\n", identity);
    GNUNET_JSONAPI_document_delete (json_obj);
    return;
  }
  identity_priv = GNUNET_IDENTITY_ego_get_private_key (ego_entry->ego);

  if (0 >= handle->rest_handle->data_size)
  {
    GNUNET_SCHEDULER_add_now (&do_error, handle);
    return;
  }

  term_data[handle->rest_handle->data_size] = '\0';
  GNUNET_memcpy (term_data,
                 handle->rest_handle->data,
                 handle->rest_handle->data_size);
  data_json = json_loads (term_data,
                          JSON_DECODE_ANY,
                          &err);
  GNUNET_assert (GNUNET_OK ==
                 GNUNET_JSON_parse (data_json, docspec,
                                    NULL, NULL));
  json_decref (data_json);
  if (NULL == json_obj)
  {
    GNUNET_log (GNUNET_ERROR_TYPE_ERROR,
                "Unable to parse JSONAPI Object from %s\n",
                term_data);
    GNUNET_SCHEDULER_add_now (&do_error, handle);
    return;
  }
  if (1 != GNUNET_JSONAPI_document_resource_count (json_obj))
  {
    GNUNET_log (GNUNET_ERROR_TYPE_ERROR,
                "Cannot create more than 1 resource! (Got %d)\n",
                GNUNET_JSONAPI_document_resource_count (json_obj));
    GNUNET_JSONAPI_document_delete (json_obj);
    GNUNET_SCHEDULER_add_now (&do_error, handle);
    return;
  }
  json_res = GNUNET_JSONAPI_document_get_resource (json_obj, 0);
  if (GNUNET_NO == GNUNET_JSONAPI_resource_check_type (json_res,
                                                       GNUNET_REST_JSONAPI_RECLAIM_ATTRIBUTE))
  {
    GNUNET_log (GNUNET_ERROR_TYPE_ERROR,
                "Unsupported JSON data type\n");
    GNUNET_JSONAPI_document_delete (json_obj);
    resp = GNUNET_REST_create_response (NULL);
    handle->proc (handle->proc_cls, resp, MHD_HTTP_CONFLICT);
    cleanup_handle (handle);
    return;
  }
  name_str = GNUNET_JSONAPI_resource_get_id (json_res);
  exp_json = GNUNET_JSONAPI_resource_read_attr (json_res,
                                                "exp");
  exp_str = json_string_value (exp_json);
  if (NULL == exp_str) {
    exp = GNUNET_TIME_UNIT_HOURS;
  } else {
    if (GNUNET_OK != GNUNET_STRINGS_fancy_time_to_relative (exp_str,
                                           &exp)) {
      exp = GNUNET_TIME_UNIT_HOURS;
    }
  }

  value_json = GNUNET_JSONAPI_resource_read_attr (json_res,
                                                  "value");
  value_str = json_string_value (value_json);
  attribute = GNUNET_RECLAIM_ATTRIBUTE_claim_new (name_str,
                                                      GNUNET_RECLAIM_ATTRIBUTE_TYPE_STRING,
                                                      value_str,
                                                      strlen (value_str) + 1);
  handle->idp = GNUNET_RECLAIM_connect (cfg);
  handle->idp_op = GNUNET_RECLAIM_attribute_store (handle->idp,
                                                             identity_priv,
                                                             attribute,
                                                             &exp,
                                                             &finished_cont,
                                                             handle);
  GNUNET_free (attribute);
  GNUNET_JSONAPI_document_delete (json_obj);
}



/**
 * Collect all attributes for an ego
 *
 */
static void
attr_collect (void *cls,
              const struct GNUNET_CRYPTO_EcdsaPublicKey *identity,
              const struct GNUNET_RECLAIM_ATTRIBUTE_Claim *attr)
{
  struct GNUNET_JSONAPI_Resource *json_resource;
  struct RequestHandle *handle = cls;
  json_t *value;
  char* tmp_value;
  
  if ((NULL == attr->name) || (NULL == attr->data))
  {
    GNUNET_RECLAIM_get_attributes_next (handle->attr_it);
    return;
  }

  GNUNET_log (GNUNET_ERROR_TYPE_DEBUG, "Adding attribute: %s\n",
              attr->name);
  json_resource = GNUNET_JSONAPI_resource_new (GNUNET_REST_JSONAPI_RECLAIM_ATTRIBUTE,
                                               attr->name);
  GNUNET_JSONAPI_document_resource_add (handle->resp_object, json_resource);

  tmp_value = GNUNET_RECLAIM_ATTRIBUTE_value_to_string (attr->type,
                                           attr->data,
                                           attr->data_size);

  value = json_string (tmp_value);

  GNUNET_JSONAPI_resource_add_attr (json_resource,
                                    "value",
                                    value);
  json_decref (value);
  GNUNET_free(tmp_value);
  GNUNET_RECLAIM_get_attributes_next (handle->attr_it);
}



/**
 * List attributes for identity request
 *
 * @param con_handle the connection handle
 * @param url the url
 * @param cls the RequestHandle
 */
static void
list_attribute_cont (struct GNUNET_REST_RequestHandle *con_handle,
                     const char* url,
                     void *cls)
{
  const struct GNUNET_CRYPTO_EcdsaPrivateKey *priv_key;
  struct RequestHandle *handle = cls;
  struct EgoEntry *ego_entry;
  char *identity;

  GNUNET_log (GNUNET_ERROR_TYPE_DEBUG, "Getting attributes for %s.\n",
              handle->url);
  if ( strlen (GNUNET_REST_API_NS_RECLAIM_ATTRIBUTES) >=
       strlen (handle->url))
  {
    GNUNET_log (GNUNET_ERROR_TYPE_ERROR, "No identity given.\n");
    GNUNET_SCHEDULER_add_now (&do_error, handle);
    return;
  }
  identity = handle->url + strlen (GNUNET_REST_API_NS_RECLAIM_ATTRIBUTES) + 1;

  for (ego_entry = handle->ego_head;
       NULL != ego_entry;
       ego_entry = ego_entry->next)
    if (0 == strcmp (identity, ego_entry->identifier))
      break;
  handle->resp_object = GNUNET_JSONAPI_document_new ();


  if (NULL == ego_entry)
  {
    //Done
    GNUNET_log (GNUNET_ERROR_TYPE_DEBUG, "Ego %s not found.\n",
                identity);
    GNUNET_SCHEDULER_add_now (&return_response, handle);
    return;
  }
  priv_key = GNUNET_IDENTITY_ego_get_private_key (ego_entry->ego);
  handle->idp = GNUNET_RECLAIM_connect (cfg);
  handle->attr_it = GNUNET_RECLAIM_get_attributes_start (handle->idp,
                                                                   priv_key,
                                                                   &collect_error_cb,
                                                                   handle,
                                                                   &attr_collect,
                                                                   handle,
                                                                   &collect_finished_cb,
                                                                   handle);
}


static void
revoke_ticket_cont (struct GNUNET_REST_RequestHandle *con_handle,
                    const char* url,
                    void *cls)
{
  const struct GNUNET_CRYPTO_EcdsaPrivateKey *identity_priv;
  const char* identity_str;
  const char* audience_str;
  const char* rnd_str;

  struct RequestHandle *handle = cls;
  struct EgoEntry *ego_entry;
  struct MHD_Response *resp;
  struct GNUNET_RECLAIM_Ticket ticket;
  struct GNUNET_JSONAPI_Document *json_obj;
  struct GNUNET_JSONAPI_Resource *json_res;
  struct GNUNET_CRYPTO_EcdsaPublicKey tmp_pk;
  char term_data[handle->rest_handle->data_size+1];
  json_t *rnd_json;
  json_t *identity_json;
  json_t *audience_json;
  json_t *data_json;
  json_error_t err;
  struct GNUNET_JSON_Specification docspec[] = {
    GNUNET_JSON_spec_jsonapi_document (&json_obj),
    GNUNET_JSON_spec_end()
  };

  if (0 >= handle->rest_handle->data_size)
  {
    GNUNET_SCHEDULER_add_now (&do_error, handle);
    return;
  }

  term_data[handle->rest_handle->data_size] = '\0';
  GNUNET_memcpy (term_data,
                 handle->rest_handle->data,
                 handle->rest_handle->data_size);
  data_json = json_loads (term_data,
                          JSON_DECODE_ANY,
                          &err);
  GNUNET_assert (GNUNET_OK ==
                 GNUNET_JSON_parse (data_json, docspec,
                                    NULL, NULL));
  json_decref (data_json);
  if (NULL == json_obj)
  {
    GNUNET_log (GNUNET_ERROR_TYPE_ERROR,
                "Unable to parse JSONAPI Object from %s\n",
                term_data);
    GNUNET_SCHEDULER_add_now (&do_error, handle);
    return;
  }
  if (1 != GNUNET_JSONAPI_document_resource_count (json_obj))
  {
    GNUNET_log (GNUNET_ERROR_TYPE_ERROR,
                "Cannot create more than 1 resource! (Got %d)\n",
                GNUNET_JSONAPI_document_resource_count (json_obj));
    GNUNET_JSONAPI_document_delete (json_obj);
    GNUNET_SCHEDULER_add_now (&do_error, handle);
    return;
  }
  json_res = GNUNET_JSONAPI_document_get_resource (json_obj, 0);
  if (GNUNET_NO == GNUNET_JSONAPI_resource_check_type (json_res,
                                                       GNUNET_REST_JSONAPI_IDENTITY_TICKET))
  {
    GNUNET_log (GNUNET_ERROR_TYPE_ERROR,
                "Unsupported JSON data type\n");
    GNUNET_JSONAPI_document_delete (json_obj);
    resp = GNUNET_REST_create_response (NULL);
    handle->proc (handle->proc_cls, resp, MHD_HTTP_CONFLICT);
    cleanup_handle (handle);
    return;
  }
  rnd_json = GNUNET_JSONAPI_resource_read_attr (json_res,
                                                "rnd");
  identity_json = GNUNET_JSONAPI_resource_read_attr (json_res,
                                                     "issuer");
  audience_json = GNUNET_JSONAPI_resource_read_attr (json_res,
                                                     "audience");
  rnd_str = json_string_value (rnd_json);
  identity_str = json_string_value (identity_json);
  audience_str = json_string_value (audience_json);

  GNUNET_STRINGS_string_to_data (rnd_str,
                                 strlen (rnd_str),
                                 &ticket.rnd,
                                 sizeof (uint64_t));
  GNUNET_STRINGS_string_to_data (identity_str,
                                 strlen (identity_str),
                                 &ticket.identity,
                                 sizeof (struct GNUNET_CRYPTO_EcdsaPublicKey));
  GNUNET_STRINGS_string_to_data (audience_str,
                                 strlen (audience_str),
                                 &ticket.audience,
                                 sizeof (struct GNUNET_CRYPTO_EcdsaPublicKey));

  for (ego_entry = handle->ego_head;
       NULL != ego_entry;
       ego_entry = ego_entry->next)
  {
    GNUNET_IDENTITY_ego_get_public_key (ego_entry->ego,
                                        &tmp_pk);
    if (0 == memcmp (&ticket.identity,
                     &tmp_pk,
                     sizeof (struct GNUNET_CRYPTO_EcdsaPublicKey)))
      break;
  }
  if (NULL == ego_entry)
  {
    GNUNET_log (GNUNET_ERROR_TYPE_ERROR,
                "Identity unknown (%s)\n", identity_str);
    GNUNET_JSONAPI_document_delete (json_obj);
    return;
  }
  identity_priv = GNUNET_IDENTITY_ego_get_private_key (ego_entry->ego);

  handle->idp = GNUNET_RECLAIM_connect (cfg);
  handle->idp_op = GNUNET_RECLAIM_ticket_revoke (handle->idp,
                                                           identity_priv,
                                                           &ticket,
                                                           &finished_cont,
                                                           handle);
  GNUNET_JSONAPI_document_delete (json_obj);
}

static void
consume_cont (void *cls,
              const struct GNUNET_CRYPTO_EcdsaPublicKey *identity,
              const struct GNUNET_RECLAIM_ATTRIBUTE_Claim *attr)
{
  struct RequestHandle *handle = cls;
  struct GNUNET_JSONAPI_Resource *json_resource;
  json_t *value;

  if (NULL == identity)
  {
    GNUNET_SCHEDULER_add_now (&return_response, handle);
    return;
  }

  GNUNET_log (GNUNET_ERROR_TYPE_DEBUG, "Adding attribute: %s\n",
              attr->name);
  json_resource = GNUNET_JSONAPI_resource_new (GNUNET_REST_JSONAPI_RECLAIM_ATTRIBUTE,
                                               attr->name);
  GNUNET_JSONAPI_document_resource_add (handle->resp_object, json_resource);

  value = json_string (attr->data);
  GNUNET_JSONAPI_resource_add_attr (json_resource,
                                    "value",
                                    value);
  json_decref (value);
}

static void
consume_ticket_cont (struct GNUNET_REST_RequestHandle *con_handle,
                     const char* url,
                     void *cls)
{
  const struct GNUNET_CRYPTO_EcdsaPrivateKey *identity_priv;
  const char* identity_str;
  const char* audience_str;
  const char* rnd_str;

  struct RequestHandle *handle = cls;
  struct EgoEntry *ego_entry;
  struct MHD_Response *resp;
  struct GNUNET_RECLAIM_Ticket ticket;
  struct GNUNET_JSONAPI_Document *json_obj;
  struct GNUNET_JSONAPI_Resource *json_res;
  struct GNUNET_CRYPTO_EcdsaPublicKey tmp_pk;
  char term_data[handle->rest_handle->data_size+1];
  json_t *rnd_json;
  json_t *identity_json;
  json_t *audience_json;
  json_t *data_json;
  json_error_t err;
  struct GNUNET_JSON_Specification docspec[] = {
    GNUNET_JSON_spec_jsonapi_document (&json_obj),
    GNUNET_JSON_spec_end()
  };

  if (0 >= handle->rest_handle->data_size)
  {
    GNUNET_SCHEDULER_add_now (&do_error, handle);
    return;
  }

  term_data[handle->rest_handle->data_size] = '\0';
  GNUNET_memcpy (term_data,
                 handle->rest_handle->data,
                 handle->rest_handle->data_size);
  data_json = json_loads (term_data,
                          JSON_DECODE_ANY,
                          &err);
  GNUNET_assert (GNUNET_OK ==
                 GNUNET_JSON_parse (data_json, docspec,
                                    NULL, NULL));
  json_decref (data_json);
  if (NULL == json_obj)
  {
    GNUNET_log (GNUNET_ERROR_TYPE_ERROR,
                "Unable to parse JSONAPI Object from %s\n",
                term_data);
    GNUNET_SCHEDULER_add_now (&do_error, handle);
    return;
  }
  if (1 != GNUNET_JSONAPI_document_resource_count (json_obj))
  {
    GNUNET_log (GNUNET_ERROR_TYPE_ERROR,
                "Cannot create more than 1 resource! (Got %d)\n",
                GNUNET_JSONAPI_document_resource_count (json_obj));
    GNUNET_JSONAPI_document_delete (json_obj);
    GNUNET_SCHEDULER_add_now (&do_error, handle);
    return;
  }
  json_res = GNUNET_JSONAPI_document_get_resource (json_obj, 0);
  if (GNUNET_NO == GNUNET_JSONAPI_resource_check_type (json_res,
                                                       GNUNET_REST_JSONAPI_IDENTITY_TICKET))
  {
    GNUNET_log (GNUNET_ERROR_TYPE_ERROR,
                "Unsupported JSON data type\n");
    GNUNET_JSONAPI_document_delete (json_obj);
    resp = GNUNET_REST_create_response (NULL);
    handle->proc (handle->proc_cls, resp, MHD_HTTP_CONFLICT);
    cleanup_handle (handle);
    return;
  }
  rnd_json = GNUNET_JSONAPI_resource_read_attr (json_res,
                                                "rnd");
  identity_json = GNUNET_JSONAPI_resource_read_attr (json_res,
                                                     "identity");
  audience_json = GNUNET_JSONAPI_resource_read_attr (json_res,
                                                     "audience");
  rnd_str = json_string_value (rnd_json);
  identity_str = json_string_value (identity_json);
  audience_str = json_string_value (audience_json);

  GNUNET_STRINGS_string_to_data (rnd_str,
                                 strlen (rnd_str),
                                 &ticket.rnd,
                                 sizeof (uint64_t));
  GNUNET_STRINGS_string_to_data (identity_str,
                                 strlen (identity_str),
                                 &ticket.identity,
                                 sizeof (struct GNUNET_CRYPTO_EcdsaPublicKey));
  GNUNET_STRINGS_string_to_data (audience_str,
                                 strlen (audience_str),
                                 &ticket.audience,
                                 sizeof (struct GNUNET_CRYPTO_EcdsaPublicKey));

  for (ego_entry = handle->ego_head;
       NULL != ego_entry;
       ego_entry = ego_entry->next)
  {
    GNUNET_IDENTITY_ego_get_public_key (ego_entry->ego,
                                        &tmp_pk);
    if (0 == memcmp (&ticket.audience,
                     &tmp_pk,
                     sizeof (struct GNUNET_CRYPTO_EcdsaPublicKey)))
      break;
  }
  if (NULL == ego_entry)
  {
    GNUNET_log (GNUNET_ERROR_TYPE_ERROR,
                "Identity unknown (%s)\n", identity_str);
    GNUNET_JSONAPI_document_delete (json_obj);
    return;
  }
  identity_priv = GNUNET_IDENTITY_ego_get_private_key (ego_entry->ego);
  handle->resp_object = GNUNET_JSONAPI_document_new ();
  handle->idp = GNUNET_RECLAIM_connect (cfg);
  handle->idp_op = GNUNET_RECLAIM_ticket_consume (handle->idp,
                                                            identity_priv,
                                                            &ticket,
                                                            &consume_cont,
                                                            handle);
  GNUNET_JSONAPI_document_delete (json_obj);
}



/**
 * Respond to OPTIONS request
 *
 * @param con_handle the connection handle
 * @param url the url
 * @param cls the RequestHandle
 */
static void
options_cont (struct GNUNET_REST_RequestHandle *con_handle,
              const char* url,
              void *cls)
{
  struct MHD_Response *resp;
  struct RequestHandle *handle = cls;

  //For now, independent of path return all options
  resp = GNUNET_REST_create_response (NULL);
  MHD_add_response_header (resp,
                           "Access-Control-Allow-Methods",
                           allow_methods);
  handle->proc (handle->proc_cls, resp, MHD_HTTP_OK);
  cleanup_handle (handle);
  return;
}

/**
 * Handle rest request
 *
 * @param handle the request handle
 */
static void
init_cont (struct RequestHandle *handle)
{
  struct GNUNET_REST_RequestHandlerError err;
  static const struct GNUNET_REST_RequestHandler handlers[] = {
    {MHD_HTTP_METHOD_GET, GNUNET_REST_API_NS_RECLAIM_ATTRIBUTES, &list_attribute_cont},
    {MHD_HTTP_METHOD_POST, GNUNET_REST_API_NS_RECLAIM_ATTRIBUTES, &add_attribute_cont},
    {MHD_HTTP_METHOD_GET, GNUNET_REST_API_NS_IDENTITY_TICKETS, &list_tickets_cont},
    {MHD_HTTP_METHOD_POST, GNUNET_REST_API_NS_IDENTITY_REVOKE, &revoke_ticket_cont},
    {MHD_HTTP_METHOD_POST, GNUNET_REST_API_NS_IDENTITY_CONSUME, &consume_ticket_cont},
    {MHD_HTTP_METHOD_OPTIONS, GNUNET_REST_API_NS_RECLAIM,
      &options_cont},
    GNUNET_REST_HANDLER_END
  };

  if (GNUNET_NO == GNUNET_REST_handle_request (handle->rest_handle,
                                               handlers,
                                               &err,
                                               handle))
  {
    handle->response_code = err.error_code;
    GNUNET_SCHEDULER_add_now (&do_error, handle);
  }
}

/**
 * If listing is enabled, prints information about the egos.
 *
 * This function is initially called for all egos and then again
 * whenever a ego's identifier changes or if it is deleted.  At the
 * end of the initial pass over all egos, the function is once called
 * with 'NULL' for 'ego'. That does NOT mean that the callback won't
 * be invoked in the future or that there was an error.
 *
 * When used with 'GNUNET_IDENTITY_create' or 'GNUNET_IDENTITY_get',
 * this function is only called ONCE, and 'NULL' being passed in
 * 'ego' does indicate an error (i.e. name is taken or no default
 * value is known).  If 'ego' is non-NULL and if '*ctx'
 * is set in those callbacks, the value WILL be passed to a subsequent
 * call to the identity callback of 'GNUNET_IDENTITY_connect' (if
 * that one was not NULL).
 *
 * When an identity is renamed, this function is called with the
 * (known) ego but the NEW identifier.
 *
 * When an identity is deleted, this function is called with the
 * (known) ego and "NULL" for the 'identifier'.  In this case,
 * the 'ego' is henceforth invalid (and the 'ctx' should also be
 * cleaned up).
 *
 * @param cls closure
 * @param ego ego handle
 * @param ctx context for application to store data for this ego
 *                 (during the lifetime of this process, initially NULL)
 * @param identifier identifier assigned by the user for this ego,
 *                   NULL if the user just deleted the ego and it
 *                   must thus no longer be used
 */
static void
list_ego (void *cls,
          struct GNUNET_IDENTITY_Ego *ego,
          void **ctx,
          const char *identifier)
{
  struct RequestHandle *handle = cls;
  struct EgoEntry *ego_entry;
  struct GNUNET_CRYPTO_EcdsaPublicKey pk;

  if ((NULL == ego) && (ID_REST_STATE_INIT == handle->state))
  {
    handle->state = ID_REST_STATE_POST_INIT;
    init_cont (handle);
    return;
  }
  if (ID_REST_STATE_INIT == handle->state) {
    ego_entry = GNUNET_new (struct EgoEntry);
    GNUNET_IDENTITY_ego_get_public_key (ego, &pk);
    ego_entry->keystring =
      GNUNET_CRYPTO_ecdsa_public_key_to_string (&pk);
    ego_entry->ego = ego;
    ego_entry->identifier = GNUNET_strdup (identifier);
    GNUNET_CONTAINER_DLL_insert_tail(handle->ego_head,handle->ego_tail, ego_entry);
  }

}

static void
rest_identity_process_request(struct GNUNET_REST_RequestHandle *rest_handle,
                              GNUNET_REST_ResultProcessor proc,
                              void *proc_cls)
{
  struct RequestHandle *handle = GNUNET_new (struct RequestHandle);
  handle->response_code = 0;
  handle->timeout = GNUNET_TIME_UNIT_FOREVER_REL;
  handle->proc_cls = proc_cls;
  handle->proc = proc;
  handle->state = ID_REST_STATE_INIT;
  handle->rest_handle = rest_handle;

  handle->url = GNUNET_strdup (rest_handle->url);
  if (handle->url[strlen (handle->url)-1] == '/')
    handle->url[strlen (handle->url)-1] = '\0';
  GNUNET_log (GNUNET_ERROR_TYPE_DEBUG,
              "Connecting...\n");
  handle->identity_handle = GNUNET_IDENTITY_connect (cfg,
                                                     &list_ego,
                                                     handle);
  handle->namestore_handle = GNUNET_NAMESTORE_connect (cfg);
  handle->timeout_task =
    GNUNET_SCHEDULER_add_delayed (handle->timeout,
                                  &do_timeout,
                                  handle);
  GNUNET_log (GNUNET_ERROR_TYPE_DEBUG,
              "Connected\n");
}

/**
 * Entry point for the plugin.
 *
 * @param cls Config info
 * @return NULL on error, otherwise the plugin context
 */
void *
libgnunet_plugin_rest_reclaim_init (void *cls)
{
  static struct Plugin plugin;
  struct GNUNET_REST_Plugin *api;

  cfg = cls;
  if (NULL != plugin.cfg)
    return NULL;                /* can only initialize once! */
  memset (&plugin, 0, sizeof (struct Plugin));
  plugin.cfg = cfg;
  api = GNUNET_new (struct GNUNET_REST_Plugin);
  api->cls = &plugin;
  api->name = GNUNET_REST_API_NS_RECLAIM;
  api->process_request = &rest_identity_process_request;
  GNUNET_asprintf (&allow_methods,
                   "%s, %s, %s, %s, %s",
                   MHD_HTTP_METHOD_GET,
                   MHD_HTTP_METHOD_POST,
                   MHD_HTTP_METHOD_PUT,
                   MHD_HTTP_METHOD_DELETE,
                   MHD_HTTP_METHOD_OPTIONS);

  GNUNET_log (GNUNET_ERROR_TYPE_DEBUG,
              _("Identity Provider REST API initialized\n"));
  return api;
}


/**
 * Exit point from the plugin.
 *
 * @param cls the plugin context (as returned by "init")
 * @return always NULL
 */
void *
libgnunet_plugin_rest_reclaim_done (void *cls)
{
  struct GNUNET_REST_Plugin *api = cls;
  struct Plugin *plugin = api->cls;
  plugin->cfg = NULL;

  GNUNET_free_non_null (allow_methods);
  GNUNET_free (api);
  GNUNET_log (GNUNET_ERROR_TYPE_DEBUG,
              "Identity Provider REST plugin is finished\n");
  return NULL;
}

/* end of plugin_rest_reclaim.c */
