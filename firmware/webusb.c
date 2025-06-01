#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "tusb.h"
#include "src/ed25519.h"
#include "src/ge.h"
#include "src/sc.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#define FLASH_TARGET_OFFSET (4*1024*1024-4096)
unsigned char public_key[32], private_key[64], seed[32];
unsigned char signature[64];
static void flash_write_keys(const uint8_t *private,const uint8_t *public) {
    // Disable interrupts to protect flash operations
    uint32_t ints = save_and_disable_interrupts();
    // Erase the entire last sector before writing
    flash_range_erase(FLASH_TARGET_OFFSET, 4096);
    // Program the private key data (must be <= sector size)
    flash_range_program(FLASH_TARGET_OFFSET, private, 64);
    // Program the public key data
    flash_range_program(FLASH_TARGET_OFFSET+64, public, 32);

    // Restore interrupts
    restore_interrupts(ints);
}
static void flash_read_key(uint8_t *data, size_t size, uint8_t type) {
    // 1 for private key, 2 for public key
    const uint8_t *flash_ptr;
    if(type==1){
        flash_ptr = (const uint8_t *)(XIP_BASE + FLASH_TARGET_OFFSET);
    }else if(type==2){
        flash_ptr = (const uint8_t *)(XIP_BASE + FLASH_TARGET_OFFSET+64);
    }
    memcpy(data, flash_ptr, size);
}
static bool keys_exist_in_flash() {
    uint8_t buf[96];
    flash_read_key(buf, 96, 1);
    for (int i = 0; i < 96; i++) {
        if (buf[i] != 0xFF) return true;
    }
    return false;
}
static void load_or_generate_keypair() {
    if (keys_exist_in_flash()) {
        flash_read_key(private_key, 64,1);
        flash_read_key(public_key, 32,2);
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
    } else{
        const uint8_t *msg = buf;
        size_t msg_len = len;
        ed25519_sign(signature, msg, msg_len, public_key, private_key);
        tud_vendor_write(signature, 64);
        tud_vendor_flush();
    }
}
void tud_vendor_rx_cb(uint8_t itf, uint8_t const* buffer, uint16_t bufsize) {
  (void) itf;

  process_webusb_request(buffer,bufsize);

  // if using RX buffered is enabled, we need to flush the buffer to make room for new data
  #if CFG_TUD_VENDOR_RX_BUFSIZE > 0
  tud_vendor_read_flush();
  #endif
}
int main()
{
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
