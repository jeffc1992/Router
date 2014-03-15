/**********************************************************************
 * file:  sr_router.c
 * date:  Mon Feb 18 12:50:42 PST 2002
 * Contact: casado@stanford.edu
 *
 * Description:
 *
 * This file contains all the functions that interact directly
 * with the routing table, as well as the main entry method
 * for routing.
 *
 **********************************************************************/

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "sr_if.h"
#include "sr_rt.h"
#include "sr_router.h"
#include "sr_protocol.h"
#include "sr_arpcache.h"
#include "sr_utils.h"

/*---------------------------------------------------------------------
 * Method: sr_init(void)
 * Scope:  Global
 *
 * Initialize the routing subsystem
 *
 *---------------------------------------------------------------------*/

void sr_init(struct sr_instance* sr)
{
    /* REQUIRES */
    assert(sr);

    /* Initialize cache and cache cleanup thread */
    sr_arpcache_init(&(sr->cache));

    pthread_attr_init(&(sr->attr));
    pthread_attr_setdetachstate(&(sr->attr), PTHREAD_CREATE_JOINABLE);
    pthread_attr_setscope(&(sr->attr), PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setscope(&(sr->attr), PTHREAD_SCOPE_SYSTEM);
    pthread_t thread;

    pthread_create(&thread, &(sr->attr), sr_arpcache_timeout, sr);
    
    /* Add initialization code here! */

} /* -- sr_init -- */

/*---------------------------------------------------------------------
 * Method: sr_handlepacket(uint8_t* p,char* interface)
 * Scope:  Global
 *
 * This method is called each time the router receives a packet on the
 * interface.  The packet buffer, the packet length and the receiving
 * interface are passed in as parameters. The packet is complete with
 * ethernet headers.
 *
 * Note: Both the packet buffer and the character's memory are handled
 * by sr_vns_comm.c that means do NOT delete either.  Make a copy of the
 * packet instead if you intend to keep it around beyond the scope of
 * the method call.
 *
 *---------------------------------------------------------------------*/

void sr_handlepacket(struct sr_instance* sr,
        uint8_t * packet/* lent */,
        unsigned int len,
        char* interface/* lent */)
{
  /* REQUIRES */
  assert(sr);
  assert(packet);
  assert(interface);
  int i = 0; 
  uint16_t ethtype= ethertype(packet);
  sr_arp_hdr_t* arp_hdr = packet+ sizeof(sr_ethernet_hdr_t);
  struct sr_arpentry *entry= NULL;
  uint8_t * new_packet = malloc(len*sizeof(uint8_t));
  sr_arp_hdr_t *new_arp_hdr =new_packet + sizeof(sr_ethernet_hdr_t);


  if(ethtype == ethertype_arp) {
        entry= sr_arpcache_lookup((&(sr->cache)), ntohl(arp_hdr->ar_tip));
	if(entry == NULL) {
		printf("Jeff: The hardware ID is not in the table!\n");
	} else {
		
		printf("Jeff: The entry is in the table!\n");
		new_arp_hdr->ar_hrd = htons(1);
		new_arp_hdr->ar_pro = htons(0x800);
		new_arp_hdr->ar_hln = 6;
		new_arp_hdr->ar_pln = 4; 
		new_arp_hdr->ar_op = htons(2); 
		for(i=0; i<ETHER_ADDR_LEN; i++) {
			new_arp_hdr->ar_sha[i] = entry->mac[i];
			new_arp_hdr->ar_tha[i] = arp_hdr->ar_sha[i];
		}
		new_arp_hdr->ar_sip = htonl(entry->ip);
		new_arp_hdr->ar_tip = arp_hdr->ar_sip;
		if(sr_send_packet(sr, new_packet, (len*sizeof(uint8_t)), interface)==-1){
			printf("SEND PACKET FAILED!\n");
		}   		
	}
 }
}/* end sr_ForwardPacket */

