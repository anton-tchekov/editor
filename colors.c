enum
{
	CHAR_VISIBLE_SPACE = ' ', /* 128, */
	CHAR_TAB_START     = '-', /* 129, */
	CHAR_TAB_MIDDLE    = '-', /* 130, */
	CHAR_TAB_END       = '>', /* 131, */
	CHAR_TAB_BOTH      = '>', /* 132, */
	CHAR_MAX
};

#define COLOR_BG       0xFF1F1F1F /* Background */
#define COLOR_FG       0xFFFFFFFF /* Foreground */
#define COLOR_GRAY     0xFF777777 /* Gray / Line Number / Visible Space */
#define COLOR_COMMENT  0xFFFF6A55 /* Comment */
#define COLOR_NUMBER   0xFF9DFE88 /* Number */
#define COLOR_STRING   0xFFCE9178 /* String */
#define COLOR_TYPE     0xFF569CD6 /* Type */
#define COLOR_KEYWORD  0xFFC586C0 /* Keyword */
#define COLOR_BRACE    0xFFFFD700 /* Brace */
#define COLOR_BRACKET  0xFFDA70D6 /* Bracket */
#define COLOR_PAREN    0xFF179FFF /* Parenthesis */
#define COLOR_FN       0xFFDCDCAA /* Function Identifier */
#define COLOR_ARRAY    0xFF9CDCFE /* Array Identifier */
#define COLOR_SEL      0xFF264F78 /* Selection */
#define COLOR_INFO     0xFF3C88CF /* Info */
#define COLOR_ERROR    0xFFFF0000 /* Error */
