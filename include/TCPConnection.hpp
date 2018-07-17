/* COPYRIGHT (c) 2016-2018 Nova Labs SRL
 *
 * All rights reserved. All use of this software and documentation is
 * subject to the License Agreement located in the file LICENSE.
 */

#pragma once

#include "lwip/api.h"
#include "lwip/netbuf.h"

namespace core {
namespace networking {
class TCPConnection
{
public:
    TCPConnection() : _netconn(nullptr), _netbuf(nullptr), _netbuf_offset(0), _connected(false)
    {}

    ~TCPConnection() {
    	disconnect();
    }

    err_t
    connect(
        const char* hostname,
        uint16_t    port
    )
    {
        ip_addr_t ip_addr;
        err_t     err;

        err = netconn_gethostbyname(hostname, &ip_addr);

        if (err != ERR_OK) {
            return err;
        }

        _netconn = netconn_new(NETCONN_TCP);

        if (_netconn == NULL) {
            return ERR_MEM;
        }

        err = netconn_connect(_netconn, &ip_addr, port);

        if (err != ERR_OK) {
            _netconn = nullptr;
            netconn_delete(_netconn);
        }

        netconn_set_nonblocking(_netconn, 0);

        _connected = (err == ERR_OK);

        return err;
    } // connect

    err_t
    read(
        char*   buffer,
        size_t  len,
        size_t* rbytes,
        int32_t timeout
    )
    {
        err_t err;
        struct netbuf* inbuf;
        uint16_t       offset = 0;
        size_t         to_read = len;


        if (timeout <= 0) {
            timeout = 100;
        }

        netconn_set_recvtimeout(_netconn, timeout);

        size_t bytes = 0;

        while (to_read > 0) {
            if (_netbuf != NULL) {
                inbuf  = _netbuf;
                offset = _netbuf_offset;
                err    = ERR_OK;
            } else {
                err    = netconn_recv(_netconn, &inbuf);
                offset = 0;
            }

            if (err != ERR_OK) {
                if (err != ERR_TIMEOUT) {
                    if (err == ERR_CLSD) {
                        _connected = false;
                    }
                }

                break;
            } else {
                int nblen = netbuf_len(inbuf) - offset;

                if (nblen > to_read) {
                    uint16_t copied = netbuf_copy_partial(inbuf, buffer + bytes, to_read, offset);
                    _netbuf = inbuf;
                    _netbuf_offset = offset + copied;
                    bytes += copied;
                    to_read -= copied;
                } else {
                	uint16_t copied = netbuf_copy_partial(inbuf, buffer + bytes, nblen, offset);
                	if(copied == nblen) {
                        netbuf_delete(inbuf);
                        _netbuf = NULL;
                        _netbuf_offset = 0;
                	} else {
                        _netbuf = inbuf;
                        _netbuf_offset = offset + copied;
                	}

                	bytes += copied;
                    to_read -= copied;
                }
            }
        }

        *rbytes = bytes;
        return err;
    } // read

    err_t
    write(
        const char* buffer,
        size_t      len,
        size_t*     wbytes,
        int32_t     timeout
    )
    {
        err_t err;

        if (timeout <= 0) {
            timeout = 100;
        }

        netconn_set_sendtimeout(_netconn, timeout);

        size_t bytes = 0;

        do {
            err      = netconn_write_partly(_netconn, buffer, len, NETCONN_NOCOPY, &bytes);
            len     -= bytes;
            *wbytes += bytes;
        } while ((err == ERR_OK) && (len > 0));

        if (err != ERR_OK) {
            if (err == ERR_CONN) {
                _connected = false;
            }
        }

        return err;
    } // write

    err_t
    disconnect()
    {
        if (_netconn != nullptr) {
            netconn_close(_netconn);
            netconn_delete(_netconn);
            _connected = false;
            _netconn   = nullptr;
        }

        return ERR_OK;
    }

    inline bool
    isConnected()
    {
        return _connected;
    }

private:
    netconn* _netconn;
    netbuf*  _netbuf;
    uint16_t _netbuf_offset;
    bool     _connected;
};
}
}
