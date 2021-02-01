/** @file sti_resolv.h
 *  @brief Functions to get DNS information
 *
 *  This contains the prototypes for the functions necessary to get DNS
 *  tyoe A information and DNS Type SRV information from DNS Servers. Key references are
 *  (1) rfc 1035 DOMAIN NAMES - IMPLEMENTATION AND SPECIFICATION
 *  (2) rfc 2782 A DNS RR for specifying the location of services (DNS SRV)
 *  (3) "DNS Primer" from Duke University (search for "CPS365 FALL 2016 DNS-Primer")
 *  (4) Port to lwIP from uIP by Jim Pettinato April 2007
 *  (5) Uip Implementation by Adam Dunkels
 *
 *  Copyright 2021 Jim Sutton <jamespsutton@cox.net>
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is furnished
 *  to do so, subject to the following conditions:
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *
 *  @author Jim Sutton <jamespsutton@cox.net>
 *  @bug No known bugs.
 */

#ifndef STI_RESOLV_H
#define STI_RESOLV_H

/** @brief Initialize this resolver
  *
  * Create a UDP connection with the DNS server so that DNS record queries can be made
  *
  * @note This function uses lwip directly. Other IP stack implementations will need to
  * provide there own IP stack implementations
  *
  * @param dnsserver_ip_addr_ptr  the IP address of the DNS Server.
  * @returns ERR_OK: UDP connection succeeded LWIP error code*/
err_t
resolv_init(ip_addr_t *dnsserver_ip_addr_ptr); /* working to pass ip_addr_t*/

/** @brief close the UDP connection and free the memory
  */
err_t
resolv_close(void);

/** @brief full function resolv query to get type A and type SRV records
  *
  * this function allows small computers to get a return buffers from the dns server
  * @param *dname  the domain name information is sought for
  * @param class  the class as specified by RFC 1035 (expect Internet Class)
  * @param type  the type as specified by RFC 1035 (expect type A or SRV)
  * @param *answer  a pointer to the buffer the DNS result should be loaded into
  * @param anslen the length of the supplied buffer
  * @returns int the length of the received buffer (number of 8 bit bytes)
  */
int
res_query(const char *dname, int class, int type, unsigned char *answer, int anslen);

/** @brief get_qname_len() - Walk through the encoded answer buffer and return
 * the length of the encoded name in chars.
 *---------------------------------------------------------------------------*/
int
get_qname_len(unsigned char *name_ptr);

/** @brief print_buf prints out a buffer. This makes it easier to troubleshoot
  * buffers sent or received from the DNS server */
void print_buf(unsigned char *buf, int length);

/** @brief format_hostname to network format.
  * Takes as input a full hostname of subnames separated by decimal points
  * and loads an output buffer with subnames preceded by subname length
  * this is well described in RFC1035
  * @param dname the domain name information is desired for
  * @param qname a pointer to the location the encoded hostname needs to be written
  * @returns The lenght of the encoded hostname in 8 bt bytes*/
int
format_hostname(unsigned char * dname, unsigned char * qname);

#endif /* STI_RESOLV_H */
