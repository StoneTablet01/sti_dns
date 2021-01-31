/*

 * DNS host name to IP address resolver.
 * This file implements a DNS host name to IP address resolver.

 * Port to lwIP from uIP
 * by Jim Pettinato April 2007

 * uIP version Copyright (c) 2002-2003, Adam Dunkels.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * RESOLV.C
 *
 * The lwIP DNS resolver functions are used to lookup a host name and
 * map it to a numerical IP address. It maintains a list of resolved
 * hostnames that can be queried with the resolv_lookup() function.
 * New hostnames can be resolved using the resolv_query() function.
 *
 * The lwIP version of the resolver also adds a non-blocking version of
 * gethostbyname() that will work with a raw API application. This function
 * checks for an IP address string first and converts it if it is valid.
 * gethostbyname() then does a resolv_lookup() to see if the name is
 * already in the table. If so, the IP is returned. If not, a query is
 * issued and the function returns with a QUERY_QUEUED status. The app
 * using the resolver must then go into a waiting state.
 *
 * Once a hostname has been resolved (or found to be non-existent),
 * the resolver code calls a specified callback function (which
 * must be implemented by the module that uses the resolver).
 *
 * Jim Sutton made the code work on the ESP32 chip. Big Problem
 * was how IP addresses were stored
 */

#include <string.h>
#include <ctype.h>
#include "lwip/stats.h"
#include "lwip/mem.h"
#include "lwip/udp.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/inet.h"
#include "netif/etharp.h"
#include "lwip/sys.h"
#include "lwip/opt.h"

#include "sti_resolv.h"
//#include "esp_system.h"
//#include "esp_event.h"
#include "esp_log.h"
//added to get error checks
#include "esp_netif.h"
//#include "esp_netif_ppp.h"

/* The maximum length of a host name supported in the name table. */
#define MAX_NAME_LENGTH 32
/* The maximum number of retries when asking for a name. */
#define MAX_RETRIES 8

#ifndef DNS_SERVER_PORT
#define DNS_SERVER_PORT 53
#endif

#define DNS_FLAG1_RD 0x01 // DNS recursion requested

/** @brief The DNS message header. \n
  The DNS header is 12 x 8-bit bytes and is defined in RFC-1035\n
  The header is used to send queries to DNS server. The header is also part of
  the answer returned by the DNS server.
  @note order of the fields in the struct must not be changed as the struct is used as
  an overlay that allows information to be extracted from the returned answer buffer*/
typedef struct s_RFC1035_HDR {
  u16_t id; /**< ID Number of the request */
  u8_t flags1; /**< Flags for QR| Opcode |AA|TC|RD */
  u8_t flags2; /**< Flags for RA| Z | RCODE | */
  u16_t qdcount; /**< number of entries in the question section */
  u16_t ancount; /**< number of resource records in the answer section */
  u16_t nscount; /**< no. of name server resource records in the authority records*/
  u16_t arcount; /**< number of resource records in the additional records section */
} RFC1035_HDR;

static struct udp_pcb *resolv_pcb = NULL; /**< UDP connection to DNS server */
static u8_t initFlag; /**< set to 1 if initialized*/
static u8_t respFlag = 0; /**< set to 1 if responce received*/
static u8_t payload_len = 0; /**< length of the received payload buffer*/
static unsigned char * user_buffer_ptr;

/** print_buf function prints out a buffer to terminal. This makes it easier to troubleshoot
  * buffers sent to or received from the DNS server */
void
print_buf(unsigned char *buf, int length){
  unsigned char * buf_char_ptr;
  static const char *TAG = "print_buf   ";
  buf_char_ptr = buf;
  for (int i=0; i < length; i++){
    if ((*buf_char_ptr > 64 && *buf_char_ptr <91) ||
      (*buf_char_ptr > 96 && *buf_char_ptr <123)){
      ESP_LOGI(TAG, "....%d Letter in received buffer: %c", i+1, *buf_char_ptr);
    }
    else{
      ESP_LOGI(TAG, "....%d Hex in received buffer   : %X", i+1, *buf_char_ptr);
    }
    buf_char_ptr++;
  }
}

int
format_hostname(unsigned char * dname, unsigned char * qname){
  int encoded_len;
  int subname_len = 0;
  unsigned char * num_ptr;

  num_ptr = qname;
  qname ++;
  for (int n = 1; n < (MAX_NAME_LENGTH + 1); n++){
    encoded_len = n; //number of char in buffer
    if( *dname == 0 ){
      *num_ptr = subname_len;
      break;
    }
    else if(*dname == '.'){
      *num_ptr = subname_len;
      num_ptr = qname;
      dname++;
      qname++;
      subname_len = 0;
    }
    else{
        *qname = *dname;
        subname_len ++;
        dname++;
        qname++;
    }
  }
  if(encoded_len >= MAX_NAME_LENGTH) encoded_len = 0;

  return encoded_len;
}

/** @brief res_query querries a DNS server and return a buffer with the answer(s)
 * The res_query() function provides an interface to the server query mechanism.
 * It constructs a query, sends it to the DNS server, awaits a response, and
 * makes preliminary checks on the reply. The reply message is left in the answer buffer
 */
int
res_query(const char *dname, int class, int type, unsigned char *answer, int anslen){
  static const char *TAG = "res_query";
  int qname_len;

  RFC1035_HDR *hdr;
  struct pbuf *p;
  unsigned char *query;

  ESP_LOGI(TAG, ".Begin res_query function");

  p = pbuf_alloc(PBUF_TRANSPORT, sizeof(RFC1035_HDR)+MAX_NAME_LENGTH+5, PBUF_RAM);
  hdr = (RFC1035_HDR *)p->payload;
  memset(hdr, 0, sizeof(RFC1035_HDR));

  user_buffer_ptr = answer;   // make start of answer header globally available

  /* Fill in header information observing Big Endian / Little Endian considerations*/
  hdr->id = htons(99);
  hdr->flags1 = DNS_FLAG1_RD; //This is 8bits so no need to worry about htons
  hdr->qdcount = htons(1); // number of questions
  query = (unsigned char *)hdr + sizeof(RFC1035_HDR);

  qname_len = format_hostname((unsigned char *) dname, (unsigned char *) query);
  query += qname_len;

  // complete the question by (1) terminating the QNAME with 0, (2) specifying
  // QTYPE and (3) specifying QCLASS
  static unsigned char endquery[5];
  endquery[0] = 0;                      // terminate the query name
  endquery[1] = 0;                      // MSB request type
  endquery[2] = (unsigned char) type;   // LSB request type
  endquery[3] = 0;                      // MSB request class
  endquery[4] = (unsigned char) class;  // LSB request class
  memcpy(query, endquery, 5);

  pbuf_realloc(p, sizeof(RFC1035_HDR) + qname_len + 5);
  respFlag = 0; //clear responce flag. It will be set to 1 when buffer received

  udp_send(resolv_pcb, p);
  ESP_LOGI(TAG, "...query sent to DNS server" );
  pbuf_free(p);

  for (int time =0; time < 10; time++){
    vTaskDelay(200 / portTICK_PERIOD_MS);
    if (respFlag == 1){
      break;
    }
  }
  if ( respFlag != 1){
    return 0;
  }

  return payload_len;
}

/*---------------------------------------------------------------------------*
 *
 * Callback for DNS responses
 *
 *---------------------------------------------------------------------------*/
static void
resolv_recv(void *s, struct udp_pcb *pcb, struct pbuf *p,
                                  const ip_addr_t *addr, u16_t port)
{
  const char* TAG = "resolv_recv ";
  ESP_LOGI(TAG, "...resolv_recv function called");
  respFlag = 1;

  ESP_LOGI(TAG, "....Buffer length from tot_len is %d", p->len);
  payload_len = p->len;

  memcpy(user_buffer_ptr, p->payload, payload_len);
  free(p);
  return;
}

err_t
resolv_init(ip_addr_t *dnsserver_ip_addr_ptr) {
  static const char *TAG = "resolv init ";
  err_t ret;

  if(resolv_pcb != NULL){
    ESP_LOGI(TAG, "...resolv_pcb exists...delete it");
    udp_remove(resolv_pcb);
  }

  resolv_pcb = udp_new();
  udp_bind(resolv_pcb, IP_ADDR_ANY, 0);

  ret = udp_connect(resolv_pcb, dnsserver_ip_addr_ptr, DNS_SERVER_PORT);
  if (ret < 0 ){
    ESP_LOGI(TAG, "...udp connect failed to: " IPSTR, IP2STR(&dnsserver_ip_addr_ptr->u_addr.ip4));
    return ERR_CONN;
  }
  else{
    ESP_LOGI(TAG, "...udp connected to: " IPSTR, IP2STR(&dnsserver_ip_addr_ptr->u_addr.ip4));
  }

  typedef void(* udp_recv_fn) (void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
  udp_recv_fn udp_r = &resolv_recv;
  udp_recv (resolv_pcb, udp_r, NULL);

  initFlag = 1;
  return ERR_OK;
}

err_t
resolv_close(void) {
  udp_remove(resolv_pcb);
  resolv_pcb = NULL;
  initFlag = 0;
  return ERR_OK;
}
