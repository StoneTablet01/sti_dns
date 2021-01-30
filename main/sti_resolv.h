/** @copyright
 * lwip DNS resolver header file.

 * Author: Jim Pettinato
 *   April 2007

 * ported from uIP resolv.c Copyright (c) 2002-2003, Adam Dunkels.
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
 */
#ifndef STI_RESOLV_H
#define STI_RESOLV_H

/** @brief Initialize this resolver
  *
  * Create a UDP connection with the DNS server so that DNS record queries can be made
  *
  * @note This function uses lwip directly. Other IP implementations will need to
  * provide there own IP stack implementations
  *
  * @param dnsserver_ip_addr_ptr  the IP address of the DNS Server.
  * @returns ERR_OK: UDP connection succeeded LWIP error code
  */
err_t
resolv_init(ip_addr_t *dnsserver_ip_addr_ptr); /* working to pass ip_addr_t*/

/** @brief close the UDP connection and free the memory
  */
err_t
resolv_close(void);

/** @brief a full function resolv query
  * this function allows small computers to get a return
  * buffer from the dns server
  */
int
res_query(const char *dname, int class, int type, unsigned char *answer, int anslen);

/** @brief get_qname_len() - Walk through the encoded answer buffer and return
 * the length of the encoded name in chars.
 *---------------------------------------------------------------------------*/
int
get_qname_len(unsigned char *name_ptr);

/** print_buf prints out a buffer. This makes it easier to troubleshoot
  * buffers sent or ceived from the DNS server */
void print_buf(unsigned char *buf, int length);

#endif /* STI_RESOLV_H */
