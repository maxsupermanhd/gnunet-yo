/*
     This file is part of GNUnet.
     (C) 2010 Christian Grothoff (and other contributing authors)

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
 * @file transport/test_plugin_transport_http.c
 * @brief testcase for plugin_transport_http.c
 * @author Matthias Wachs
 */

#include "platform.h"
#include "gnunet_constants.h"
#include "gnunet_common.h"
#include "gnunet_getopt_lib.h"
#include "gnunet_hello_lib.h"
#include "gnunet_os_lib.h"
#include "gnunet_peerinfo_service.h"
#include "gnunet_plugin_lib.h"
#include "gnunet_protocols.h"
#include "gnunet_program_lib.h"
#include "gnunet_signatures.h"
#include "gnunet_service_lib.h"
#include "gnunet_crypto_lib.h"

#include "plugin_transport.h"
#include "gnunet_statistics_service.h"
#include "transport.h"
#include <curl/curl.h>

#define VERBOSE GNUNET_YES
#define DEBUG GNUNET_NO
#define DEBUG_CURL GNUNET_NO
#define HTTP_BUFFER_SIZE 2048

#define PLUGIN libgnunet_plugin_transport_template

/**
 * How long until we give up on transmitting the message?
 */
#define TIMEOUT GNUNET_TIME_relative_multiply (GNUNET_TIME_UNIT_SECONDS, 90)

/**
 * How long until we give up on transmitting the message?
 */
#define TEST_TIMEOUT GNUNET_TIME_relative_multiply (GNUNET_TIME_UNIT_SECONDS, 10)

/**
 * How long between recieve and send?
 */
#define WAIT_INTERVALL GNUNET_TIME_relative_multiply (GNUNET_TIME_UNIT_SECONDS, 1)



/**
 *  Struct for plugin addresses
 */
struct Plugin_Address
{
  /**
   * Next field for linked list
   */
  struct Plugin_Address * next;

  /**
   * buffer containing data to send
   */
  void * addr;

  /**
   * amount of data to sent
   */
  size_t addrlen;
};

/**
 *  Message to send using http
 */
struct HTTP_Message
{
  /**
   * buffer
   */
  unsigned char buf[HTTP_BUFFER_SIZE];

  /**
   * current position in buffer
   */
  size_t pos;

  /**
   * buffer size
   */
  size_t size;

  /**
   * data size
   */
  size_t len;
};


/**
 *  Struct for plugin addresses
 */
struct HTTP_Transfer
{
  /**
   * amount of bytes we recieved
   */
  size_t data_size;

  /**
   * buffer for http transfers
   */
  unsigned char buf[HTTP_BUFFER_SIZE];

  /**
   * buffer size this transfer
   */
  size_t size;

  /**
   * amount of bytes we recieved
   */
  size_t pos;

  /**
   * HTTP Header result for transfer
   */
  unsigned int http_result_code;

  /**
   * did the test fail?
   */
  unsigned int test_failed;

  /**
   * was this test already executed?
   */
  unsigned int test_executed;
};


/**
 * Network format for IPv4 addresses.
 */
struct IPv4HttpAddress
{
  /**
   * IPv4 address, in network byte order.
   */
  uint32_t ipv4_addr;

  /**
   * Port number, in network byte order.
   */
  uint16_t u_port;

};


/**
 * Network format for IPv6 addresses.
 */
struct IPv6HttpAddress
{
  /**
   * IPv6 address.
   */
  struct in6_addr ipv6_addr;

  /**
   * Port number, in network byte order.
   */
  uint16_t u6_port;

};


/**
 * Our public key.
 */
/* static struct GNUNET_CRYPTO_RsaPublicKeyBinaryEncoded my_public_key; */

/**
 * Our public key.
 */
static struct GNUNET_CRYPTO_RsaPublicKeyBinaryEncoded my_public_key;

/**
 * Our identity.
 */
static struct GNUNET_PeerIdentity my_identity;

/**
 * Our private key.
 */
static struct GNUNET_CRYPTO_RsaPrivateKey *my_private_key;

/**
 * Peer's port
 */
static long long unsigned int port;

/**
 * Peer's port
 */
static char * test_addr;

/**
 * Our scheduler.
 */
struct GNUNET_SCHEDULER_Handle *sched;

/**
 * Our statistics handle.
 */
struct GNUNET_STATISTICS_Handle *stats;


/**
 * Our configuration.
 */
const struct GNUNET_CONFIGURATION_Handle *cfg;

/**
 * Number of neighbours we'd like to have.
 */
static uint32_t max_connect_per_transport;

/**
 * Environment for this plugin.
 */
static struct GNUNET_TRANSPORT_PluginEnvironment env;

/**
 *handle for the api provided by this plugin
 */
static struct GNUNET_TRANSPORT_PluginFunctions *api;

/**
 * ID of the task controlling the testcase timeout
 */
static GNUNET_SCHEDULER_TaskIdentifier ti_timeout;

static GNUNET_SCHEDULER_TaskIdentifier ti_send;

//const struct GNUNET_PeerIdentity * p;

/**
 * buffer for data to send
 */
static struct HTTP_Message buffer_out;

/**
 * buffer for data to recieve
 */
static struct HTTP_Message buffer_in;


struct Plugin_Address * addr_head;

/**
 * Did the test pass or fail?
 */
static int fail_notify_address;
/**
 * Did the test pass or fail?
 */
static int fail_notify_address_count;

/**
 * Did the test pass or fail?
 */
static int fail_pretty_printer;

/**
 * Did the test pass or fail?
 */
static int fail_pretty_printer_count;

/**
 * Did the test pass or fail?
 */
static int fail_addr_to_str;

/**
 * No. of msgs transmitted successfully to local addresses
 */
static int fail_msgs_transmited_to_local_addrs;

/**
 * Test: transmit msg of max. size
 */
static int fail_msg_transmited_bigger_max_size;

/**
 * Test: transmit msg of max. size
 */
static int fail_msg_transmited_max_size;

/**
 * Test: transmit 2 msgs. in in send operation
 */
static int fail_multiple_msgs_in_transmission;

/**
 * Test: connect to peer without peer identification
 */
static struct HTTP_Transfer test_no_ident;

/**
 * Test: connect to peer without peer identification
 */
static struct HTTP_Transfer test_too_short_ident;

/**
 * Test: connect to peer without peer identification
 */
static struct HTTP_Transfer test_too_long_ident;

/**
 * Test: connect to peer with valid peer identification
 */
static struct HTTP_Transfer test_valid_ident;

/**
 * Did the test pass or fail?
 */
static int fail;

/**
 * Number of local addresses
 */
static unsigned int count_str_addr;

CURL *curl_handle;

/**
 * cURL Multihandle
 */
static CURLM *multi_handle;

/**
 * The task sending data
 */
static GNUNET_SCHEDULER_TaskIdentifier http_task_send;

/**
 * Shutdown testcase
 */
static void
shutdown_clean ()
{
  struct Plugin_Address * cur;
  struct Plugin_Address * tmp;

  /* Evaluate results  */
  fail = 0;
  if ((fail_notify_address == GNUNET_YES) || (fail_pretty_printer == GNUNET_YES) || (fail_addr_to_str == GNUNET_YES))
  {
    GNUNET_log (GNUNET_ERROR_TYPE_DEBUG, "Test plugin functions failed\n");
    fail = 1;
  }
  if ((test_no_ident.test_failed == GNUNET_YES) || (test_too_short_ident.test_failed == GNUNET_YES) || (test_too_long_ident.test_failed == GNUNET_YES) || (test_valid_ident.test_failed == GNUNET_YES))
  {
    GNUNET_log (GNUNET_ERROR_TYPE_DEBUG, "Test connect with wrong data failed\n");
    fail = 1;
  }
  if ((fail_msgs_transmited_to_local_addrs != count_str_addr) || (fail_msg_transmited_max_size == GNUNET_YES) || (fail_msg_transmited_bigger_max_size == GNUNET_YES) || (fail_multiple_msgs_in_transmission != GNUNET_NO))
  {
    GNUNET_log (GNUNET_ERROR_TYPE_DEBUG, "Test sending with plugin failed\n");
    fail = 1;
  }
  if (fail != 1)
  {
    GNUNET_log (GNUNET_ERROR_TYPE_DEBUG, "All tests successful\n");
  }

  curl_multi_cleanup(multi_handle);

  if (NULL != curl_handle)
    curl_easy_cleanup (curl_handle);

  /* cleaning addresses */
  while (addr_head != NULL)
  {
    cur = addr_head;
    tmp = addr_head->next;
    GNUNET_free (addr_head->addr);
    GNUNET_free (addr_head);
    addr_head=tmp;
  }

  if (ti_send != GNUNET_SCHEDULER_NO_TASK)
  {
    GNUNET_SCHEDULER_cancel(sched,ti_send);
    ti_send = GNUNET_SCHEDULER_NO_TASK;
  }

  if (http_task_send != GNUNET_SCHEDULER_NO_TASK)
  {
    GNUNET_SCHEDULER_cancel(sched,http_task_send);
    http_task_send = GNUNET_SCHEDULER_NO_TASK;
  }

  if (ti_timeout != GNUNET_SCHEDULER_NO_TASK)
  {
    GNUNET_SCHEDULER_cancel(sched,ti_timeout);
    ti_timeout = GNUNET_SCHEDULER_NO_TASK;
  }

  GNUNET_free(test_addr);
  GNUNET_log (GNUNET_ERROR_TYPE_DEBUG, "Unloading http plugin\n");
  GNUNET_assert (NULL == GNUNET_PLUGIN_unload ("libgnunet_plugin_transport_http", api));

  GNUNET_SCHEDULER_shutdown(sched);
  GNUNET_DISK_directory_remove ("/tmp/test_plugin_transport_http");

  GNUNET_log (GNUNET_ERROR_TYPE_DEBUG, "Exiting testcase\n");
  exit(fail);
  return;
}


/**
 * Continuation called after plugin send message
 * @cls closure
 * @target target
 * @result GNUNET_OK or GNUNET_SYSERR
 */

static void task_send_cont (void *cls,
                            const struct GNUNET_PeerIdentity * target,
                            int result)
{
  struct Plugin_Address * tmp_addr;
  tmp_addr = addr_head;

  if ((cls == &fail_msgs_transmited_to_local_addrs) && (result == GNUNET_OK))
  {
    fail_msgs_transmited_to_local_addrs++;
    if (fail_msgs_transmited_to_local_addrs == count_str_addr)
    {
      GNUNET_log (GNUNET_ERROR_TYPE_DEBUG, "Message sent to %u addresses!\n",fail_msgs_transmited_to_local_addrs);
    }
    return;
  }

  if ((cls == &fail_msg_transmited_bigger_max_size) && (result == GNUNET_SYSERR))
  {
    GNUNET_log (GNUNET_ERROR_TYPE_DEBUG, "Message bigger max msg size was not sent!\n");
    fail_msg_transmited_bigger_max_size = GNUNET_NO;
    return;
  }

  if ((cls == &fail_msg_transmited_max_size) && (result == GNUNET_OK))
  {
    GNUNET_log (GNUNET_ERROR_TYPE_DEBUG, "Message with max msg size succesfully sent!\n",fail_msgs_transmited_to_local_addrs);
    fail_msg_transmited_max_size = GNUNET_NO;
    shutdown_clean();
  }
}

#if 0
/**
 * Task sending recieved message back to peer
 * @cls closure
 * @tc task context
 */

static void
task_send (void *cls,
            const struct GNUNET_SCHEDULER_TaskContext *tc)
{
  ti_timeout = GNUNET_SCHEDULER_NO_TASK;
  if (0 != (tc->reason & GNUNET_SCHEDULER_REASON_SHUTDOWN))
    return;

  if (GNUNET_YES==sent)
    return;
/*
  struct GNUNET_MessageHeader * msg = cls;
  unsigned int len = ntohs(msg->size);
  const char * msgc = (const char *) msg;

  api->send(api->cls, p, msgc, len, 0, TIMEOUT, NULL,NULL, 0, GNUNET_NO, &task_send_cont, NULL);
  */
  sent = GNUNET_YES;
}
#endif

/**
 * Recieves messages from plugin, in real world transport
 */
static struct GNUNET_TIME_Relative
receive (void *cls,
         const struct GNUNET_PeerIdentity * peer,
         const struct GNUNET_MessageHeader * message,
         uint32_t distance,
         struct Session *session,
         const char *sender_address,
         uint16_t sender_address_len)
{
  GNUNET_log (GNUNET_ERROR_TYPE_DEBUG, "Testcase recieved new message from peer `%s' with type %u and length %u\n",  GNUNET_i2s(peer), ntohs(message->type), ntohs(message->size));
  if ((ntohs(message->type) == 40) &&   (fail_multiple_msgs_in_transmission == 1))
    fail_multiple_msgs_in_transmission++;
  if ((ntohs(message->type) == 41) &&   (fail_multiple_msgs_in_transmission == 2))
    fail_multiple_msgs_in_transmission = GNUNET_NO;
  return GNUNET_TIME_UNIT_ZERO;
}

static size_t send_function (void *stream, size_t size, size_t nmemb, void *ptr)
{
  unsigned int len;

  len = buffer_out.len;

  if (( buffer_out.pos == len) || (len > (size * nmemb)))
    return 0;
  memcpy(stream, buffer_out.buf, len);
  buffer_out.pos = len;
  return len;
}

static size_t recv_function (void *ptr, size_t size, size_t nmemb, void *ctx)
{

  if (buffer_in.pos + size * nmemb > buffer_in.size)
    return 0;                   /* overflow */

  buffer_in.len = size * nmemb;
  memcpy (&buffer_in.buf[buffer_in.pos], ptr, size * nmemb);
  buffer_in.pos += size * nmemb;
  buffer_in.len = buffer_in.pos;
  buffer_in.buf[buffer_in.pos] = '\0';
  return buffer_in.pos;
}

static size_t header_function( void *ptr, size_t size, size_t nmemb, void *stream)
{
  struct HTTP_Transfer * res = (struct HTTP_Transfer *) stream;
  char * tmp;
  unsigned int len = size * nmemb;

  tmp = GNUNET_malloc (  len+1 );
  memcpy(tmp,ptr,len);
  if (tmp[len-2] == 13)
    tmp[len-2]= '\0';
#if DEBUG_CURL
  GNUNET_log (GNUNET_ERROR_TYPE_DEBUG, "Header: `%s'\n",tmp);
#endif
  if (0==strcmp (tmp,"HTTP/1.1 100 Continue"))
  {
    res->http_result_code=100;
  }
  if (0==strcmp (tmp,"HTTP/1.1 200 OK"))
  {
    res->http_result_code=200;
  }
  if (0==strcmp (tmp,"HTTP/1.1 400 Bad Request"))
  {
    res->http_result_code=400;
  }
  if (0==strcmp (tmp,"HTTP/1.1 404 Not Found"))
  {
    res->http_result_code=404;
  }
  if (0==strcmp (tmp,"HTTP/1.1 413 Request entity too large"))
  {
    res->http_result_code=413;
  }

  GNUNET_free (tmp);
  return size * nmemb;
}

static size_t send_prepare( struct HTTP_Transfer * result);

static void run_connection_tests( );

static void send_execute (void *cls,
             const struct GNUNET_SCHEDULER_TaskContext *tc)
{
  struct HTTP_Transfer *res;

  int running;
  struct CURLMsg *msg;
  CURLMcode mret;

  res = (struct HTTP_Transfer *) cls;
  http_task_send = GNUNET_SCHEDULER_NO_TASK;
  if (0 != (tc->reason & GNUNET_SCHEDULER_REASON_SHUTDOWN))
    return;

  do
    {
      running = 0;
      mret = curl_multi_perform (multi_handle, &running);
      if (running == 0)
        {
          do
            {

              msg = curl_multi_info_read (multi_handle, &running);
              if (msg == NULL)
                break;
              /* get session for affected curl handle */
              //cs = find_session_by_curlhandle (msg->easy_handle);
              //GNUNET_assert ( cs != NULL );
              switch (msg->msg)
                {

                case CURLMSG_DONE:
                  if ( (msg->data.result != CURLE_OK) &&
                       (msg->data.result != CURLE_GOT_NOTHING) )
                    {

                    GNUNET_log(GNUNET_ERROR_TYPE_INFO,
                               _("curl failed for `%s' at %s:%d: `%s'\n"),
                               "curl_multi_perform",
                               __FILE__,
                               __LINE__,
                               curl_easy_strerror (msg->data.result));
                    /* sending msg failed*/
                    curl_easy_cleanup(curl_handle);
                    curl_handle=NULL;

                    run_connection_tests();
                    }
                  if (res == &test_no_ident)
                  {
                    if  ((res->http_result_code==404) && (buffer_in.len==208))
                    {
                      GNUNET_log (GNUNET_ERROR_TYPE_INFO, _("Connecting to peer without any peer identification: test passed\n"));
                      res->test_failed = GNUNET_NO;
                    }
                    else
                      GNUNET_log (GNUNET_ERROR_TYPE_INFO, _("Connecting to peer without any peer identification: test failed\n"));
                  }
                  if (res == &test_too_short_ident)
                  {
                    if  ((res->http_result_code==404) && (buffer_in.len==208))
                    {
                      GNUNET_log (GNUNET_ERROR_TYPE_INFO, _("Connecting to peer with too short peer identification: test passed\n"));
                      res->test_failed = GNUNET_NO;
                    }
                    else
                      GNUNET_log (GNUNET_ERROR_TYPE_INFO, _("Connecting to peer with too short peer identification: test failed\n"));
                  }
                  if (res == &test_too_long_ident)
                  {
                    if  ((res->http_result_code==404) && (buffer_in.len==208))
                      {
                        GNUNET_log (GNUNET_ERROR_TYPE_INFO, _("Connecting to peer with too long peer identification: test passed\n"));
                      res->test_failed = GNUNET_NO;
                      }
                    else
                      GNUNET_log (GNUNET_ERROR_TYPE_INFO, _("Connecting to peer with too long peer identification: test failed\n"));
                  }
                  if (res == &test_valid_ident)
                  {
                    if  ((res->http_result_code==400))
                    {
                      GNUNET_log (GNUNET_ERROR_TYPE_INFO, _("Connecting to peer with valid peer identification: test passed\n"));
                      res->test_failed = GNUNET_NO;
                    }
                    else
                      GNUNET_log (GNUNET_ERROR_TYPE_INFO, _("Connecting to peer with valid peer identification: test failed\n"));
                  }
                  curl_easy_cleanup(curl_handle);
                  curl_handle=NULL;

                  run_connection_tests();
                  return;
                default:
                  break;
                }

            }
          while ( (running > 0) );
        }
    }
  while (mret == CURLM_CALL_MULTI_PERFORM);
  send_prepare(cls);
}

/**
 * Function setting up file descriptors and scheduling task to run
 * @param ses session to send data to
 * @return bytes sent to peer
 */
static size_t send_prepare( struct HTTP_Transfer * result)
{
  fd_set rs;
  fd_set ws;
  fd_set es;
  int max;
  struct GNUNET_NETWORK_FDSet *grs;
  struct GNUNET_NETWORK_FDSet *gws;
  long to;
  CURLMcode mret;

  max = -1;
  FD_ZERO (&rs);
  FD_ZERO (&ws);
  FD_ZERO (&es);
  mret = curl_multi_fdset (multi_handle, &rs, &ws, &es, &max);
  if (mret != CURLM_OK)
    {
      GNUNET_log (GNUNET_ERROR_TYPE_ERROR,
                  _("%s failed at %s:%d: `%s'\n"),
                  "curl_multi_fdset", __FILE__, __LINE__,
                  curl_multi_strerror (mret));
      return -1;
    }
  mret = curl_multi_timeout (multi_handle, &to);
  if (mret != CURLM_OK)
    {
      GNUNET_log (GNUNET_ERROR_TYPE_ERROR,
                  _("%s failed at %s:%d: `%s'\n"),
                  "curl_multi_timeout", __FILE__, __LINE__,
                  curl_multi_strerror (mret));
      return -1;
    }

  grs = GNUNET_NETWORK_fdset_create ();
  gws = GNUNET_NETWORK_fdset_create ();
  GNUNET_NETWORK_fdset_copy_native (grs, &rs, max + 1);
  GNUNET_NETWORK_fdset_copy_native (gws, &ws, max + 1);
  http_task_send = GNUNET_SCHEDULER_add_select (sched,
                                   GNUNET_SCHEDULER_PRIORITY_DEFAULT,
                                   GNUNET_SCHEDULER_NO_TASK,
                                   GNUNET_TIME_relative_multiply (GNUNET_TIME_UNIT_SECONDS, 0),
                                   grs,
                                   gws,
                                   &send_execute,
                                   result);
  GNUNET_NETWORK_fdset_destroy (gws);
  GNUNET_NETWORK_fdset_destroy (grs);

  /* FIXME: return bytes REALLY sent */
  return 0;
}

/**
 * function to send data to server
 */
static int send_data( struct HTTP_Transfer * result, char * url)
{

  curl_handle = curl_easy_init();
  if( NULL == curl_handle)
  {
    printf("easy_init failed \n");
    return GNUNET_SYSERR;
  }
#if DEBUG_CURL
  curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);
#endif
  curl_easy_setopt(curl_handle, CURLOPT_URL, url);
  curl_easy_setopt(curl_handle, CURLOPT_PUT, 1L);
  curl_easy_setopt (curl_handle, CURLOPT_HEADERFUNCTION, &header_function);
  curl_easy_setopt (curl_handle, CURLOPT_WRITEHEADER, result);
  curl_easy_setopt (curl_handle, CURLOPT_WRITEFUNCTION, &recv_function);
  curl_easy_setopt (curl_handle, CURLOPT_WRITEDATA, result);
  curl_easy_setopt (curl_handle, CURLOPT_READFUNCTION, &send_function);
  curl_easy_setopt (curl_handle, CURLOPT_READDATA, result);
  curl_easy_setopt(curl_handle, CURLOPT_INFILESIZE_LARGE, (curl_off_t) buffer_out.len);
  curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 30);
  curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, 20);

  curl_multi_add_handle(multi_handle, curl_handle);

  send_prepare(result);

  return GNUNET_OK;
}


/**
 * Plugin notifies transport (aka testcase) about its addresses
 */
void
notify_address (void *cls,
                const char *name,
                const void *addr,
                uint16_t addrlen,
                struct GNUNET_TIME_Relative expires)
{
  char address[INET6_ADDRSTRLEN];
  unsigned int port;
  struct Plugin_Address * pl_addr;
  struct Plugin_Address * cur;

  if (addrlen == (sizeof (struct IPv4HttpAddress)))
    {
      inet_ntop(AF_INET, (struct in_addr *) addr,address,INET_ADDRSTRLEN);
      port = ntohs(((struct IPv4HttpAddress *) addr)->u_port);
    }
  else if (addrlen == (sizeof (struct IPv6HttpAddress)))
    {
      inet_ntop(AF_INET6, (struct in6_addr *) addr,address,INET6_ADDRSTRLEN);
      port = ntohs(((struct IPv6HttpAddress *) addr)->u6_port);
    }
  else
    {
      GNUNET_break (0);
      return;
    }
  GNUNET_log (GNUNET_ERROR_TYPE_DEBUG, 
	      _("Transport plugin notification for address: `%s':%u\n"),
	      address,
	      port);
  pl_addr = GNUNET_malloc (sizeof (struct Plugin_Address) );
  pl_addr->addrlen = addrlen;
  pl_addr->addr = GNUNET_malloc(addrlen);
  memcpy(pl_addr->addr,addr,addrlen);
  pl_addr->next = NULL;

  if ( NULL == addr_head)
    {
      addr_head = pl_addr;
    }
  else
    {
      cur = addr_head;
      while (NULL != cur->next)
	{
	  cur = cur->next;
	}
      cur->next = pl_addr;
    }
  fail_notify_address_count++;
  fail_notify_address = GNUNET_NO;
}

/**
 * Setup plugin environment
 */
static void
setup_plugin_environment ()
{
  env.cfg = cfg;
  env.sched = sched;
  env.stats = stats;
  env.my_identity = &my_identity;
  env.cls = &env;
  env.receive = &receive;
  env.notify_address = &notify_address;
  env.max_connections = max_connect_per_transport;
}


/**
 * Task shutting down testcase if it a timeout occurs
 */
static void
task_timeout (void *cls,
            const struct GNUNET_SCHEDULER_TaskContext *tc)
{
  ti_timeout = GNUNET_SCHEDULER_NO_TASK;
  if (0 != (tc->reason & GNUNET_SCHEDULER_REASON_SHUTDOWN))
    return;

  GNUNET_log (GNUNET_ERROR_TYPE_DEBUG, "Testcase timeout\n");
  fail = GNUNET_YES;
  shutdown_clean();
  return;
}

static void pretty_printer_cb (void *cls,
                               const char *address)
{
  if (NULL==address)
    return;
  GNUNET_log (GNUNET_ERROR_TYPE_DEBUG,"Plugin returned pretty address: `%s'\n",address);
  fail_pretty_printer_count++;
}

/**
 * Runs every single test to test the plugin
 */
static void run_connection_tests( )
{
  char * host_str = NULL;
  /* resetting buffers */
  buffer_in.size = HTTP_BUFFER_SIZE;
  buffer_in.pos = 0;
  buffer_in.len = 0;

  buffer_out.size = HTTP_BUFFER_SIZE;
  buffer_out.pos = 0;
  buffer_out.len = 0;


  if (test_no_ident.test_executed == GNUNET_NO)
  {
    /* Connecting to peer without identification */
    char * ident = "";
    host_str = GNUNET_malloc (strlen ("http:///")+ 1 + strlen (test_addr)+ strlen (ident));
    GNUNET_asprintf (&host_str, "http://%s/%s",test_addr,ident);
    GNUNET_log (GNUNET_ERROR_TYPE_INFO, _("Connecting to peer without any peer identification.\n"));
    test_no_ident.test_executed = GNUNET_YES;
    send_data ( &test_no_ident, host_str);
    GNUNET_free (host_str);
    return;
  }
  if (test_too_short_ident.test_executed == GNUNET_NO)
  {
    char * ident = "AAAAAAAAAA";
    /* Connecting to peer with too short identification */
    host_str = GNUNET_malloc (strlen ("http:///")+ 1 + strlen (test_addr)+ strlen (ident));
    GNUNET_asprintf (&host_str, "http://%s/%s",test_addr,ident);
    GNUNET_log (GNUNET_ERROR_TYPE_INFO, _("Connecting to peer with too short peer identification.\n"));
    test_too_short_ident.test_executed = GNUNET_YES;
    send_data ( &test_too_short_ident, host_str);
    GNUNET_free (host_str);
    return;
  }

  if (test_too_long_ident.test_executed == GNUNET_NO)
  {
    char * ident = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";

    /* Connecting to peer with too long identification */
    host_str = GNUNET_malloc (strlen ("http:///")+ 1 + strlen (test_addr)+ strlen (ident));
    GNUNET_asprintf (&host_str, "http://%s/%s",test_addr,ident);
    GNUNET_log (GNUNET_ERROR_TYPE_INFO, _("Connecting to peer with too long peer identification.\n"));
    test_too_long_ident.test_executed = GNUNET_YES;
    send_data ( &test_too_long_ident, host_str);
    GNUNET_free (host_str);
    return;
  }
  if (test_valid_ident.test_executed == GNUNET_NO)
  {
    struct GNUNET_CRYPTO_HashAsciiEncoded ident;
    GNUNET_CRYPTO_hash_to_enc(&my_identity.hashPubKey,&ident);
    host_str = GNUNET_malloc (strlen ("http:///")+ 1 + strlen (test_addr)+ strlen ((char *) &ident));
    GNUNET_asprintf (&host_str, "http://%s/%s",test_addr,(char *) &ident);

    GNUNET_log (GNUNET_ERROR_TYPE_INFO, _("Connecting to peer with valid peer identification.\n"));
    test_valid_ident.test_executed = GNUNET_YES;
    send_data ( &test_valid_ident, host_str);
    GNUNET_free (host_str);
    return;
  }
  /* Using one of the addresses the plugin proposed */
  GNUNET_assert (addr_head->addr != NULL);

  struct Plugin_Address * tmp_addr;
  struct GNUNET_MessageHeader msg;
  char * tmp = GNUNET_malloc(sizeof(struct GNUNET_MessageHeader));
  char address[INET6_ADDRSTRLEN];
  unsigned int port;
  unsigned int type = 10;

  msg.size=htons(sizeof(struct GNUNET_MessageHeader));
  tmp_addr = addr_head;
  /* send a message to all addresses advertised by plugin */

  int count = 0;
  while (tmp_addr != NULL)
  {
    if (tmp_addr->addrlen == (sizeof (struct IPv4HttpAddress)))
      {
        inet_ntop(AF_INET, (struct in_addr *) tmp_addr->addr,address,INET_ADDRSTRLEN);
        port = ntohs(((struct IPv4HttpAddress *) tmp_addr->addr)->u_port);
        GNUNET_log (GNUNET_ERROR_TYPE_DEBUG,"Sending message to addres no. %u: `%s':%u\n", count,address, port);
      }
    else if (tmp_addr->addrlen == (sizeof (struct IPv6HttpAddress)))
      {
        inet_ntop(AF_INET6, (struct in6_addr *) tmp_addr->addr,address,INET6_ADDRSTRLEN);
        port = ntohs(((struct IPv6HttpAddress *) tmp_addr->addr)->u6_port);
        GNUNET_log (GNUNET_ERROR_TYPE_DEBUG,"Sending message to addres no. %u: `%s':%u\n", count,address,port);
      }
    else
      {
        GNUNET_break (0);
        return;
      }
    msg.type=htons(type);
    memcpy(tmp,&msg,sizeof(struct GNUNET_MessageHeader));
    api->send(api->cls, &my_identity, tmp, sizeof(struct GNUNET_MessageHeader), 0, TIMEOUT, NULL,tmp_addr->addr, tmp_addr->addrlen, GNUNET_YES, &task_send_cont, &fail_msgs_transmited_to_local_addrs);
    tmp_addr = tmp_addr->next;
    count ++;
    type ++;
  }

  /* send a multiple GNUNET_messages at a time*/
  GNUNET_free(tmp);
  tmp = GNUNET_malloc(4 * sizeof(struct GNUNET_MessageHeader));
  struct GNUNET_MessageHeader * msg1 = (struct GNUNET_MessageHeader *) tmp;
  msg1->size = htons(2 * sizeof(struct GNUNET_MessageHeader));
  msg1->type = htons(40);
  struct GNUNET_MessageHeader * msg2 = &msg1[2];
  msg2->size = htons(2 * sizeof(struct GNUNET_MessageHeader));
  msg2->type = htons(41);
  api->send(api->cls, &my_identity, tmp, 4 * sizeof(struct GNUNET_MessageHeader), 0, TIMEOUT, NULL,addr_head->addr, addr_head->addrlen, GNUNET_YES, &task_send_cont, &fail_multiple_msgs_in_transmission);


  /* send a multiple GNUNET_messages at a time, second message has incorrect size*/
  GNUNET_free(tmp);
  tmp = GNUNET_malloc(4 * sizeof(struct GNUNET_MessageHeader));
  msg1 = (struct GNUNET_MessageHeader *) tmp;
  msg1->size = htons(2 * sizeof(struct GNUNET_MessageHeader));
  msg1->type = htons(40);
  msg2 = &msg1[2];
  msg2->size = htons(3 * sizeof(struct GNUNET_MessageHeader));
  msg2->type = htons(41);
  api->send(api->cls, &my_identity, tmp, 4 * sizeof(struct GNUNET_MessageHeader), 0, TIMEOUT, NULL,addr_head->addr, addr_head->addrlen, GNUNET_YES, &task_send_cont, NULL);


  /* send a multiple GNUNET_messages at a time, second message has incorrect size*/
/*  GNUNET_log (GNUNET_ERROR_TYPE_DEBUG,"Ping Hello Message\n");
  GNUNET_free(tmp);
  tmp = GNUNET_malloc(425);
  msg1 = (struct GNUNET_MessageHeader *) tmp;
  msg1->size = htons(353);
  msg1->type = htons(16);
  msg2 = &tmp[353];
  msg2->size = htons(72);
  msg2->type = htons(32);
  api->send(api->cls, &my_identity, tmp, 425, 0, TIMEOUT, NULL,addr_head->addr, addr_head->addrlen, GNUNET_YES, &task_send_cont, NULL);*/


  /* send a message with size GNUNET_SERVER_MAX_MESSAGE_SIZE )*/
  GNUNET_free(tmp);
  tmp = GNUNET_malloc(GNUNET_SERVER_MAX_MESSAGE_SIZE);
  uint16_t t2 = (uint16_t)GNUNET_SERVER_MAX_MESSAGE_SIZE;
  msg.size = htons(t2);
  memcpy(tmp,&msg,sizeof(struct GNUNET_MessageHeader));
  api->send(api->cls, &my_identity, tmp, GNUNET_SERVER_MAX_MESSAGE_SIZE, 0, TIMEOUT, NULL,addr_head->addr, addr_head->addrlen, GNUNET_YES, &task_send_cont, &fail_msg_transmited_bigger_max_size);

  /* send a message with size GNUNET_SERVER_MAX_MESSAGE_SIZE-1  */
  GNUNET_free(tmp);
  tmp = GNUNET_malloc(GNUNET_SERVER_MAX_MESSAGE_SIZE-1);
  uint16_t t = (uint16_t)GNUNET_SERVER_MAX_MESSAGE_SIZE-1;
  msg.size = htons(t);
  memcpy(tmp,&msg,sizeof(struct GNUNET_MessageHeader));
  api->send(api->cls, &my_identity, tmp, GNUNET_SERVER_MAX_MESSAGE_SIZE-1, 0, TIMEOUT, NULL,addr_head->addr, addr_head->addrlen, GNUNET_YES, &task_send_cont, &fail_msg_transmited_max_size);
  GNUNET_free(tmp);
  GNUNET_log (GNUNET_ERROR_TYPE_DEBUG,"No more tests to run\n");
}


/**
 * Runs the test.
 *
 * @param cls closure
 * @param s scheduler to use
 * @param c configuration to use
 */
static void
run (void *cls,
     struct GNUNET_SCHEDULER_Handle *s,
     char *const *args,
     const char *cfgfile, const struct GNUNET_CONFIGURATION_Handle *c)
{
  char * libname;
  sched = s;
  cfg = c;
  char *keyfile;
  unsigned long long tneigh;
  struct Plugin_Address * cur;
  const char * addr_str;


  unsigned int suggest_res;

  fail_pretty_printer = GNUNET_YES;
  fail_notify_address = GNUNET_YES;
  fail_addr_to_str = GNUNET_YES;
  fail_msgs_transmited_to_local_addrs = 0;
  fail_msg_transmited_max_size = GNUNET_YES;
  fail_multiple_msgs_in_transmission = GNUNET_YES;

  addr_head = NULL;
  count_str_addr = 0;
  /* parse configuration */
  if ((GNUNET_OK !=
       GNUNET_CONFIGURATION_get_value_number (c,
                                              "TRANSPORT",
                                              "NEIGHBOUR_LIMIT",
                                              &tneigh)) ||
      (GNUNET_OK !=
       GNUNET_CONFIGURATION_get_value_filename (c,
                                                "GNUNETD",
                                                "HOSTKEY", &keyfile)))
    {
      GNUNET_log (GNUNET_ERROR_TYPE_ERROR,
                  _
                  ("Transport service is lacking key configuration settings.  Exiting.\n"));
      GNUNET_SCHEDULER_shutdown (s);
      fail = 1;
      return;
    }

  if ((GNUNET_OK !=
      GNUNET_CONFIGURATION_get_value_number (cfg,
                                             "transport-http",
                                             "PORT",
                                             &port)) ||
     (port > 65535) || (port == 0))
  {
    GNUNET_log_from (GNUNET_ERROR_TYPE_ERROR,
                     "http",
                     _
                     ("Require valid port number for transport plugin `%s' in configuration!\n"),
                     "transport-http");
  }

  max_connect_per_transport = (uint32_t) tneigh;
  my_private_key = GNUNET_CRYPTO_rsa_key_create_from_file (keyfile);
  GNUNET_free (keyfile);
  if (my_private_key == NULL)
    {
      GNUNET_log (GNUNET_ERROR_TYPE_ERROR,
                  _("Transport service could not access hostkey.  Exiting.\n"));
      GNUNET_SCHEDULER_shutdown (s);
      fail = 1;
      return;
    }

  GNUNET_CRYPTO_rsa_key_get_public (my_private_key, &my_public_key);
  GNUNET_CRYPTO_hash (&my_public_key, sizeof (my_public_key), &my_identity.hashPubKey);

  /* assertions before start */
  GNUNET_assert ((port > 0) && (port <= 65535));
  GNUNET_assert(&my_public_key != NULL);
  GNUNET_assert(&my_identity.hashPubKey != NULL);

  /* load plugins... */
  setup_plugin_environment ();
  GNUNET_log (GNUNET_ERROR_TYPE_INFO, _("Loading HTTP transport plugin `%s'\n"),"libgnunet_plugin_transport_http");
  GNUNET_asprintf (&libname, "libgnunet_plugin_transport_http");
  api = GNUNET_PLUGIN_load (libname, &env);
  GNUNET_free (libname);
  if (api == NULL)
  {
    GNUNET_log (GNUNET_ERROR_TYPE_ERROR,
                _("Failed to load transport plugin for http\n"));
    fail = 1;
    return;
  }

  ti_timeout = GNUNET_SCHEDULER_add_delayed (sched, TEST_TIMEOUT, &task_timeout, NULL);

  /* testing plugin functionality */
  GNUNET_assert (0!=fail_notify_address_count);
  GNUNET_log (GNUNET_ERROR_TYPE_INFO, _("Transport plugin returned %u addresses to connect to\n"),  fail_notify_address_count);

  /* testing pretty printer with all addresses obtained from the plugin*/
  cur = addr_head;
  while (cur != NULL)
  {

    api->address_pretty_printer (api->cls, "http",cur->addr,cur->addrlen, GNUNET_NO,TEST_TIMEOUT, &pretty_printer_cb,NULL);
    addr_str = api->address_to_string (api->cls, cur->addr, cur->addrlen);
    suggest_res = api->check_address (api->cls, cur->addr, cur->addrlen);

    GNUNET_assert (GNUNET_OK == suggest_res);
    GNUNET_assert (NULL != addr_str);
    count_str_addr++;
    GNUNET_free ( (char *) addr_str);
    cur = cur->next;
  }
  GNUNET_assert (fail_pretty_printer_count > 0);
  GNUNET_assert (fail_pretty_printer_count==fail_notify_address_count);
  GNUNET_assert (fail_pretty_printer_count==count_str_addr);
  fail_pretty_printer=GNUNET_NO;
  fail_addr_to_str=GNUNET_NO;

  /* Suggesting addresses with wrong port*/
  struct IPv4HttpAddress failing_addr;
  failing_addr.ipv4_addr = htonl(INADDR_LOOPBACK);
  failing_addr.u_port = htons(0);
  suggest_res = api->check_address (api->cls,&failing_addr,sizeof (struct IPv4HttpAddress));
  GNUNET_assert (GNUNET_SYSERR == suggest_res);

  /* Suggesting addresses with wrong size*/
  failing_addr.ipv4_addr = htonl(INADDR_LOOPBACK);
  failing_addr.u_port = htons(0);
  suggest_res = api->check_address (api->cls,&failing_addr,sizeof (struct IPv6HttpAddress));
  GNUNET_assert (GNUNET_SYSERR == suggest_res);

  /* Suggesting addresses with wrong address*/
  failing_addr.ipv4_addr = htonl(INADDR_LOOPBACK);
  failing_addr.u_port = htons(12389);
  suggest_res = api->check_address (api->cls,&failing_addr,sizeof (struct IPv4HttpAddress));
  GNUNET_assert (GNUNET_SYSERR == suggest_res);

  /* test sending to client */
  multi_handle = curl_multi_init();

  /* Setting up buffers */
  buffer_in.size = HTTP_BUFFER_SIZE;
  buffer_in.pos = 0;
  buffer_in.len = 0;

  buffer_out.size = HTTP_BUFFER_SIZE;
  buffer_out.pos = 0;
  buffer_out.len = 0;

  /* Setting up connection tests */

  /* Test: connecting without a peer identification */
  test_no_ident.test_executed = GNUNET_NO;
  test_no_ident.test_failed = GNUNET_YES;

  /* Test: connecting with too short peer identification */
  test_too_short_ident.test_executed = GNUNET_NO;
  test_too_short_ident.test_failed = GNUNET_YES;

  /* Test: connecting with too long peer identification */
  test_too_long_ident.test_executed = GNUNET_NO;
  test_too_long_ident.test_failed = GNUNET_YES;

  /* Test: connecting with valid identification */
  test_valid_ident.test_executed = GNUNET_NO;
  test_valid_ident.test_failed = GNUNET_YES;

  test_addr = (char *) api->address_to_string (api->cls,addr_head->addr,addr_head->addrlen);
  run_connection_tests();

  /* testing finished */

  return;
}


/**
 * The main function for the transport service.
 *
 * @param argc number of arguments from the command line
 * @param argv command line arguments
 * @return 0 ok, 1 on error
 */
int
main (int argc, char *const *argv)
{

  static struct GNUNET_GETOPT_CommandLineOption options[] = {
    GNUNET_GETOPT_OPTION_END
  };
  int ret;
  char *const argv_prog[] = {
    "test_plugin_transport_http",
    "-c",
    "test_plugin_transport_data_http.conf",
    "-L",
#if VERBOSE
    "DEBUG",
#else
    "WARNING",
#endif
    NULL
  };
  GNUNET_log_setup ("test_plugin_transport_http",
#if VERBOSE
                    "DEBUG",
#else
                    "WARNING",
#endif
                    NULL);

  ret = (GNUNET_OK ==
         GNUNET_PROGRAM_run (5,
                             argv_prog,
                             "test_plugin_transport_http",
                             "testcase", options, &run, NULL)) ? GNUNET_NO : GNUNET_YES;

    GNUNET_DISK_directory_remove ("/tmp/test_plugin_transport_http");

  return fail;
}

/* end of test_plugin_transport_http.c */
