#include "remote_font.h"

#include <string.h>

/* Each glyph is drawn as it looks, one string per row, a hash for a set
 * pixel. Readable at the cost of a few kilobytes of flash, which the RP2350
 * has to spare, and it makes a wrong glyph visible in review rather than
 * hidden in a hex table. */
typedef struct {
    char code;
    char rows[REMOTE_FONT_GLYPH_HEIGHT][REMOTE_FONT_GLYPH_WIDTH + 1];
} RemoteFontGlyph;

static const RemoteFontGlyph glyphs[] = {
    {' ', {"     ", "     ", "     ", "     ", "     ", "     ", "     "}},
    {'A', {" ### ", "#   #", "#   #", "#####", "#   #", "#   #", "#   #"}},
    {'B', {"#### ", "#   #", "#   #", "#### ", "#   #", "#   #", "#### "}},
    {'C', {" ### ", "#   #", "#    ", "#    ", "#    ", "#   #", " ### "}},
    {'D', {"#### ", "#   #", "#   #", "#   #", "#   #", "#   #", "#### "}},
    {'E', {"#####", "#    ", "#    ", "#### ", "#    ", "#    ", "#####"}},
    {'F', {"#####", "#    ", "#    ", "#### ", "#    ", "#    ", "#    "}},
    {'G', {" ### ", "#   #", "#    ", "# ###", "#   #", "#   #", " ### "}},
    {'H', {"#   #", "#   #", "#   #", "#####", "#   #", "#   #", "#   #"}},
    {'I', {"#####", "  #  ", "  #  ", "  #  ", "  #  ", "  #  ", "#####"}},
    {'J', {"  ###", "   # ", "   # ", "   # ", "   # ", "#  # ", " ##  "}},
    {'K', {"#   #", "#  # ", "# #  ", "##   ", "# #  ", "#  # ", "#   #"}},
    {'L', {"#    ", "#    ", "#    ", "#    ", "#    ", "#    ", "#####"}},
    {'M', {"#   #", "## ##", "# # #", "# # #", "#   #", "#   #", "#   #"}},
    {'N', {"#   #", "#   #", "##  #", "# # #", "#  ##", "#   #", "#   #"}},
    {'O', {" ### ", "#   #", "#   #", "#   #", "#   #", "#   #", " ### "}},
    {'P', {"#### ", "#   #", "#   #", "#### ", "#    ", "#    ", "#    "}},
    {'Q', {" ### ", "#   #", "#   #", "#   #", "# # #", "#  # ", " ## #"}},
    {'R', {"#### ", "#   #", "#   #", "#### ", "# #  ", "#  # ", "#   #"}},
    {'S', {" ####", "#    ", "#    ", " ### ", "    #", "    #", "#### "}},
    {'T', {"#####", "  #  ", "  #  ", "  #  ", "  #  ", "  #  ", "  #  "}},
    {'U', {"#   #", "#   #", "#   #", "#   #", "#   #", "#   #", " ### "}},
    {'V', {"#   #", "#   #", "#   #", "#   #", "#   #", " # # ", "  #  "}},
    {'W', {"#   #", "#   #", "#   #", "# # #", "# # #", "## ##", "#   #"}},
    {'X', {"#   #", "#   #", " # # ", "  #  ", " # # ", "#   #", "#   #"}},
    {'Y', {"#   #", "#   #", " # # ", "  #  ", "  #  ", "  #  ", "  #  "}},
    {'Z', {"#####", "    #", "   # ", "  #  ", " #   ", "#    ", "#####"}},
    {'0', {" ### ", "#   #", "#  ##", "# # #", "##  #", "#   #", " ### "}},
    {'1', {"  #  ", " ##  ", "  #  ", "  #  ", "  #  ", "  #  ", " ### "}},
    {'2', {" ### ", "#   #", "    #", "   # ", "  #  ", " #   ", "#####"}},
    {'3', {"#####", "   # ", "  #  ", "   # ", "    #", "#   #", " ### "}},
    {'4', {"   # ", "  ## ", " # # ", "#  # ", "#####", "   # ", "   # "}},
    {'5', {"#####", "#    ", "#### ", "    #", "    #", "#   #", " ### "}},
    {'6', {"  ## ", " #   ", "#    ", "#### ", "#   #", "#   #", " ### "}},
    {'7', {"#####", "    #", "   # ", "  #  ", " #   ", " #   ", " #   "}},
    {'8', {" ### ", "#   #", "#   #", " ### ", "#   #", "#   #", " ### "}},
    {'9', {" ### ", "#   #", "#   #", " ####", "    #", "   # ", " ##  "}},
    {':', {"     ", "  #  ", "  #  ", "     ", "  #  ", "  #  ", "     "}},
    {'/', {"    #", "    #", "   # ", "  #  ", " #   ", "#    ", "#    "}},
    {'-', {"     ", "     ", "     ", "#####", "     ", "     ", "     "}},
    {'.', {"     ", "     ", "     ", "     ", "     ", " ##  ", " ##  "}},
    {',', {"     ", "     ", "     ", "     ", " ##  ", "  #  ", " #   "}},
    {'_', {"     ", "     ", "     ", "     ", "     ", "     ", "#####"}},
    {'?', {" ### ", "#   #", "    #", "   # ", "  #  ", "     ", "  #  "}},
    {'!', {"  #  ", "  #  ", "  #  ", "  #  ", "  #  ", "     ", "  #  "}},
    {'(', {"   # ", "  #  ", " #   ", " #   ", " #   ", "  #  ", "   # "}},
    {')', {" #   ", "  #  ", "   # ", "   # ", "   # ", "  #  ", " #   "}},
    {'%', {"##   ", "##  #", "   # ", "  #  ", " #   ", "#  ##", "   ##"}},
    {'=', {"     ", "     ", "#####", "     ", "#####", "     ", "     "}},
    {'+', {"     ", "  #  ", "  #  ", "#####", "  #  ", "  #  ", "     "}},
    {'\'', {" ##  ", "  #  ", " #   ", "     ", "     ", "     ", "     "}},
    {'"', {" # # ", " # # ", " # # ", "     ", "     ", "     ", "     "}},
    {'&', {" #   ", "# #  ", "# #  ", " #   ", "# # #", "#  # ", " ## #"}},
    {'#', {" # # ", " # # ", "#####", " # # ", "#####", " # # ", " # # "}},
    {'@', {" ### ", "#   #", "# ###", "# # #", "# ###", "#    ", " ### "}},
    {';', {"     ", " ##  ", " ##  ", "     ", " ##  ", "  #  ", " #   "}},
    {'<', {"   # ", "  #  ", " #   ", "#    ", " #   ", "  #  ", "   # "}},
    {'>', {" #   ", "  #  ", "   # ", "    #", "   # ", "  #  ", " #   "}},
    {'*', {"     ", "# # #", " ### ", "#####", " ### ", "# # #", "     "}},
    {'~', {"     ", "     ", " #  #", "# # #", "#  # ", "     ", "     "}},
};

static char fold_to_upper_case(char character) {
    if(character >= 'a' && character <= 'z') {
        return (char)(character - ('a' - 'A'));
    }
    return character;
}

/* NULL when there is no glyph: the caller leaves the cell empty. */
static const RemoteFontGlyph* glyph_for(char character) {
    char folded = fold_to_upper_case(character);
    for(size_t glyph_index = 0; glyph_index < sizeof(glyphs) / sizeof(glyphs[0]); glyph_index++) {
        if(glyphs[glyph_index].code == folded) {
            return &glyphs[glyph_index];
        }
    }
    return NULL;
}

static void draw_glyph(
    RemoteBitmap* bitmap,
    int x,
    int y,
    const RemoteFontGlyph* glyph,
    int scale,
    RemoteBitmapColour colour) {
    for(int row = 0; row < REMOTE_FONT_GLYPH_HEIGHT; row++) {
        for(int column = 0; column < REMOTE_FONT_GLYPH_WIDTH; column++) {
            if(glyph->rows[row][column] == '#') {
                remote_bitmap_fill_rectangle(bitmap, x + column * scale, y + row * scale, scale, scale, colour);
            }
        }
    }
}

void remote_font_draw_text(
    RemoteBitmap* bitmap,
    int x,
    int y,
    const char* text,
    int scale,
    RemoteBitmapColour colour) {
    if(scale < 1) {
        return;
    }
    int pen_x = x;
    for(const char* character = text; *character != '\0'; character++) {
        const RemoteFontGlyph* glyph = glyph_for(*character);
        if(glyph != NULL) {
            draw_glyph(bitmap, pen_x, y, glyph, scale, colour);
        }
        /* Stop advancing once the pen is off the right edge; the width
         * function reports the full extent regardless. */
        if(pen_x > REMOTE_BITMAP_WIDTH) {
            return;
        }
        pen_x += REMOTE_FONT_ADVANCE * scale;
    }
}

int remote_font_text_width(const char* text, int scale) {
    if(scale < 1) {
        return 0;
    }
    return (int)strlen(text) * REMOTE_FONT_ADVANCE * scale;
}
