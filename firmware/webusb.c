#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "tusb.h"
#include "src/ed25519.h"
#include "src/ge.h"
#include "src/sc.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "usb_descriptors.h"

#define FLASH_TARGET_OFFSET (4*1024*1024-4096)
unsigned char public_key[32], private_key[64], seed[32];

#define URL  "localhost:443/index.html"

const tusb_desc_webusb_url_t desc_url = {
        .bLength         = 3 + sizeof(URL) - 1,
        .bDescriptorType = 3, // WEBUSB URL type
        .bScheme         = 1, // 0: http, 1: https
        .url             = URL
};


static void flash_write_keys(const uint8_t *private, const uint8_t *public) {
    uint8_t flash_buffer[256] = {0};
    memcpy(flash_buffer ,"\xDE\xAD\xBE\xEF", 4);
    memcpy(flash_buffer + 4,private,64);
    memcpy(flash_buffer + 68,public,32);
    // Disable interrupts to protect flash operations
    uint32_t ints = save_and_disable_interrupts();
    // Erase the entire last sector before writing
    flash_range_erase(FLASH_TARGET_OFFSET, 4096);
    // Program the private and public key data (must be <= sector size)
    flash_range_program(FLASH_TARGET_OFFSET, flash_buffer, 256);
    // Restore interrupts
    restore_interrupts(ints);
}

static void flash_read_key(uint8_t *data, size_t size, uint8_t type) {
    // 1 for private key, 2 for public key
    const uint8_t *flash_ptr;
    if (type == 1) {
        flash_ptr = (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET + 4);
    } else if (type == 2) {
        flash_ptr = (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET + 68);
    }
    memcpy(data, flash_ptr, size);
}

static bool keys_exist_in_flash() {
    uint8_t buf[4];
    const uint8_t *flash_ptr = (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET);
    memcpy(buf,flash_ptr,4);
    if(memcmp(buf,"\xDE\xAD\xBE\xEF",4)==0){
        return true;
    }
    return false;
}

static void load_or_generate_keypair() {
    if (keys_exist_in_flash()) {
        flash_read_key(private_key, 64, 1);
        flash_read_key(public_key, 32, 2);
    } else {
        ed25519_create_seed(seed);
        ed25519_create_keypair(public_key, private_key, seed);
        flash_write_keys(private_key, public_key);
    }
}

void process_webusb_request(const uint8_t *buf, uint16_t len) {
    if (memcmp(buf, "PAIR", 4) == 0) {
        // Send public key to host
        load_or_generate_keypair();
        tud_vendor_write(public_key, 32);
        tud_vendor_flush();
    } else {
        if(keys_exist_in_flash()){
            unsigned char signature[64];
            uint8_t msg[len];
            memcpy(msg,buf,len);
            size_t msg_len = len;
            flash_read_key(private_key, 64, 1);
            flash_read_key(public_key, 32, 2);
            ed25519_sign(signature, msg, msg_len, public_key, private_key);
            tud_vendor_write(signature, 32);
            tud_vendor_flush();
            tud_vendor_write(signature+32, 32);
            tud_vendor_flush();
        }else {
            tud_vendor_write("1234", 5);
            tud_vendor_flush();
        }
    }
}

//--------------------------------------------------------------------+
// WebUSB use vendor class
//--------------------------------------------------------------------+

// Invoked when a control transfer occurred on an interface of this class
// Driver response accordingly to the request and the transfer stage (setup/data/ack)
// return false to stall control endpoint (e.g unsupported request)
bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) {
    // nothing to with DATA & ACK stage
    if (stage != CONTROL_STAGE_SETUP) return true;

    switch (request->bmRequestType_bit.type) {
        case TUSB_REQ_TYPE_VENDOR:
            switch (request->bRequest) {
                case VENDOR_REQUEST_WEBUSB:
                    // match vendor request in BOS descriptor
                    // Get landing page url
                    return tud_control_xfer(rhport, request, (void *) (uintptr_t) &desc_url, desc_url.bLength);

                case VENDOR_REQUEST_MICROSOFT:
                    if (request->wIndex == 7) {
                        // Get Microsoft OS 2.0 compatible descriptor
                        uint16_t total_len;
                        memcpy(&total_len, desc_ms_os_20 + 8, 2);

                        return tud_control_xfer(rhport, request, (void *) (uintptr_t) desc_ms_os_20, total_len);
                    } else {
                        return false;
                    }

                default:
                    break;
            }
            break;

        case TUSB_REQ_TYPE_CLASS:
            if (request->bRequest == 0x22) {
                // Webserial simulate the CDC_REQUEST_SET_CONTROL_LINE_STATE (0x22) to connect and disconnect.
                //   web_serial_connected = (request->wValue != 0);

                // Always lit LED if connected
//                if (web_serial_connected) {
//                    board_led_write(true);
//                    blink_interval_ms = BLINK_ALWAYS_ON;
//
//                    tud_vendor_write_str("\r\nWebUSB interface connected\r\n");
//                    tud_vendor_write_flush();
//                } else {
//                    blink_interval_ms = BLINK_MOUNTED;
//                }

                // response with status OK
                return tud_control_status(rhport, request);
            }
            break;

        default:
            break;
    }

    // stall unknown request
    return false;
}


void tud_vendor_rx_cb(uint8_t itf, uint8_t const *buffer, uint16_t bufsize) {
  (void) itf;

  process_webusb_request(buffer, bufsize);

  // if using RX buffered is enabled, we need to flush the buffer to make room for new data
  #if CFG_TUD_VENDOR_RX_BUFSIZE > 0
  tud_vendor_read_flush();
  #endif
}

int main() {
    stdio_init_all();
    tusb_rhport_init_t dev_init = {
    .role = TUSB_ROLE_DEVICE,
    .speed = TUSB_SPEED_AUTO
    };
    tusb_init(BOARD_TUD_RHPORT, &dev_init);
    
    
    while (true) {
        tud_task();
    }
}
