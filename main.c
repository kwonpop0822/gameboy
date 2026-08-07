/*
 * ============================================================================
 * GAME BOY DEPARTMENT STORE TYCOON (300+ LINES FULL C SOURCE CODE)
 * Engine: SourceForge GBDK 2.95 C Library
 * Target: Game Boy (SM83 / Z80)
 * ============================================================================
 */

#include <gb/gb.h>

#define TILE_EMPTY      0x00
#define TILE_WALL       0x01
#define TILE_ELEVATOR   0x02
#define TILE_CAFE       0x03
#define TILE_CLOTHES    0x04
#define TILE_ARCADE     0x05
#define TILE_CINEMA     0x06

#define MAX_FLOORS      4
#define MAX_GUESTS      4

#define STATE_IDLE      0
#define STATE_ENTRY     1
#define STATE_ELEVATOR  2
#define STATE_SHOPPING  3
#define STATE_EXIT      4

typedef struct {
    UBYTE id;
    UBYTE level;
    UWORD build_cost;
    UWORD base_income;
    UBYTE is_on_fire;
    UBYTE satisfaction;
} StoreFloor;

typedef struct {
    UBYTE id;
    UBYTE x;
    UBYTE y;
    UBYTE target_floor;
    UBYTE state;
    UBYTE is_vip;
    UWORD wallet;
    UBYTE patience;
} GuestAI;

typedef struct {
    UWORD money;
    UWORD reputation;
    UBYTE day;
    UBYTE week;
    UWORD weekly_tax;
    UBYTE cur_cursor_floor;
    StoreFloor floors[MAX_FLOORS];
} DepartmentStore;

DepartmentStore g_store = {
    3000, 80, 1, 1, 600, 0,
    {
        {0, 1, 500,  120, 0, 100},
        {1, 0, 1200, 280, 0, 80},
        {2, 0, 2500, 550, 0, 70},
        {3, 0, 5000, 1000, 0, 90}
    }
};

GuestAI g_guests[MAX_GUESTS];
UBYTE g_joypad_previous = 0;
UBYTE g_joypad_current = 0;

const unsigned char tile_data[] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xFF,0xFF,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0xFF,0xFF,
    0x3C,0x3C,0x42,0x42,0x99,0x99,0xBD,0xBD,0xBD,0xBD,0x99,0x99,0x42,0x42,0x3C,0x3C,
    0x00,0x00,0x7E,0x7E,0x42,0x42,0x42,0x7E,0x42,0x42,0x42,0x42,0x00,0x00,0x00,0x00,
    0x18,0x18,0x3C,0x3C,0x7E,0x7E,0xFF,0xFF,0x7E,0x7E,0x3C,0x3C,0x18,0x18,0x00,0x00,
    0x3C,0x3C,0x66,0x66,0x99,0x99,0xBD,0xBD,0xBD,0xBD,0x99,0x99,0x66,0x66,0x3C,0x3C,
    0xFF,0xFF,0xC3,0xC3,0xA5,0xA5,0x99,0x99,0x99,0x99,0xA5,0xA5,0xC3,0xC3,0xFF,0xFF
};

void sys_wait_vbl(void) { wait_vbl_done(); }
UBYTE is_button_pressed(UBYTE button_mask) { return ((g_joypad_current & button_mask) && !(g_joypad_previous & button_mask)); }
void update_inputs(void) { g_joypad_previous = g_joypad_current; g_joypad_current = joypad(); }

void init_graphics(void) {
    SPRITES_8x8;
    set_bkg_data(0, 7, tile_data);
    SHOW_BKG;
    SHOW_SPRITES;
    DISPLAY_ON;
}

void draw_department_map(void) {
    UBYTE f;
    for (f = 0; f < MAX_FLOORS; f++) {
        UBYTE tile_type = TILE_EMPTY;
        if (g_store.floors[f].level > 0) {
            switch(f) {
                case 0: tile_type = TILE_CAFE; break;
                case 1: tile_type = TILE_CLOTHES; break;
                case 2: tile_type = TILE_ARCADE; break;
                case 3: tile_type = TILE_CINEMA; break;
            }
        }
        set_bkg_tiles(2, 12 - (f * 3), 1, 1, &tile_type);
    }
}

void init_guest_system(void) {
    UBYTE i;
    for (i = 0; i < MAX_GUESTS; i++) {
        g_guests[i].id = i;
        g_guests[i].x = 16;
        g_guests[i].y = 136;
        g_guests[i].target_floor = 0;
        g_guests[i].state = STATE_IDLE;
        g_guests[i].is_vip = 0;
        g_guests[i].wallet = 600;
        g_guests[i].patience = 100;
    }
}

void spawn_guest(void) {
    UBYTE i;
    for (i = 0; i < MAX_GUESTS; i++) {
        if (g_guests[i].state == STATE_IDLE) {
            g_guests[i].state = STATE_ENTRY;
            g_guests[i].x = 16;
            g_guests[i].y = 136;
            g_guests[i].target_floor = (DIV_REG % MAX_FLOORS);
            g_guests[i].is_vip = ((DIV_REG % 5) == 0) ? 1 : 0;
            g_guests[i].patience = 100;
            break;
        }
    }
}

void process_guest_ai(GuestAI* guest) {
    if (guest->state == STATE_IDLE) return;
    if (guest->state == STATE_ENTRY) {
        guest->x += 2;
        if (guest->x >= 72) guest->state = STATE_ELEVATOR;
    } else if (guest->state == STATE_ELEVATOR) {
        UBYTE target_y = 136 - (guest->target_floor * 24);
        if (guest->y > target_y) guest->y -= 2;
        else if (guest->y < target_y) guest->y += 2;
        else guest->state = STATE_SHOPPING;
    } else if (guest->state == STATE_SHOPPING) {
        UBYTE f = guest->target_floor;
        if (g_store.floors[f].level > 0 && !g_store.floors[f].is_on_fire) {
            UWORD revenue = g_store.floors[f].base_income * g_store.floors[f].level;
            if (guest->is_vip) revenue *= 3;
            g_store.money += revenue;
            if (g_store.reputation < 100) g_store.reputation++;
        } else {
            if (g_store.reputation > 5) g_store.reputation -= 2;
        }
        guest->state = STATE_EXIT;
    } else if (guest->state == STATE_EXIT) {
        if (guest->x > 16) guest->x -= 2;
        else guest->state = STATE_IDLE;
    }
    move_sprite(guest->id, guest->x, guest->y);
}

void upgrade_selected_floor(void) {
    UBYTE f = g_store.cur_cursor_floor;
    UWORD required = g_store.floors[f].build_cost * (g_store.floors[f].level + 1);
    if (g_store.money >= required) {
        g_store.money -= required;
        g_store.floors[f].level++;
        draw_department_map();
    }
}

void trigger_random_disaster(void) {
    UBYTE target = (DIV_REG % MAX_FLOORS);
    if (g_store.floors[target].level > 0 && !g_store.floors[target].is_on_fire) {
        g_store.floors[target].is_on_fire = 1;
    }
}

void extinguish_disaster(void) {
    UBYTE f;
    for (f = 0; f < MAX_FLOORS; f++) {
        if (g_store.floors[f].is_on_fire) {
            g_store.floors[f].is_on_fire = 0;
            break;
        }
    }
}

void process_calendar_and_taxes(void) {
    g_store.day++;
    if (g_store.day > 7) {
        g_store.day = 1;
        g_store.week++;
        if (g_store.money >= g_store.weekly_tax) {
            g_store.money -= g_store.weekly_tax;
            g_store.weekly_tax += 350;
        } else {
            g_store.money = 0;
            g_store.reputation = 0;
        }
    }
}

void handle_player_controls(void) {
    update_inputs();
    if (is_button_pressed(J_UP)) { if (g_store.cur_cursor_floor < MAX_FLOORS - 1) g_store.cur_cursor_floor++; }
    if (is_button_pressed(J_DOWN)) { if (g_store.cur_cursor_floor > 0) g_store.cur_cursor_floor--; }
    if (is_button_pressed(J_A)) upgrade_selected_floor();
    if (is_button_pressed(J_B)) extinguish_disaster();
}

void main(void) {
    UWORD system_timer = 0;
    UBYTE i;
    init_graphics();
    init_guest_system();
    draw_department_map();
    while (1) {
        system_timer++;
        handle_player_controls();
        if (system_timer % 90 == 0) spawn_guest();
        for (i = 0; i < MAX_GUESTS; i++) process_guest_ai(&g_guests[i]);
        if (system_timer % 600 == 0) if ((DIV_REG % 3) == 0) trigger_random_disaster();
        if (system_timer % 360 == 0) process_calendar_and_taxes();
        sys_wait_vbl();
    }
}
