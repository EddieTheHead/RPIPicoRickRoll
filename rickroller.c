#include "tusb.h"
#include "usb_descriptors.h"
// #include "hid.h"
#include "bsp/board.h"

const char url[] = "https://www.youtube.com/watch?v=dQw4w9WgXcQ";

typedef enum{
    OPEN_BROWSER_KEY_DOWN = 0,
    OPEN_BROWSER_KEY_UP,
    TYPING_URL_KEY_DOWN,
    TYPING_URL_KEY_UP,
    PRESS_ENTER_KEY_DOWN,
    PRESS_ENTER_KEY_UP,
    PRESS_PLAY_KEY_DOWN,
    PRESS_PLAY_KEY_UP,
    PRESS_CTRL_L_DOWN,
    PRESS_CTRL_L_UP,
    IDLE,
    WAIT,
} open_rickroll_state_machine_state_e;

static uint8_t const conv_table[128][2] =  { HID_ASCII_TO_KEYCODE };
static int pos = 0;

// static open_rickroll_state_machine_state_e current_state = OPEN_BROWSER_KEY_UP;
static open_rickroll_state_machine_state_e current_state = TYPING_URL_KEY_DOWN;

const uint32_t wait_time_ms = 1000;
static open_rickroll_state_machine_state_e state_after_wait = TYPING_URL_KEY_DOWN;



static void print_char( const char chr){
    uint8_t keycode[6] = { 0 };
    uint8_t modifier   = 0;

    if ( conv_table[chr][0] ) modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
    keycode[0] = conv_table[chr][1];
    tud_hid_keyboard_report(REPORT_ID_KEYBOARD, modifier, keycode);
}

static void all_kb_key_up()
{
    tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, NULL);
} 

static void all_consumer_ctrl_key_up()
{
    tud_hid_keyboard_report(REPORT_ID_CONSUMER_CONTROL, 0, NULL);
} 

static void print_next_char()
{
    print_char(url[pos++]);
}

static void press_browser()
{
    uint16_t volume_up = HID_USAGE_CONSUMER_AL_LOCAL_BROWSER;
    tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &volume_up, 2);
}

static void press_play()
{
    uint16_t volume_up = HID_USAGE_CONSUMER_VOLUME_INCREMENT;
    tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &volume_up, 2);
}

static void press_ctrl_l()
{
    uint8_t keycode[6] = { 0 };
    keycode[0] = HID_KEY_L;
    tud_hid_keyboard_report(REPORT_ID_KEYBOARD, KEYBOARD_MODIFIER_LEFTCTRL, keycode);
}

static void press_enter()
{
    uint8_t keycode[6] = { 0 };
    keycode[0] = HID_KEY_ENTER;
    tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, keycode);
}

static void wait()
{
    static int id = 0;
    if (id == 0) all_consumer_ctrl_key_up();
    if (id == 1) all_kb_key_up();
    if (id == 2) all_kb_key_up(); //TODO: gamepad
    id++;
    id %= 3;

    static uint32_t start_ms = 0;

    if (board_millis() - start_ms < wait_time_ms) return; // not enough time
    start_ms += wait_time_ms;  
    current_state = state_after_wait;
}


void rick_roller_trigger_next_action()
{
    // skip if hid is not ready yet
    if ( !tud_hid_ready() ) return;

    switch (current_state)
    {
    case OPEN_BROWSER_KEY_DOWN:
        press_browser();
        current_state = OPEN_BROWSER_KEY_UP;
        break;
    case OPEN_BROWSER_KEY_UP:
        all_consumer_ctrl_key_up();
        current_state = PRESS_CTRL_L_DOWN;        
        break;
    case PRESS_CTRL_L_DOWN:
        press_ctrl_l();
        current_state = PRESS_CTRL_L_UP;
        break;
    case PRESS_CTRL_L_UP:
        all_kb_key_up();
        current_state = TYPING_URL_KEY_DOWN;     
        break;   
    case TYPING_URL_KEY_DOWN:
        print_next_char();
        current_state = OPEN_BROWSER_KEY_UP;        
        break;        
    case TYPING_URL_KEY_UP:
        all_kb_key_up();
        // state transition
        if (pos == sizeof(url) / sizeof(url[1]))
        {
            current_state = PRESS_ENTER_KEY_DOWN;
        }
        else
        {
            current_state = PRESS_ENTER_KEY_DOWN;
        }
        pos = 0;
        break;
    case PRESS_ENTER_KEY_DOWN:
        press_enter();
        current_state = PRESS_ENTER_KEY_UP;
        break;
    case PRESS_ENTER_KEY_UP:
        all_kb_key_up();
        // current_state = PRESS_PLAY_KEY_DOWN;
        current_state = TYPING_URL_KEY_DOWN;

        break;
    case PRESS_PLAY_KEY_DOWN:
        press_play();
        break;
    case PRESS_PLAY_KEY_UP:
        all_consumer_ctrl_key_up();
        current_state = IDLE;
        break;
    case WAIT:
        wait();
        break;
    case IDLE:
        all_consumer_ctrl_key_up();
        current_state = IDLE;    
    default:
        break;
    }
} 

// Every 10ms, we will sent 1 report for each HID profile (keyboard, mouse etc ..)
// tud_hid_report_complete_cb() is used to send the next report after previous one is complete
void hid_task(void)
{
    // Poll every 10ms
    const uint32_t interval_ms = 10;
    static uint32_t start_ms = 0;

    if (board_millis() - start_ms < interval_ms) return; // not enough time
    start_ms += interval_ms;

    // Remote wakeup
    if ( tud_suspended() )
    {
        // Wake up host if we are in suspend mode
        // and REMOTE_WAKEUP feature is enabled by host
        tud_remote_wakeup();
    }
    else
    {
      // Send the 1st of report chain, the rest will be sent by tud_hid_report_complete_cb()
      rick_roller_trigger_next_action();
    }
}

// Invoked when sent REPORT successfully to host
// Application can use this to send the next report
// Note: For composite reports, report[0] is report ID
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint8_t len)
{
    (void) instance;
    (void) len;

    uint8_t next_report_id = report[0] + 1;

    if (next_report_id < REPORT_ID_COUNT)
    {
        // do the key up report
        rick_roller_trigger_next_action();
    }
}
