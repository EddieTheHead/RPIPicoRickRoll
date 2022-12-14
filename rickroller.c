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
    INITIAL_WAIT,
    WAIT_FOR_BROWSER,
    WAIT_FOR_YOUTUBE,
} open_rickroll_state_machine_state_e;

static uint8_t const conv_table[128][2] =  { HID_ASCII_TO_KEYCODE };
static int pos = 0;

// static open_rickroll_state_machine_state_e current_state = OPEN_BROWSER_KEY_UP;
static open_rickroll_state_machine_state_e current_state = INITIAL_WAIT;



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
    // uint16_t cmd = 0xf0;
    // tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &cmd, 2);
    uint8_t keycode[6] = { 0 };
    keycode[0] = 0xf0;
    tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, keycode);
}

static void press_play()
{
    // uint16_t cmd = HID_USAGE_CONSUMER_PLAY_PAUSE;
    // tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &cmd, 2);
    print_char(' ');
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

static void initial_wait()
{
    const uint32_t wait_time_ms = 2000;
    static int id = 0;
    if (id == 0) all_consumer_ctrl_key_up();
    if (id == 1) all_kb_key_up();
    if (id == 2) all_kb_key_up(); //TODO: gamepad
    id++;
    id %= 3;

    static uint32_t start_ms = 0;

    if (board_millis() - start_ms < wait_time_ms) return; // not enough time
    start_ms += wait_time_ms;  
    current_state = OPEN_BROWSER_KEY_DOWN;
}

static void wait_for_browser()
{
    const uint32_t wait_time_ms = 3000;
    static int id = 0;
    if (id == 0) all_consumer_ctrl_key_up();
    if (id == 1) all_kb_key_up();
    if (id == 2) all_kb_key_up(); //TODO: gamepad
    id++;
    id %= 3;

    static uint32_t start_ms = 0;

    if (board_millis() - start_ms < wait_time_ms) return; // not enough time
    start_ms += wait_time_ms;  
    current_state = PRESS_CTRL_L_DOWN;
}

static void wait_for_youtube()
{
    const uint32_t wait_time_ms = 5000;
    static int id = 0;
    if (id == 0) all_consumer_ctrl_key_up();
    if (id == 1) all_kb_key_up();
    if (id == 2) all_kb_key_up(); //TODO: gamepad
    id++;
    id %= 3;

    static uint32_t start_ms = 0;

    if (board_millis() - start_ms < wait_time_ms) return; // not enough time
    start_ms += wait_time_ms;  
    current_state = PRESS_PLAY_KEY_DOWN;
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
        // all_consumer_ctrl_key_up();
        all_kb_key_up();
        current_state = WAIT_FOR_BROWSER;        
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
        current_state = TYPING_URL_KEY_UP;        
        break;        
    case TYPING_URL_KEY_UP:
        all_kb_key_up();
        // state transition
        if (pos == sizeof(url) / sizeof(url[1]))
        {
            current_state = PRESS_ENTER_KEY_DOWN;
            pos = 0;
        }
        else
        {
            current_state = TYPING_URL_KEY_DOWN;
        }
        break;
    case PRESS_ENTER_KEY_DOWN:
        press_enter();
        current_state = PRESS_ENTER_KEY_UP;
        break;
    case PRESS_ENTER_KEY_UP:
        all_kb_key_up();
        current_state = WAIT_FOR_YOUTUBE;
        break;
    case PRESS_PLAY_KEY_DOWN:
        press_play();
        current_state = PRESS_PLAY_KEY_UP;
        break;
    case PRESS_PLAY_KEY_UP:
        // all_consumer_ctrl_key_up();
        all_kb_key_up();
        current_state = IDLE;
        break;
    case INITIAL_WAIT:
        initial_wait();
        break;
    case WAIT_FOR_BROWSER:
        wait_for_browser();
        break;       
    case WAIT_FOR_YOUTUBE:
        wait_for_youtube();
        break;         
    case IDLE:
        all_consumer_ctrl_key_up();
        current_state = IDLE;    
    default:
        break;
    }
} 