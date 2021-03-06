/*
 * This file is part of GNUnet
 * Copyright (C) 2013 GNUnet e.V.
 *
 * GNUnet is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * GNUnet is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file psyc/test_psyc_api_join.c
 * @brief Testbed test for the PSYC API.
 * @author xrs
 */

/**
 * Lessons Learned:
 * - define topology in config
 * - psyc slave join needs part to end (same with master)
 * - GNUNET_SCHEDULER_add_delayed return value will outdate at call time
 * - main can not contain GNUNET_log()
 */

#include "platform.h"
#include "gnunet_crypto_lib.h"
#include "gnunet_common.h"
#include "gnunet_util_lib.h"
#include "gnunet_testbed_service.h"
#include "gnunet_psyc_util_lib.h"
#include "gnunet_psyc_service.h"
#include "psyc_test_lib.h"

static struct pctx PEERS[2];

static int pids;


static void 
shutdown_task (void *cls)
{
  if (NULL != timeout_task_id) {
    GNUNET_SCHEDULER_cancel (timeout_task_id);
    timeout_task_id = NULL;
  }

  for (int i=0;i<2;i++) {
    GNUNET_free (PEERS[i].channel_pub_key);

    if (NULL != PEERS[i].psyc)
    {
      if (0 == i) 
        GNUNET_PSYC_master_stop (PEERS[i].psyc, GNUNET_NO, NULL, NULL);
      else 
        GNUNET_PSYC_slave_part (PEERS[i].psyc, GNUNET_NO, NULL, NULL);
    }
  }

  for (int i=0;i<MAX_TESTBED_OPS;i++)
    if (NULL != op[i])
      GNUNET_TESTBED_operation_done (op[i]);

  GNUNET_log (GNUNET_ERROR_TYPE_INFO, "Shut down!\n");
}

static void
timeout_task (void *cls)
{
  GNUNET_log (GNUNET_ERROR_TYPE_INFO, "Timeout!\n");

  timeout_task_id = NULL;

  result = GNUNET_SYSERR;
  GNUNET_SCHEDULER_shutdown ();
}

static void
join_decision_cb (void *cls,
                  const struct GNUNET_PSYC_JoinDecisionMessage *dcsn,
                  int is_admitted,
                  const struct GNUNET_PSYC_Message *join_msg)
{
  GNUNET_log (GNUNET_ERROR_TYPE_INFO,
              "slave: got join decision: %s\n", 
              (GNUNET_YES == is_admitted) ? "admitted":"rejected");

  result = (GNUNET_YES == is_admitted) ? GNUNET_OK : GNUNET_SYSERR;

  GNUNET_SCHEDULER_shutdown ();
}

static void
join_request_cb (void *cls,
                 const struct GNUNET_PSYC_JoinRequestMessage *req,
                 const struct GNUNET_CRYPTO_EcdsaPublicKey *slave_key,
                 const struct GNUNET_PSYC_Message *join_msg,
                 struct GNUNET_PSYC_JoinHandle *jh)
{
  struct GNUNET_HashCode slave_key_hash;

  GNUNET_log (GNUNET_ERROR_TYPE_INFO, "master: got join request.\n");

  GNUNET_CRYPTO_hash (slave_key, sizeof (*slave_key), &slave_key_hash);

  GNUNET_PSYC_join_decision (jh, GNUNET_YES, 0, NULL, NULL);
}

static void
psyc_da ()
{
  GNUNET_log (GNUNET_ERROR_TYPE_INFO, "disconnect form PSYC service\n");
}

static void *
psyc_ca (void *cls, const struct GNUNET_CONFIGURATION_Handle *cfg) 
{
  struct pctx *peer = (struct pctx*) cls;

  // Case: master role
  if (0 == peer->idx) {
    GNUNET_log (GNUNET_ERROR_TYPE_INFO, "Connecting to PSYC as master ...\n");

    peer->psyc = (struct GNUNET_PSYC_Master *) 
      GNUNET_PSYC_master_start (cfg,
                                peer->channel_key,
                                GNUNET_PSYC_CHANNEL_PRIVATE,
                                NULL,
                                join_request_cb,
                                NULL,
                                NULL,
                                cls);
    return peer->psyc;
  }

  GNUNET_log (GNUNET_ERROR_TYPE_INFO, "Connecting to PSYC as slave ...\n");

  struct GNUNET_PSYC_Environment *env = GNUNET_PSYC_env_create ();
  GNUNET_PSYC_env_add (env, GNUNET_PSYC_OP_ASSIGN, "_foo", "bar baz", 7);
  GNUNET_PSYC_env_add (env, GNUNET_PSYC_OP_ASSIGN, "_foo_bar", "foo bar baz", 11);

  struct GNUNET_PSYC_Message *
    join_msg = GNUNET_PSYC_message_create ("_request_join", env, "some data", 40);

  peer->psyc = (struct GNUNET_PSYC_Slave *)
    GNUNET_PSYC_slave_join (cfg,
                            peer->channel_pub_key,
                            peer->id_key,
                            GNUNET_PSYC_SLAVE_JOIN_NONE,
                            peer->peer_id_master, 
                            0,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            join_decision_cb,
                            cls,
                            join_msg);

  GNUNET_free (join_msg);
  peer->channel = GNUNET_PSYC_slave_get_channel (peer->psyc);
  GNUNET_PSYC_env_destroy (env);

  return peer->psyc;
}

static void
service_connect (void *cls,
                 struct GNUNET_TESTBED_Operation *op,
                 void *ca_result,
                 const char *emsg)
{
  GNUNET_assert (NULL != ca_result);

  GNUNET_log (GNUNET_ERROR_TYPE_INFO, "Connected to the service\n");
}

static void 
connect_to_services (void *cls)
{
  for (int i = 0; i < 2; i++)
  {
    PEERS[i].peer_id_master = PEERS[0].peer_id;

    op[op_cnt++] = 
      GNUNET_TESTBED_service_connect (NULL, PEERS[i].testbed_peer, "psyc", 
                                      &service_connect, &PEERS[i], &psyc_ca, 
                                      &psyc_da, &PEERS[i]);
  }
}

static void
pinfo_cb (void *cls,
          struct GNUNET_TESTBED_Operation *operation,
          const struct GNUNET_TESTBED_PeerInformation *pinfo,
          const char *emsg)
{
  struct pctx *peer = (struct pctx*) cls;

  peer->peer_id = pinfo->result.id;

  pids++;
  if (pids < 2)
    return;
  GNUNET_log (GNUNET_ERROR_TYPE_INFO, "Got all IDs, starting test\n");

  GNUNET_SCHEDULER_add_now (&connect_to_services, NULL);
}

static void
testbed_master (void *cls,
     struct GNUNET_TESTBED_RunHandle *h,
     unsigned int num_peers,
     struct GNUNET_TESTBED_Peer **p,
     unsigned int links_succeeded,
     unsigned int links_failed)
{
  struct GNUNET_CRYPTO_EddsaPrivateKey *channel_key = NULL;

  GNUNET_log (GNUNET_ERROR_TYPE_INFO, "Connected to testbed_master\n");

  // Set up shutdown logic
  GNUNET_SCHEDULER_add_shutdown (&shutdown_task, NULL); 
  timeout_task_id = 
    GNUNET_SCHEDULER_add_delayed (GNUNET_TIME_relative_multiply(GNUNET_TIME_UNIT_SECONDS, 15),
                                  &timeout_task, NULL);
  GNUNET_assert (NULL != timeout_task_id);

  // Set up channel key
  channel_key = GNUNET_CRYPTO_eddsa_key_create ();
  GNUNET_assert (NULL != channel_key);
  
  // Set up information contexts for peers
  for (int i=0 ; i < 2 ; i++)
  {
    PEERS[i].idx = i;
    PEERS[i].testbed_peer = p[i];

    // Create "egos"
    PEERS[i].id_key = GNUNET_CRYPTO_ecdsa_key_create ();

    // Set up channel keys shared by master and slave
    PEERS[i].channel_key = channel_key; 

    PEERS[i].channel_pub_key = 
      GNUNET_malloc (sizeof (struct GNUNET_CRYPTO_EddsaPublicKey));
    // Get public key 
    GNUNET_CRYPTO_eddsa_key_get_public (PEERS[i].channel_key, 
                                        PEERS[i].channel_pub_key);
    // Get peerinfo
    op[op_cnt++] = 
      GNUNET_TESTBED_peer_get_information (p[i], 
                                           GNUNET_TESTBED_PIT_IDENTITY, 
                                           pinfo_cb, &PEERS[i]);
  }
}

int 
main (int argc, char *argv[])
{
  int ret; 

  ret = GNUNET_TESTBED_test_run ("test_psyc_api_join", "test_psyc.conf",
                                 2, 0LL, NULL, NULL, 
                                 &testbed_master, NULL);

  if ( (GNUNET_OK != ret) || (GNUNET_OK != result) )
    return 1;

  return 0;
}

/* end of test_psyc_api_join.c */
