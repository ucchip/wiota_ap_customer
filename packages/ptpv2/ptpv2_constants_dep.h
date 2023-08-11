
/* constants_dep.h */

#ifndef CONSTANTS_DEP_H
#define CONSTANTS_DEP_H

/**
*\file
* \brief Plateform-dependent constants definition
*
* This header defines all includes and constants which are plateform-dependent
*
* ptpdv2 is only implemented for linux, NetBSD and FreeBSD
 */

/* platform dependent */

#include <sys/types.h>
#include <sys/socket.h>

#define IF_NAMESIZE 2
#define INET_ADDRSTRLEN 16

#define IFACE_NAME_LENGTH IF_NAMESIZE
#define NET_ADDRESS_LENGTH INET_ADDRSTRLEN

#define IFCONF_LENGTH 10

#define adjtimex ntp_adjtime

#include <machine/endian.h>

#define CLOCK_IDENTITY_LENGTH 8
#define ADJ_FREQ_MAX 512000

#define ADJ_FREQUENCY 0x0002 /* frequency offset */

/* NTP userland likes the MOD_ prefix better */
#define MOD_FREQUENCY ADJ_FREQUENCY

/* UDP/IPv4 dependent */

#define SUBDOMAIN_ADDRESS_LENGTH 4
#define PORT_ADDRESS_LENGTH 2
#define PTP_UUID_LENGTH 6
#define CLOCK_IDENTITY_LENGTH 8
#define FLAG_FIELD_LENGTH 2

#define PACKET_SIZE 300 //ptpdv1 value kept because of use of TLV...

#define PTP_EVENT_PORT 319
#define PTP_GENERAL_PORT 320

#define DEFAULT_PTP_DOMAIN_ADDRESS "224.0.1.129"
#define PEER_PTP_DOMAIN_ADDRESS "224.0.0.107"

#define MM_STARTING_BOUNDARY_HOPS 0x7fff

/* others */

#define SCREEN_BUFSZ 128
#define SCREEN_MAXSZ 80
#endif /*CONSTANTS_DEP_H_*/
