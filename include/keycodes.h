#ifndef KEYCODES_H
#define KEYCODES_H

// Based of GLFW keycodes
// Mouse buttons are included and do not have spearate keycodes

/* The unknown key */
#define CS_KEY_UNKNOWN -1

/* Printable keys */
#define CS_KEY_SPACE		 32
#define CS_KEY_APOSTROPHE	 39 /* ' */
#define CS_KEY_COMMA		 44 /* , */
#define CS_KEY_MINUS		 45 /* - */
#define CS_KEY_PERIOD		 46 /* . */
#define CS_KEY_SLASH		 47 /* / */
#define CS_KEY_0			 48
#define CS_KEY_1			 49
#define CS_KEY_2			 50
#define CS_KEY_3			 51
#define CS_KEY_4			 52
#define CS_KEY_5			 53
#define CS_KEY_6			 54
#define CS_KEY_7			 55
#define CS_KEY_8			 56
#define CS_KEY_9			 57
#define CS_KEY_SEMICOLON	 59 /* ; */
#define CS_KEY_EQUAL		 61 /* = */
#define CS_KEY_A			 65
#define CS_KEY_B			 66
#define CS_KEY_C			 67
#define CS_KEY_D			 68
#define CS_KEY_E			 69
#define CS_KEY_F			 70
#define CS_KEY_G			 71
#define CS_KEY_H			 72
#define CS_KEY_I			 73
#define CS_KEY_J			 74
#define CS_KEY_K			 75
#define CS_KEY_L			 76
#define CS_KEY_M			 77
#define CS_KEY_N			 78
#define CS_KEY_O			 79
#define CS_KEY_P			 80
#define CS_KEY_Q			 81
#define CS_KEY_R			 82
#define CS_KEY_S			 83
#define CS_KEY_T			 84
#define CS_KEY_U			 85
#define CS_KEY_V			 86
#define CS_KEY_W			 87
#define CS_KEY_X			 88
#define CS_KEY_Y			 89
#define CS_KEY_Z			 90
#define CS_KEY_LEFT_BRACKET	 91	 /* [ */
#define CS_KEY_BACKSLASH	 92	 /* \ */
#define CS_KEY_RIGHT_BRACKET 93	 /* ] */
#define CS_KEY_GRAVE_ACCENT	 96	 /* ` */
#define CS_KEY_WORLD_1		 161 /* non-US #1 */
#define CS_KEY_WORLD_2		 162 /* non-US #2 */

/* FunctCSn keys */
#define CS_KEY_ESCAPE		 256
#define CS_KEY_ENTER		 257
#define CS_KEY_TAB			 258
#define CS_KEY_BACKSPACE	 259
#define CS_KEY_INSERT		 260
#define CS_KEY_DELETE		 261
#define CS_KEY_RIGHT		 262
#define CS_KEY_LEFT			 263
#define CS_KEY_DOWN			 264
#define CS_KEY_UP			 265
#define CS_KEY_PAGE_UP		 266
#define CS_KEY_PAGE_DOWN	 267
#define CS_KEY_HOME			 268
#define CS_KEY_END			 269
#define CS_KEY_CAPS_LOCK	 280
#define CS_KEY_SCROLL_LOCK	 281
#define CS_KEY_NUM_LOCK		 282
#define CS_KEY_PRINT_SCREEN	 283
#define CS_KEY_PAUSE		 284
#define CS_KEY_F1			 290
#define CS_KEY_F2			 291
#define CS_KEY_F3			 292
#define CS_KEY_F4			 293
#define CS_KEY_F5			 294
#define CS_KEY_F6			 295
#define CS_KEY_F7			 296
#define CS_KEY_F8			 297
#define CS_KEY_F9			 298
#define CS_KEY_F10			 299
#define CS_KEY_F11			 300
#define CS_KEY_F12			 301
#define CS_KEY_F13			 302
#define CS_KEY_F14			 303
#define CS_KEY_F15			 304
#define CS_KEY_F16			 305
#define CS_KEY_F17			 306
#define CS_KEY_F18			 307
#define CS_KEY_F19			 308
#define CS_KEY_F20			 309
#define CS_KEY_F21			 310
#define CS_KEY_F22			 311
#define CS_KEY_F23			 312
#define CS_KEY_F24			 313
#define CS_KEY_F25			 314
#define CS_KEY_KP_0			 320
#define CS_KEY_KP_1			 321
#define CS_KEY_KP_2			 322
#define CS_KEY_KP_3			 323
#define CS_KEY_KP_4			 324
#define CS_KEY_KP_5			 325
#define CS_KEY_KP_6			 326
#define CS_KEY_KP_7			 327
#define CS_KEY_KP_8			 328
#define CS_KEY_KP_9			 329
#define CS_KEY_KP_DECIMAL	 330
#define CS_KEY_KP_DIVIDE	 331
#define CS_KEY_KP_MULTIPLY	 332
#define CS_KEY_KP_SUBTRACT	 333
#define CS_KEY_KP_ADD		 334
#define CS_KEY_KP_ENTER		 335
#define CS_KEY_KP_EQUAL		 336
#define CS_KEY_LEFT_SHIFT	 340
#define CS_KEY_LEFT_CONTROL	 341
#define CS_KEY_LEFT_ALT		 342
#define CS_KEY_LEFT_SUPER	 343
#define CS_KEY_RIGHT_SHIFT	 344
#define CS_KEY_RIGHT_CONTROL 345
#define CS_KEY_RIGHT_ALT	 346
#define CS_KEY_RIGHT_SUPER	 347
#define CS_KEY_MENU			 348

#define CS_MOUSE_1 349
#define CS_MOUSE_2 350
#define CS_MOUSE_3 351
#define CS_MOUSE_4 352

// The last keu, and the number of keycodes
#define CS_KEY_LAST CS_MOUSE_4
#endif