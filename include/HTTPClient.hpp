/* COPYRIGHT (c) 2016-2018 Nova Labs SRL
 *
 * All rights reserved. All use of this software and documentation is
 * subject to the License Agreement located in the file LICENSE.
 */

#pragma once

#include <core/StringBuffer.hpp>

#include <functional>

namespace core {
namespace networking {
class HTTPClient
{
    friend class HTTPClientCallbacks;

public:
    using DataCallback   = bool(const char* data, size_t len);
    using StatusCallback = bool(uint32_t status);
    using EventCallback  = bool(void);

public:
    HTTPClient() : _status_code(0), _on_body() {}

    bool
    get(
        const char* url
    )
    {
        return request("GET", url);
    }

    bool
    post(
        const char* url
    )
    {
        return request("POST", url);
    }

    void
    onStatus(
        std::function<StatusCallback>&& f
    )
    {
        _on_status = std::move(f);
    }

    void
    onStatus(
        auto&& f
    )
    {
        _on_status = std::move(f);
    }

    void
    onBody(
        std::function<DataCallback>&& f
    )
    {
        _on_body = std::move(f);
    }

    void
    onBody(
        auto&& f
    )
    {
        _on_body = std::move(f);
    }

    void
    onMessageComplete(
        std::function<EventCallback>&& f
    )
    {
        _on_message_complete = std::move(f);
    }

    void
    onMessageComplete(
        auto&& f
    )
    {
        _on_message_complete = std::move(f);
    }

private:
    bool
    request(
        const char* method,
        const char* url
    );


    uint32_t _status_code;

    std::function<StatusCallback> _on_status;
    std::function<DataCallback>   _on_body;
    std::function<EventCallback>  _on_message_complete;
};
}
}
