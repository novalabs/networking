/* COPYRIGHT (c) 2016-2018 Nova Labs SRL
 *
 * All rights reserved. All use of this software and documentation is
 * subject to the License Agreement located in the file LICENSE.
 */

#include <core/networking/HTTPClient.hpp>

#ifndef HTTP_CLIENT_BUFFER_SIZE
#define HTTP_CLIENT_BUFFER_SIZE 1024
#endif

#include <core/networking/TCPConnection.hpp>

#include <core/StringBuffer.hpp>

#include <memory>

#include "http_parser.h"
#include "yuarel.h"

namespace core {
namespace networking {
/* Helper class used to bridge C callbacks with C++ ones (and to access HTTPClient privates)
 *
 */
class HTTPClientCallbacks
{
public:
    static int
    status_callback(
        http_parser* p,
        const char*  at,
        size_t       length
    )
    {
        HTTPClient* _this = reinterpret_cast<HTTPClient*>(p->data);

        _this->_status_code = p->status_code;

        if (_this->_on_status) {
            _this->_on_status(p->status_code);
        }

        return 0;
    }

    static int
    headers_complete(
        http_parser* p
    )
    {
        return 0;
    }

    static int
    body_callback(
        http_parser* p,
        const char*  at,
        size_t       length
    )
    {
        HTTPClient* _this = reinterpret_cast<HTTPClient*>(p->data);

        if (_this->_on_body) {
            _this->_on_body(at, length);
        }

        return 0;
    }

    static int
    message_complete(
        http_parser* p
    )
    {
        HTTPClient* _this = reinterpret_cast<HTTPClient*>(p->data);

        if (_this->_on_message_complete) {
            _this->_on_message_complete();
        }

        return 0;
    }
};

/*! \brief Manages a copy of a C style string
 *
 */
class StringDuplicate
{
public:
    StringDuplicate(
        const char* s
    ) : _size(strlen(s)), _s(new char[_size])
    {
        if (_s) {
            ::strcpy(_s.get(), s);
        }
    }

    ~StringDuplicate() {}

    inline
    operator bool()
    {
        return (bool)_s;
    }

    inline
    operator char*()
    {
        return _s.get();
    }

    inline std::size_t
    size()
    {
        return _size;
    }

private:
    std::size_t _size;
    std::unique_ptr<char[]> _s;
};

bool
HTTPClient::request(
    const char* method,
    const char* url_string
)
{
    {
        bool success = false;
        _status_code = 0;

        // Make a copy of the url string, as the parser will modify it.
        StringDuplicate url_string2(url_string);

        if (!url_string2) {
            return false;
        }

        struct yuarel url;

        if (-1 == yuarel_parse(&url, url_string2)) {
            return false;
        }

        TCPConnection conn;

        http_parser parser;
        http_parser_settings settings;

        http_parser_settings_init(&settings);
        http_parser_init(&parser, HTTP_RESPONSE);

        parser.data = this;

        settings.on_status = HTTPClientCallbacks::status_callback;
        settings.on_body   = HTTPClientCallbacks::body_callback;
        settings.on_headers_complete = HTTPClientCallbacks::headers_complete;
        settings.on_message_complete = HTTPClientCallbacks::message_complete;

        size_t         l   = 0;
        volatile err_t err = 0;

        if (url.port == 0) {
            url.port = 80;
        }

        // This will be used both for request string and as buffer for the response
        core::StringBuffer<HTTP_CLIENT_BUFFER_SIZE> buffer;

        buffer.appendFormat("%s /%s", method, url.path);

        if (url.query != 0) {
            buffer.appendFormat("?%s", url.query);
        }

        buffer.appendFormat(" HTTP/1.1\r\nHost: %s\r\n\r\n", url.host);

        err = conn.connect(url.host, url.port);

        if (err == ERR_OK) {
            err = conn.write(buffer, buffer.length(), &l, 5000);

            if (err == ERR_OK) {
                bool loop = true;

                do {
                    err = conn.read(buffer, buffer.size(), &l, 5000);
                    http_parser_execute(&parser, &settings, buffer, l);

                    if (err < 0) {
                        http_parser_execute(&parser, &settings, buffer, 0);
                        loop    = false;
                        success = parser.content_length == 0;
                    }
                } while (loop);
            }

            conn.disconnect();
        }

        success &= _status_code == 200; // We can only accept 200 as success

        return success;
    }
} // HTTPClient::request
}
}
