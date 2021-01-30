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
 * Nest step continued cleanup



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

/* The maximum number of table entries to maintain locally */
#ifndef LWIP_RESOLV_ENTRIES
#define LWIP_RESOLV_ENTRIES 4
#endif

#ifndef DNS_SERVER_PORT
#define DNS_SERVER_PORT 53
#endif


#define MESSAGE_HEADER_LEN 12
#define MESSAGE_RESPONSE 1
#define MESSAGE_T_SRV 33
#define MESSAGE_C_IN 1

#define MAX_DOMAIN_LEN 25

/** @brief The DNS message header. \n
  The DNS header is 12 8-bit bytes and is defined in RFC-1035\n
  The header is used to send queries to DNS server. The header is also part of
  the answer returned by the DNS server.
  @note order of the fields in the struct must not be changed as the struct is used as
  an overlay that allows information to be extracted from the returned answer buffer*/
typedef struct s_dns_hdr {
  u16_t id; /**< ID Number of the request */
  u8_t flags1; /**< Flags for QR| Opcode |AA|TC|RD */
  u8_t flags2; /**< Flags for RA| Z | RCODE | */
#define DNS_FLAG1_RESPONSE        0x80
#define DNS_FLAG1_OPCODE_STATUS   0x10
#define DNS_FLAG1_OPCODE_INVERSE  0x08
#define DNS_FLAG1_OPCODE_STANDARD 0x00
#define DNS_FLAG1_AUTHORATIVE     0x04
#define DNS_FLAG1_TRUNC           0x02
#define DNS_FLAG1_RD              0x01
#define DNS_FLAG2_RA              0x80
#define DNS_FLAG2_ERR_MASK        0x0f
#define DNS_FLAG2_ERR_NONE        0x00
#define DNS_FLAG2_ERR_NAME        0x03
  u16_t numquestions; /**< Number of questions asked of DNS server */
  u16_t numanswers; /**< Number of answers from DNS server */
  u16_t numauthrr; /**< number of name server resource records in the authority records*/
  u16_t numextrarr; /** Number of extra records in the reply */
} DNS_HDR;

/** DNS answer message structure for "A" type record requests.
  The DNS answer starts with either a domain name or a pointer
  to a name already present somewhere in the packet (buffer). After the name, additional
  data follows. The purpose of this structure is to extract this information by using
  the structure as an overlay on the buffer received from the DNS Server. Details of
  the order and type of the data are defined in RFC-1035.
  @note This structure can only be used on Class 1 Type 1 requests which return a 4 character
  IP address.
  @note order of the fields in the struct must not be changed as the struct is used as
  an overlay that allows information to be extracted from the returned answer buffer*
  @note all multi-byte data in the buffer is organized as high order byte / low
  order byte (BIG ENDIAN). Adjustments must be made using the htons or ntohs fucncitons
  to allow use on both Big and Little Endian machines.*/
typedef struct s_dns_answer {
  u16_t type; /**< specifies the meaning of the data in the RDATA field. */
  u16_t class; /**< Class 0x0001 represents Internet addresses */
  u16_t ttl[2]; /**< The number of seconds the results can be cached */
  u16_t len; /**< The length of the RDATA field. Four (4) for IP4 addresses */
  char ipchars[4]; /**< IPaddr organized as four addresses (ie. 192.168.1.1) in BE format */
} DNS_ANSWER;


static struct udp_pcb *resolv_pcb = NULL; /**< UDP connection to DNS server */
static struct ip4_addr serverIP; /**<the adress of the DNS server to use */
static u8_t initFlag; /**< set to 1 if initialized*/
static u8_t respFlag = 0; /**< set to 1 if responce received*/
static u8_t payload_len = 0; /**< length of the received payload buffer*/
static unsigned char * user_buffer_ptr;


/** print_buf prints out a buffer. This makes it easier to troubleshoot
  * buffers sent or ceived from the DNS server */
void print_buf(unsigned char *buf, int length) {
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
  } // check printer buffer end *
}

/** @brief * get_qname_len() - Walk through the encoded answer buffer and return
 * the length of the encoded name in chars. length of zero indicates a Problem
 * condition
 *---------------------------------------------------------------------------*/
int
get_qname_len(unsigned char *name_ptr){

  int qname_len =0;

  while(qname_len < MAX_NAME_LENGTH){
    if (*name_ptr == 0){
      qname_len++;
      break;
    }
    else if (*name_ptr == 0xC0){
      qname_len += 2;
      break;
    }
    else{
      qname_len += (*name_ptr + 1);
      name_ptr += (*name_ptr + 1);
    }
  } //end while loop

  if (qname_len == MAX_NAME_LENGTH){
    qname_len = 0;
  }
  return qname_len;
}

/**Querry a DNS server and return a buffer with the answer(s)
 * The res_query_jps() function provides an interface to the server query mechanism.
 * It constructs a query, sends it to the DNS server, awaits a response, and
 * makes preliminary checks on the reply. The query requests information of the
 * specified type and class for the specified fully-qualified domain name dname.
 * The reply message is left in the answer buffer
 */
int
res_query_jps(const char *dname, int class, int type, unsigned char *answer, int anslen){
  static const char *TAG = "res_query_jps";
  ESP_LOGI(TAG, "");
  ESP_LOGI(TAG, ".Begin res_query_jps function");

  static u8_t n;
  DNS_HDR *hdr;
  struct pbuf *p;
  char *query, *nptr;
  const char *pHostname;

  p = pbuf_alloc(PBUF_TRANSPORT, sizeof(DNS_HDR)+MAX_NAME_LENGTH+5, PBUF_RAM);
  hdr = (DNS_HDR *)p->payload;
  memset(hdr, 0, sizeof(DNS_HDR));

  // make start of answer header globally available
  user_buffer_ptr = answer;

  /* Fill in header information observing Big Endian / Little Endian considerations*/
  hdr->id = htons(99);
  hdr->flags1 = DNS_FLAG1_RD; //This is 8bits so no need to worry about htons
  hdr->numquestions = htons(1);
  query = (char *)hdr + sizeof(DNS_HDR);

  /* Convert hostname into suitable query format. */
  pHostname = dname;
  --pHostname;
  int qname_len = 0;
  do
  {
    ++pHostname;
    nptr = query;
    ++query;
    for(n = 0; *pHostname != '.' && *pHostname != 0; ++pHostname)
    {
      *query = *pHostname;
      ++query;
      ++n;
      qname_len ++;
    }
    *nptr = n;
    qname_len ++;
  }
  while(*pHostname != 0);

  // complete the question by (1) terminating the QNAME with 0, (2) specifying
  // QTYPE and (3) specifying QCLASS
  static unsigned char endquery[] = {0,0,1,0,1};
  endquery[2] = (unsigned char) type;
  endquery[4] = (unsigned char) class;

  memcpy(query, endquery, 5);

  pbuf_realloc(p, sizeof(DNS_HDR) + qname_len + 5);
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

  ESP_LOGI(TAG, "...payload length from parse = %d", payload_len);

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

  char *pHostname;
  //const char *aHostname; /*pointer for parsing returned buffer*/
  DNS_ANSWER *ans;
  DNS_HDR *hdr;

  //static u8_t nquestions,
  static u8_t nanswers;
  //static u8_t i;
  //unsigned char * buf_char_ptr;
  respFlag = 1;

  ESP_LOGI(TAG, "....Buffer length from tot_len is %d", p->len);

  hdr = (DNS_HDR *)p->payload;


  // next section if only asking for id 99 - no need to do anything with tables

  payload_len = 12; /*header length*/
  payload_len += get_qname_len((unsigned char *)p->payload + 12); /*qname len*/
  payload_len += 4; /* Query Type and Query Class*/
  pHostname = p->payload + payload_len; //pHostname now points to the start of the RR
  nanswers = htons(hdr->numanswers);

  while(nanswers > 0){
    /* The first byte in the answer resource record determines if it
       is a compressed record or a normal one. */

    int rr_name_len =0; //resource record name length
    rr_name_len = get_qname_len( (unsigned char *) pHostname);
    //ESP_LOGI(TAG, "....rr_name_len = %d", rr_name_len);
    payload_len += rr_name_len;
    pHostname += rr_name_len; //phostname now points to first byte Post Hostname in RR

    ans = (DNS_ANSWER *)pHostname;
    /* printf("Answer: type %x, class %x, ttl %x, length %x\n",
       htons(ans->type), htons(ans->class), (htons(ans->ttl[0])
         << 16) | htons(ans->ttl[1]), htons(ans->len)); */

    /* dtermine if the anser is for an A type query or and SRV query */
    if((htons(ans->type) == 1) && (htons(ans->class) == 1) && (htons(ans->len) == 4) ){
      payload_len += sizeof(DNS_ANSWER);
      //ESP_LOGI(TAG, "....Header + Question + Name + DNS_Answer  length %d", payload_len);
    }
    else{
      if((htons(ans->type) == 33) && (htons(ans->class) == 1) ){
        payload_len += (sizeof(DNS_ANSWER) - 4 + htons(ans->len));
        //ESP_LOGI(TAG, "....Header + Question + Compressed Name length %d", payload_len);
      }
    }
    --nanswers;
  }
  memcpy(user_buffer_ptr, p->payload, payload_len);
  free(p);
  return;
}

err_t
resolv_init(ip_addr_t *dnsserver_ip_addr_ptr) {
  static const char *TAG = "resolv init ";
  ESP_LOGI(TAG, "...dnsserver is                : " IPSTR, IP2STR(&dnsserver_ip_addr_ptr->u_addr.ip4));
  //static u8_t i;

  serverIP.addr = dnsserver_ip_addr_ptr->u_addr.ip4.addr;

  if(resolv_pcb != NULL){
    ESP_LOGI(TAG, "...resolv_pcb exists...delete it");
    udp_remove(resolv_pcb);
  }
    /* TODO: check for valid IP address for DNS server? */
  resolv_pcb = udp_new();
  udp_bind(resolv_pcb, IP_ADDR_ANY, 0);

  err_t ret;
  ret = udp_connect(resolv_pcb, dnsserver_ip_addr_ptr, DNS_SERVER_PORT);
  if (ret < 0 ){

    ESP_LOGI(TAG, "...udp connect failed to       : " IPSTR, IP2STR(&dnsserver_ip_addr_ptr->u_addr.ip4));
  }
  else{
    ESP_LOGI(TAG, "...udp connected to            : " IPSTR, IP2STR(&dnsserver_ip_addr_ptr->u_addr.ip4));
  }

  typedef void(* udp_recv_fn) (void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
  udp_recv_fn udp_r = &resolv_recv;
  udp_recv (resolv_pcb, udp_r, NULL);

  initFlag = 1;
  return ERR_OK;
}
