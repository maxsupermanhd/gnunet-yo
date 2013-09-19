/*
 This file is part of GNUnet.
 (C) 2010-2013 Christian Grothoff (and other contributing authors)

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
 * @file ats/perf_ats_logging.c
 * @brief ats benchmark: logging for performance tests
 * @author Christian Grothoff
 * @author Matthias Wachs
 */
#include "platform.h"
#include "gnunet_util_lib.h"
#include "perf_ats.h"

#define LOGGING_FREQUENCY GNUNET_TIME_relative_multiply (GNUNET_TIME_UNIT_MILLISECONDS, 500)


/**
 * Logging task
 */
static GNUNET_SCHEDULER_TaskIdentifier log_task;

/**
 * Reference to perf_ats' masters
 */
static int num_peers;
static char *name;

/**
 * A single logging time step for a partner
 */
struct PartnerLoggingTimestep
{
  /**
   * Peer
   */
  struct BenchmarkPeer *slave;

  /**
   * Total number of messages this peer has sent
   */
  unsigned int total_messages_sent;

  /**
   * Total number of bytes this peer has sent
   */
  unsigned int total_bytes_sent;

  /**
   * Total number of messages this peer has received
   */
  unsigned int total_messages_received;

  /**
   * Total number of bytes this peer has received
   */
  unsigned int total_bytes_received;
};


/**
 * A single logging time step for a peer
 */
struct PeerLoggingTimestep
{
  /**
   * Next in DLL
   */
  struct PeerLoggingTimestep *next;

  /**
   * Prev in DLL
   */
  struct PeerLoggingTimestep *prev;

  /**
   * Logging timestamp
   */
  struct GNUNET_TIME_Absolute timestamp;

  /**
   * Total number of messages this peer has sent
   */
  unsigned int total_messages_sent;

  /**
   * Total number of bytes this peer has sent
   */
  unsigned int total_bytes_sent;

  /**
   * Total number of messages this peer has received
   */
  unsigned int total_messages_received;

  /**
   * Total number of bytes this peer has received
   */
  unsigned int total_bytes_received;

  /**
   * Logs for slaves
   */
  struct PartnerLoggingTimestep *slaves_log;
};

/**
 * Entry for a benchmark peer
 */
struct LoggingPeer
{
  /**
   * Peer
   */
  struct BenchmarkPeer *peer;

  /**
   * Start time
   */
  struct GNUNET_TIME_Absolute start;

  /**
   * DLL for logging entries: head
   */
  struct PeerLoggingTimestep *head;

  /**
   * DLL for logging entries: tail
   */
  struct PeerLoggingTimestep *tail;
};

/**
 * Log structure of length num_peers
 */
static struct LoggingPeer *lp;


static void
write_to_file ()
{
  struct GNUNET_DISK_FileHandle *f;
  char * filename;
  char *data;
  struct PeerLoggingTimestep *cur;
  int c_m;
  //int c_s;
  unsigned int throughput_recv;
  unsigned int throughput_send;
  double mult;

  GNUNET_asprintf (&filename, "%llu_%s.data", GNUNET_TIME_absolute_get().abs_value_us,name);

  f = GNUNET_DISK_file_open (filename,
      GNUNET_DISK_OPEN_WRITE | GNUNET_DISK_OPEN_CREATE,
      GNUNET_DISK_PERM_USER_READ | GNUNET_DISK_PERM_USER_WRITE);
  if (NULL == f)
  {
    GNUNET_log (GNUNET_ERROR_TYPE_ERROR, "Cannot open log file `%s'\n", filename);
    GNUNET_free (filename);
    return;
  }

  for (c_m = 0; c_m < num_peers; c_m++)
  {
    for (cur = lp[c_m].head; NULL != cur; cur = cur->next)
    {
      mult = (1.0 * 1000 * 1000) /  (LOGGING_FREQUENCY.rel_value_us);
      if (NULL != cur->prev)
      {
        throughput_send = cur->total_bytes_sent - cur->prev->total_bytes_sent;
        throughput_recv = cur->total_bytes_received - cur->prev->total_bytes_received;
      }
      else
      {
        throughput_send = cur->total_bytes_sent;
        throughput_recv = cur->total_bytes_received;
      }
      throughput_send *= mult;
      throughput_recv *= mult;


      GNUNET_log(GNUNET_ERROR_TYPE_INFO,
          "Master [%u]: timestamp %llu %llu %u %u %u ; %u %u %u\n", lp[c_m].peer->no,
          cur->timestamp, GNUNET_TIME_absolute_get_difference(lp[c_m].start,cur->timestamp).rel_value_us / 1000,
          cur->total_messages_sent, cur->total_bytes_sent, throughput_send,
          cur->total_messages_received, cur->total_bytes_received, throughput_recv);

//      for ()

      GNUNET_asprintf (&data, "%llu;%llu;%u;%u;%u;%u;%u;%u\n",
          cur->timestamp,
          GNUNET_TIME_absolute_get_difference(lp[c_m].start,cur->timestamp).rel_value_us / 1000,
          cur->total_messages_sent, cur->total_bytes_sent, throughput_send,
          cur->total_messages_received, cur->total_bytes_received, throughput_recv);

      if (GNUNET_SYSERR == GNUNET_DISK_file_write(f, data, strlen(data)))
        GNUNET_log (GNUNET_ERROR_TYPE_ERROR, "Cannot write data to log file `%s'\n", filename);
      GNUNET_free (data);
    }
  }

  if (GNUNET_SYSERR == GNUNET_DISK_file_close(f))
  {
    GNUNET_log (GNUNET_ERROR_TYPE_ERROR, "Cannot close log file `%s'\n", filename);
    GNUNET_free (filename);
    return;
  }

  GNUNET_log (GNUNET_ERROR_TYPE_INFO, "Data file successfully written to log file `%s'\n", filename);
  GNUNET_free (filename);
}

static void
collect_log_task (void *cls, const struct GNUNET_SCHEDULER_TaskContext *tc)
{
  int c_m;
  int c_s;
  struct PeerLoggingTimestep *mlt;
  struct PartnerLoggingTimestep *slt;
  struct BenchmarkPartner *p;

  log_task = GNUNET_SCHEDULER_NO_TASK;

  for (c_m = 0; c_m < num_peers; c_m++)
  {
    mlt = GNUNET_malloc (sizeof (struct PeerLoggingTimestep));
    GNUNET_CONTAINER_DLL_insert_tail(lp[c_m].head, lp[c_m].tail, mlt);

    /* Collect data */
    mlt->timestamp = GNUNET_TIME_absolute_get();
    mlt->total_bytes_sent = lp[c_m].peer->total_bytes_sent;
    mlt->total_messages_sent = lp[c_m].peer->total_messages_sent;
    mlt->total_bytes_received = lp[c_m].peer->total_bytes_received;
    mlt->total_messages_received = lp[c_m].peer->total_messages_received;

    mlt->slaves_log = GNUNET_malloc (lp[c_m].peer->num_partners *
        sizeof (struct PartnerLoggingTimestep));

    for (c_s = 0; c_s < lp[c_m].peer->num_partners; c_s++)
    {
      p = &lp[c_m].peer->partners[c_s];
      slt = &mlt->slaves_log[c_s];
      slt->slave = p->dest;
      slt->total_bytes_sent = p->dest->total_bytes_sent;
      slt->total_messages_sent = p->dest->total_messages_sent;
      slt->total_bytes_received = p->dest->total_bytes_received;
      slt->total_messages_received = p->dest->total_messages_received;

      GNUNET_log(GNUNET_ERROR_TYPE_INFO,
          "Master [%u]: slave [%u]\n",
          lp->peer->no, p->dest->no);
    }
  }

  if (tc->reason == GNUNET_SCHEDULER_REASON_SHUTDOWN)
    return;

  log_task = GNUNET_SCHEDULER_add_delayed (LOGGING_FREQUENCY,
      &collect_log_task, NULL);
}


void
perf_logging_stop ()
{
  int c_m;
  struct GNUNET_SCHEDULER_TaskContext tc;
  struct PeerLoggingTimestep *cur;

  if (GNUNET_SCHEDULER_NO_TASK != log_task)
    GNUNET_SCHEDULER_cancel (log_task);
  log_task = GNUNET_SCHEDULER_NO_TASK;
  tc.reason = GNUNET_SCHEDULER_REASON_SHUTDOWN;
  collect_log_task (NULL, &tc);

  GNUNET_log(GNUNET_ERROR_TYPE_INFO,
      _("Stop logging\n"));

  write_to_file ();

  for (c_m = 0; c_m < num_peers; c_m++)
  {
    while (NULL != (cur = lp[c_m].head))
    {
      GNUNET_CONTAINER_DLL_remove (lp[c_m].head, lp[c_m].tail, cur);
      GNUNET_free (cur->slaves_log);
      GNUNET_free (cur);
    }
  }

  GNUNET_free (lp);
}

void
perf_logging_start (char * testname, struct BenchmarkPeer *masters, int num_masters)
{
  int c_m;
  GNUNET_log(GNUNET_ERROR_TYPE_INFO,
      _("Start logging `%s'\n"), testname);

  num_peers = num_masters;
  name = testname;

  lp = GNUNET_malloc (num_masters * sizeof (struct LoggingPeer));

  for (c_m = 0; c_m < num_masters; c_m ++)
  {
    lp[c_m].peer = &masters[c_m];
    lp[c_m].start = GNUNET_TIME_absolute_get();
  }

  /* Schedule logging task */
  log_task = GNUNET_SCHEDULER_add_now (&collect_log_task, NULL);
}
/* end of file perf_ats_logging.c */

