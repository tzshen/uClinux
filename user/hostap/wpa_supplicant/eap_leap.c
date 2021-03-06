/*
 * WPA Supplicant / EAP-LEAP
 * Copyright (c) 2004, Jouni Malinen <jkmaline@cc.hut.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 * See README and COPYING for more details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>

#include "common.h"
#include "eapol_sm.h"
#include "eap.h"
#include "wpa_supplicant.h"
#include "config.h"
#include "ms_funcs.h"
#include "md5.h"

#define LEAP_VERSION 1
#define LEAP_CHALLENGE_LEN 8
#define LEAP_RESPONSE_LEN 24


struct eap_leap_data {
	enum {
		LEAP_WAIT_CHALLENGE,
		LEAP_WAIT_SUCCESS,
		LEAP_WAIT_RESPONSE,
		LEAP_DONE
	} state;

	u8 peer_challenge[LEAP_CHALLENGE_LEN];
	u8 peer_response[LEAP_RESPONSE_LEN];

	u8 ap_challenge[LEAP_CHALLENGE_LEN];
	u8 ap_response[LEAP_RESPONSE_LEN];
};


static void * eap_leap_init(struct eap_sm *sm)
{
	struct eap_leap_data *data;

	data = malloc(sizeof(*data));
	if (data == NULL)
		return NULL;
	memset(data, 0, sizeof(*data));
	data->state = LEAP_WAIT_CHALLENGE;

	sm->leap_done = FALSE;
	return data;
}


static void eap_leap_deinit(struct eap_sm *sm, void *priv)
{
	free(priv);
}


static u8 * eap_leap_process_request(struct eap_sm *sm, void *priv,
				     u8 *reqData, size_t reqDataLen,
				     size_t *respDataLen)
{
	struct eap_leap_data *data = priv;
	struct wpa_ssid *config = sm->eapol->config;
	struct eap_hdr *req, *resp;
	u8 *pos, *challenge, challenge_len;

	wpa_printf(MSG_DEBUG, "EAP-LEAP: Processing EAP-Request");

	req = (struct eap_hdr *) reqData;
	pos = (u8 *) (req + 1);
	if (reqDataLen < sizeof(*req) + 4 || *pos != EAP_TYPE_LEAP) {
		wpa_printf(MSG_INFO, "EAP-LEAP: Invalid EAP-Request frame");
		sm->ignore = TRUE;
		return NULL;
	}
	pos++;

	if (*pos != LEAP_VERSION) {
		wpa_printf(MSG_WARNING, "EAP-LEAP: Unsupported LEAP version "
			   "%d", *pos);
		sm->ignore = TRUE;
		return NULL;
	}
	pos++;

	pos++; /* skip unused byte */

	challenge_len = *pos++;
	if (challenge_len != LEAP_CHALLENGE_LEN ||
	    challenge_len > reqDataLen - sizeof(*req) - 4) {
		wpa_printf(MSG_INFO, "EAP-LEAP: Invalid challenge "
			   "(challenge_len=%d reqDataLen=%d",
			   challenge_len, reqDataLen);
		sm->ignore = TRUE;
		return NULL;
	}
	sm->ignore = FALSE;
	challenge = pos;
	memcpy(data->peer_challenge, challenge, LEAP_CHALLENGE_LEN);
	wpa_hexdump(MSG_MSGDUMP, "EAP-LEAP: Challenge from AP",
		    challenge, LEAP_CHALLENGE_LEN);

	wpa_printf(MSG_DEBUG, "EAP-LEAP: Generating Challenge Response");
	sm->allowNotifications = TRUE;
	sm->methodState = METHOD_CONT;

	*respDataLen = sizeof(struct eap_hdr) + 1 + 3 + LEAP_RESPONSE_LEN +
		config->identity_len;
	resp = malloc(*respDataLen);
	if (resp == NULL)
		return NULL;
	resp->code = EAP_CODE_RESPONSE;
	resp->identifier = req->identifier;
	resp->length = htons(*respDataLen);
	pos = (u8 *) (resp + 1);
	*pos++ = EAP_TYPE_LEAP;
	*pos++ = LEAP_VERSION;
	*pos++ = 0; /* unused */
	*pos++ = LEAP_RESPONSE_LEN;
	nt_challenge_response(challenge,
			      config->password, config->password_len, pos);
	memcpy(data->peer_response, pos, LEAP_RESPONSE_LEN);
	wpa_hexdump(MSG_MSGDUMP, "EAP-LEAP: Response", pos, LEAP_RESPONSE_LEN);
	pos += LEAP_RESPONSE_LEN;
	memcpy(pos, config->identity, config->identity_len);

	data->state = LEAP_WAIT_SUCCESS;

	return (u8 *) resp;
}


static u8 * eap_leap_process_success(struct eap_sm *sm, void *priv,
				     u8 *reqData, size_t reqDataLen,
				     size_t *respDataLen)
{
	struct eap_leap_data *data = priv;
	struct wpa_ssid *config = sm->eapol->config;
	struct eap_hdr *req, *resp;
	u8 *pos;

	wpa_printf(MSG_DEBUG, "EAP-LEAP: Processing EAP-Success");

	if (data->state != LEAP_WAIT_SUCCESS) {
		wpa_printf(MSG_INFO, "EAP-LEAP: EAP-Success received in "
			   "unexpected state (%d) - ignored", data->state);
		sm->ignore = TRUE;
		return NULL;
	}

	req = (struct eap_hdr *) reqData;

	*respDataLen = sizeof(struct eap_hdr) + 1 + 3 + LEAP_CHALLENGE_LEN +
		config->identity_len;
	resp = malloc(*respDataLen);
	if (resp == NULL)
		return NULL;
	resp->code = EAP_CODE_REQUEST;
	resp->identifier = req->identifier;
	resp->length = htons(*respDataLen);
	pos = (u8 *) (resp + 1);
	*pos++ = EAP_TYPE_LEAP;
	*pos++ = LEAP_VERSION;
	*pos++ = 0; /* unused */
	*pos++ = LEAP_CHALLENGE_LEN;
	if (hostapd_get_rand(pos, LEAP_CHALLENGE_LEN)) {
		wpa_printf(MSG_WARNING, "EAP-LEAP: Failed to read random data "
			   "for challenge");
		free(resp);
		sm->ignore = TRUE;
		return NULL;
	}
	memcpy(data->ap_challenge, pos, LEAP_CHALLENGE_LEN);
	wpa_hexdump(MSG_MSGDUMP, "EAP-LEAP: Challenge to AP/AS", pos,
		    LEAP_CHALLENGE_LEN);
	pos += LEAP_CHALLENGE_LEN;
	memcpy(pos, config->identity, config->identity_len);

	data->state = LEAP_WAIT_RESPONSE;
	sm->allowNotifications = TRUE;
	sm->methodState = METHOD_CONT;

	return (u8 *) resp;
}


static u8 * eap_leap_process_response(struct eap_sm *sm, void *priv,
				      u8 *reqData, size_t reqDataLen,
				      size_t *respDataLen)
{
	struct eap_leap_data *data = priv;
	struct wpa_ssid *config = sm->eapol->config;
	struct eap_hdr *resp;
	u8 *pos, response_len, pw_hash[16], pw_hash_hash[16],
		expected[LEAP_RESPONSE_LEN];

	wpa_printf(MSG_DEBUG, "EAP-LEAP: Processing EAP-Response");

	resp = (struct eap_hdr *) reqData;
	pos = (u8 *) (resp + 1);
	if (reqDataLen < sizeof(*resp) + 4 || *pos != EAP_TYPE_LEAP) {
		wpa_printf(MSG_INFO, "EAP-LEAP: Invalid EAP-Response frame");
		sm->ignore = TRUE;
		return NULL;
	}
	pos++;

	if (*pos != LEAP_VERSION) {
		wpa_printf(MSG_WARNING, "EAP-LEAP: Unsupported LEAP version "
			   "%d", *pos);
		sm->ignore = TRUE;
		return NULL;
	}
	pos++;

	pos++; /* skip unused byte */

	response_len = *pos++;
	if (response_len != LEAP_RESPONSE_LEN ||
	    response_len > reqDataLen - sizeof(*resp) - 4) {
		wpa_printf(MSG_INFO, "EAP-LEAP: Invalid response "
			   "(response_len=%d reqDataLen=%d",
			   response_len, reqDataLen);
		sm->ignore = TRUE;
		return NULL;
	}

	wpa_hexdump(MSG_DEBUG, "EAP-LEAP: Response from AP",
		    pos, LEAP_RESPONSE_LEN);
	memcpy(data->ap_response, pos, LEAP_RESPONSE_LEN);

	nt_password_hash(config->password, config->password_len, pw_hash);
	hash_nt_password_hash(pw_hash, pw_hash_hash);
	challenge_response(data->ap_challenge, pw_hash_hash, expected);
	if (memcmp(pos, expected, LEAP_RESPONSE_LEN) != 0) {
		wpa_printf(MSG_WARNING, "EAP-LEAP: AP sent an invalid "
			   "response - authentication failed");
		wpa_hexdump(MSG_DEBUG, "EAP-LEAP: Expected response from AP",
			    expected, LEAP_RESPONSE_LEN);
		sm->ignore = TRUE;
		sm->methodState = METHOD_DONE;
		sm->decision = DECISION_FAIL;
		return NULL;
	}

	sm->ignore = FALSE;

	sm->methodState = METHOD_DONE;
	sm->decision = DECISION_UNCOND_SUCC;

	/* LEAP is somewhat odd method since it sends EAP-Success in the middle
	 * of the authentication. Use special variable to transit EAP state
	 * machine to SUCCESS state. */
	sm->leap_done = TRUE;

	free(sm->eapKeyData);
	sm->eapKeyData = malloc(16);
	if (sm->eapKeyData) {
		u8 pw_hash_hash[16], pw_hash[16];
		MD5_CTX context;

		nt_password_hash(config->password, config->password_len,
				 pw_hash);
		hash_nt_password_hash(pw_hash, pw_hash_hash);
		wpa_hexdump(MSG_DEBUG, "EAP-LEAP: pw_hash_hash",
			    pw_hash_hash, 16);
		wpa_hexdump(MSG_DEBUG, "EAP-LEAP: peer_challenge",
			    data->peer_challenge, LEAP_CHALLENGE_LEN);
		wpa_hexdump(MSG_DEBUG, "EAP-LEAP: peer_response",
			    data->peer_response, LEAP_RESPONSE_LEN);
		wpa_hexdump(MSG_DEBUG, "EAP-LEAP: ap_challenge",
			    data->ap_challenge, LEAP_CHALLENGE_LEN);
		wpa_hexdump(MSG_DEBUG, "EAP-LEAP: ap_response",
			    data->ap_response, LEAP_RESPONSE_LEN);

		MD5Init(&context);
		MD5Update(&context, pw_hash_hash, 16);
		MD5Update(&context, data->ap_challenge, LEAP_CHALLENGE_LEN);
		MD5Update(&context, data->ap_response, LEAP_RESPONSE_LEN);
		MD5Update(&context, data->peer_challenge, LEAP_CHALLENGE_LEN);
		MD5Update(&context, data->peer_response, LEAP_RESPONSE_LEN);
		MD5Final(sm->eapKeyData, &context);
		wpa_hexdump(MSG_DEBUG, "EAP-LEAP: master key", sm->eapKeyData,
			    16);
		sm->eapKeyDataLen = 16;
	}

	data->state = LEAP_DONE;

	/* No more authentication messages expected; AP will send EAPOL-Key
	 * frames if encryption is enabled. */
	return NULL;
}


static u8 * eap_leap_process(struct eap_sm *sm, void *priv,
			    u8 *reqData, size_t reqDataLen,
			    size_t *respDataLen)
{
	struct wpa_ssid *config = sm->eapol->config;
	struct eap_hdr *eap;

	if (config == NULL || config->password == NULL) {
		wpa_printf(MSG_INFO, "EAP-LEAP: Password not configured");
		eap_sm_request_password(sm, config);
		sm->ignore = TRUE;
		return NULL;
	}

	eap = (struct eap_hdr *) reqData;

	if (reqDataLen < sizeof(*eap)) {
		wpa_printf(MSG_INFO, "EAP-LEAP: Invalid frame");
		sm->ignore = TRUE;
		return NULL;
	}

	sm->leap_done = FALSE;

	switch (eap->code) {
	case EAP_CODE_REQUEST:
		return eap_leap_process_request(sm, priv, reqData, reqDataLen,
						respDataLen);
	case EAP_CODE_SUCCESS:
		return eap_leap_process_success(sm, priv, reqData, reqDataLen,
						respDataLen);
	case EAP_CODE_RESPONSE:
		return eap_leap_process_response(sm, priv, reqData, reqDataLen,
						 respDataLen);
	default:
		wpa_printf(MSG_INFO, "EAP-LEAP: Unexpected EAP code (%d) - "
			   "ignored", eap->code);
		sm->ignore = TRUE;
		return NULL;
	}
}


const struct eap_method eap_method_leap =
{
	.method = EAP_TYPE_LEAP,
	.init = eap_leap_init,
	.deinit = eap_leap_deinit,
	.process = eap_leap_process,
};
