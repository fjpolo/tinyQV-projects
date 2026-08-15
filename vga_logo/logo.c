// #define PP_DEBUG

//#define PERFORMANCE_TEST
#include <uart.h>
#if 1
#define printf uart_printf
#define putc(C, X) uart_putc(C)
#else
#define printf(...)
#define putc(C, X)
#endif
#include <mul.h>

#include "ctype.h"

#define PP_IMPLEMENTATION
//#define PP_DEBUG
#include "pretty-poly.h"

#define assert(X)

const int WIDTH = 256;
const int HEIGHT = 192;
extern uint32_t* vga_gfx_back_buffer;

void callback(const pp_tile_t* tile) {
  debug_tile(tile);

  uint8_t* alpha_data = tile->data;
  for(int y = 0; y < tile->h; y++) {
    if (tile->y + y < 0 || tile->y + y >= HEIGHT) continue;
    uint32_t* buffer = &vga_gfx_back_buffer[(tile->y + y) << 4];

    for(int x = 0, xoff=tile->x; x < tile->w; ) {
      if (xoff < 0) { x += -xoff; xoff = 0; continue; }
      if (xoff >= WIDTH) break;

      uint32_t data = 0;
      uint32_t mask = 0;
      int addr = xoff >> 4;
      for (int i = (xoff & 0xF) << 1; i < 32 && x < tile->w && xoff < WIDTH; i+=2, ++x, ++xoff) {
        if (alpha_data[x] > 1) {
          data |= 1u << i;
        }
        else if (alpha_data[x] == 1) {
          data |= 2u << i;
        }
        mask |= 3u << i;
      }

      buffer[addr] &= ~mask;
      buffer[addr] |= data;
    }

    alpha_data += tile->stride;
  }
}

// this is a terrible svg path parser, it only supports a tiny subset of
// features, indeed it just barely manages to extract this logo
// perhaps it could be fun to extend this in future - it would
// provide a simple way to import vector artwork into projects
pp_point_t point_on_cubic_bezier(int t, pp_point_t s, pp_point_t cp1, pp_point_t cp2, pp_point_t e) {
  int t2 = t * t;
  int t3 = t * t * t;
  return (pp_point_t) {
    .x = ((s.x << 6) + mul32x16(mul32x16(-s.x, 3 * 16) + mul32x16(mul32x16(s.x, 3 * 4) - mul32x16(s.x, t), t), t)
          + mul32x16(mul32x16(cp1.x, 3 * 16) + mul32x16(-mul32x16(cp1.x, 6 * 4) + mul32x16(cp1.x, 3 * t),  t), t)
          + mul32x16((mul32x16(cp2.x, 3 * 4) - mul32x16(cp2.x, 3 * t)), t2)
          + mul32x16(e.x, t3)) >> 6,
    .y = ((s.y << 6) + mul32x16(mul32x16(-s.y, 3 * 16) + mul32x16(mul32x16(s.y, 3 * 4) - mul32x16(s.y, t), t), t)
          + mul32x16(mul32x16(cp1.y, 3 * 16) + mul32x16(-mul32x16(cp1.y, 6 * 4) + mul32x16(cp1.y, 3 * t),  t), t)
          + mul32x16((mul32x16(cp2.y, 3 * 4) - mul32x16(cp2.y, 3 * t)), t2)
          + mul32x16(e.y, t3)) >> 6
  };
}

typedef enum {
  NONE, MOVE, MOVE_RELATIVE, LINE, LINE_RELATIVE, HORIZONTAL, HORIZONTAL_RELATIVE, VERTICAL, VERTICAL_RELATIVE, CUBIC_BEZIER, CUBIC_BEZIER_RELATIVE
} command_t;

// list of supported svg path commands, can either be upper (absolute) or lower 
// (relative) case:
//   - M = moveto
//   - C = curveto
//   - Z = closepath
//   - L = lineto
//   - H = horizontal lineto
//   - V = vertical lineto
//
// not yet supported:
//   - S = smooth curveto
//   - Q = quadratic Bézier curve
//   - T = smooth quadratic Bézier curveto
//   - A = elliptical Arc

// extract the next token from the path
void get_next_token(const char **caret, char *token) {
  // skip any leading whitespace
  while(**caret == ' ') {
    (*caret)++;
  }
  
  // is the first char a supported command?
  if(strchr("mMcClLhHvVzZ", **caret)) {
    // copy command as token to return and increment caret
    token[0] = **caret;
    token[1] = '\0';
    (*caret)++;
    return;
  }

  // is the first char an unsupported command?
  if(strchr("sSqQtTaA", **caret)) {
    assert(!"error: SVG path command not supported.");
  }

  // return the next number
  while(true) {
    if(**caret == ' ' || **caret == 'z' || **caret == '\n') {
      *token = '\0';
      break;
    }
    *token++ = **caret;
    (*caret)++;
  }

  return;
}

bool is_numeric(char *token) {
  while(*token != '\0') {
    if((*token < '0' || *token > '9') && *token != '-') { return false; }
    token++;
  }
  return true;
}

// cor this code, just don't even look at it
int get_svg_path_point_count(const char *path) {
  const char **caret = &path;

  command_t command = NONE;

  char token[64];
  int count = 0;
  while(true) {
    get_next_token(caret, token);

    // no more tokens, end of contour
    if(strlen(token) == 0) {
      break;
    }

    putc(*token, stdout);

    // if token is command then execute it and continue
    switch(token[0]) {
      case 'M': { command = MOVE; continue; } break;
      case 'm': { command = MOVE_RELATIVE; continue; } break;
      case 'L': { command = LINE; continue; } break;
      case 'l': { command = LINE_RELATIVE; continue; } break;
      case 'H': { command = HORIZONTAL; continue; } break;
      case 'h': { command = HORIZONTAL_RELATIVE; continue; } break;
      case 'V': { command = VERTICAL; continue; } break;
      case 'v': { command = VERTICAL_RELATIVE; continue; } break;
      case 'C': { command = CUBIC_BEZIER; continue; } break;
      case 'c': { command = CUBIC_BEZIER_RELATIVE; continue; } break;
      case 'z': { return count; } break;
    }

    // extract control points and end point    
    if(command == CUBIC_BEZIER_RELATIVE || command == CUBIC_BEZIER) {
      get_next_token(caret, token);
      get_next_token(caret, token);
      get_next_token(caret, token);
      get_next_token(caret, token);
      get_next_token(caret, token);
      count += 4;
    }

    if(command == MOVE || command == MOVE_RELATIVE || command == LINE || command == LINE_RELATIVE) {
      get_next_token(caret, token);
      count += 1;
    }

    if (command == HORIZONTAL || command == HORIZONTAL_RELATIVE || command == VERTICAL || command == VERTICAL_RELATIVE) {
      count += 1;
    }
  }

  return count;
}

int get_svg_path_count(const char *caret) {
  int count = 0;
  char *found = strchr(caret, 'z');
  while(found) {
    caret = found + 1;
    count++;
    found = strchr(caret, 'z');
  }
  return count;
}

// parses the contour starting at the caret and returns the points and point
// count
static pp_point_t last;
static pp_point_t point_buffer[4096];
static pp_point_t* point_buffer_ptr;
pp_path_t parse_svg_path_contour(const char **caret) {  
  
  pp_path_t result;
  result.count = get_svg_path_point_count(*caret);
  result.points = point_buffer_ptr;
  point_buffer_ptr += result.count;
  printf("Points %ld, total points %d\r\n", result.count, point_buffer_ptr - point_buffer);

  command_t command = NONE;

  debug("> start of contour\r\n");

  char token[16];
  int i = 0;
  while(true) {
    get_next_token(caret, token);

    // no more tokens, end of contour
    if(strlen(token) == 0) {
      break;
    }

    // if token is command then execute it and continue
    switch(token[0]) {
      case 'M': { command = MOVE; continue; } break;
      case 'm': { command = MOVE_RELATIVE; continue; } break;
      case 'L': { command = LINE; continue; } break;
      case 'l': { command = LINE_RELATIVE; continue; } break;
      case 'H': { command = HORIZONTAL; continue; } break;
      case 'h': { command = HORIZONTAL_RELATIVE; continue; } break;
      case 'V': { command = VERTICAL; continue; } break;
      case 'v': { command = VERTICAL_RELATIVE; continue; } break;
      case 'C': { command = CUBIC_BEZIER; continue; } break;
      case 'c': { command = CUBIC_BEZIER_RELATIVE; continue; } break;
      case 'z': { return result; } break;
    }

    // extract control points and end point    
    if(command == CUBIC_BEZIER_RELATIVE) {
      pp_point_t c1;
      c1.x = atoi(token) + last.x;
      get_next_token(caret, token);
      c1.y = atoi(token) + last.y;

      pp_point_t c2;
      get_next_token(caret, token);
      c2.x = atoi(token) + last.x;
      get_next_token(caret, token);
      c2.y = atoi(token) + last.y;

      pp_point_t point;
      get_next_token(caret, token);
      point.x = atoi(token) + last.x;
      get_next_token(caret, token);
      point.y = atoi(token) + last.y;

      result.points[i++] = point_on_cubic_bezier(1, last, c1, c2, point);
      result.points[i++] = point_on_cubic_bezier(2, last, c1, c2, point);
      result.points[i++] = point_on_cubic_bezier(3, last, c1, c2, point);
      result.points[i++] = point;

      last = point;

      debug("  + %d, %d (bezier)\n", point.x, point.y);
    }

    if(command == CUBIC_BEZIER) {
      pp_point_t c1;
      c1.x = atoi(token);
      get_next_token(caret, token);
      c1.y = atoi(token);

      pp_point_t c2;
      get_next_token(caret, token);
      c2.x = atoi(token);
      get_next_token(caret, token);
      c2.y = atoi(token);

      pp_point_t point;
      get_next_token(caret, token);
      point.x = atoi(token);
      get_next_token(caret, token);
      point.y = atoi(token);

      result.points[i++] = point_on_cubic_bezier(1, last, c1, c2, point);
      result.points[i++] = point_on_cubic_bezier(2, last, c1, c2, point);
      result.points[i++] = point_on_cubic_bezier(3, last, c1, c2, point);
      result.points[i++] = point;

      last = point;

      debug("  + %d, %d (bezier)\n", point.x, point.y);
    }

    if (command == HORIZONTAL) {
      pp_point_t point;
      point.x = atoi(token);
      point.y = last.y;
      result.points[i++] = point;
      last = point;

      debug("  + %d, %d\n", point.x, point.y);      
    }

    if (command == HORIZONTAL_RELATIVE) {
      pp_point_t point;
      point.x = atoi(token) + last.x;
      point.y = last.y;
      result.points[i++] = point;
      last = point;

      debug("  + %d, %d\n", point.x, point.y);      
    }

    if (command == VERTICAL) {
      pp_point_t point;
      point.x = last.x;
      point.y = atoi(token);
      result.points[i++] = point;
      last = point;

      debug("  + %d, %d\n", point.x, point.y);      
    }

    if (command == VERTICAL_RELATIVE) {
      pp_point_t point;
      point.x = last.x;
      point.y = atoi(token) + last.y;
      result.points[i++] = point;
      last = point;

      debug("  + %d, %d\n", point.x, point.y);      
    }

    if(command == MOVE || command == LINE) {
      pp_point_t point;
      point.x = atoi(token);
      get_next_token(caret, token);
      point.y = atoi(token);
      result.points[i++] = point;
      last = point;

      debug("  + %d, %d\n", point.x, point.y);
    }

    if(command == MOVE_RELATIVE || command == LINE_RELATIVE) {
      pp_point_t point;
      point.x = atoi(token) + last.x;
      get_next_token(caret, token);
      point.y = atoi(token) + last.y;
      result.points[i++] = point;
      last = point;

      debug("  + %d, %d\n", point.x, point.y);
    }

  }

  return result;
}

static pp_path_t countour_buffer[64];
pp_poly_t parse_svg_path(const char *path) {
  pp_poly_t result;

  // allocate enough contours to store the path data
  result.count = get_svg_path_count(path);
  printf("Countours %ld\r\n", result.count);
  result.paths = countour_buffer;

  // parse each contour
  point_buffer_ptr = point_buffer;
  const char *caret = path;
  const char *path_end = path + strlen(path);
  int i = 0;
  while(caret < path_end) {
    result.paths[i] = parse_svg_path_contour(&caret);
    i++;
  }

  return result;
}

static pp_mat3_t t;
static pp_poly_t polygon;
static pp_rect_t bounds;
static bool poly_inited = false;
void draw_logo(int angle) {

  // setup polygon renderer  
  pp_tile_callback(callback);
  pp_antialias(PP_AA_X4);
  pp_clip(0, 0, WIDTH, HEIGHT);
  
  if (!poly_inited)
  {
    // parse logo svg path into contours
    // Pretty Poly logo
    //const char logo_svg_path[] = "M3260 3933 c-168 -179 -274 -287 -503 -520 -248 -253 -248 -253 -1442 -253 -657 0 -1195 -3 -1195 -6 0 -9 124 -189 132 -192 5 -2 8 -9 8 -15 0 -6 9 -20 19 -31 11 -12 27 -35 38 -53 10 -18 31 -50 47 -73 29 -41 29 -41 -59 -173 -49 -73 -93 -133 -97 -135 -4 -2 -8 -9 -8 -14 0 -6 -17 -34 -38 -62 -21 -28 -42 -57 -46 -64 -6 -10 154 -12 805 -12 446 0 814 -1 816 -4 2 -2 -9 -23 -25 -47 -34 -51 -104 -188 -122 -239 -7 -19 -16 -44 -20 -55 -53 -128 -67 -261 -69 -641 -1 -117 -4 -164 -13 -171 -7 -6 -31 -13 -53 -16 -22 -3 -47 -8 -56 -12 -19 -7 -32 20 -50 110 -7 33 -13 61 -15 63 -6 8 -85 -51 -115 -86 -83 -97 -98 -161 -80 -347 20 -205 30 -241 83 -294 45 -46 99 -67 205 -80 126 -15 263 -65 396 -145 35 -20 113 -100 158 -161 24 -33 49 -66 56 -72 6 -7 13 -18 15 -24 3 -9 10 -9 27 0 12 7 20 19 17 26 -2 7 1 16 9 18 7 3 28 36 46 74 30 63 32 76 32 168 0 55 -4 111 -10 125 -6 14 -10 27 -9 30 0 3 -12 27 -28 54 -28 48 -28 48 11 90 82 86 150 228 169 351 6 43 17 61 78 130 39 44 73 82 76 85 43 43 192 269 185 280 -3 6 -2 10 4 10 27 0 190 372 210 480 5 30 13 87 17 125 4 39 8 75 9 80 1 6 3 30 3 55 2 45 2 45 734 43 403 -2 729 0 723 5 -5 4 -14 15 -20 24 -5 9 -65 98 -132 197 -68 99 -123 186 -123 192 0 7 6 20 14 28 8 9 69 97 135 196 122 180 122 180 -494 183 -564 2 -640 5 -606 26 4 3 11 23 15 44 3 21 13 61 21 88 8 27 31 108 51 179 20 72 45 162 55 200 11 39 28 102 39 140 10 39 23 87 30 108 6 21 10 43 8 48 -2 6 -32 -20 -68 -58z m-2188 -993 c-3 -149 1 -152 43 -24 14 43 35 98 46 122 20 43 35 50 87 36 19 -6 22 -11 17 -35 -4 -15 -15 -46 -26 -68 -10 -22 -19 -46 -19 -54 0 -7 -4 -17 -9 -23 -5 -5 -16 -29 -24 -54 -15 -45 -15 -45 18 -82 40 -43 51 -98 41 -195 -12 -112 -50 -143 -177 -143 -43 0 -81 5 -84 10 -8 13 -14 476 -7 578 5 73 5 73 51 70 46 -3 46 -3 43 -138z m476 107 c2 -10 1 -29 -2 -43 -6 -22 -11 -24 -70 -24 -63 0 -64 0 -70 -31 -3 -17 -6 -62 -6 -100 0 -69 0 -69 56 -69 55 0 55 0 52 -42 -3 -43 -3 -43 -55 -46 -53 -3 -53 -3 -53 -93 0 -89 0 -89 70 -89 70 0 70 0 70 -39 0 -48 -4 -50 -127 -50 -69 0 -94 4 -104 15 -9 11 -10 51 -5 162 4 81 7 187 6 236 -2 135 9 234 27 238 8 2 59 1 112 -2 83 -4 96 -8 99 -23z m820 21 c8 -8 12 -53 12 -132 1 -104 12 -189 33 -246 4 -8 8 -28 11 -45 15 -90 19 -111 26 -120 5 -5 10 -30 12 -55 3 -44 3 -45 -30 -48 -44 -4 -59 10 -67 61 -3 23 -10 60 -15 82 -5 22 -12 62 -15 89 -8 59 -21 50 -34 -24 -14 -80 -39 -189 -46 -200 -8 -14 -58 -13 -72 1 -12 12 -9 56 7 94 4 11 15 52 25 90 9 39 26 106 37 150 13 48 23 122 25 185 2 58 6 111 8 118 6 15 67 16 83 0z m869 -32 c43 -46 47 -85 36 -334 -8 -192 -11 -211 -31 -240 -43 -59 -157 -65 -212 -11 -37 37 -43 100 -36 357 6 182 7 195 28 222 30 36 71 50 133 45 41 -4 56 -10 82 -39z m485 -86 c3 -75 15 -159 28 -210 12 -47 26 -105 31 -130 5 -25 14 -65 20 -90 20 -85 18 -95 -19 -98 -41 -4 -62 12 -62 48 0 15 -6 45 -13 66 -7 22 -13 44 -13 49 0 6 -4 39 -9 75 -8 65 -8 65 -25 -5 -10 -38 -23 -101 -31 -140 -7 -38 -19 -76 -25 -82 -16 -17 -59 -17 -72 0 -11 13 -8 31 48 242 23 85 34 156 40 245 10 156 12 162 59 158 36 -3 36 -3 43 -128z m-2979 78 c3 -24 4 -82 3 -129 -3 -87 -3 -87 43 -92 106 -13 134 -53 135 -195 0 -93 -16 -142 -54 -162 -28 -15 -193 -30 -205 -19 -6 6 -11 132 -13 324 -4 315 -4 315 41 315 45 0 45 0 50 -42z m1007 -238 c0 -280 0 -280 46 -280 45 0 45 0 42 -42 -3 -43 -3 -43 -121 -46 -140 -3 -157 3 -157 58 0 40 0 40 45 40 45 0 45 0 46 68 1 37 3 123 5 192 2 69 3 162 4 208 0 82 0 82 45 82 45 0 45 0 45 -280z m312 3 c3 -278 3 -278 46 -281 43 -3 43 -3 40 -45 -3 -42 -3 -42 -118 -45 -134 -3 -170 7 -170 47 0 37 17 51 62 51 43 0 40 -26 39 305 -1 77 2 164 5 193 6 52 6 52 50 52 44 0 44 0 46 -277z m698 148 c0 -128 0 -128 58 -134 50 -5 61 -10 89 -42 24 -28 33 -50 39 -92 16 -130 -14 -214 -84 -232 -40 -11 -168 -18 -175 -11 -8 8 -18 626 -11 633 4 4 25 7 46 7 38 0 38 0 38 -129z m815 84 c0 -40 0 -40 -66 -43 -66 -3 -66 -3 -72 -160 -3 -86 -6 -208 -7 -271 0 -63 -3 -118 -6 -123 -3 -4 -23 -8 -45 -8 -39 0 -39 0 -39 325 0 326 0 326 118 323 117 -3 117 -3 117 -43z";

    // Tiny Tapeout logo
    const char logo_svg_path[] = "M3410 964 c-445 0 -839 219 -1081 555 -79 110 -142 232 -185 364 h737 v901 h393 v-243 h227 v878 c-30 3 -61 4 -92 4 -622 0 -1128 -506 -1128 -1128 0 -103 14 -202 40 -296 h-210 c-22 95 -33 194 -33 296 0 734 597 1331 1331 1331 31 0 61 -1 92 -3 17 -1 34 -3 51 -5 h4 c118 -13 231 -41 338 -83 v-994 H4280 v-364 H3275 v-295 H3660 v-364 H2593 c205 -216 496 -351 817 -351 622 0 1128 506 1128 1128 0 401 -210 754 -526 954 v234 c433 -220 730 -670 730 -1188 -1 -734 -598 -1331 -1332 -1331z";

    // Duck from https://www.svgrepo.com/svg/117055/small-duck (CC0)
    /* char logo_svg_path[] = "M105572 101811 c9889 -6368 27417 -16464 28106 -42166 c0536 -20278 -9971 -49506 -49155 -50878 \
  C53041 7659 39900 28251 36071 46739 l-0928 -0126 c-1932 0 -3438 1280 -5340 2889 c-2084 1784 -4683 3979 -7792 4308 \
  c-3573 0361 -8111 -1206 -11698 -2449 c-4193 -1431 -6624 -2047 -8265 -0759 c-1503 1163 -2178 3262 -2028 6226 \
  c0331 6326 4971 18917 16016 25778 c7670 4765 16248 5482 20681 5482 c0006 0 0006 0 0006 0 \
  c2370 0 4945 -0239 7388 -0726 c2741 4218 5228 7476 6037 9752 c2054 5851 -27848 25087 -27848 55010 \
  c0 29916 22013 48475 56727 48475 l55004 0 c30593 0 70814 -29908 75291 -92480 C180781 132191 167028 98150 105572 101811 \
  z M18941 77945 C8775 71617 4992 58922 5294 55525 c0897 0240 2194 0689 3228 1042 \
  c4105 1415 9416 3228 14068 2707 c4799 -0499 8253 -3437 10778 -5574 c0607 -0509 1393 -1176 1872 -1491 \
  c0870 0315 0962 0693 1176 3140 c0196 2260 0473 5370 2362 9006 c1437 2761 3581 5705 5646 8542 \
  c1701 2336 4278 5871 4535 6404 c-0445 1184 -4907 3282 -12229 3282 C30177 82591 23690 80904 18941 77945z \
  M56860 49368 c0 -4938 4001 -8943 8931 -8943 c4941 0 8942 4005 8942 8943 c0 4931 -4001 8942 -8942 8942 \
  C60854 58311 56860 54299 56860 49368z M149159 155398 l-20630 11169 l13408 9293 c0 0 -49854 15813 -72198 -6885 \
  c-11006 -11160 -13060 -28533 4124 -38840 c17184 -10312 84609 3943 84609 3943 L134295 147800 L149159 155398z"; */
    
    polygon = parse_svg_path(logo_svg_path);

    printf("1\n");

    // determine extreme bounds and scaling factor to fit on canvas
    bounds = pp_polygon_bounds(&polygon);

    poly_inited = true;
  }

  float scale = 0.9f * (float)HEIGHT / (float)bounds.h;
  printf("Scale %.5f\n", scale);

/*#ifdef PERFORMANCE_TEST
  uint64_t min_time = 1000000000;
  for (int j = 0; j < 15; ++j) {
    //uint64_t start_time = get_time_ns();

    for (int i = 0; i < 60; ++i) {
      draw_polygon(polygon, (pp_point_t){0, 0}, 65536);
    }

    min_time = std::min(min_time, get_time_ns() - start_time);
  }

  printf("Draw time: %lldus\n", min_time / 1000);
#else*/
  t = pp_mat3_identity();
  pp_mat3_translate(&t, WIDTH/2, HEIGHT/2);
  pp_mat3_scale(&t, scale, scale);
  pp_mat3_rotate(&t, angle);
  pp_mat3_translate(&t, -bounds.x - bounds.w / 2, -bounds.y - bounds.h / 2);
  pp_transform(&t);

#if 0
  printf("Transform: %.4f %.4f %.1f\r\n", t.v00, t.v01, t.v02);
  printf("Transform: %.4f %.4f %.1f\r\n", t.v10, t.v11, t.v12);
  printf("Transform: %.4f %.4f %.1f\r\n", t.v20, t.v21, t.v22);
#endif

  printf("Render\n");
  pp_render(&polygon);
//#endif
}
