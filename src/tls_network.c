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

#include "tls_network.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <log.h>

void InitTimer(Timer *run_timer)
{
    run_timer->end_time = (struct timeval){0, 0};
}

int left_ms(Timer *run_timer)
{
    struct timeval curr_time, res;
    gettimeofday(&curr_time, NULL);
    timersub(&run_timer->end_time, &curr_time, &res);
    return (res.tv_sec < 0) ? 0 : ((res.tv_sec * 1000) + (res.tv_usec / 1000));
}

void countdown(Timer *run_timer, unsigned int timeout)
{
    struct timeval curr_time;
    gettimeofday(&curr_time, NULL);
    struct timeval interval = {timeout, 0};
    timeradd(&curr_time, &interval, &run_timer->end_time);
}

void countdown_ms(Timer *run_timer, unsigned int timeout)
{
    struct timeval curr_time;
    gettimeofday(&curr_time, NULL);
    struct timeval interval = {timeout / 1000, (timeout % 1000) * 1000};
    timeradd(&curr_time, &interval, &run_timer->end_time);
}

char expired(Timer *run_timer)
{
    struct timeval curr_time, res;
    gettimeofday(&curr_time, NULL);
    timersub(&run_timer->end_time, &curr_time, &res);
    return res.tv_sec < 0 || (res.tv_sec == 0 && res.tv_usec <= 0);
}

void NewNetwork(Network *n)
{
    n->my_socket = 0;
    n->mqttread = tls_read;
    n->mqttwrite = tls_write;
    n->disconnect = tls_disconnect;
}

void tls_debug(void *ctx, int level,
               const char *file, int line, const char *str)
{
    log_debug("TLS_LOG: %s:%04d: %s", file, line, str);
}

int init_tls(Network *n, certsParseMode mode, char *ca_crt, char *client_cert, char *client_key, char *cert_password)
{
    //TODO:1: move it to connect method. it should be configurable by user.
    char *clientName = "dummyDeviceID";
    int rc = -1;

    mbedtls_net_init(&(n->server_fd));
    mbedtls_ssl_init(&(n->ssl));
    mbedtls_ssl_config_init(&(n->conf));
    mbedtls_x509_crt_init(&(n->cacert));
    mbedtls_ctr_drbg_init(&(n->ctr_drbg));
    mbedtls_entropy_init(&(n->entropy));
#if defined(USE_CLIENT_CERTS)
    mbedtls_x509_crt_init(&(n->clicert));
    mbedtls_pk_init(&(n->pkey));
#endif
    if ((rc = mbedtls_ctr_drbg_seed(&n->ctr_drbg, mbedtls_entropy_func, &n->entropy,
                                    (const unsigned char *)clientName,
                                    strlen(clientName))) != 0)
    {
        log_trace("mbedtls_ctr_drbg_seed failed with return code = %d \n", rc);
        return -1;
    }
    rc = (mode == EMBED) ? mbedtls_x509_crt_parse(&(n->cacert), ca_crt, strlen(ca_crt) + 1) : mbedtls_x509_crt_parse_file(&(n->cacert), ca_crt);

    if (rc != 0)
    {
        log_trace("mbedtls_x509_crt_parse_file failed for Server CA certificate with return code = 0x%x", -rc);
        return -1;
    }

#if defined(USE_CLIENT_CERTS)

    rc = (mode == EMBED) ? mbedtls_x509_crt_parse(&n->clicert, client_cert, strlen(client_cert) + 1) : mbedtls_x509_crt_parse_file(&n->clicert, client_cert);

    if (rc != 0)
    {
        log_trace("mbedtls_x509_crt_parse_file failed for Device Certificate  with return code = 0x%x", -rc);
        return -1;
    }

    rc = (mode == EMBED) ? mbedtls_pk_parse_key(&(n->pkey), client_key, strlen(client_key) + 1, cert_password, strlen(cert_password)) : mbedtls_pk_parse_keyfile(&(n->pkey), client_key, cert_password);

    if (rc != 0)
    {
        log_trace("mbedtls_pk_parse_keyfile failed for Device Private Key with return code = 0x%x", -rc);
        return -1;
    }
#endif
    return 0;
}

int tls_read(Network *n, unsigned char *buffer, int len, int timeout_ms)
{
    int rc = -1, bytes = 0;
    struct timeval interval = {timeout_ms / 1000, (timeout_ms % 1000) * 1000};

    if (interval.tv_sec < 0 || (interval.tv_sec == 0 && interval.tv_usec <= 0))
    {
        interval.tv_sec = 0;
        interval.tv_usec = 100;
    }
    setsockopt(n->server_fd.fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&interval, sizeof(struct timeval));
    while (bytes < len)
    {
        rc = mbedtls_ssl_read(&(n->ssl), buffer, len);
        if (rc == 0 || rc == -76)
            break;
        else if (rc < 0)
        {
            log_trace("mbedtls_ssl_read failed with rc = %d\n", rc);
            bytes = -1;
            break;
        }
        else
            bytes += rc;
    }
    return bytes;
}

int tls_write(Network *n, unsigned char *buffer, int len, int timeout_ms)
{
    size_t writtenLength = 0;
    int rc = -1;

    while (writtenLength < len)
    {
        rc = mbedtls_ssl_write(&(n->ssl), (unsigned char *)(buffer + writtenLength), (len - writtenLength));

        if (rc > 0)
        {
            writtenLength += rc;
            continue;
        }
        else if (rc == 0)
        {
            log_trace("write timeout");
            return writtenLength;
        }
        else
        {
            log_trace("write fail");
            return -1;
        }
    }
    return writtenLength;
}

int ConnectNetwork(Network *n, char *addr, int port, certsParseMode mode, char *ca_crt, char *client_cert, char *client_key, char *cert_password)
{
    int rc = -1;
    char port_char[5]; // max 4 digit port number
    uint32_t flags;

    //TODO: Hardcode the value of hostname & port.
    sprintf(port_char, "%d", port);

    if (rc = init_tls(n, mode, ca_crt, client_cert, client_key, cert_password) != 0)
    {
        log_debug("intializing TLS failed\n");
        return -1;
    }
    #if defined(USE_CLIENT_CERTS)
        if ((rc = mbedtls_ssl_conf_own_cert(&n->conf, &n->clicert, &n->pkey)) != 0)
        {
            log_trace("mbedtls_ssl_conf_own_cert failed with return code = 0x%x", -rc);
            return -1;
        }
        if ((rc = mbedtls_ssl_set_hostname(&(n->ssl), addr)) != 0)
        {
            log_trace("mbedtls_ssl_set_hostname failed with rc = 0x%x", -rc);
            return -1;
        }
    #endif
    if ((rc = mbedtls_net_connect(&n->server_fd, addr, port_char, MBEDTLS_NET_PROTO_TCP)) != 0)
    {
        log_debug("mbedtls_net_connect failed with return code = 0x%x", -rc);
        return -1;
    }

    if ((rc = mbedtls_ssl_config_defaults(&(n->conf), MBEDTLS_SSL_IS_CLIENT,
                                          MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
    {
        log_trace("mbedtls_ssl_config_defaults failed with return code = 0x%x", -rc);
        return -1;
    }

    mbedtls_ssl_conf_max_version(&n->conf, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
    mbedtls_ssl_conf_min_version(&n->conf, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
    mbedtls_ssl_conf_authmode(&n->conf, MBEDTLS_SSL_VERIFY_REQUIRED);
    mbedtls_ssl_conf_ca_chain(&n->conf, &n->cacert, NULL);
    mbedtls_ssl_conf_rng(&n->conf, mbedtls_ctr_drbg_random, &n->ctr_drbg);
    //TODO: enable logging only on debug builds.
    mbedtls_ssl_conf_dbg(&n->conf, tls_debug, stdout);
    mbedtls_ssl_set_bio(&n->ssl, &n->server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

    if ((rc = mbedtls_ssl_setup(&(n->ssl), &(n->conf))) != 0)
    {
        log_trace(" failed! mbedtls_ssl_setup returned %d", rc);
        return -1;
    }
    while ((rc = mbedtls_ssl_handshake(&n->ssl)) != 0)
    {
        if (rc != MBEDTLS_ERR_SSL_WANT_READ && rc != MBEDTLS_ERR_SSL_WANT_WRITE)
        {
            log_debug("mbedtls_ssl_handshake failed with rc = 0x%x", -rc);
            break;
        }
    }
    if (rc != 0)
    {
        log_debug("Connection disconnected with rc = %d.\n Quitting..", rc);
        return -1;
    }

    if ((flags = mbedtls_ssl_get_verify_result(&n->ssl)) != 0)
    {
        char vrfy_buf[512];
        mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), "  ! ", flags);
        log_error("Cert verification failed: %s", vrfy_buf);
    }
    else
    {
        log_debug("cetificates are verified");
    }

    n->my_socket = n->server_fd.fd;
    log_debug("TLS connection established");

    return 0;
}

void tls_disconnect(Network *n)
{
    log_trace("disconnecting TLS...");

    //TODO: ensure connected got clean close and remove this.
    // do ret = mbedtls_ssl_close_notify(&n.ssl );
    // while( ret == MBEDTLS_ERR_SSL_WANT_WRITE );

    mbedtls_ssl_close_notify(&(n->ssl));
    mbedtls_net_free(&(n->server_fd));
    mbedtls_x509_crt_free(&(n->cacert));
#if defined(USE_CLIENT_CERTS)
    mbedtls_x509_crt_free(&(n->clicert));
    mbedtls_pk_free(&(n->pkey));
#endif
    mbedtls_ssl_free(&(n->ssl));
    mbedtls_ssl_config_free(&(n->conf));
    mbedtls_ctr_drbg_free(&(n->ctr_drbg));
    mbedtls_entropy_free(&(n->entropy));
}