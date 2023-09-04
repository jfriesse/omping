/*
 * Copyright (c) 2010-2011, Red Hat, Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND RED HAT, INC. DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL RED HAT, INC. BE LIABLE
 * FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Author: Jan Friesse <jfriesse@redhat.com>
 */

#define __STDC_LIMIT_MACROS

#include <sys/types.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <err.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "addrfunc.h"
#include "omping.h"
#include "cli.h"
#include "cliprint.h"
#include "logging.h"

/*
 * Parse command line.
 * argc and argv are passed from main function. instance is omping instance. Instance will be filled
 * with following items. local_ifname will be allocated and filled by name
 * of local ethernet interface. ip_ver will be filled by forced ip version or will
 * be 0. mcast_addr will be filled by requested mcast address or will be NULL. Port will be filled
 * by requested port (string value) or will be NULL. aii_list will be initialized and requested
 * hostnames will be stored there. ttl is pointer where user set TTL or default TTL will be stored.
 * single_addr is boolean set if only one remote address is entered. quiet is flag for quiet mode.
 * cont_stat is flag for enable continuous statistic. timeout_time is number of miliseconds after
 * which client exits regardless to number of received/sent packets. wait_for_finish_time is number
 * of miliseconds to wait before exit to allow other nodes not to screw up final statistics.
 * dup_buf_items is number of items which should be stored in duplicate packet detection buffer.
 * Default is MIN_DUP_BUF_ITEMS for intervals > 1, or DUP_BUF_SECS value divided by ping interval
 * in seconds or 0, which is used for disabling duplicate detection. rate_limit_time is maximum
 * time between two received packets. sndbuf_size is size of socket buffer to allocate for sending
 * packets. rcvbuf_size is size of socket buffer to allocate for receiving packets. Both
 * sndbuf_size and rcvbuf_size are set to 0 if user doesn't supply option. send_count_queries is by
 * default set to 0, but may be overwritten by user and it means that after sending that number of
 * queries, client is put to stop state. auto_exit is boolean variable which is enabled by default
 * and can be disabled by -E option. If auto_exit is enabled, loop will end if every client is in
 * STOP state.
 */
int
cli_parse(int argc, char * const argv[], struct omping_instance *instance)
{
	struct ai_item *ai_item;
	struct ifaddrs *ifa_list, *ifa_local;
	char *ep;
	char *mcast_addr_s;
	const char *port_s;
	double numd;
	int ch;
	int force;
	int no_ai;
	int num;
	int res;
	int rate_limit_time_set;
	int show_ver;
	int wait_for_finish_time_set;
	unsigned int ifa_flags;

	instance->auto_exit = 1;
	instance->cont_stat = 0;
	instance->dup_buf_items = MIN_DUP_BUF_ITEMS;
	instance->ip_ver = 0;
	instance->local_ifname = NULL;
	mcast_addr_s = NULL;
	instance->op_mode = OMPING_OP_MODE_NORMAL;
	instance->quiet = 0;
	instance->send_count_queries = 0;
	instance->sndbuf_size = 0;
	instance->single_addr = 0;
	instance->rate_limit_time = 0;
	instance->rcvbuf_size = 0;
	instance->timeout_time = 0;
	instance->ttl = DEFAULT_TTL;
	instance->transport_method = SF_TM_ASM;
	instance->wait_time = DEFAULT_WAIT_TIME;
	instance->wait_for_finish_time = 0;

	force = 0;
	ifa_flags = IFF_MULTICAST;
	port_s = DEFAULT_PORT_S;
	rate_limit_time_set = 0;
	show_ver = 0;
	wait_for_finish_time_set = 0;

	logging_set_verbose(0);

	while ((ch = getopt(argc, argv, "46CDEFqVvc:i:M:m:O:p:R:r:S:T:t:w:")) != -1) {
		switch (ch) {
		case '4':
			instance->ip_ver = 4;
			break;
		case '6':
			instance->ip_ver = 6;
			break;
		case 'C':
			instance->cont_stat++;
			break;
		case 'D':
			instance->dup_buf_items = 0;
			break;
		case 'E':
			instance->auto_exit = 0;
			break;
		case 'F':
			force++;
			break;
		case 'q':
			instance->quiet++;
			break;
		case 'V':
			show_ver++;
			break;
		case 'v':
			logging_set_verbose(logging_get_verbose() + 1);
			break;
		case 'c':
			numd = strtod(optarg, &ep);
			if (numd < 1 || *ep != '\0' || (uint64_t)numd >= ((uint64_t)~0)) {
				warnx("illegal number, -c argument -- %s", optarg);
				goto error_usage_exit;
			}
			instance->send_count_queries= (uint64_t)numd;
			break;
		case 'i':
			numd = strtod(optarg, &ep);
			if (numd < 0 || *ep != '\0' || numd * 1000 > INT32_MAX) {
				warnx("illegal number, -i argument -- %s", optarg);
				goto error_usage_exit;
			}
			instance->wait_time = (int)(numd * 1000.0);
			break;
		case 'M':
			if (strcmp(optarg, "asm") == 0) {
				instance->transport_method = SF_TM_ASM;
				ifa_flags = IFF_MULTICAST;
			} else if (strcmp(optarg, "ssm") == 0 && sf_is_ssm_supported()) {
				instance->transport_method = SF_TM_SSM;
				ifa_flags = IFF_MULTICAST;
			} else if (strcmp(optarg, "ipbc") == 0 && sf_is_ipbc_supported()) {
				instance->transport_method = SF_TM_IPBC;
				ifa_flags = IFF_BROADCAST;
			} else {
				warnx("illegal parameter, -M argument -- %s", optarg);
				goto error_usage_exit;
			}
			break;
		case 'm':
			mcast_addr_s = optarg;
			break;
		case 'O':
			if (strcmp(optarg, "normal") == 0) {
				instance->op_mode = OMPING_OP_MODE_NORMAL;
			/*
			 * Temporarily disabled
			 *
			} else if (strcmp(optarg, "server") == 0) {
				*op_mode = OMPING_OP_MODE_SERVER;
			*/
			} else if (strcmp(optarg, "client") == 0) {
				instance->op_mode = OMPING_OP_MODE_CLIENT;
			} else {
				warnx("illegal parameter, -O argument -- %s", optarg);
				goto error_usage_exit;
			}
			break;
		case 'p':
			port_s = optarg;
			break;
		case 'R':
			numd = strtod(optarg, &ep);
			if (numd < MIN_RCVBUF_SIZE || *ep != '\0' || numd > INT32_MAX) {
				warnx("illegal number, -R argument -- %s", optarg);
				goto error_usage_exit;
			}
			instance->rcvbuf_size = (int)numd;
			break;
		case 'r':
			numd = strtod(optarg, &ep);
			if (numd < 0 || *ep != '\0' || numd * 1000 > INT32_MAX) {
				warnx("illegal number, -r argument -- %s", optarg);
				goto error_usage_exit;
			}
			instance->rate_limit_time = (int)(numd * 1000.0);
			rate_limit_time_set = 1;
			break;
		case 'S':
			numd = strtod(optarg, &ep);
			if (numd < MIN_SNDBUF_SIZE || *ep != '\0' || numd > INT32_MAX) {
				warnx("illegal number, -S argument -- %s", optarg);
				goto error_usage_exit;
			}
			instance->sndbuf_size = (int)numd;
			break;
		case 't':
			num = strtol(optarg, &ep, 10);
			if (num <= 0 || num > ((uint8_t)~0) || *ep != '\0') {
				warnx("illegal number, -t argument -- %s", optarg);
				goto error_usage_exit;
			}
			instance->ttl = num;
			break;
		case 'T':
			numd = strtod(optarg, &ep);
			if (numd < 0 || *ep != '\0' || numd * 1000 > INT32_MAX) {
				warnx("illegal number, -T argument -- %s", optarg);
				goto error_usage_exit;
			}
			instance->timeout_time = (int)(numd * 1000.0);
			break;
		case 'w':
			numd = strtod(optarg, &ep);
			if ((numd < 0 && numd != -1) || *ep != '\0' || numd * 1000 > INT32_MAX) {
				warnx("illegal number, -w argument -- %s", optarg);
				goto error_usage_exit;
			}
			wait_for_finish_time_set = 1;
			instance->wait_for_finish_time = (int)(numd * 1000.0);
			break;
		case '?':
			goto error_usage_exit;
			/* NOTREACHED */
			break;

		}
	}

	argc -= optind;
	argv += optind;

	/*
	 * Param checking
	 */
	if (show_ver == 1) {
		cliprint_version();
		exit(0);
	}

	if (show_ver > 1) {
		if (instance->op_mode != OMPING_OP_MODE_NORMAL) {
			warnx("op_mode must be set to normal for remote version display.");
			goto error_usage_exit;
		}

		instance->op_mode = OMPING_OP_MODE_SHOW_VERSION;
	}

	if (force < 1) {
		if (instance->wait_time < DEFAULT_WAIT_TIME) {
			warnx("illegal nmber, -i argument %u ms < %u ms. Use -F to force.",
			    instance->wait_time, DEFAULT_WAIT_TIME);
			goto error_usage_exit;
		}

		if (instance->ttl < DEFAULT_TTL) {
			warnx("illegal nmber, -t argument %u < %u. Use -F to force.",
			    instance->ttl, DEFAULT_TTL);
			goto error_usage_exit;
		}
	}

	if (force < 2) {
		if (instance->wait_time == 0) {
			warnx("illegal nmber, -i argument %u ms < 1 ms. Use -FF to force.",
			    instance->wait_time);
			goto error_usage_exit;
		}
	}

	if (instance->transport_method == SF_TM_IPBC) {
		if (instance->ip_ver == 6) {
			warnx("illegal transport method, -M argument ipbc is mutually exclusive "
			    "with -6 option");
			goto error_usage_exit;
		}

		instance->ip_ver = 4;
	}

	/*
	 * Computed params
	 */
	if (!wait_for_finish_time_set) {
		instance->wait_for_finish_time = instance->wait_time * DEFAULT_WFF_TIME_MUL;
		if (instance->wait_for_finish_time < DEFAULT_WAIT_TIME) {
			instance->wait_for_finish_time = DEFAULT_WAIT_TIME;
		}
	}

	if (instance->wait_time == 0) {
		instance->dup_buf_items = 0;
	} else {
		/*
		 * + 1 is for eliminate trucate errors
		 */
		instance->dup_buf_items = ((DUP_BUF_SECS * 1000) / instance->wait_time) + 1;

		if (instance->dup_buf_items < MIN_DUP_BUF_ITEMS) {
			instance->dup_buf_items = MIN_DUP_BUF_ITEMS;
		}
	}

	if (!rate_limit_time_set) {
		instance->rate_limit_time = instance->wait_time;

	}

	TAILQ_INIT(&instance->remote_addrs);

	no_ai = aii_parse_remote_addrs(&instance->remote_addrs, argc, argv, port_s,
	    instance->ip_ver);
	if (no_ai < 1) {
		warnx("at least one remote addresses should be specified");
		goto error_usage_exit;
	}

	instance->ip_ver = aii_return_ip_ver(&instance->remote_addrs, instance->ip_ver,
	    mcast_addr_s, port_s);

	if (aii_find_local(&instance->remote_addrs, &instance->ip_ver, &ifa_list, &ifa_local,
	    &ai_item, ifa_flags) < 0) {
		errx(1, "Can't find local address in arguments");
	}

	/*
	 * Change aii_list to struct of sockaddr_storage(s)
	 */
	aii_list_ai_to_sa(&instance->remote_addrs, instance->ip_ver);

	/*
	 * Find local addr and copy that. Also remove that from list
	 */
	aii_ifa_local_to_ai(&instance->remote_addrs, ai_item, ifa_local, instance->ip_ver,
	    &instance->local_addr, &instance->single_addr);

	/*
	 * Store local ifname
	 */
	instance->local_ifname = strdup(ifa_local->ifa_name);
	if (instance->local_ifname == NULL) {
		errx(1, "Can't alloc memory");
	}

	switch (instance->transport_method) {
	case SF_TM_ASM:
	case SF_TM_SSM:
		/*
		 * Convert mcast addr to something useful
		 */
		aii_mcast_to_ai(instance->ip_ver, &instance->mcast_addr, mcast_addr_s, port_s);
		break;
	case SF_TM_IPBC:
		/*
		 * Convert broadcast addr to something useful
		 */
		res = aii_ipbc_to_ai(&instance->mcast_addr, mcast_addr_s, port_s, ifa_local);
		if (res == -1) {
			warnx("illegal broadcast address, -M argument doesn't match with local"
			    " broadcast address");
			goto error_usage_exit;
		}
		break;
	}

	/*
	 * Assign port from mcast_addr
	 */
	instance->port = af_sa_port(AF_CAST_SA(&instance->mcast_addr.sas));

	freeifaddrs(ifa_list);

	return (0);

error_usage_exit:
	cliprint_usage();
	exit(1);
	/* NOTREACHED */
	return (-1);
}
