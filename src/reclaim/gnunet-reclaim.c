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
 * @file src/reclaim/gnunet-reclaim.c
 * @brief Identity Provider utility
 *
 */

#include "platform.h"
#include "gnunet_util_lib.h"
#include "gnunet_namestore_service.h"
#include "gnunet_reclaim_service.h"
#include "gnunet_identity_service.h"
#include "gnunet_signatures.h"

/**
 * return value
 */
static int ret;

/**
 * List attribute flag
 */
static int list;

/**
 * Relying party
 */
static char* rp;

/**
 * The attribute
 */
static char* attr_name;

/**
 * Attribute value
 */
static char* attr_value;

/**
 * Attributes to issue
 */
static char* issue_attrs;

/**
 * Ticket to consume
 */
static char* consume_ticket;

/**
 * Attribute type
 */
static char* type_str;

/**
 * Ticket to revoke
 */
static char* revoke_ticket;

/**
 * Ego name
 */
static char* ego_name;

/**
 * Identity handle
 */
static struct GNUNET_IDENTITY_Handle *identity_handle;

/**
 * reclaim handle
 */
static struct GNUNET_RECLAIM_Handle *reclaim_handle;

/**
 * reclaim operation
 */
static struct GNUNET_RECLAIM_Operation *reclaim_op;

/**
 * Attribute iterator
 */
static struct GNUNET_RECLAIM_AttributeIterator *attr_iterator;

/**
 * Master ABE key
 */
static struct GNUNET_CRYPTO_AbeMasterKey *abe_key;

/**
 * ego private key
 */
static const struct GNUNET_CRYPTO_EcdsaPrivateKey *pkey;

/**
 * rp public key
 */
static struct GNUNET_CRYPTO_EcdsaPublicKey rp_key;

/**
 * Ticket to consume
 */
static struct GNUNET_RECLAIM_Ticket ticket;

/**
 * Attribute list
 */
static struct GNUNET_RECLAIM_ATTRIBUTE_ClaimList *attr_list;

/**
 * Attribute expiration interval
 */
static struct GNUNET_TIME_Relative exp_interval;

/**
 * Timeout task
 */
static struct GNUNET_SCHEDULER_Task *timeout;

/**
 * Cleanup task
 */
static struct GNUNET_SCHEDULER_Task *cleanup_task;

/**
 * Claim to store
 */
struct GNUNET_RECLAIM_ATTRIBUTE_Claim *claim;

static void
do_cleanup(void *cls)
{
  cleanup_task = NULL;
  if (NULL != timeout)
    GNUNET_SCHEDULER_cancel (timeout);
  if (NULL != reclaim_op)
    GNUNET_RECLAIM_cancel (reclaim_op);
  if (NULL != attr_iterator)
    GNUNET_RECLAIM_get_attributes_stop (attr_iterator);
  if (NULL != reclaim_handle)
    GNUNET_RECLAIM_disconnect (reclaim_handle);
  if (NULL != identity_handle)
    GNUNET_IDENTITY_disconnect (identity_handle);
  if (NULL != abe_key)
    GNUNET_free (abe_key);
  if (NULL != attr_list)
    GNUNET_free (attr_list);
}

static void
ticket_issue_cb (void* cls,
                 const struct GNUNET_RECLAIM_Ticket *ticket)
{
  char* ticket_str;
  reclaim_op = NULL;
  if (NULL != ticket) {
    ticket_str = GNUNET_STRINGS_data_to_string_alloc (ticket,
                                                      sizeof (struct GNUNET_RECLAIM_Ticket));
    printf("%s\n",
           ticket_str);
    GNUNET_free (ticket_str);
  }
  cleanup_task = GNUNET_SCHEDULER_add_now (&do_cleanup, NULL);
}

static void
store_attr_cont (void *cls,
                 int32_t success,
                 const char*emsg)
{
  reclaim_op = NULL;
  if (GNUNET_SYSERR == success) {
    fprintf (stderr,
             "%s\n", emsg);
  }
  cleanup_task = GNUNET_SCHEDULER_add_now (&do_cleanup, NULL);
}

static void
process_attrs (void *cls,
         const struct GNUNET_CRYPTO_EcdsaPublicKey *identity,
         const struct GNUNET_RECLAIM_ATTRIBUTE_Claim *attr)
{
  char *value_str;
  const char* attr_type;

  if (NULL == identity)
  {
    reclaim_op = NULL;
    cleanup_task = GNUNET_SCHEDULER_add_now (&do_cleanup, NULL);
    return;
  }
  if (NULL == attr)
  {
    ret = 1;
    return;
  }
  value_str = GNUNET_RECLAIM_ATTRIBUTE_value_to_string (attr->type,
                                                        attr->data,
                                                        attr->data_size);
  attr_type = GNUNET_RECLAIM_ATTRIBUTE_number_to_typename (attr->type);
  fprintf (stdout,
           "%s: %s [%s,v%u]\n", attr->name, value_str, attr_type, attr->version);
}


static void
iter_error (void *cls)
{
  attr_iterator = NULL;
  fprintf (stderr,
           "Failed to iterate over attributes\n");
  cleanup_task = GNUNET_SCHEDULER_add_now (&do_cleanup, NULL);
}

static void
timeout_task (void *cls)
{
  timeout = NULL;
  ret = 1;
  fprintf (stderr,
           "Timeout\n");
  if (NULL == cleanup_task)
    cleanup_task = GNUNET_SCHEDULER_add_now (&do_cleanup, NULL);
}

static void
process_rvk (void *cls, int success, const char* msg)
{
  reclaim_op = NULL;
  if (GNUNET_OK != success)
  {
    fprintf (stderr,
             "Revocation failed.\n");
    ret = 1;
  }
  cleanup_task = GNUNET_SCHEDULER_add_now (&do_cleanup, NULL);
}

static void
iter_finished (void *cls)
{
  char *data;
  size_t data_size;
  int type;

  attr_iterator = NULL;
  if (list)
  {
    cleanup_task = GNUNET_SCHEDULER_add_now (&do_cleanup, NULL);
    return;
  }

  if (issue_attrs)
  {
    reclaim_op = GNUNET_RECLAIM_ticket_issue (reclaim_handle,
                                              pkey,
                                              &rp_key,
                                              attr_list,
                                              &ticket_issue_cb,
                                              NULL);
    return;
  }
  if (consume_ticket)
  {
    reclaim_op = GNUNET_RECLAIM_ticket_consume (reclaim_handle,
                                                pkey,
                                                &ticket,
                                                &process_attrs,
                                                NULL);
    timeout = GNUNET_SCHEDULER_add_delayed (GNUNET_TIME_relative_multiply(GNUNET_TIME_UNIT_SECONDS, 10),
                                            &timeout_task,
                                            NULL);
    return;
  }
  if (revoke_ticket)
  {
    reclaim_op = GNUNET_RECLAIM_ticket_revoke (reclaim_handle,
                                               pkey,
                                               &ticket,
                                               &process_rvk,
                                               NULL);
    return;
  }
  if (attr_name)
  {
    if (NULL == type_str)
      type = GNUNET_RECLAIM_ATTRIBUTE_TYPE_STRING;
    else
      type = GNUNET_RECLAIM_ATTRIBUTE_typename_to_number (type_str);

    GNUNET_assert (GNUNET_SYSERR != GNUNET_RECLAIM_ATTRIBUTE_string_to_value (type,
                                                                              attr_value,
                                                                              (void**)&data,
                                                                              &data_size));
    if (NULL != claim)
    {
      claim->type = type;
      claim->data = data;
      claim->data_size = data_size;
    }
    else
    {
      claim = GNUNET_RECLAIM_ATTRIBUTE_claim_new (attr_name,
                                                  type,
                                                  data,
                                                  data_size);
    }
    reclaim_op = GNUNET_RECLAIM_attribute_store (reclaim_handle,
                                                 pkey,
                                                 claim,
                                                 &exp_interval,
                                                 &store_attr_cont,
                                                 NULL);
    GNUNET_free (data);
    GNUNET_free (claim);
    return;
  }
  cleanup_task = GNUNET_SCHEDULER_add_now (&do_cleanup, NULL);
}

static void
iter_cb (void *cls,
         const struct GNUNET_CRYPTO_EcdsaPublicKey *identity,
         const struct GNUNET_RECLAIM_ATTRIBUTE_Claim *attr)
{
  struct GNUNET_RECLAIM_ATTRIBUTE_ClaimListEntry *le;
  char *attrs_tmp;
  char *attr_str;
  const char *attr_type;

  if ((NULL != attr_name) && (NULL != claim))
  {
    if (0 == strcasecmp (attr_name, attr->name))
    {
      claim = GNUNET_RECLAIM_ATTRIBUTE_claim_new (attr->name,
                                                            attr->type,
                                                            attr->data,
                                                            attr->data_size);
    }
  }
  else if (issue_attrs)
  {
    attrs_tmp = GNUNET_strdup (issue_attrs);
    attr_str = strtok (attrs_tmp, ",");
    while (NULL != attr_str) {
      if (0 != strcasecmp (attr_str, attr->name)) {
        attr_str = strtok (NULL, ",");
        continue;
      }
      le = GNUNET_new (struct GNUNET_RECLAIM_ATTRIBUTE_ClaimListEntry);
      le->claim = GNUNET_RECLAIM_ATTRIBUTE_claim_new (attr->name,
                                                      attr->type,
                                                      attr->data,
                                                      attr->data_size);
      le->claim->version = attr->version;
      GNUNET_CONTAINER_DLL_insert (attr_list->list_head,
                                   attr_list->list_tail,
                                   le);
      break;
    }
    GNUNET_free (attrs_tmp);
  } 
  else if (list)
  {
    attr_str = GNUNET_RECLAIM_ATTRIBUTE_value_to_string (attr->type,
                                                         attr->data,
                                                         attr->data_size);
    attr_type = GNUNET_RECLAIM_ATTRIBUTE_number_to_typename (attr->type);
    fprintf (stdout,
             "%s: %s [%s,v%u]\n", attr->name, attr_str, attr_type, attr->version);
  }
  GNUNET_RECLAIM_get_attributes_next (attr_iterator);
}

static void
start_get_attributes ()
{
  if (NULL == pkey)
  {
    fprintf (stderr,
             "Ego %s not found\n", ego_name);
    cleanup_task = GNUNET_SCHEDULER_add_now (&do_cleanup, NULL);
    return;
  }

  if (NULL != rp)
    GNUNET_CRYPTO_ecdsa_public_key_from_string (rp,
                                                strlen (rp),
                                                &rp_key);
  if (NULL != consume_ticket)
    GNUNET_STRINGS_string_to_data (consume_ticket,
                                   strlen (consume_ticket),
                                   &ticket,
                                   sizeof (struct GNUNET_RECLAIM_Ticket));
  if (NULL != revoke_ticket)
    GNUNET_STRINGS_string_to_data (revoke_ticket,
                                   strlen (revoke_ticket),
                                   &ticket,
                                   sizeof (struct GNUNET_RECLAIM_Ticket));

  attr_list = GNUNET_new (struct GNUNET_RECLAIM_ATTRIBUTE_ClaimList);
  claim = NULL;
  attr_iterator = GNUNET_RECLAIM_get_attributes_start (reclaim_handle,
                                                       pkey,
                                                       &iter_error,
                                                       NULL,
                                                       &iter_cb,
                                                       NULL,
                                                       &iter_finished,
                                                       NULL);


}

static int init = GNUNET_YES;

static void
ego_cb (void *cls,
        struct GNUNET_IDENTITY_Ego *ego,
        void **ctx,
        const char *name)
{
  if (NULL == name) {
    if (GNUNET_YES == init) {
      init = GNUNET_NO;
      start_get_attributes();
    }
    return;
  }
  if (0 != strcmp (name, ego_name))
    return;
  pkey = GNUNET_IDENTITY_ego_get_private_key (ego);
}


static void
run (void *cls,
     char *const *args,
     const char *cfgfile,
     const struct GNUNET_CONFIGURATION_Handle *c)
{
  ret = 0;
  if (NULL == ego_name)
  {
    ret = 1;
    fprintf (stderr,
             _("Ego is required\n"));
    return;
  }

  if ( (NULL == attr_value) && (NULL != attr_name) )
  {
    ret = 1;
    fprintf (stderr,
             _("Attribute value missing!\n"));
    return;
  }

  if ( (NULL == rp) && (NULL != issue_attrs) )
  {
    ret = 1;
    fprintf (stderr,
             _("Requesting party key is required!\n"));
    return;
  }

  reclaim_handle = GNUNET_RECLAIM_connect (c);
  //Get Ego
  identity_handle = GNUNET_IDENTITY_connect (c,
                                             &ego_cb,
                                             NULL);


}


int
main(int argc, char *const argv[])
{
  exp_interval = GNUNET_TIME_UNIT_HOURS;
  struct GNUNET_GETOPT_CommandLineOption options[] = {

    GNUNET_GETOPT_option_string ('a',
                                 "add",
                                 NULL,
                                 gettext_noop ("Add attribute"),
                                 &attr_name),

    GNUNET_GETOPT_option_string ('V',
                                 "value",
                                 NULL,
                                 gettext_noop ("Attribute value"),
                                 &attr_value),
    GNUNET_GETOPT_option_string ('e',
                                 "ego",
                                 NULL,
                                 gettext_noop ("Ego"),
                                 &ego_name),
    GNUNET_GETOPT_option_string ('r',
                                 "rp",
                                 NULL,
                                 gettext_noop ("Audience (relying party)"),
                                 &rp),
    GNUNET_GETOPT_option_flag ('D',
                               "dump",
                               gettext_noop ("List attributes for Ego"),
                               &list),
    GNUNET_GETOPT_option_string ('i',
                                 "issue",
                                 NULL,
                                 gettext_noop ("Issue a ticket"),
                                 &issue_attrs),
    GNUNET_GETOPT_option_string ('C',
                                 "consume",
                                 NULL,
                                 gettext_noop ("Consume a ticket"),
                                 &consume_ticket),
    GNUNET_GETOPT_option_string ('R',
                                 "revoke",
                                 NULL,
                                 gettext_noop ("Revoke a ticket"),
                                 &revoke_ticket),
    GNUNET_GETOPT_option_string ('t',
                                 "type",
                                 NULL,
                                 gettext_noop ("Type of attribute"),
                                 &type_str),
    GNUNET_GETOPT_option_relative_time ('E',
                                        "expiration",
                                        NULL,
                                        gettext_noop ("Expiration interval of the attribute"),
                                        &exp_interval),

    GNUNET_GETOPT_OPTION_END
  };
  if (GNUNET_OK != GNUNET_PROGRAM_run (argc, argv, "ct",
                                       "ct", options,
                                       &run, NULL))
    return 1;
  else
    return ret;
}
