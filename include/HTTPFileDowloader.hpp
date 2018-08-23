/* COPYRIGHT (c) 2016-2018 Nova Labs SRL
 *
 * All rights reserved. All use of this software and documentation is
 * subject to the License Agreement located in the file LICENSE.
 */

#ifdef HAVE_PACKAGE_FATFS_WRAPPER

#include <core/networking/HTTPClient.hpp>
#include <core/fatfs_wrapper/fatfsWrapper.h>

#include <Module.hpp>

namespace core {
namespace networking {
bool
HTTPFileDownload(
    const char* url,
    const char* filename
)
{
    bool success = true;

    HTTPClient http_client;

    FIL file;

    memset(&file, 0, sizeof(FIL));

    http_client.onStatus([&](uint32_t status) {
                // If status 200 => open file
                if (status == 200) {
                    FRESULT err = wf_open(&file, filename, FA_WRITE | FA_CREATE_ALWAYS);
                    success &= (err == FR_OK || err == FR_EXIST);
                } else {
                    success = false;
                }

                return 0;
            });

    http_client.onBody([&](const char* data, std::size_t size) {
                // Write data
                if (success) {

                    std::size_t written = 0;
                    FRESULT err = FR_OK;

                    int s = size;

                    while((err == FR_OK) && (s > 512)) {
                        size_t tmp = 0;
                        err = wf_write(&file, data, 512, &tmp);
                        data += 512;
                        s -= 512;
                        written += tmp;
                    }

                    if((err == FR_OK) && (s > 0)) {
                        size_t tmp = 0;
                        err = wf_write(&file, data, s, &tmp);
                        written += tmp;
                    }

                    if ((err != FR_OK) || (written < size)) {
                        wf_close(&file);
                        // Delete the partial file
                        wf_unlink(filename);
                        success = false;
                    }
                }

                return 0;
            });

    http_client.onMessageComplete([&]() {
                if (success) {
                    wf_close(&file);
                }

                return 0;
            });

    success &= http_client.get(url);

    return success;
} // HTTPFileDownload
}
}
#else // ifdef HAVE_PACKAGE_FATFS_WRAPPER
#error "FatFS is not available"
#endif // ifdef HAVE_PACKAGE_FATFS_WRAPPER
