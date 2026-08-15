#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <uart.h>
#include <csr.h>
#include <peripherals/vga_gfx.h>
#include <peripherals/vga_console.h>
#include <gamepad.h>

#include "mtwister.h"

#define WIDTH 256
#define HEIGHT 192

#define BIRD_HEIGHT 12
#define BIRD_WIDTH 16
#define BIRD_MIN_X 24
#define BIRD_MAX_X (BIRD_MIN_X + BIRD_WIDTH - 1)

uint32_t bird_sprite[BIRD_HEIGHT] = {
    0b00000000000000010000000000000000,
    0b00000000000101010101000000000000,
    0b00000001010101010101010000000000,
    0b00000101010101010111111100000001,
    0b00010101010101011101010101000101,
    0b01010001010101010101010101010101,
    0b01010101010101010101010101010101,
    0b01010101010101010101010101010101,
    0b00010101010101010101010101000101,
    0b00000101010101010101010100000001,
    0b00000001010101010101010000000000,
    0b00000000000101010101000000000000
};

#define WALL_WIDTH 16

static MTRand rand;

#define Y_SHIFT 8
static int height;
static int vel;

#define X_SHIFT 8
static int scroll_speed = 256;

typedef struct {
    int x;
    int gap_y;
    int gap_height;
} wall_t;

#define MAX_WALLS 4
wall_t walls[MAX_WALLS];
static int first_wall_idx;
static int num_walls;
static int score;

void make_wall() {
    if (num_walls == MAX_WALLS) return;

    int wall_idx = (first_wall_idx + num_walls) & (MAX_WALLS - 1);
    wall_t* wall = &walls[wall_idx];
    wall->x = WIDTH << X_SHIFT;
    wall->gap_height = 40 + (genRand32(&rand) % 40);
    wall->gap_y = 2 + (genRand32(&rand) % (HEIGHT - 4 - wall->gap_height));

    ++num_walls;
}

void update_pos(bool pressed) {
    if (pressed) {
        if (vel < -2 << Y_SHIFT) vel = -2 << Y_SHIFT;
        else vel -= 20;
    }
    else {
        vel += 20;
    }

    height += vel;
}

bool update_walls() {
    int row = height >> Y_SHIFT;

    for (int idx = first_wall_idx, end = (first_wall_idx + num_walls) & (MAX_WALLS - 1); idx != end; idx = (idx + 1) & (MAX_WALLS - 1)) {
        wall_t* wall = &walls[idx];
        wall->x -= scroll_speed;

        int wall_x = wall->x >> X_SHIFT;

        if (wall_x < 0) {
            first_wall_idx = (first_wall_idx + 1) & (MAX_WALLS - 1);
            num_walls--;
        }

        if (wall_x + WALL_WIDTH >= BIRD_MIN_X && wall_x <= BIRD_MAX_X) {
            if (row < wall->gap_y || row + BIRD_HEIGHT > wall->gap_y + wall->gap_height) {
                return false;
            }
        }
    }

    return true;
}

void display() {
    // TODO: Expect this is too slow, could change to just patching the front buffer in vsync

    // Clear
    vga_gfx_clear(0, vga_gfx_back_buffer);

    // Move and display
    int bird_y = height >> Y_SHIFT;

    // TODO: Make sprite and change to blit
    //vga_gfx_box(1, vga_gfx_back_buffer, BIRD_MIN_X, bird_y, BIRD_WIDTH, BIRD_HEIGHT);
    vga_gfx_simple_blit(vga_gfx_back_buffer, bird_sprite, BIRD_MIN_X, bird_y, BIRD_WIDTH, BIRD_HEIGHT);

    #if 1
    for (int idx = first_wall_idx, end = (first_wall_idx + num_walls) & (MAX_WALLS - 1); idx != end; idx = (idx + 1) & (MAX_WALLS - 1)) {
        wall_t* wall = &walls[idx];
        int wall_x = wall->x >> X_SHIFT;
        vga_gfx_box(2, vga_gfx_back_buffer, wall_x, 0, WALL_WIDTH, wall->gap_y);
        vga_gfx_box(2, vga_gfx_back_buffer, wall_x, wall->gap_y + wall->gap_height, WALL_WIDTH, HEIGHT - 1 - (wall->gap_y + wall->gap_height));
    }
    #endif

    vga_gfx_flip();
}

int main(void) {
    seedRand(&rand, 1234);

    gamepad_enable();

    vga_gfx_set_colour1(0x0c);
    vga_gfx_set_colour2(0x03);
    vga_gfx_set_colour3(0x04);

    vga_console_reset();
    vga_console_set_bgcolour(0x10);
    vga_console_set_colour1(0x0c);
    vga_console_set_text(1, "Game Over", false);

    while (true) {
        vga_gfx_clear(0, vga_gfx_front_buffer);
        vga_gfx_start();

        // Sync
        delay_us(2000*1000);

        height = 100 << Y_SHIFT; vel = 0;

        num_walls = 0;
        score = 0;
        make_wall();

        display();
        delay_us(500*1000);

        while (true) {
            update_pos(gamepad_1_up());

            if (height < 0 || height > (HEIGHT - BIRD_HEIGHT) << Y_SHIFT || !update_walls()) {
                break;
            }

            if (walls[first_wall_idx].x < (45 << X_SHIFT) && num_walls == 1) {
                ++score;
                make_wall();
            }

            display();
        }

        // Switch to VGA console to display game over message
        vga_console_start();

        char buf[10];
        if (score < 100)
            snprintf(buf, sizeof(buf), "Score: %d ", score);
        else
            snprintf(buf, sizeof(buf), "Score:%d", score);
        vga_console_set_text(21, buf, false);
        
        delay_us(7000*1000);
    }
}
