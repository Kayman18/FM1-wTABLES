/* Exercise the real app, framebuffer, input queues and audio callback through
 * the existing host platform. No replacement splash/timer implementation. */
#define main fm1_script_main
#include "../fm1/base/host/plat_host.c"
#undef main
#include "pw_table_layout.h"

#define CHECK(condition, message) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL startup_rc1:%d: %s\n", __LINE__, message); \
        return 1; \
    } \
} while (0)

static void fresh_host(void)
{
    app_host_reboot();
    now_ms = held_btn = held_keys = ui_due = 0;
    audio_due_ms = 0;
    mi_r = mi_w = 0;
    memset(enc_acc, 0, sizeof enc_acc);
    peak_out = 0;
    boot();
}

static void incoming(unsigned port, unsigned status, unsigned note, unsigned value)
{
    min_q[mi_w % MQ] = (status << 8) | (note << 16) | (value << 24);
    min_port[mi_w++ % MQ] = port;
}

static unsigned progress_pixels(void)
{
    unsigned count = 0;
    const uint16_t cyan = swap16(RGB(0,229,235));
    for (unsigned y = WT_SPLASH_PROGRESS_Y + 2; y < WT_SPLASH_PROGRESS_Y + WT_SPLASH_PROGRESS_H - 2; ++y)
        for (unsigned x = WT_SPLASH_PROGRESS_X + 2; x < WT_SPLASH_PROGRESS_X + WT_SPLASH_PROGRESS_W - 2; ++x)
            count += fb[y * 240 + x] == cyan;
    return count;
}

static int timing_and_input(void)
{
    fresh_host();
    CHECK(splash_active && splash_started, "host boot must show the splash immediately");
    CHECK(splash_start_ticks == 0, "deadline starts after first visible frame");
    CHECK(progress_pixels() == 0, "progress starts empty");
    held_btn = (1u << B_PLAY) | (1u << B_FX);
    held_keys = 1u;
    enc_acc[EN_K1] = 7;
    incoming(PORT_USB, 0x90, 60, 100);
    incoming(PORT_TRS, 0x90, 64, 100);
    incoming(PORT_BLE, 0x90, 67, 100);
    incoming(PORT_USB, 0xb0, 64, 127);
    run_ms(1000);
    CHECK(splash_active, "splash still active at one second");
    CHECK(pw_tables_ready() && !pw_tables_busy() && pw_tables_bank() == 0,
          "wavetable preparation must finish while the splash is showing");
    CHECK(peak_out == 0 && core_held_count() == 0, "startup keys and MIDI must remain silent");
    CHECK(!st.playing && page == 0 && st.note == 48, "startup controls must not alter the patch/page");
    CHECK(enc_acc[EN_K1] == 0 && mi_r == mi_w, "startup inputs must be drained, not deferred");
    unsigned first_progress = progress_pixels();
    run_ms(2000);
    unsigned half_progress = progress_pixels();
    CHECK(first_progress > 0 && half_progress > first_progress, "loading bar must advance");
    CHECK(half_progress >= 75u * 6u && half_progress <= 83u * 6u, "bar is near halfway at three seconds");
    run_ms(2999);
    CHECK(now_ms == 5999 && splash_active, "never dismiss before all six seconds have elapsed");
    incoming(PORT_USB, 0x90, 72, 127); /* last pre-deadline queue entry */
    enc_acc[EN_K2] = 99;
    run_ms(1);
    CHECK(now_ms == 6000 && !splash_active, "dismiss at six seconds even between scheduled UI frames");
    CHECK(splash_start_ticks == 0 && !st.playing && page == 0 && st.position == 0,
          "finishing must not replay input or restart the clock");
    CHECK(mi_r == mi_w && enc_acc[EN_K2] == 0, "final pre-ready MIDI/encoder entries must be consumed");
    CHECK(splash_blocked_keys == 1u, "a key held across the deadline stays blocked until released");
    unsigned completed_blits = blits;
    run_ms(1000);
    CHECK(core_held_count() == 0 && peak_out == 0, "held startup controls must not sound after readiness");
    CHECK(!st.playing && page == 0, "held startup buttons must not activate on readiness");
    CHECK(blits == completed_blits, "startup progress redraws must stop after readiness");
    held_btn = held_keys = 0;
    run_ms(40);
    CHECK(splash_blocked_keys == 0, "releasing a held startup key unblocks it");
    held_keys = 1u;
    run_ms(100);
    CHECK(core_held_count() == 1 && peak_out > 0, "a fresh post-startup key press plays normally");
    held_keys = 0;
    run_ms(20);
    incoming(PORT_USB, 0x90, 60, 100);
    run_ms(20);
    CHECK(core_note_held(PORT_USB, 0, 60), "post-startup MIDI note-on works");
    incoming(PORT_USB, 0x80, 60, 0);
    run_ms(20);
    CHECK(!core_note_held(PORT_USB, 0, 60), "post-startup MIDI note-off is not lost");
    puts("PASS: exact 6000 ms deadline, progressive loading, bank preparation, silent/drained startup inputs, held-key release, and normal post-startup audio");
    return 0;
}

static int early_draw_and_wrap(void)
{
    fresh_host();
    app_host_reboot();
    now_ms = 100;
    app_splash();              /* hardware calls this before app_init */
    now_ms += 437;
    app_init();
    app_ui_init();
    CHECK(splash_start_ticks == 100 && splash_elapsed_ms() == 437,
          "engine/UI initialization must preserve the first-draw timestamp");
    now_ms = 6099;
    app_frame();
    CHECK(splash_active, "early platform startup counts toward the six seconds");
    now_ms = 6100;
    app_input_only();
    CHECK(!splash_active, "early-hook deadline expires after exactly six visible seconds");
    const uint32_t start = 0xfff00000u, ticks_per_ms = 24000u;
    CHECK(splash_elapsed_ticks(start + 5999u * ticks_per_ms, start, ticks_per_ms) == 5999u,
          "24 MHz device clock works across counter wrap before deadline");
    CHECK(splash_elapsed_ticks(start + 6000u * ticks_per_ms, start, ticks_per_ms) == 6000u,
          "24 MHz device clock works across counter wrap at deadline");
    CHECK(splash_elapsed_ticks(start + 6001u * ticks_per_ms, start, ticks_per_ms) == 6000u,
          "elapsed progress saturates at the deadline");
    puts("PASS: early hardware splash timestamp survives initialization; 24 MHz timer wrap and deadline saturation");
    return 0;
}

static int recovery_and_reentry(void)
{
    fresh_host();
    run_ms(100);
    held_btn = (1u << B_OCTDN) | (1u << B_OCTUP);
    run_ms(160);
    CHECK(splash_active && update_state == 1, "bootloader hold timer remains active during splash");
    CHECK(fb[0] != 0, "bootloader overlay must draw above the splash");
    held_btn = 0;
    run_ms(80);
    CHECK(update_state == 2 && fb[0] != 0, "bootloader cancellation stays visible above the splash");
    run_ms(800);
    CHECK(update_state == 0 && splash_active && fb[0] == 0, "remaining splash resumes after cancellation");
    CHECK(splash_start_ticks == 0, "recovery cancellation must not restart six-second timer");
    run_ms(5999u - now_ms);
    CHECK(splash_active, "recovery does not dismiss startup prematurely");
    run_ms(1);
    CHECK(!splash_active, "recovery does not extend the startup deadline");
    app_ui_init();
    run_ms(20);
    CHECK(!splash_active && splash_start_ticks == 0, "post-OTA UI initialization must not replay the splash");
    fresh_host();
    run_ms(1000);
    app_ui_init();             /* an aborted OTA can return during startup */
    run_ms(20);
    CHECK(!splash_active && splash_start_ticks == 0, "early OTA return dismisses rather than restarts the splash");
    fresh_host();
    run_ms(100);
    held_btn = (1u << B_OCTDN) | (1u << B_OCTUP);
    run_ms(5001);
    CHECK(update_state == 3 && splash_active && fb[0] != 0,
          "a full five-second updater hold reaches handoff while splash is active");
    CHECK(peak_out == 0, "updater handoff during startup remains silent");
    puts("PASS: bootloader priority, cancellation, five-second handoff, and no splash replay after OTA UI reinitialization");
    return 0;
}

int main(void)
{
    data_init();
    FILE *f = fopen("data/factory-mips.pwlc", "rb");
    CHECK(f != NULL, "matching factory wavetable fixture must be present");
    CHECK(fread(data_area + PW_TABLE_FLASH_START - PLAT_DATA_LO, 1, PW_TABLE_BYTES, f) == PW_TABLE_BYTES,
          "factory wavetable fixture must be complete");
    CHECK(fgetc(f) == EOF, "factory wavetable fixture must have the expected size");
    fclose(f);
    if (timing_and_input() || early_draw_and_wrap() || recovery_and_reentry()) return 1;
    puts("PASS: RC1 splash integration regression");
    return 0;
}
