#ifndef RICKROLLER_H
#define RICKROLLER_H

#include <stdint.h>

void hid_task(void);
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint8_t len);

#endif  /*RICKROLLER_H*/