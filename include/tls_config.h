#ifndef __TLS_CONFIG_
#define __TLS_CONFIG_

#if defined(Z_SECURE_CONNECTION)
#include "tls_network.h"
#else
#include "MQTTLinux.h"
#endif

#endif