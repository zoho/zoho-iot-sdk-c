/*******************************************************************************
 * Copyright (c) 2014, 2015 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 * 
 *******************************************************************************/

#ifndef __TLS_NETWORK_H__
#define __TLS_NETWORK_H__

#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/certs.h"
#include "mbedtls/config.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "zclient_constants.h"
#include <sys/time.h>

typedef struct
{
	struct timeval end_time;
} Timer;

char TimerIsExpired(Timer *);
void TimerCountdownMS(Timer *, unsigned int);
void TimerCountdown(Timer *, unsigned int);
int TimerLeftMS(Timer *);
void TimerInit(Timer *);

typedef struct Network Network;

struct Network
{
	int my_socket;
	int (*mqttread)(Network *, unsigned char *, int, int);
	int (*mqttwrite)(Network *, unsigned char *, int, int);

	mbedtls_ssl_context ssl;
	mbedtls_net_context server_fd;
	mbedtls_ssl_config conf;
	mbedtls_x509_crt cacert;
	mbedtls_x509_crt clicert;
	mbedtls_pk_context pkey;
	mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context ctr_drbg;
};

void NetworkInit(Network *);

int NetworkConnectTLS(Network *, char *addr, int port, certsParseMode mode, char *ca_crt, char *client_cert, char *client_key, char *cert_password);
int NetworkConnect(Network* n, char* addr, int port);
int tls_write(Network *n, unsigned char *buffer, int len, int timeout_ms);
int tls_read(Network *n, unsigned char *buffer, int len, int timeout_ms);
void NetworkDisconnect(Network *n);
#endif