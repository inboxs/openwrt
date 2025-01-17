/*
 * iwinfo - Wireless Information Library - Linux Wireless Extension Backend
 *
 *   Copyright (C) 2009 Jo-Philipp Wich <xm@subsignal.org>
 *
 * The iwinfo library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * The iwinfo library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with the iwinfo library. If not, see http://www.gnu.org/licenses/.
 *
 * Parts of this code are derived from the Linux wireless tools, iwlib.c,
 * iwlist.c and iwconfig.c in particular.
 */

#include <stdbool.h>

#include "iwinfo_wext.h"
#include "api/mtk.h"

struct survey_table
{
	char channel[4];
	char ssid[33];
	char len[4];
	char bssid[20];
	char security[23];
	char *crypto;
	char signal[9];
};

static int mtk_wrq(struct iwreq *wrq, const char *ifname, int cmd, void *data, size_t len)
{
	strncpy(wrq->ifr_name, ifname, IFNAMSIZ);

	if(data != NULL)
	{
		if(len < IFNAMSIZ)
		{
			memcpy(wrq->u.name, data, len);
		}
		else
		{
			wrq->u.data.pointer = data;
			wrq->u.data.length = len;
		}
	}

	return iwinfo_ioctl(cmd, wrq);
}

static int mtk_get80211priv(const char *ifname, int op, void *data, size_t len)
{
	struct iwreq iwr;

	if(mtk_wrq(&iwr, ifname, op, data, len) < 0)
		return -1;

	return iwr.u.data.length;
}

static int mtk_isap(const char *ifname)
{
	return !strncmp(ifname, "ra", 2);
}

static int mtk_iscli(const char *ifname)
{
	return !strncmp(ifname, "apcli", 5);
}

static int mtk_iswds(const char *ifname)
{
	return !strncmp(ifname, "wds", 3);
}

static int mtk_probe(const char *ifname)
{
	return mtk_isap(ifname) || mtk_iscli(ifname) || mtk_iswds(ifname);
}

static void mtk_close(void)
{
	/* Nop */
}

static int mtk_get_mode(const char *ifname, int *buf)
{
	if(mtk_isap(ifname))
		*buf = IWINFO_OPMODE_MASTER;
	else if(mtk_iscli(ifname))
		*buf = IWINFO_OPMODE_CLIENT;
	else if(mtk_iswds(ifname))
		*buf = IWINFO_OPMODE_WDS;
	else
		*buf = IWINFO_OPMODE_UNKNOWN;
	return 0;
}

static int mtk_get_ssid(const char *ifname, char *buf)
{
	return wext_ops.ssid(ifname, buf);
}

static int mtk_get_bssid(const char *ifname, char *buf)
{
	return wext_ops.bssid(ifname, buf);
}

static int mtk_get_bitrate(const char *ifname, int *buf)
{
	return wext_ops.bitrate(ifname, buf);
}

static int mtk_get_channel(const char *ifname, int *buf)
{
	return wext_ops.channel(ifname, buf);
}

static int mtk_get_frequency(const char *ifname, int *buf)
{
	return -1;
}

static int mtk_get_txpower(const char *ifname, int *buf)
{
	return wext_ops.txpower(ifname, buf);
}

static int mtk_get_signal(const char *ifname, int *buf)
{
	return -1;
}

static int mtk_get_noise(const char *ifname, int *buf)
{
	return -1;
}

static int mtk_get_quality(const char *ifname, int *buf)
{
	return -1;
}

static int mtk_get_quality_max(const char *ifname, int *buf)
{
	return -1;
}

static int mtk_get_rate(MACHTTRANSMIT_SETTING HTSetting)

{
	int MCSMappingRateTable[] =
	{2,  4,   11,  22, /* CCK*/
	12, 18,   24,  36, 48, 72, 96, 108, /* OFDM*/
	13, 26,   39,  52,  78, 104, 117, 130, 26,  52,  78, 104, 156, 208, 234, 260, /* 20MHz, 800ns GI, MCS: 0 ~ 15*/
	39, 78,  117, 156, 234, 312, 351, 390,										  /* 20MHz, 800ns GI, MCS: 16 ~ 23*/
	27, 54,   81, 108, 162, 216, 243, 270, 54, 108, 162, 216, 324, 432, 486, 540, /* 40MHz, 800ns GI, MCS: 0 ~ 15*/
	81, 162, 243, 324, 486, 648, 729, 810,										  /* 40MHz, 800ns GI, MCS: 16 ~ 23*/
	14, 29,   43,  57,  87, 115, 130, 144, 29, 59,   87, 115, 173, 230, 260, 288, /* 20MHz, 400ns GI, MCS: 0 ~ 15*/
	43, 87,  130, 173, 260, 317, 390, 433,										  /* 20MHz, 400ns GI, MCS: 16 ~ 23*/
	30, 60,   90, 120, 180, 240, 270, 300, 60, 120, 180, 240, 360, 480, 540, 600, /* 40MHz, 400ns GI, MCS: 0 ~ 15*/
	90, 180, 270, 360, 540, 720, 810, 900};

	int rate_count = sizeof(MCSMappingRateTable)/sizeof(int);
	int rate_index = 0;
	int value = 0;

	if (HTSetting.field.MODE >= MODE_HTMIX)
	{
    		rate_index = 12 + ((UCHAR)HTSetting.field.BW *24) + ((UCHAR)HTSetting.field.ShortGI *48) + ((UCHAR)HTSetting.field.MCS);
	}
	else if (HTSetting.field.MODE == MODE_OFDM)
		rate_index = (UCHAR)(HTSetting.field.MCS) + 4;
	else if (HTSetting.field.MODE == MODE_CCK)   
		rate_index = (UCHAR)(HTSetting.field.MCS);

	if (rate_index < 0)
		rate_index = 0;
    
	if (rate_index >= rate_count)
		rate_index = rate_count-1;

	value = (MCSMappingRateTable[rate_index] * 5)/10;

	return value;
}

static int mtk_get_assoclist(const char *ifname, char *buf, int *len)
{
	struct iwinfo_assoclist_entry entry;
	static RT_802_11_MAC_TABLE mt;
	MACHTTRANSMIT_SETTING rxrate;

	int mtlen = sizeof(RT_802_11_MAC_TABLE);
	int i;

	if (mtk_get80211priv(ifname, RTPRIV_IOCTL_GET_MAC_TABLE_STRUCT, &mt, mtlen) > 0)
	{
		*len = 0;
		for (i = 0; i < mt.Num; i++)
		{
			memset(&entry, 0, sizeof(entry));

			memcpy(entry.mac, &mt.Entry[i].Addr, 6);

			if(mt.Entry[i].AvgRssi0 > mt.Entry[i].AvgRssi1)
				entry.signal = mt.Entry[i].AvgRssi0;
			else
				entry.signal = mt.Entry[i].AvgRssi1;

			entry.noise  = -95;

			entry.inactive = mt.Entry[i].ConnectedTime * 1000;

			rxrate.word = mt.Entry[i].LastRxRate;
			entry.tx_rate.rate = mtk_get_rate(mt.Entry[i].TxRate) * 1000;
			entry.rx_rate.rate = mtk_get_rate(rxrate) * 1000;

			entry.tx_rate.mcs = mt.Entry[i].TxRate.field.MCS;
			entry.rx_rate.mcs = rxrate.field.MCS;

			entry.tx_packets = mt.Entry[i].TxPackets;
			entry.rx_packets = mt.Entry[i].RxPackets;

			if(mt.Entry[i].TxRate.field.BW) entry.tx_rate.is_40mhz = 1;
			if(mt.Entry[i].TxRate.field.ShortGI) entry.tx_rate.is_short_gi = 1;
			if(rxrate.field.BW) entry.rx_rate.is_40mhz = 1;
			if(rxrate.field.ShortGI) entry.rx_rate.is_short_gi = 1;

			memcpy(buf + *len, &entry, sizeof(struct iwinfo_assoclist_entry));
			*len += sizeof(struct iwinfo_assoclist_entry);
		}

		return 0;
	}

	return -1;
}

static int mtk_get_txpwrlist(const char *ifname, char *buf, int *len)
{
	struct iwinfo_txpwrlist_entry entry;
	int i;

	for (i = 0; i < 100; i++)
	{
		entry.dbm = iwinfo_mw2dbm(i);
		entry.mw  = i;

		memcpy(&buf[i * sizeof(entry)], &entry, sizeof(entry));
	}

	*len = i * sizeof(entry);
	return 0;
}

static int ascii2num(char ascii)
{
	int num;
	if ((ascii >= '0') && (ascii <= '9'))
		num=ascii - 48;
	else if ((ascii >= 'a') && (ascii <= 'f'))
		num=ascii - 'a' + 10;
        else if ((ascii >= 'A') && (ascii <= 'F'))
		num=ascii - 'A' + 10;
	else
		num = 0;
	return num;
}

static void next_field(char **line, char *output, int n) {
	char *l = *line;
	int i;

	memcpy(output, *line, n);
	*line = &l[n];

	for (i = n - 1; i > 0; i--) {
		if (output[i] != ' ')
			break;
		output[i] = '\0';
	}
}

static int mtk_get_scan(const char *ifname, struct survey_table *st)
{
	int survey_count = 0;
	char *s = calloc(1, IWINFO_BUFSIZE + 1);
	char ss[64] = "SiteSurvey=1";
	char *line, *start ,*p;
	char buf[128];

	if (!s)
		return -1;

	if(mtk_get80211priv(ifname, RTPRIV_IOCTL_SET, ss, sizeof(ss)) < 0)
		return -1;

	sleep(5);

	if(mtk_get80211priv(ifname, RTPRIV_IOCTL_GSITESURVEY, s, IWINFO_BUFSIZE) < 1) {
		free(s);
		return -1;
	}

	start = s;

	line = strtok(start, "\n");

	while (line) {
		if (!strncmp("Ch ", line, 3) || !strncmp("No ", line, 3))
			break;

		line = strtok(NULL, "\n");
	}

	if (!line)
		return -1;

	line = strtok(NULL, "\n");

	while (line && (survey_count < 64))
	{
		memset(&st[survey_count], 0, sizeof(st[survey_count]));

#ifdef MT7615
		char number[4] = {0};	
		next_field(&line, number, 4);
		if (!isdigit(number[0]))
		{
			line = strtok(NULL, "\n");
			continue;
		}
#endif

		next_field(&line, st[survey_count].channel, sizeof(st->channel));
		next_field(&line, st[survey_count].ssid, sizeof(st->ssid));
#ifndef MT7615
		next_field(&line, st[survey_count].len, sizeof(st->len));
#endif
		next_field(&line, st[survey_count].bssid, sizeof(st->bssid));
		next_field(&line, st[survey_count].security, sizeof(st->security));
		st[survey_count].crypto = strstr(st[survey_count].security, "/");
		if (st[survey_count].crypto)
		{
			*st[survey_count].crypto = '\0';
			st[survey_count].crypto++;
		}
		next_field(&line, st[survey_count].signal, sizeof(st->signal));

		next_field(&line, buf, sizeof(buf));	/* W-Mode */
		next_field(&line, buf, sizeof(buf));	/* ExtCH */
		next_field(&line, buf, sizeof(buf));	/* NT */

#ifdef MT7615
		next_field(&line, st[survey_count].len, sizeof(st->len));
#endif

		line = strtok(NULL, "\n");

		/* skip hidden ssid */
#ifdef MT7615
		if (st[survey_count].ssid == NULL || strcmp(st[survey_count].ssid, " ") == 0)
			continue;
#endif
		if (!strcmp(st[survey_count].len, "0"))
			continue;

		survey_count++;
	}
	free(s);
	return survey_count;
}

static int mtk_get_scanlist(const char *ifname, char *buf, int *len)
{
	int i = 0, h, sc;
	struct iwinfo_scanlist_entry sce;
	struct survey_table stl[64];
	
	sc = mtk_get_scan(ifname, stl);
	*len = 0;
	
	for (i = 0; i < sc; i++) {
		memset(&sce, 0, sizeof(sce));

		if (strstr(stl[i].security,"UNKNOW"))
			continue;

		for (h = 0; h < 6; h++)
			sce.mac[h] = (uint8_t)(ascii2num(stl[i].bssid[h * 3]) * 16 + ascii2num(stl[i].bssid[h * 3 + 1]));

		strcpy(sce.ssid, stl[i].ssid);

		sce.channel = atoi(stl[i].channel);
		sce.quality = atoi(stl[i].signal);
		sce.quality_max = 100;
		sce.signal = atoi(stl[i].signal);
		sce.mode = IWINFO_OPMODE_MASTER;

		if (!strcmp(stl[i].security, "NONE") || !strcmp(stl[i].security, "OPEN")) {
			sce.crypto.enabled = 0;
		} else {
			sce.crypto.enabled = 1;

			if (strstr(stl[i].security,"WPA3"))
				sce.crypto.wpa_version |= 1 << 2;

			if (strstr(stl[i].security,"WPA2"))
				sce.crypto.wpa_version |= 1 << 1;

			if (strstr(stl[i].security,"WPAPSK") || strstr(stl[i].security,"WPA1PSK"))
				sce.crypto.wpa_version |= 1 << 0;

			if (strstr(stl[i].security,"PSK"))
				sce.crypto.auth_suites |= IWINFO_KMGMT_PSK;

			if (!strcmp(stl[i].crypto,"AES"))
				sce.crypto.pair_ciphers = IWINFO_CIPHER_CCMP;
			else if (!strcmp(stl[i].crypto,"TKIP"))
				sce.crypto.pair_ciphers = IWINFO_CIPHER_TKIP;
		}
		
		memcpy(buf + *len, &sce, sizeof(struct iwinfo_scanlist_entry));
		*len += sizeof(struct iwinfo_scanlist_entry);
	}

	return 0;
}

static int mtk_get_freqlist(const char *ifname, char *buf, int *len)
{
	return wext_ops.freqlist(ifname, buf, len);
}

static int mtk_get_country(const char *ifname, char *buf)
{
	sprintf(buf, "00");
	return 0;
}

static int mtk_get_countrylist(const char *ifname, char *buf, int *len)
{
	int count;
	struct iwinfo_country_entry *e = (struct iwinfo_country_entry *)buf;
	const struct iwinfo_iso3166_label *l;

	for (l = IWINFO_ISO3166_NAMES, count = 0; l->iso3166; l++, e++, count++)
	{
		e->iso3166 = l->iso3166;
		e->ccode[0] = (l->iso3166 / 256);
		e->ccode[1] = (l->iso3166 % 256);
		e->ccode[2] = 0;
	}

	*len = (count * sizeof(struct iwinfo_country_entry));
	return 0;
}

static int mtk_get_hwmodelist(const char *ifname, int *buf)
{
	if (mtk_isap(ifname)) {
		if (strlen(ifname) > 3 && !strncmp(ifname, "rai", 3)) {
			*buf = IWINFO_80211_N | IWINFO_80211_AC;
		} else {
			*buf = IWINFO_80211_B | IWINFO_80211_G | IWINFO_80211_N;
		}
	} else if (mtk_iscli(ifname)) {
		if (strlen(ifname) > 6 && !strncmp(ifname, "apclii", 6)) {
			*buf = IWINFO_80211_N | IWINFO_80211_AC;
		} else {
			*buf = IWINFO_80211_B | IWINFO_80211_G | IWINFO_80211_N;
		}
	}

	return 0;
}

static int mtk_get_htmodelist(const char *ifname, int *buf)
{
	if (mtk_isap(ifname)) {
		if (strlen(ifname) > 3 && !strncmp(ifname, "rai", 3)) {
			*buf = IWINFO_HTMODE_VHT20 | IWINFO_HTMODE_VHT40 | IWINFO_HTMODE_VHT80;
		} else {
			*buf = IWINFO_HTMODE_HT20 | IWINFO_HTMODE_HT40;
		}
	} else if (mtk_iscli(ifname)) {
		if (strlen(ifname) > 6 && !strncmp(ifname, "apclii", 6)) {
			*buf = IWINFO_HTMODE_VHT20 | IWINFO_HTMODE_VHT40 | IWINFO_HTMODE_VHT80;
		} else {
			*buf = IWINFO_HTMODE_HT20 | IWINFO_HTMODE_HT40;
		}
	}

	return 0;
}

static int mtk_get_encryption(const char *ifname, char *buf)
{
	/* No reliable crypto info in mtk */
	return -1;
}

static int mtk_get_phyname(const char *ifname, char *buf)
{
	/* No suitable api in mtk */
	strcpy(buf, ifname);
	return 0;
}

static int mtk_get_mbssid_support(const char *ifname, int *buf)
{
	return -1;
}

static int mtk_get_hardware_id(const char *ifname, char *buf)
{
	struct iwinfo_hardware_id *id = (struct iwinfo_hardware_id *)buf;
	char data[10];

	memset(id, 0, sizeof(struct iwinfo_hardware_id));

	return iwinfo_hardware_id_from_mtd(id);
}

static int mtk_get_hardware_name(const char *ifname, char *buf)
{
	struct iwinfo_hardware_id id;
	struct iwinfo_hardware_entry *e;

	if (mtk_get_hardware_id(ifname, (char *)&id))
		return -1;

	e = iwinfo_hardware(&id);
	if (!e)
		return -1;

	strcpy(buf, e->device_name);

	return 0;
}

static int mtk_get_txpower_offset(const char *ifname, int *buf)
{
	/* Stub */
	*buf = 0;
	return -1;
}

static int mtk_get_frequency_offset(const char *ifname, int *buf)
{
	/* Stub */
	*buf = 0;
	return -1;
}

const struct iwinfo_ops mtk_ops = {
	.name             = "mtk",
	.probe            = mtk_probe,
	.channel          = mtk_get_channel,
	.frequency        = mtk_get_frequency,
	.frequency_offset = mtk_get_frequency_offset,
	.txpower          = mtk_get_txpower,
	.txpower_offset   = mtk_get_txpower_offset,
	.bitrate          = mtk_get_bitrate,
	.signal           = mtk_get_signal,
	.noise            = mtk_get_noise,
	.quality          = mtk_get_quality,
	.quality_max      = mtk_get_quality_max,
	.mbssid_support   = mtk_get_mbssid_support,
	.hwmodelist       = mtk_get_hwmodelist,
	.htmodelist       = mtk_get_htmodelist,
	.mode             = mtk_get_mode,
	.ssid             = mtk_get_ssid,
	.bssid            = mtk_get_bssid,
	.country          = mtk_get_country,
	.hardware_id      = mtk_get_hardware_id,
	.hardware_name    = mtk_get_hardware_name,
	.encryption       = mtk_get_encryption,
	.phyname          = mtk_get_phyname,
	.assoclist        = mtk_get_assoclist,
	.txpwrlist        = mtk_get_txpwrlist,
	.scanlist         = mtk_get_scanlist,
	.freqlist         = mtk_get_freqlist,
	.countrylist      = mtk_get_countrylist,
	.close            = mtk_close
};
