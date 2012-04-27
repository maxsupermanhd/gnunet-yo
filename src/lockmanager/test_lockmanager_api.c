/*
     This file is part of GNUnet.
     (C) 2012 Christian Grothoff (and other contributing authors)

     GNUnet is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published
     by the Free Software Foundation; either version 2, or (at your
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
 * @file lockmanager/test_lockmanager_api.c
 * @brief Test cases for lockmanager_api.c
 * @author Sree Harsha Totakura
 */

#include "platform.h"
#include "gnunet_util_lib.h"
#include "gnunet_lockmanager_service.h"

#define VERBOSE 1

#define VERBOSE_ARM 1

#define LOG(kind,...) \
  GNUNET_log_from (kind, "test-lockmanager-api",__VA_ARGS__)

#define TIME_REL_SECONDS(min) \
  GNUNET_TIME_relative_multiply (GNUNET_TIME_UNIT_SECONDS, min)

/**
 * The testing result
 */
static int result;

/**
 * The process id of the GNUNET ARM process
 */
static struct GNUNET_OS_Process *arm_pid = NULL;

/**
 * Configuration Handle
 */
static struct GNUNET_CONFIGURATION_Handle *config;

/**
 * The handle to the lockmanager service
 */
static struct GNUNET_LOCKMANAGER_Handle *handle;

/**
 * The locking request
 */
static struct GNUNET_LOCKMANAGER_LockingRequest *request;

/**
 * Shutdown nicely
 *
 * @param cls
 * @param tc the task context
 */
static void
do_shutdown (void *cls, const const struct GNUNET_SCHEDULER_TaskContext *tc)
{
  if (NULL != request)
    {
      GNUNET_LOCKMANAGER_cancel_request (request);
      request = NULL;
    }
  GNUNET_LOCKMANAGER_disconnect (handle);
  if (0 != GNUNET_OS_process_kill (arm_pid, SIGTERM))
    {
      LOG (GNUNET_ERROR_TYPE_DEBUG,
           "Kill gnunet-service-arm manually\n");
    }
  GNUNET_OS_process_wait (arm_pid);
  GNUNET_OS_process_close (arm_pid);
  if (GNUNET_SYSERR != result)
    result = GNUNET_OK;
}


/**
 * Shutdown nicely
 *
 * @param cls
 * @param tc the task context
 */
static void
do_abort (void *cls, const const struct GNUNET_SCHEDULER_TaskContext *tc)
{
  result = GNUNET_SYSERR;
  do_shutdown (cls, tc);
}

/**
 * Callback for lock status changes
 *
 * @param cls the closure from GNUNET_LOCKMANAGER_lock call
 *
 * @param domain_name the locking domain of the lock 
 *
 * @param lock the lock for which this status is relevant
 *
 * @param status GNUNET_LOCKMANAGER_SUCCESS if the lock has been successfully
 *          acquired; GNUNET_LOCKMANAGER_RELEASE when the acquired lock is lost
 */
static void 
status_cb (void *cls,
           const char *domain_name,
           uint32_t lock,
           enum GNUNET_LOCKMANAGER_Status status)
{
  GNUNET_SCHEDULER_add_delayed (TIME_REL_SECONDS (1),
                                &do_shutdown,
                                NULL);
}


/**
 * Testing function
 *
 * @param cls NULL
 * @param tc the task context
 */
static void
test (void *cls, const struct GNUNET_SCHEDULER_TaskContext *tc)
{  
  handle = GNUNET_LOCKMANAGER_connect (config);
  GNUNET_assert (NULL != handle);
  
  request = GNUNET_LOCKMANAGER_acquire_lock (handle,
                                             "GNUNET_LOCKMANAGER_TESTING",
                                             99,
                                             &status_cb,
                                             NULL);
  GNUNET_SCHEDULER_add_delayed (TIME_REL_SECONDS (10),
                                &do_abort,
                                NULL);
}


/**
 * Main point of test execution
 */
static void
run (void *cls, char *const *args, const char *cfgfile,
     const struct GNUNET_CONFIGURATION_Handle *cfg)
{
  config = GNUNET_CONFIGURATION_dup (cfg);
  arm_pid = 
    GNUNET_OS_start_process (GNUNET_YES, NULL, NULL, "gnunet-service-arm",
                             "gnunet-service-arm",
#if VERBOSE_ARM
                             "-L", "DEBUG",
#endif
                             "-c", "test_lockmanager_api.conf", NULL);

  GNUNET_assert (NULL != arm_pid);
  GNUNET_SCHEDULER_add_delayed (TIME_REL_SECONDS (1),
                                &test,
                                NULL);
}


/**
 * Main function
 */
int main (int argc, char **argv)
{
  int ret;

  char *const argv2[] = { "test-lockmanager-api",
                          "-c", "test_lockmanager_api.conf",
#if VERBOSE
                          "-L", "DEBUG",
#endif
                          NULL
  };
  
  struct GNUNET_GETOPT_CommandLineOption options[] = {
    GNUNET_GETOPT_OPTION_END
  };

  ret =
    GNUNET_PROGRAM_run ((sizeof (argv2) / sizeof (char *)) - 1, argv2,
                        "test-lockmanager-api", "nohelp", options, &run, NULL);

  if (GNUNET_OK != ret)
  {
    LOG (GNUNET_ERROR_TYPE_WARNING, "run failed with error code %d\n",
         ret);
    return 1;
  }
  if (GNUNET_SYSERR == result)
  {
    LOG (GNUNET_ERROR_TYPE_WARNING, "test failed\n");
    return 1;
  }
  LOG (GNUNET_ERROR_TYPE_INFO, "test ok\n");
  return 0;
}
