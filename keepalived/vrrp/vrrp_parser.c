/* 
 * Soft:        Keepalived is a failover program for the LVS project
 *              <www.linuxvirtualserver.org>. It monitor & manipulate
 *              a loadbalanced server pool using multi-layer checks.
 * 
 * Part:        Configuration file parser/reader. Place into the dynamic
 *              data structure representation the conf file representing
 *              the loadbalanced server pool.
 *  
 * Author:      Alexandre Cassen, <acassen@linux-vs.org>
 *              
 *              This program is distributed in the hope that it will be useful,
 *              but WITHOUT ANY WARRANTY; without even the implied warranty of
 *              MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *              See the GNU General Public License for more details.
 *
 *              This program is free software; you can redistribute it and/or
 *              modify it under the terms of the GNU General Public License
 *              as published by the Free Software Foundation; either version
 *              2 of the License, or (at your option) any later version.
 *
 * Copyright (C) 2001-2012 Alexandre Cassen, <acassen@gmail.com>
 */

#include "vrrp_parser.h"
#include "vrrp_data.h"
#include "vrrp_sync.h"
#include "vrrp_index.h"
#include "vrrp_if.h"
#include "vrrp_vmac.h"
#include "vrrp.h"
#include "global_data.h"
#include "global_parser.h"
#include "logger.h"
#include "parser.h"
#include "memory.h"
#include "bitops.h"

/* Static addresses handler */
static void
static_addresses_handler(vector_t *strvec)
{
	alloc_value_block(strvec, alloc_saddress);
}

/* Static routes handler */
static void
static_routes_handler(vector_t *strvec)
{
	alloc_value_block(strvec, alloc_sroute);
}

/* Static rules handler */
static void
static_rules_handler(vector_t *strvec)
{
	alloc_value_block(strvec, alloc_srule);
}

/* VRRP handlers */
static void
vrrp_sync_group_handler(vector_t *strvec)
{
	list l;
	element e;
	vrrp_sgroup_t *sg;
	char* gname;

	if (vector_count(strvec) != 2) {
		log_message(LOG_INFO, "vrrp_sync_group must have a name - skipping");
		skip_block();
		return;
	}

	gname = vector_slot(strvec, 1);

	/* check group doesn't already exist */
	if (!LIST_ISEMPTY(vrrp_data->vrrp_sync_group)) {
		l = vrrp_data->vrrp_sync_group;
		for (e = LIST_HEAD(l); e; ELEMENT_NEXT(e)) {
			sg = ELEMENT_DATA(e);
			if (!strcmp(gname,sg->gname)) {
				log_message(LOG_INFO, "vrrp sync group %s already defined", gname);
				skip_block();
				return;
			}
		}
	}

	alloc_vrrp_sync_group(gname);
}
static void
vrrp_group_handler(vector_t *strvec)
{
	vrrp_sgroup_t *vgroup = LIST_TAIL_DATA(vrrp_data->vrrp_sync_group);
	vgroup->iname = read_value_block();
}
static void
vrrp_gnotify_backup_handler(vector_t *strvec)
{
	vrrp_sgroup_t *vgroup = LIST_TAIL_DATA(vrrp_data->vrrp_sync_group);
	vgroup->script_backup = set_value(strvec);
	vgroup->notify_exec = 1;
}
static void
vrrp_gnotify_master_handler(vector_t *strvec)
{
	vrrp_sgroup_t *vgroup = LIST_TAIL_DATA(vrrp_data->vrrp_sync_group);
	vgroup->script_master = set_value(strvec);
	vgroup->notify_exec = 1;
}
static void
vrrp_gnotify_fault_handler(vector_t *strvec)
{
	vrrp_sgroup_t *vgroup = LIST_TAIL_DATA(vrrp_data->vrrp_sync_group);
	vgroup->script_fault = set_value(strvec);
	vgroup->notify_exec = 1;
}
static void
vrrp_gnotify_handler(vector_t *strvec)
{
	vrrp_sgroup_t *vgroup = LIST_TAIL_DATA(vrrp_data->vrrp_sync_group);
	vgroup->script = set_value(strvec);
	vgroup->notify_exec = 1;
}
static void
vrrp_gsmtp_handler(vector_t *strvec)
{
	vrrp_sgroup_t *vgroup = LIST_TAIL_DATA(vrrp_data->vrrp_sync_group);
	vgroup->smtp_alert = 1;
}
static void
vrrp_gglobal_tracking_handler(vector_t *strvec)
{
	vrrp_sgroup_t *vgroup = LIST_TAIL_DATA(vrrp_data->vrrp_sync_group);
	vgroup->global_tracking = 1;
}
static void
vrrp_handler(vector_t *strvec)
{
	list l;
	element e;
	vrrp_t *vrrp;
	char *iname;

	if (vector_count(strvec) != 2) {
		log_message(LOG_INFO, "vrrp_instance must have a name");
		skip_block();
		return;
	}

	iname = vector_slot(strvec,1);

	/* Make sure the vrrp instance doesn't already exist */
	if (!LIST_ISEMPTY(vrrp_data->vrrp)) {
		l = vrrp_data->vrrp;
		for (e = LIST_HEAD(l); e; ELEMENT_NEXT(e)) {
			vrrp = ELEMENT_DATA(e);
			if (!strcmp(iname,vrrp->iname)) {
				log_message(LOG_INFO, "vrrp instance %s already defined", iname );
				skip_block();
				return;
			}
		}
	}

	alloc_vrrp(iname);
}
static void
vrrp_vmac_handler(vector_t *strvec)
{
	vrrp_t *vrrp = LIST_TAIL_DATA(vrrp_data->vrrp);

	__set_bit(VRRP_VMAC_BIT, &vrrp->vmac_flags);

	if (vector_size(strvec) >= 2)
		strncpy(vrrp->vmac_ifname, vector_slot(strvec, 1), IFNAMSIZ - 1);
}
static void
vrrp_vmac_xmit_base_handler(vector_t *strvec)
{
	vrrp_t *vrrp = LIST_TAIL_DATA(vrrp_data->vrrp);

	__set_bit(VRRP_VMAC_XMITBASE_BIT, &vrrp->vmac_flags);
}
static void
vrrp_unicast_peer_handler(vector_t *strvec)
{
	alloc_value_block(strvec, alloc_vrrp_unicast_peer);
}
static void
vrrp_native_ipv6_handler(vector_t *strvec)
{
	vrrp_t *vrrp = LIST_TAIL_DATA(vrrp_data->vrrp);

	if (vrrp->family == AF_INET) {
		log_message(LOG_INFO,"(%s): Cannot specify native_ipv6 with IPv4 addresses", vrrp->iname);
		return;
	}

	vrrp->family = AF_INET6;
	vrrp->version = VRRP_VERSION_3;
}
static void
vrrp_state_handler(vector_t *strvec)
{
	char *str = vector_slot(strvec, 1);
	vrrp_t *vrrp = LIST_TAIL_DATA(vrrp_data->vrrp);
	vrrp_sgroup_t *vgroup = vrrp->sync;

	if (!strcmp(str, "MASTER")) {
		vrrp->wantstate = VRRP_STATE_MAST;
		vrrp->init_state = VRRP_STATE_MAST;
	}

	/* set eventual sync group */
	if (vgroup)
		vgroup->state = vrrp->wantstate;
}
static void
vrrp_int_handler(vector_t *strvec)
{
	vrrp_t *vrrp = LIST_TAIL_DATA(vrrp_data->vrrp);
	char *name = vector_slot(strvec, 1);

	vrrp->ifp = if_get_by_ifname(name);
	if (!vrrp->ifp) {
		log_message(LOG_INFO, "Cant find interface %s for vrrp_instance %s !!!"
				    , name, vrrp->iname);
		return;
	}
}
static void
vrrp_track_int_handler(vector_t *strvec)
{
	alloc_value_block(strvec, alloc_vrrp_track);
}
static void
vrrp_track_scr_handler(vector_t *strvec)
{
	alloc_value_block(strvec, alloc_vrrp_track_script);
}
static void
vrrp_dont_track_handler(vector_t *strvec)
{
	vrrp_t *vrrp = LIST_TAIL_DATA(vrrp_data->vrrp);
	vrrp->dont_track_primary = 1;
}
static void
vrrp_srcip_handler(vector_t *strvec)
{
	vrrp_t *vrrp = LIST_TAIL_DATA(vrrp_data->vrrp);
	struct sockaddr_storage *saddr = &vrrp->saddr;
	int ret;

	ret = inet_stosockaddr(vector_slot(strvec, 1), 0, saddr);
	if (ret < 0) {
		log_message(LOG_ERR, "Configuration error: VRRP instance[%s] malformed unicast"
				     " src address[%s]. Skipping..."
				   , vrrp->iname, FMT_STR_VSLOT(strvec, 1));
		return;
	}

	if (vrrp->family == AF_UNSPEC)
		vrrp->family = saddr->ss_family;
	else if (saddr->ss_family != vrrp->family) {
		log_message(LOG_ERR, "Configuration error: VRRP instance[%s] and unicast src address"
				     "[%s] MUST be of the same family !!! Skipping..."
				   , vrrp->iname, FMT_STR_VSLOT(strvec, 1));
		saddr->ss_family = AF_UNSPEC;
	}
}
static void
vrrp_vrid_handler(vector_t *strvec)
{
	vrrp_t *vrrp = LIST_TAIL_DATA(vrrp_data->vrrp);
	vrrp->vrid = atoi(vector_slot(strvec, 1));

	if (VRRP_IS_BAD_VID(vrrp->vrid)) {
		log_message(LOG_INFO, "VRRP Error : VRID not valid !");
		log_message(LOG_INFO,
		       "             must be between 1 & 255. reconfigure !");

		vrrp->vrid = 0;
		return;
	}

	alloc_vrrp_bucket(vrrp);
}
static void
vrrp_prio_handler(vector_t *strvec)
{
	vrrp_t *vrrp = LIST_TAIL_DATA(vrrp_data->vrrp);
	vrrp->effective_priority = vrrp->base_priority = atoi(vector_slot(strvec, 1));

	if (VRRP_IS_BAD_PRIORITY(vrrp->base_priority)) {
		log_message(LOG_INFO, "VRRP Error : Priority not valid !");
		log_message(LOG_INFO,
		       "             must be between 1 & 255. reconfigure !");
		log_message(LOG_INFO,
			    "             Using default value : %d\n", VRRP_PRIO_DFL);
		vrrp->effective_priority = vrrp->base_priority = VRRP_PRIO_DFL;
	}
}
static void
vrrp_adv_handler(vector_t *strvec)
{
	vrrp_t *vrrp = LIST_TAIL_DATA(vrrp_data->vrrp);
	vrrp->adver_int = atof(vector_slot(strvec, 1)) * 100; /* multiply with 100 to get decimal value */

	/* Simple check - just positive */
	if (VRRP_IS_BAD_ADVERT_INT(vrrp->adver_int)) {
		log_message(LOG_INFO, "(%s): Advert interval not valid !", vrrp->iname);
		log_message(LOG_INFO, "%*smust be >=1sec for VRRPv2 or >=0.01sec for VRRPv3.", (int)strlen(vrrp->iname) + 4, "");
		log_message(LOG_INFO, "%*sUsing default value : 1sec", (int)strlen(vrrp->iname) + 4, "");
		vrrp->adver_int = VRRP_ADVER_DFL * 100;
	}
	vrrp->adver_int *= TIMER_HZ / 100.0;
}
static void
vrrp_debug_handler(vector_t *strvec)
{
	vrrp_t *vrrp = LIST_TAIL_DATA(vrrp_data->vrrp);
	vrrp->debug = atoi(vector_slot(strvec, 1));

	if (VRRP_IS_BAD_DEBUG_INT(vrrp->debug)) {
		log_message(LOG_INFO, "VRRP Error : Debug value not valid !");
		log_message(LOG_INFO, "             must be between 0-4");
		vrrp->debug = 0;
	}
}
static void
vrrp_nopreempt_handler(vector_t *strvec)
{
	vrrp_t *vrrp = LIST_TAIL_DATA(vrrp_data->vrrp);
	vrrp->nopreempt = 1;
}
static void	/* backwards compatibility */
vrrp_preempt_handler(vector_t *strvec)
{
	vrrp_t *vrrp = LIST_TAIL_DATA(vrrp_data->vrrp);
	vrrp->nopreempt = 0;
}
static void
vrrp_preempt_delay_handler(vector_t *strvec)
{
	vrrp_t *vrrp = LIST_TAIL_DATA(vrrp_data->vrrp);
	vrrp->preempt_delay = atoi(vector_slot(strvec, 1));

	if (VRRP_IS_BAD_PREEMPT_DELAY(vrrp->preempt_delay)) {
		log_message(LOG_INFO, "VRRP Error : Preempt_delay not valid !");
		log_message(LOG_INFO, "             must be between 0-%d",
		       TIMER_MAX_SEC);
		vrrp->preempt_delay = 0;
	}
	vrrp->preempt_delay *= TIMER_HZ;
	vrrp->preempt_time = timer_add_long(timer_now(), vrrp->preempt_delay);
}
static void
vrrp_notify_backup_handler(vector_t *strvec)
{
	vrrp_t *vrrp = LIST_TAIL_DATA(vrrp_data->vrrp);
	vrrp->script_backup = set_value(strvec);
	vrrp->notify_exec = 1;
}
static void
vrrp_notify_master_handler(vector_t *strvec)
{
	vrrp_t *vrrp = LIST_TAIL_DATA(vrrp_data->vrrp);
	vrrp->script_master = set_value(strvec);
	vrrp->notify_exec = 1;
}
static void
vrrp_notify_fault_handler(vector_t *strvec)
{
	vrrp_t *vrrp = LIST_TAIL_DATA(vrrp_data->vrrp);
	vrrp->script_fault = set_value(strvec);
	vrrp->notify_exec = 1;
}
static void
vrrp_notify_stop_handler(vector_t *strvec)
{
	vrrp_t *vrrp = LIST_TAIL_DATA(vrrp_data->vrrp);
	vrrp->script_stop = set_value(strvec);
	vrrp->notify_exec = 1;
}
static void
vrrp_notify_handler(vector_t *strvec)
{
	vrrp_t *vrrp = LIST_TAIL_DATA(vrrp_data->vrrp);
	vrrp->script = set_value(strvec);
	vrrp->notify_exec = 1;
}
static void
vrrp_smtp_handler(vector_t *strvec)
{
	vrrp_t *vrrp = LIST_TAIL_DATA(vrrp_data->vrrp);
	vrrp->smtp_alert = 1;
}
static void
vrrp_lvs_syncd_handler(vector_t *strvec)
{
	vrrp_t *vrrp = LIST_TAIL_DATA(vrrp_data->vrrp);
	vrrp->lvs_syncd_if = set_value(strvec);
}
static void
vrrp_garp_delay_handler(vector_t *strvec)
{
	vrrp_t *vrrp = LIST_TAIL_DATA(vrrp_data->vrrp);
	vrrp->garp_delay = atoi(vector_slot(strvec, 1)) * TIMER_HZ;
	if (vrrp->garp_delay < TIMER_HZ)
		vrrp->garp_delay = TIMER_HZ;
}
static void
vrrp_garp_refresh_handler(vector_t *strvec)
{
	vrrp_t *vrrp = LIST_TAIL_DATA(vrrp_data->vrrp);
	vrrp->garp_refresh.tv_sec = atoi(vector_slot(strvec, 1));
}
static void
vrrp_garp_rep_handler(vector_t *strvec)
{
	vrrp_t *vrrp = LIST_TAIL_DATA(vrrp_data->vrrp);
	vrrp->garp_rep = atoi(vector_slot(strvec, 1));
	if (vrrp->garp_rep < 1)
		vrrp->garp_rep = 1;
}
static void
vrrp_garp_refresh_rep_handler(vector_t *strvec)
{
	vrrp_t *vrrp = LIST_TAIL_DATA(vrrp_data->vrrp);
	vrrp->garp_refresh_rep = atoi(vector_slot(strvec, 1));
	if (vrrp->garp_refresh_rep < 1)
		vrrp->garp_refresh_rep = 1;
}
static void
vrrp_auth_type_handler(vector_t *strvec)
{
	vrrp_t *vrrp = LIST_TAIL_DATA(vrrp_data->vrrp);
	char *str = vector_slot(strvec, 1);

	if (!strcmp(str, "AH"))
		vrrp->auth_type = VRRP_AUTH_AH;
	else if (!strcmp(str, "PASS"))
		vrrp->auth_type = VRRP_AUTH_PASS;
}
static void
vrrp_auth_pass_handler(vector_t *strvec)
{
	vrrp_t *vrrp = LIST_TAIL_DATA(vrrp_data->vrrp);
	char *str = vector_slot(strvec, 1);
	int max_size = sizeof (vrrp->auth_data);
	int str_len = strlen(str);

	if (str_len > max_size) {
		str_len = max_size;
		log_message(LOG_INFO,
			    "Truncating auth_pass to %d characters", max_size);
	}

	memset(vrrp->auth_data, 0, max_size);
	memcpy(vrrp->auth_data, str, str_len);
}
static void
vrrp_vip_handler(vector_t *strvec)
{
	vrrp_t *vrrp = LIST_TAIL_DATA(vrrp_data->vrrp);
	char *buf;
	char *str = NULL;
	vector_t *vec = NULL;
	int nbvip = 0;
	int address_family;

	/* Check if some VIPs have already been configured on this interface */
	if (!LIST_ISEMPTY(vrrp->vip))
		nbvip = LIST_SIZE(vrrp->vip);

	buf = (char *) MALLOC(MAXBUF);
	while (read_line(buf, MAXBUF)) {
		address_family = AF_UNSPEC;
		vec = alloc_strvec(buf);
		if (vec) {
			str = vector_slot(vec, 0);
			if (!strcmp(str, EOB)) {
				free_strvec(vec);
				break;
			}

			if (vector_size(vec)) {
				nbvip++;
				if (nbvip > VRRP_MAX_VIP) {
					log_message(LOG_INFO,
					       "VRRP_Instance(%s) "
					       "more than %d VIPs: extra added the excluded vip block.",
					       vrrp->iname, VRRP_MAX_VIP);
					log_message(LOG_INFO,
					       "  => Declare extra VIPs into"
					       " the excluded vip block");

					alloc_vrrp_evip(vec);
					if (!LIST_ISEMPTY(vrrp->evip))
						address_family = IP_FAMILY((ip_address_t*)LIST_TAIL_DATA(vrrp->evip));
				} else {
					alloc_vrrp_vip(vec);
					if (!LIST_ISEMPTY(vrrp->vip))
						address_family = IP_FAMILY((ip_address_t*)LIST_TAIL_DATA(vrrp->vip));
				}
			}

			if (address_family != AF_UNSPEC) {
				if (vrrp->family == AF_UNSPEC)
					vrrp->family = address_family;
				else if (address_family != vrrp->family) {
					log_message(LOG_INFO, "(%s): address family must match VRRP instance [%s] - ignoring", vrrp->iname, buf);
					if (nbvip > VRRP_MAX_VIP)
						free_list_element(vrrp->evip, LIST_TAIL_DATA(vrrp->evip));
					else
						free_list_element(vrrp->vip, LIST_TAIL_DATA(vrrp->vip));
				}
			}

			free_strvec(vec);
		}
		memset(buf, 0, MAXBUF);
	}
	FREE(buf);
}
static void
vrrp_evip_handler(vector_t *strvec)
{
	alloc_value_block(strvec, alloc_vrrp_evip);
}
static void
vrrp_vroutes_handler(vector_t *strvec)
{
	alloc_value_block(strvec, alloc_vrrp_vroute);
}
static void
vrrp_vrules_handler(vector_t *strvec)
{
	alloc_value_block(strvec, alloc_vrrp_vrule);
}
static void
vrrp_script_handler(vector_t *strvec)
{
	alloc_vrrp_script(vector_slot(strvec, 1));
}
static void
vrrp_vscript_script_handler(vector_t *strvec)
{
	vrrp_script_t *vscript = LIST_TAIL_DATA(vrrp_data->vrrp_script);
	vscript->script = set_value(strvec);
}
static void
vrrp_vscript_interval_handler(vector_t *strvec)
{
	vrrp_script_t *vscript = LIST_TAIL_DATA(vrrp_data->vrrp_script);
	vscript->interval = atoi(vector_slot(strvec, 1)) * TIMER_HZ;
	if (vscript->interval < TIMER_HZ)
		vscript->interval = TIMER_HZ;
}
static void
vrrp_vscript_timeout_handler(vector_t *strvec)
{
	vrrp_script_t *vscript = LIST_TAIL_DATA(vrrp_data->vrrp_script);
	vscript->timeout = atoi(vector_slot(strvec, 1)) * TIMER_HZ;
	if (vscript->timeout < TIMER_HZ)
		vscript->timeout = TIMER_HZ;
}
static void
vrrp_vscript_weight_handler(vector_t *strvec)
{
	vrrp_script_t *vscript = LIST_TAIL_DATA(vrrp_data->vrrp_script);
	vscript->weight = atoi(vector_slot(strvec, 1));
}
static void
vrrp_vscript_rise_handler(vector_t *strvec)
{
	vrrp_script_t *vscript = LIST_TAIL_DATA(vrrp_data->vrrp_script);
	vscript->rise = atoi(vector_slot(strvec, 1));
	if (vscript->rise < 1)
		vscript->rise = 1;
}
static void
vrrp_vscript_fall_handler(vector_t *strvec)
{
	vrrp_script_t *vscript = LIST_TAIL_DATA(vrrp_data->vrrp_script);
	vscript->fall = atoi(vector_slot(strvec, 1));
	if (vscript->fall < 1)
		vscript->fall = 1;
}

static void
vrrp_version_handler(vector_t *strvec)
{
	vrrp_t *vrrp = LIST_TAIL_DATA(vrrp_data->vrrp);

	uint8_t version = atoi(vector_slot(strvec, 1));
	if (VRRP_IS_BAD_VERSION(version)) {
		log_message(LOG_INFO, "VRRP Error : Version not valid !\n");
		log_message(LOG_INFO, "             must be between either 2 or 3. reconfigure !\n");
		return;
	}

	if ((vrrp->version && vrrp->version != version) ||
	    (version == VRRP_VERSION_2 && vrrp->family == AF_INET6)) {
		log_message(LOG_INFO, "(%s): vrrp_version conflicts with configured or deduced version; ignoring.", vrrp->iname);
		return;
	}

	vrrp->version = version;
}

static void
vrrp_accept_handler(vector_t *strvec)
{
	vrrp_t *vrrp = LIST_TAIL_DATA(vrrp_data->vrrp);

	vrrp->accept = true;
}

vector_t *
vrrp_init_keywords(void)
{
	/* global definitions mapping */
	global_init_keywords();

	/* Static routes mapping */
	install_keyword_root("static_ipaddress", &static_addresses_handler);
	install_keyword_root("static_routes", &static_routes_handler);
	install_keyword_root("static_rules", &static_rules_handler);

	/* VRRP Instance mapping */
	install_keyword_root("vrrp_sync_group", &vrrp_sync_group_handler);
	install_keyword("group", &vrrp_group_handler);
	install_keyword("notify_backup", &vrrp_gnotify_backup_handler);
	install_keyword("notify_master", &vrrp_gnotify_master_handler);
	install_keyword("notify_fault", &vrrp_gnotify_fault_handler);
	install_keyword("notify", &vrrp_gnotify_handler);
	install_keyword("smtp_alert", &vrrp_gsmtp_handler);
	install_keyword("global_tracking", &vrrp_gglobal_tracking_handler);
	install_keyword_root("vrrp_instance", &vrrp_handler);
	install_keyword("use_vmac", &vrrp_vmac_handler);
	install_keyword("vmac_xmit_base", &vrrp_vmac_xmit_base_handler);
	install_keyword("unicast_peer", &vrrp_unicast_peer_handler);
	install_keyword("native_ipv6", &vrrp_native_ipv6_handler);
	install_keyword("state", &vrrp_state_handler);
	install_keyword("interface", &vrrp_int_handler);
	install_keyword("dont_track_primary", &vrrp_dont_track_handler);
	install_keyword("track_interface", &vrrp_track_int_handler);
	install_keyword("track_script", &vrrp_track_scr_handler);
	install_keyword("mcast_src_ip", &vrrp_srcip_handler);
	install_keyword("unicast_src_ip", &vrrp_srcip_handler);
	install_keyword("virtual_router_id", &vrrp_vrid_handler);
	install_keyword("version", &vrrp_version_handler);
	install_keyword("priority", &vrrp_prio_handler);
	install_keyword("advert_int", &vrrp_adv_handler);
	install_keyword("virtual_ipaddress", &vrrp_vip_handler);
	install_keyword("virtual_ipaddress_excluded", &vrrp_evip_handler);
	install_keyword("virtual_routes", &vrrp_vroutes_handler);
	install_keyword("virtual_rules", &vrrp_vrules_handler);
	install_keyword("accept", &vrrp_accept_handler);
	install_keyword("preempt", &vrrp_preempt_handler);
	install_keyword("nopreempt", &vrrp_nopreempt_handler);
	install_keyword("preempt_delay", &vrrp_preempt_delay_handler);
	install_keyword("debug", &vrrp_debug_handler);
	install_keyword("notify_backup", &vrrp_notify_backup_handler);
	install_keyword("notify_master", &vrrp_notify_master_handler);
	install_keyword("notify_fault", &vrrp_notify_fault_handler);
	install_keyword("notify_stop", &vrrp_notify_stop_handler);
	install_keyword("notify", &vrrp_notify_handler);
	install_keyword("smtp_alert", &vrrp_smtp_handler);
	install_keyword("lvs_sync_daemon_interface", &vrrp_lvs_syncd_handler);
	install_keyword("garp_master_delay", &vrrp_garp_delay_handler);
	install_keyword("garp_master_refresh", &vrrp_garp_refresh_handler);
	install_keyword("garp_master_repeat", &vrrp_garp_rep_handler);
	install_keyword("garp_master_refresh_repeat", &vrrp_garp_refresh_rep_handler);
	install_keyword("authentication", NULL);
	install_sublevel();
	install_keyword("auth_type", &vrrp_auth_type_handler);
	install_keyword("auth_pass", &vrrp_auth_pass_handler);
	install_sublevel_end();
	install_keyword_root("vrrp_script", &vrrp_script_handler);
	install_keyword("script", &vrrp_vscript_script_handler);
	install_keyword("interval", &vrrp_vscript_interval_handler);
	install_keyword("timeout", &vrrp_vscript_timeout_handler);
	install_keyword("weight", &vrrp_vscript_weight_handler);
	install_keyword("rise", &vrrp_vscript_rise_handler);
	install_keyword("fall", &vrrp_vscript_fall_handler);

	return keywords;
}
