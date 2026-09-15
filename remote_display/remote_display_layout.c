#include "remote_display_layout.h"

#include <string.h>

#include "remote_font.h"
#include "remote_qr.h"

/* The KD8 proposal. The column beside the code is 136 pixels, eleven cells
 * at scale two, which is why every hint line below is at most eleven
 * characters and every error code the protocol defines fits (they are
 * bounded at eleven for the Flipper's band, and the same bound serves here). */
#define HEADER_HEIGHT 36
#define CODE_REGION_SIDE 264
#define COLUMN_X CODE_REGION_SIDE
#define COLUMN_WIDTH (REMOTE_BITMAP_WIDTH - COLUMN_X)
#define PAGE_REGION_Y HEADER_HEIGHT
#define PAGE_REGION_HEIGHT 44
#define HINT_REGION_Y (PAGE_REGION_Y + PAGE_REGION_HEIGHT)
#define HINT_REGION_HEIGHT 60
#define DELIVERED_REGION_Y (HINT_REGION_Y + HINT_REGION_HEIGHT)
#define DELIVERED_REGION_HEIGHT 80
#define ERROR_REGION_HEIGHT 40
#define ERROR_REGION_Y (REMOTE_BITMAP_HEIGHT - ERROR_REGION_HEIGHT)

#define TITLE_SCALE 3
#define COLUMN_SCALE 2
#define COUNT_SCALE 5
#define COLUMN_LINE_HEIGHT (REMOTE_FONT_GLYPH_HEIGHT * COLUMN_SCALE + 4)
#define COLUMN_TEXT_X (COLUMN_X + 6)
/* Cells that fit in the column at COLUMN_SCALE, less nothing: 136 / 12. */
#define COLUMN_CELLS (COLUMN_WIDTH / (REMOTE_FONT_ADVANCE * COLUMN_SCALE))
#define MARGIN 8

/* Enough for the longest label written here plus a terminator; anything
 * longer is cut to the cells its region holds. */
#define LABEL_CAPACITY 40

static const RemoteLayoutRectangle regions[RemoteLayoutRegionCount] = {
    [RemoteLayoutRegionHeader] = {0, 0, REMOTE_BITMAP_WIDTH, HEADER_HEIGHT},
    [RemoteLayoutRegionCode] = {0, HEADER_HEIGHT, CODE_REGION_SIDE, CODE_REGION_SIDE},
    [RemoteLayoutRegionPage] = {COLUMN_X, PAGE_REGION_Y, COLUMN_WIDTH, PAGE_REGION_HEIGHT},
    [RemoteLayoutRegionHint] = {COLUMN_X, HINT_REGION_Y, COLUMN_WIDTH, HINT_REGION_HEIGHT},
    [RemoteLayoutRegionDelivered] = {COLUMN_X, DELIVERED_REGION_Y, COLUMN_WIDTH, DELIVERED_REGION_HEIGHT},
    [RemoteLayoutRegionError] = {COLUMN_X, ERROR_REGION_Y, COLUMN_WIDTH, ERROR_REGION_HEIGHT},
};

RemoteLayoutRectangle remote_display_layout_region(RemoteLayoutRegion region) {
    return regions[region];
}

void remote_display_state_initialise(RemoteDisplayState* display_state) {
    memset(display_state, 0, sizeof(*display_state));
    display_state->status = RemoteDisplayStatusReady;
    display_state->page = RemoteDisplayPageNone;
}

static bool status_is_known(int status) {
    return status >= 0 && status < (int)RemoteDisplayStatusCount;
}

static bool page_shows_a_code(int page) {
    return page == RemoteDisplayPageWifi || page == RemoteDisplayPageGuest;
}

static bool is_link_screen(const RemoteDisplayState* display_state) {
    return !display_state->link_connected || display_state->show_diagnostics;
}

/* Copies at most cell_count characters so a value can never run past the
 * cells its region has. */
static void cut_to_cells(char* destination, size_t capacity, const char* source, int cell_count) {
    size_t limit = (size_t)cell_count < capacity - 1 ? (size_t)cell_count : capacity - 1;
    size_t length = 0;
    while(length < limit && source[length] != '\0') {
        destination[length] = source[length];
        length++;
    }
    destination[length] = '\0';
}

/* Decimal text without printf: the firmware links no stdio. */
static void write_unsigned(char* destination, size_t capacity, unsigned int value) {
    char reversed[11];
    size_t length = 0;
    do {
        reversed[length++] = (char)('0' + (value % 10));
        value /= 10;
    } while(value > 0 && length < sizeof(reversed));
    size_t written = 0;
    while(length > 0 && written + 1 < capacity) {
        destination[written++] = reversed[--length];
    }
    destination[written] = '\0';
}

static void draw_centred(RemoteBitmap* bitmap, int y, const char* text, int scale) {
    int width = remote_font_text_width(text, scale);
    remote_font_draw_text(bitmap, (REMOTE_BITMAP_WIDTH - width) / 2, y, text, scale, RemoteBitmapBlack);
}

static void draw_column_line(RemoteBitmap* bitmap, int y, const char* text) {
    char cut[LABEL_CAPACITY];
    cut_to_cells(cut, sizeof(cut), text, COLUMN_CELLS);
    remote_font_draw_text(bitmap, COLUMN_TEXT_X, y, cut, COLUMN_SCALE, RemoteBitmapBlack);
}

static const char* status_label(int status) {
    switch(status) {
    case RemoteDisplayStatusReady:
        return "READY";
    case RemoteDisplayStatusPresenting:
        return "PRESENTING";
    case RemoteDisplayStatusGuestConnected:
        return "GUEST JOINED";
    case RemoteDisplayStatusTerminating:
        return "ENDING";
    case RemoteDisplayStatusRecoveryRequired:
        return "RECOVERY";
    default:
        return "UNKNOWN";
    }
}

static void render_header(const RemoteDisplayState* display_state, RemoteBitmap* bitmap) {
    remote_font_draw_text(bitmap, MARGIN, MARGIN, "STOPBATH", TITLE_SCALE, RemoteBitmapBlack);
    const char* label = status_label(display_state->status);
    int width = remote_font_text_width(label, TITLE_SCALE);
    remote_font_draw_text(bitmap, REMOTE_BITMAP_WIDTH - MARGIN - width, MARGIN, label, TITLE_SCALE, RemoteBitmapBlack);
}

/* The code, centred in its square. A payload the encoder refuses (empty, or
 * beyond the ceiling in remote_qr.h) shows a distinct message in the square
 * instead, never a truncated or unscannable code (spec 2.7). */
static void render_code(const RemoteDisplayState* display_state, RemoteBitmap* bitmap) {
    if(!page_shows_a_code(display_state->page)) {
        return;
    }
    RemoteLayoutRectangle area = regions[RemoteLayoutRegionCode];
    int inset = (area.width - REMOTE_LAYOUT_CODE_SIDE) / 2;
    RemoteLayoutRectangle square = {area.x + inset, area.y + inset, REMOTE_LAYOUT_CODE_SIDE, REMOTE_LAYOUT_CODE_SIDE};

    /* The matrix is a few hundred bytes and lives here for the call; nothing
     * allocates. */
    RemoteQrMatrix matrix;
    if(remote_qr_encode(display_state->payload, &matrix)) {
        remote_qr_draw(&matrix, bitmap, square);
        return;
    }
    const char* first = "CODE TOO BIG";
    const char* second = "TO SHOW HERE";
    int scale = 3;
    int centre_y = square.y + square.height / 2;
    remote_font_draw_text(bitmap, square.x + (square.width - remote_font_text_width(first, scale)) / 2, centre_y - 30, first, scale, RemoteBitmapBlack);
    remote_font_draw_text(bitmap, square.x + (square.width - remote_font_text_width(second, scale)) / 2, centre_y + 6, second, scale, RemoteBitmapBlack);
}

static void render_page(const RemoteDisplayState* display_state, RemoteBitmap* bitmap) {
    if(!page_shows_a_code(display_state->page)) {
        return;
    }
    int y = PAGE_REGION_Y + 6;
    if(display_state->page == RemoteDisplayPageWifi) {
        draw_column_line(bitmap, y, "WIFI 1/2");
        draw_column_line(bitmap, y + COLUMN_LINE_HEIGHT, "JOIN WI-FI");
    } else {
        draw_column_line(bitmap, y, "GALLERY 2/2");
        draw_column_line(bitmap, y + COLUMN_LINE_HEIGHT, "SEE PHOTOS");
    }
}

/* What the keys do now. The photographer reads this; the guest reads the
 * code. Every line is at most eleven characters (COLUMN_CELLS). */
static void render_hint(const RemoteDisplayState* display_state, RemoteBitmap* bitmap) {
    int y = HINT_REGION_Y + 4;
    const char* lines[3] = {NULL, NULL, NULL};
    switch(display_state->status) {
    case RemoteDisplayStatusReady:
        lines[0] = "KEY0 =";
        lines[1] = "NEW SESSION";
        break;
    case RemoteDisplayStatusPresenting:
    case RemoteDisplayStatusGuestConnected:
        lines[0] = "KEY1 = NEXT";
        lines[1] = "KEY0 HOLD =";
        lines[2] = "END SESSION";
        break;
    case RemoteDisplayStatusTerminating:
        lines[0] = "ENDING";
        lines[1] = "PLEASE WAIT";
        break;
    case RemoteDisplayStatusRecoveryRequired:
        lines[0] = "SEE THE";
        lines[1] = "DASHBOARD";
        break;
    default:
        break;
    }
    for(int line_index = 0; line_index < 3; line_index++) {
        if(lines[line_index] != NULL) {
            draw_column_line(bitmap, y + line_index * COLUMN_LINE_HEIGHT, lines[line_index]);
        }
    }
}

static void render_delivered(const RemoteDisplayState* display_state, RemoteBitmap* bitmap) {
    if(!status_is_known(display_state->status) || display_state->status == RemoteDisplayStatusReady) {
        return;
    }
    draw_column_line(bitmap, DELIVERED_REGION_Y + 4, "PHOTOS");
    char count_text[LABEL_CAPACITY];
    int scale = COUNT_SCALE;
    if(display_state->delivered_count > 9999u) {
        /* Beyond the protocol's bound; say so rather than overrun the column. */
        cut_to_cells(count_text, sizeof(count_text), "9999+", 5);
        scale = 4;
    } else {
        write_unsigned(count_text, sizeof(count_text), display_state->delivered_count);
    }
    remote_font_draw_text(bitmap, COLUMN_TEXT_X, DELIVERED_REGION_Y + 4 + COLUMN_LINE_HEIGHT + 4, count_text, scale, RemoteBitmapBlack);
}

static void render_error(const RemoteDisplayState* display_state, RemoteBitmap* bitmap) {
    if(display_state->error_code[0] == '\0') {
        return;
    }
    RemoteLayoutRectangle area = regions[RemoteLayoutRegionError];
    remote_bitmap_fill_rectangle(bitmap, area.x, area.y, area.width, area.height, RemoteBitmapBlack);
    char cut[LABEL_CAPACITY];
    cut_to_cells(cut, sizeof(cut), display_state->error_code, COLUMN_CELLS);
    int text_height = REMOTE_FONT_GLYPH_HEIGHT * COLUMN_SCALE;
    remote_font_draw_text(bitmap, COLUMN_TEXT_X, area.y + (area.height - text_height) / 2, cut, COLUMN_SCALE, RemoteBitmapWhite);
}

static void render_two_line_screen(RemoteBitmap* bitmap, const char* first, const char* second) {
    draw_centred(bitmap, 70, "STOPBATH", 5);
    draw_centred(bitmap, 150, first, TITLE_SCALE);
    if(second != NULL) {
        draw_centred(bitmap, 195, second, COLUMN_SCALE);
    }
}

static void render_diagnostic_line(RemoteBitmap* bitmap, int y, const char* label, uint32_t value) {
    char line[LABEL_CAPACITY];
    cut_to_cells(line, sizeof(line), label, 22);
    size_t used = strlen(line);
    while(used < 24 && used + 1 < sizeof(line)) {
        line[used++] = ' ';
    }
    line[used] = '\0';
    char number[12];
    write_unsigned(number, sizeof(number), (unsigned int)value);
    size_t digit = 0;
    while(number[digit] != '\0' && used + 1 < sizeof(line)) {
        line[used++] = number[digit++];
    }
    line[used] = '\0';
    remote_font_draw_text(bitmap, MARGIN, y, line, COLUMN_SCALE, RemoteBitmapBlack);
}

static void render_diagnostics(const RemoteDisplayState* display_state, RemoteBitmap* bitmap) {
    const RemoteDisplayDiagnostics* diagnostics = &display_state->diagnostics;
    remote_font_draw_text(bitmap, MARGIN, MARGIN, "DIAGNOSTICS", TITLE_SCALE, RemoteBitmapBlack);
    int y = HEADER_HEIGHT + 8;
    const int step = COLUMN_LINE_HEIGHT + 6;
    render_diagnostic_line(bitmap, y, "RECONNECTIONS", diagnostics->reconnections);
    y += step;
    render_diagnostic_line(bitmap, y, "MALFORMED RECEIVED", diagnostics->malformed_received);
    y += step;
    render_diagnostic_line(bitmap, y, "VERSION MISMATCHES", diagnostics->version_mismatches);
    y += step;
    render_diagnostic_line(bitmap, y, "DROPPED NO LINK", diagnostics->events_dropped_no_link);
    y += step;
    render_diagnostic_line(bitmap, y, "DROPPED OUTPUT FULL", diagnostics->events_dropped_by_output_full);
    y += step;
    render_diagnostic_line(bitmap, y, "HANDSHAKE RETRIES", diagnostics->handshake_retries);
    y += step;
    render_diagnostic_line(bitmap, y, "FULL REFRESHES", diagnostics->full_refreshes);
    y += step;
    render_diagnostic_line(bitmap, y, "PARTIAL REFRESHES", diagnostics->partial_refreshes);
    y += step;
    render_diagnostic_line(bitmap, y, "LAST REFRESH MS", diagnostics->last_refresh_milliseconds);
}

void remote_display_layout_render(const RemoteDisplayState* display_state, RemoteBitmap* bitmap) {
    remote_bitmap_clear(bitmap, RemoteBitmapWhite);
    if(display_state->show_diagnostics) {
        render_diagnostics(display_state, bitmap);
        return;
    }
    if(!display_state->link_connected) {
        if(display_state->link_incompatible) {
            render_two_line_screen(bitmap, "INCOMPATIBLE", "UPDATE THE REMOTE");
        } else if(display_state->link_connecting) {
            render_two_line_screen(bitmap, "CONNECTING", "PLEASE WAIT");
        } else {
            render_two_line_screen(bitmap, "NOT CONNECTED", "RECONNECT THE CABLE");
        }
        return;
    }
    render_header(display_state, bitmap);
    render_code(display_state, bitmap);
    render_page(display_state, bitmap);
    render_hint(display_state, bitmap);
    render_delivered(display_state, bitmap);
    render_error(display_state, bitmap);
}

static bool link_screens_equal(const RemoteDisplayState* before, const RemoteDisplayState* after) {
    if(before->show_diagnostics != after->show_diagnostics || before->link_connected != after->link_connected ||
       before->link_connecting != after->link_connecting || before->link_incompatible != after->link_incompatible) {
        return false;
    }
    if(after->show_diagnostics) {
        return memcmp(&before->diagnostics, &after->diagnostics, sizeof(after->diagnostics)) == 0;
    }
    return true;
}

unsigned remote_display_layout_changed_regions(const RemoteDisplayState* before, const RemoteDisplayState* after) {
    const unsigned every_region = (1u << RemoteLayoutRegionCount) - 1u;
    if(is_link_screen(before) || is_link_screen(after)) {
        if(is_link_screen(before) && is_link_screen(after)) {
            return link_screens_equal(before, after) ? 0u : every_region;
        }
        return every_region;
    }
    unsigned changed = 0u;
    if(before->status != after->status) {
        changed |= (1u << RemoteLayoutRegionHeader) | (1u << RemoteLayoutRegionHint);
        /* The count is hidden while ready and shown otherwise. */
        if(before->status == RemoteDisplayStatusReady || after->status == RemoteDisplayStatusReady) {
            changed |= 1u << RemoteLayoutRegionDelivered;
        }
    }
    if(before->page != after->page) {
        changed |= (1u << RemoteLayoutRegionCode) | (1u << RemoteLayoutRegionPage) | (1u << RemoteLayoutRegionHint);
    }
    if(strcmp(before->payload, after->payload) != 0) {
        changed |= 1u << RemoteLayoutRegionCode;
    }
    if(before->delivered_count != after->delivered_count) {
        changed |= 1u << RemoteLayoutRegionDelivered;
    }
    if(strcmp(before->error_code, after->error_code) != 0) {
        changed |= 1u << RemoteLayoutRegionError;
    }
    return changed;
}
