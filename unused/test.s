;******************** (C) COPYRIGHT HAW-Hamburg ********************************
;* File Name          : main.s
;* Author             : Silke Behn
;* Version            : V1.0
;* Date               : 01.06.2021
;* Description        : This is a simple main.
;*
;* Tetris in ARM Assembly
;*  ~ Anton Tchekov
;*******************************************************************************

	EXTERN initITSboard
	EXTERN GUI_init

;********************************************
; Data section, aligned on 4-byte boundary
;********************************************

	AREA MyData, DATA, align = 2

; Constants
PERIPH_BASE        EQU 0x40000000
AHB1PERIPH_BASE    EQU (PERIPH_BASE + 0x20000)
GPIOF_BASE         EQU (AHB1PERIPH_BASE + 0x1400)
GPIOF_IDR          EQU (GPIOF_BASE + 0x10)

; LCD DC
GPIOF_BSRR         EQU (GPIOF_BASE + 0x18)
GPIO_PIN_13        EQU 0x2000

; LCD CS
GPIOD_BASE         EQU (AHB1PERIPH_BASE + 0x0C00)
GPIOD_BSRR         EQU (GPIOD_BASE + 0x18)
GPIO_PIN_14        EQU 0x4000

; SPI
APB2PERIPH_BASE    EQU (PERIPH_BASE + 0x10000)
SPI1_BASE          EQU (APB2PERIPH_BASE + 0x3000)
SPI1_SR            EQU (SPI1_BASE + 0x08)
SPI1_DR            EQU (SPI1_BASE + 0x0C)

SPI_SR_RXNE        EQU (1 << 0)
SPI_SR_TXE         EQU (1 << 1)
SPI_SR_BSY         EQU (1 << 7)

DEFAULT_BRIGHTNESS EQU 800

DEBOUNCE_TICKS     EQU 5

FIELD_WIDTH        EQU 10
FIELD_HEIGHT       EQU 20
BLOCK_SIZE         EQU 16
PIXEL_WIDTH        EQU (FIELD_WIDTH * BLOCK_SIZE)
PIXEL_HEIGHT       EQU (FIELD_HEIGHT * BLOCK_SIZE)
FIELD_BYTES        EQU (FIELD_WIDTH * FIELD_HEIGHT)

PIECE_X            EQU 0
PIECE_Y            EQU 1
PIECE_ROTATION     EQU 2
PIECE_TYPE         EQU 3

pieces
	; 0
	DCW 0x0000, 0x0000, 0x0000, 0x0000
	DCB 0xFF, 0xFF, 0xFF, 0x00 ; White

	; I
	DCW 0x0F00, 0x2222, 0x00F0, 0x4444
	DCB 0x00, 0xFF, 0xFF, 0x00 ; Cyan

	; J
	DCW 0x44C0, 0x8E00, 0x6440, 0x0E20
	DCB 0x00, 0x00, 0xFF, 0x00 ; Blue

	; L
	DCW 0x4460, 0x0E80, 0xC440, 0x2E00
	DCB 0xFF, 0xC0, 0x00, 0x00 ; Orange

	; O
	DCW 0x0660, 0x0660, 0x0660, 0x0660
	DCB 0xFF, 0xFF, 0x00, 0x00 ; Yellow

	; S
	DCW 0x06C0, 0x8C40, 0x6C00, 0x4620
	DCB 0x00, 0xFF, 0x00, 0x00 ; Green

	; T
	DCW 0x0E40, 0x4C40, 0x4E00, 0x4640
	DCB 0xFF, 0x00, 0xFF, 0x00 ; Purple

	; Z
	DCW 0x0C60, 0x4C80, 0xC600, 0x2640
	DCB 0xFF, 0x00, 0x00, 0x00 ; Red

; Variables
bag                DCB  1, 2, 3, 4, 5, 6, 7
bag_idx            DCB  7
rng_state          DCD  314159265
field              FILL FIELD_BYTES
cur_field          FILL FIELD_BYTES
prev_field         FILL FIELD_BYTES
field_anim0        FILL FIELD_BYTES
field_anim1        FILL FIELD_BYTES
current_piece
cp_x               DCB  4
cp_y               DCB  0
cp_rotation        DCB  0
cp_type            DCB  6
key_cnt            FILL 3
ticks              DCB  0
color_rgb565       DCW  0x0000
score              DCW  0

; Fonts
digits
	DCW 0xC003, 0x6006, 0x300C, 0x1FF8, 0x1FF8, 0x1FF8, 0x1FF8, 0x1FF8
	DCW 0x1FF8, 0x1FF8, 0x1FF8, 0x1FF8, 0x3FFC, 0x7FFE, 0xFFFF, 0xFFFF
	DCW 0xFFFF, 0x7FFE, 0x3FFC, 0x1FF8, 0x1FF8, 0x1FF8, 0x1FF8, 0x1FF8
	DCW 0x1FF8, 0x1FF8, 0x1FF8, 0x1FF8, 0x1FF8, 0x300C, 0x6006, 0xC003

	DCW 0xFFFF, 0xFFFE, 0xFFFC, 0xFFF8, 0xFFF8, 0xFFF8, 0xFFF8, 0xFFF8
	DCW 0xFFF8, 0xFFF8, 0xFFF8, 0xFFF8, 0xFFFC, 0xFFFE, 0xFFFF, 0xFFFF
	DCW 0xFFFF, 0xFFFE, 0xFFFC, 0xFFF8, 0xFFF8, 0xFFF8, 0xFFF8, 0xFFF8
	DCW 0xFFF8, 0xFFF8, 0xFFF8, 0xFFF8, 0xFFF8, 0xFFFC, 0xFFFE, 0xFFFF

	DCW 0xC003, 0xE006, 0xF00C, 0xFFF8, 0xFFF8, 0xFFF8, 0xFFF8, 0xFFF8
	DCW 0xFFF8, 0xFFF8, 0xFFF8, 0xFFF8, 0xFFFC, 0xFFFE, 0xC003, 0x8001
	DCW 0xC003, 0x7FFF, 0x3FFF, 0x1FFF, 0x1FFF, 0x1FFF, 0x1FFF, 0x1FFF
	DCW 0x1FFF, 0x1FFF, 0x1FFF, 0x1FFF, 0x1FFF, 0x300F, 0x6007, 0xC003

	DCW 0xC003, 0xE006, 0xF00C, 0xFFF8, 0xFFF8, 0xFFF8, 0xFFF8, 0xFFF8
	DCW 0xFFF8, 0xFFF8, 0xFFF8, 0xFFF8, 0xFFFC, 0xFFFE, 0xC003, 0x8001
	DCW 0xC003, 0xFFFE, 0xFFFC, 0xFFF8, 0xFFF8, 0xFFF8, 0xFFF8, 0xFFF8
	DCW 0xFFF8, 0xFFF8, 0xFFF8, 0xFFF8, 0xFFF8, 0xF00C, 0xE006, 0xC003

	DCW 0xFFFF, 0x7FFE, 0x3FFC, 0x1FF8, 0x1FF8, 0x1FF8, 0x1FF8, 0x1FF8
	DCW 0x1FF8, 0x1FF8, 0x1FF8, 0x1FF8, 0x3FFC, 0x7FFE, 0xC003, 0x8001
	DCW 0xC003, 0xFFFE, 0xFFFC, 0xFFF8, 0xFFF8, 0xFFF8, 0xFFF8, 0xFFF8
	DCW 0xFFF8, 0xFFF8, 0xFFF8, 0xFFF8, 0xFFF8, 0xFFFC, 0xFFFE, 0xFFFF

	DCW 0xC003, 0x6007, 0x300F, 0x1FFF, 0x1FFF, 0x1FFF, 0x1FFF, 0x1FFF
	DCW 0x1FFF, 0x1FFF, 0x1FFF, 0x1FFF, 0x3FFF, 0x7FFF, 0xC003, 0x8001
	DCW 0xC003, 0xFFFE, 0xFFFC, 0xFFF8, 0xFFF8, 0xFFF8, 0xFFF8, 0xFFF8
	DCW 0xFFF8, 0xFFF8, 0xFFF8, 0xFFF8, 0xFFF8, 0xF00C, 0xE006, 0xC003

	DCW 0xC003, 0x6007, 0x300F, 0x1FFF, 0x1FFF, 0x1FFF, 0x1FFF, 0x1FFF
	DCW 0x1FFF, 0x1FFF, 0x1FFF, 0x1FFF, 0x3FFF, 0x7FFF, 0xC003, 0x8001
	DCW 0xC003, 0x7FFE, 0x3FFC, 0x1FF8, 0x1FF8, 0x1FF8, 0x1FF8, 0x1FF8
	DCW 0x1FF8, 0x1FF8, 0x1FF8, 0x1FF8, 0x1FF8, 0x300C, 0x6006, 0xC003

	DCW 0xC003, 0xE006, 0xF00C, 0xFFF8, 0xFFF8, 0xFFF8, 0xFFF8, 0xFFF8
	DCW 0xFFF8, 0xFFF8, 0xFFF8, 0xFFF8, 0xFFFC, 0xFFFE, 0xFFFF, 0xFFFF
	DCW 0xFFFF, 0xFFFE, 0xFFFC, 0xFFF8, 0xFFF8, 0xFFF8, 0xFFF8, 0xFFF8
	DCW 0xFFF8, 0xFFF8, 0xFFF8, 0xFFF8, 0xFFF8, 0xFFFC, 0xFFFE, 0xFFFF

	DCW 0xC003, 0x6006, 0x300C, 0x1FF8, 0x1FF8, 0x1FF8, 0x1FF8, 0x1FF8
	DCW 0x1FF8, 0x1FF8, 0x1FF8, 0x1FF8, 0x3FFC, 0x7FFE, 0xC003, 0x8001
	DCW 0xC003, 0x7FFE, 0x3FFC, 0x1FF8, 0x1FF8, 0x1FF8, 0x1FF8, 0x1FF8
	DCW 0x1FF8, 0x1FF8, 0x1FF8, 0x1FF8, 0x1FF8, 0x300C, 0x6006, 0xC003

	DCW 0xC003, 0x6006, 0x300C, 0x1FF8, 0x1FF8, 0x1FF8, 0x1FF8, 0x1FF8
	DCW 0x1FF8, 0x1FF8, 0x1FF8, 0x1FF8, 0x3FFC, 0x7FFE, 0xC003, 0x8001
	DCW 0xC003, 0xFFFE, 0xFFFC, 0xFFF8, 0xFFF8, 0xFFF8, 0xFFF8, 0xFFF8
	DCW 0xFFF8, 0xFFF8, 0xFFF8, 0xFFF8, 0xFFF8, 0xF00C, 0xE006, 0xC003


;********************************************
; Code section, aligned on 8-byte boundary
;********************************************

	AREA |.text|, CODE, READONLY, ALIGN = 3

;--------------------------------------------
; main subroutine
;--------------------------------------------
	EXPORT main [CODE]

main PROC
	BL    initITSboard
	MOV   R0, #DEFAULT_BRIGHTNESS
	BL    GUI_init

	BL    draw_grid
	BL    new_piece

	MOV  R0, #240
	MOV  R1, #16
	MOV  R2, #0
	BL   lcd_digit

	MOV  R0, #260
	MOV  R1, #16
	MOV  R2, #0
	BL   lcd_digit

	BL   draw_score

forever
	LDR   R0, =GPIOF_IDR
	LDR   R0, [R0]

	; button press debounce
	LDR   R1, =key_cnt
	MOV   R3, #2
	MOV   R4, #4
	LDR   R7, =current_piece

button_loop
	TST   R0, R4
	BNE   button_release

	LDRB  R2, [R1, R3]
	CMP   R2, #DEBOUNCE_TICKS
	BEQ   button_next
	ADD   R2, #1
	CMP   R2, #DEBOUNCE_TICKS
	BNE   button_next

	; button press
	CMP   R3, #2
	BNE   button_rotate


	LDRSB  R5, [R7, #PIECE_X]
	SUB   R6, R5, #1
	STRB  R6, [R7, #PIECE_X]

	PUSH  { R0-R3 }
	LDR   R0, =field
	LDR   R1, =current_piece
	BL    valid_position
	CMP   R0, #0
	POP   { R0-R3 }
	MOVNE R6, R5
	STRB  R6, [R7, #PIECE_X]
	B     button_next

button_rotate
	CMP   R3, #1
	BNE   button_right

	LDRB  R5, [R7, #PIECE_ROTATION]
	ADD   R6, R5, #1
	CMP   R6, #4
	MOVEQ R6, #0

	STRB  R6, [R7, #PIECE_ROTATION]

	PUSH  { R0-R3 }
	LDR   R0, =field
	LDR   R1, =current_piece
	BL    valid_position
	CMP   R0, #0
	POP   { R0-R3 }
	MOVNE R6, R5
	STRB  R6, [R7, #PIECE_ROTATION]
	B     button_next

button_right
	CMP   R3, #0
	BNE   button_next

	LDRSB  R5, [R7, #PIECE_X]
	ADD   R6, R5, #1
	STRB  R6, [R7, #PIECE_X]

	PUSH  { R0-R3 }
	LDR   R0, =field
	LDR   R1, =current_piece
	BL    valid_position
	CMP   R0, #0
	POP   { R0-R3 }
	MOVNE R6, R5
	STRB  R6, [R7, #PIECE_X]
	B     button_next

button_release
	MOV   R2, #0

button_next
	STRB  R2, [R1, R3]

	SUB   R3, #1
	LSRS  R4, #1
	BNE   button_loop

	; ticks
	LDR   R0, =ticks
	LDRB  R1, [R0]
	ADD   R1, #1
	STRB  R1, [R0]
	CMP   R1, #200
	BNE   game_loop_skip

	; update
	MOV   R1, #0
	STRB  R1, [R0]

	BL    update_field
	BL    draw_field

	LDR   R0, =cp_y
	LDRB  R1, [R0]
	ADD   R1, #1
	STRB  R1, [R0]

	PUSH  { R0-R3 }
	LDR   R0, =field
	LDR   R1, =current_piece
	BL    valid_position
	CMP   R0, #0
	POP   { R0-R3 }
	BEQ   game_loop_skip


	SUB   R1, #1
	STRB  R1, [R0]
	LDR   R0, =field
	BL    to_field
	BL    field_rows
	BL    new_piece

	LDR   R0, =field
	LDR   R1, =current_piece
	BL    valid_position
	CMP   R0, #0
	POP   { R0-R3 }
	BEQ   game_loop_skip
	BL    clear_field

	LDR   R0, =score
	MOV   R1, #0
	STRH  R1, [R0]
	BL    draw_score

game_loop_skip
	BL   delay
	B    forever

	ENDP


; --- DRAW SCORE ---
; Divide by 10: (x * 205) >> 11
draw_score
	PUSH { LR }

	LDR  R4, =score
	LDRH R4, [R4]

	MOV  R5, #205
	MOV  R6, #10

	MUL  R7, R4, R5
	LSR  R7, #11
	MUL  R2, R7, R6
	SUB  R2, R4, R2

	MOV  R0, #220
	MOV  R1, #16
	BL   lcd_digit

	MOV  R4, R7

	MUL  R7, R4, R5
	LSR  R7, #11
	MUL  R2, R7, R6
	SUB  R2, R4, R2

	MOV  R0, #200
	MOV  R1, #16
	BL   lcd_digit

	MOV  R2, R7

	MOV  R0, #180
	MOV  R1, #16
	BL   lcd_digit

	POP  { PC }

; --- DELAY LONG ---
delay_long
	MOV  R0, #0x500000

loopl
	SUBS R0, #1
	BNE  loopl

	BX   LR


; --- DELAY ---
delay
	MOV  R0, #0x10000

loop
	SUBS R0, #1
	BNE  loop

	BX   LR


; --- FIELD ROWS ---
field_rows
	PUSH { LR }

	LDR  R0, =field
	LDR  R1, =field_anim0
	BL   copy_field

	LDR  R0, =field
	LDR  R1, =field_anim1
	BL   copy_field

	LDR  R10, =field_anim1

	LDR  R4, =field
	MOV  R0, #0                  ; y = 0
	MOV  R9, #0

rows_y_loop
	CMP  R0, #FIELD_BYTES        ; y < FIELD_BYTES
	BNE  rows_continue

	CMP  R9, #0
	BEQ  rows_finish


	PUSH { R9 }

	MOV  R12, #3

three_times

	PUSH { R12 }

	LDR  R0, =field_anim1
	LDR  R1, =cur_field
	BL   copy_field
	BL   draw_field
	BL   delay_long

	LDR  R0, =field_anim0
	LDR  R1, =cur_field
	BL   copy_field
	BL   draw_field
	BL   delay_long

	POP  { R12 }

	SUBS R12, #1
	BNE  three_times

	POP  { R9 }

	MOV  R0, #1
	SUB  R9, #1
	LSL  R0, R9
	LDR  R1, =score
	LDRH R2, [R1]
	ADD  R2, R0
	STRH R2, [R1]
	BL   draw_score


rows_finish
	POP  { PC }

rows_continue
	MOV  R1, #0                  ; x = 0

rows_x_loop
	CMP  R1, #FIELD_WIDTH        ; x == FIELD_WIDTH
	BEQ  rows_x_end

	ADD  R6, R0, R1              ; field[y + x] == 0 ?
	LDRB R5, [R4, R6]
	CMP  R5, #0
	BEQ  rows_x_end

	ADD  R1, #1                  ; ++x
	B    rows_x_loop

rows_x_end
	CMP  R1, #FIELD_WIDTH        ; x == FIELD_WIDTH
	BNE  shift_field_end

	ADD  R9, #1

	; Clear line for animation
	MOV  R5, #0
	ADD  R2, R0, #10

clear_line
	SUB  R2, #1
	STRB R5, [R10, R2]
	CMP  R2, R0
	BNE  clear_line

	; Shift field down
	SUBS R2, R0, #1
	ADD  R3, R2, #FIELD_WIDTH

shift_field
	BMI  shift_field_end
	LDRB R5, [R4, R2]
	STRB R5, [R4, R3]
	SUB  R3, #1
	SUBS R2, #1
	B    shift_field

shift_field_end
	ADD  R0, #FIELD_WIDTH
	B    rows_y_loop


; --- CLEAR FIELD ---
clear_field
	MOV   R0, #0
	LDR   R1, =field
	MOV   R2, #(FIELD_BYTES - 4)

clear_field_loop
	STR   R0, [R1, R2]
	SUBS  R2, #4
	BPL   clear_field_loop

	BX    LR


; --- UPDATE FIELD ---
update_field
	PUSH  { LR }

	LDR   R0, =field
	LDR   R1, =cur_field
	BL    copy_field

	LDR   R0, =cur_field
	BL    to_field

	POP   { PC }


	LTORG


; --- COPY FIELD ---
; R0: src | R1: dst
copy_field
	MOV   R2, #(FIELD_BYTES - 4)

copy_field_loop
	LDR   R3, [R0, R2]
	STR   R3, [R1, R2]
	SUBS  R2, #4
	BPL   copy_field_loop

	BX    LR


; --- DRAW FIELD ---
draw_field
	PUSH  { LR }

	MOV   R4, #0
	MOV   R6, #1

	LDR   R8, =(pieces + 8)
	LDR   R9, =cur_field

draw_field_y_loop
	MOV   R5, #0
	MOV   R7, #1

draw_field_x_loop
	; get piece type
	LDRB  R10, [R9]
	LDRB  R11, [R9, #(prev_field - cur_field)]
	CMP   R10, R11
	BEQ   draw_field_skip

	; get piece color offset (base + 12 * type)
	LSL   R12, R10, #3 ; * 8
	LSL   R11, R10, #2 ; * 4
	ADD   R11, R12     ; * x * 8 + x * 4 = x * 12
	ADD   R11, R8      ; + base

	; choose color
	LDR   R0, [R11]
	LDR   R1, [R11, #1]
	LDR   R2, [R11, #2]
	BL    lcd_color

	; draw rect
	MOV   R0, R7
	MOV   R1, R6
	MOV   R2, #(BLOCK_SIZE - 1)
	MOV   R3, #(BLOCK_SIZE - 1)
	BL    lcd_rect

draw_field_skip
	ADD   R9, #1
	ADD   R5, #1
	ADD   R7, #BLOCK_SIZE

	CMP   R5, #FIELD_WIDTH
	BLT   draw_field_x_loop

	ADD   R4, #1
	ADD   R6, #BLOCK_SIZE

	CMP   R4, #FIELD_HEIGHT
	BLT   draw_field_y_loop

	LDR   R0, =cur_field
	LDR   R1, =prev_field
	BL    copy_field

	POP   { PC }


; --- DRAW GRID ---
draw_grid
	PUSH  { LR }

	; Set color to black
	MOV   R0, #0                      ; R
	MOV   R1, #0                      ; G
	MOV   R2, #0                      ; B
	BL    lcd_color

	; Vertical lines
	MOV   R4, #0                      ; X
	LDR   R8, =PIXEL_WIDTH            ; Max X

draw_grid_vertical_loop
	MOV   R0, R4                      ; X
	MOV   R1, #0                      ; Y
	MOV   R2, #1                      ; W
	LDR   R3, =PIXEL_HEIGHT           ; H
	BL    lcd_rect

	ADD   R4, #BLOCK_SIZE             ; X += W
	CMP   R4, R8
	BLE   draw_grid_vertical_loop

	; Horizontal lines
	MOV   R4, #0                      ; Y
	LDR   R8, =PIXEL_HEIGHT           ; Max Y

draw_grid_horizontal_loop
	MOV   R0, #0                      ; X
	MOV   R1, R4                      ; Y
	LDR   R2, =PIXEL_WIDTH            ; W
	MOV   R3, #1                      ; H
	BL    lcd_rect

	ADD   R4, #BLOCK_SIZE             ; Y += H
	CMP   R4, R8
	BLE   draw_grid_horizontal_loop

	POP   { PC }


; --- TO FIELD ---
to_field
	MOV   R8, R0                    ; R0 = field
	LDR   R0, =current_piece

	LDRSB R1, [R0, #PIECE_X]        ; R1 = col
	LDRSB R2, [R0, #PIECE_Y]        ; R2 = row
	ADD   R3, R1, #4

	LDRB  R4, [R0, #PIECE_TYPE]     ; R4 = current_piece.type
	LDR   R5, =pieces               ; R5 = pieces[current_piece.type].blocks[current_piece.rotation]
	LSL   R6, R4, #3                ; type * 8
	LSL   R7, R4, #2                ; type * 4
	ADD   R6, R7                    ; type * 8 + type * 4 = type * 12
	ADD   R5, R6                    ; base + offset

	LDRB  R6, [R0, #PIECE_ROTATION]
	LDRH  R5, [R5, R6, LSL #1]

	MOV   R7, #0x8000               ; R7 = bit

	MOV   R9, #FIELD_WIDTH

to_field_loop
	TST   R5, R7
	BEQ   to_field_skip

	MUL   R10, R2, R9
	ADD   R10, R1
	STRB  R4, [R8, R10]

to_field_skip
	ADD   R1, #1
	CMP   R1, R3
	BNE   to_field_next
	SUB   R1, #4
	ADD   R2, #1

to_field_next
	LSRS  R7, #1
	BNE   to_field_loop

	BX    LR


; --- VALID POSITION ---
valid_position
	PUSH  { R4-R10, LR }

	LDR   R8, =field
	LDR   R0, =current_piece

	LDRSB R1, [R0, #PIECE_X]        ; R1 = col
	LDRSB R2, [R0, #PIECE_Y]        ; R2 = row
	ADD   R3, R1, #4

	LDRB  R4, [R0, #PIECE_TYPE]     ; R4 = current_piece.type
	LDR   R5, =pieces               ; R5 = pieces[current_piece.type].blocks[current_piece.rotation]
	LSL   R6, R4, #3                ; type * 8
	LSL   R7, R4, #2                ; type * 4
	ADD   R6, R7                    ; type * 8 + type * 4 = type * 12
	ADD   R5, R6                    ; base + offset

	LDRB  R6, [R0, #PIECE_ROTATION]
	LDRH  R5, [R5, R6, LSL #1]

	MOV   R7, #0x8000               ; R7 = bit

	MOV   R9, #FIELD_WIDTH

valid_position_loop
	TST   R5, R7
	BEQ   valid_position_skip

	CMP   R1, #0
	BLT   valid_position_fail

	CMP   R2, #0
	BLT   valid_position_fail

	CMP   R1, #FIELD_WIDTH
	BGE   valid_position_fail

	CMP   R2, #FIELD_HEIGHT
	BGE   valid_position_fail

	MUL   R10, R2, R9
	ADD   R10, R1
	LDRB  R4, [R8, R10]
	CMP   R4, #0
	BNE   valid_position_fail


valid_position_skip
	ADD   R1, #1
	CMP   R1, R3
	BNE   valid_position_next
	SUB   R1, #4
	ADD   R2, #1

valid_position_next
	LSRS  R7, #1
	BNE   valid_position_loop

	MOV   R0, #0                    ; return valid
	POP   { R4-R10, PC }

valid_position_fail
	MOV   R0, #1                    ; return fail
	POP   { R4-R10, PC }


; --- NEW PIECE ---
new_piece
	PUSH  { LR }

	LDR   R4, =current_piece

	MOV   R5, #4
	STRB  R5, [R4, #PIECE_X]
	MOV   R5, #0
	STRB  R5, [R4, #PIECE_Y]
	MOV   R5, #0
	STRB  R5, [R4, #PIECE_ROTATION]

	LDR   R5, =bag
	LDR   R9, =bag_idx
	LDRB  R10, [R9]
	CMP   R10, #7
	BNE   next_idx

	; Shuffle
	MOV   R8, #6

shuffle_loop
	BL    rand
	AND   R0, #0x07
	CMP   R0, #7
	MOVEQ R0, #0

	; Swap
	LDRB  R6, [R5, R8]
	LDRB  R7, [R5, R0]

	STRB  R6, [R5, R0]
	STRB  R7, [R5, R8]

	SUBS  R8, #1
	BPL   shuffle_loop

	MOV   R10, #0

next_idx
	LDRB  R0, [R5, R10]
	STRB  R0, [R4, #PIECE_TYPE]
	ADD   R10, #1
	STRB  R10, [R9]

	POP   { PC }


; --- RAND ---
; xorshift random number generator
; R0: pseudo random value
rand
	LDR   R1, =rng_state
	LDR   R0, [R1]

	LSL   R2, R0, #13
	EOR   R0, R2

	LSR   R2, R0, #17
	EOR   R0, R2

	LSL   R2, R0, #5
	EOR   R0, R2

	STR   R0, [R1]

	BX    LR


; --- LCD COLOR ---
; R0: Red, R1: Green, R2: Blue | RGB888 -> RGB565
lcd_color
	LDR   R3, =color_rgb565

	AND   R0, #0xF8
	LSL   R0, #8

	AND   R1, #0xFC
	LSL   R1, #3

	LSR   R2, #3

	ORR   R0, R1
	ORR   R0, R2

	STRH  R0, [R3]

	BX    LR

; --- LCD PIN ---
lcd_cs_0
	LDR   R0, =GPIOD_BSRR
	MOV   R1, #(GPIO_PIN_14 << 16)
	STR   R1, [R0]
	BX    LR

lcd_cs_1
	LDR   R0, =GPIOD_BSRR
	MOV   R1, #GPIO_PIN_14
	STR   R1, [R0]
	BX    LR

lcd_dc_0
	LDR   R0, =GPIOF_BSRR
	MOV   R1, #(GPIO_PIN_13 << 16)
	STR   R1, [R0]
	BX    LR

lcd_dc_1
	LDR   R0, =GPIOF_BSRR
	MOV   R1, #GPIO_PIN_13
	STR   R1, [R0]
	BX    LR


; --- LCD WRITE BYTE ---
lcd_write_byte
	LDR   R1, =SPI1_SR
	LDR   R2, =SPI1_DR

txe_loop
	LDR   R3, [R1]
	AND   R3, #0xFF
	ANDS  R3, #SPI_SR_TXE
	BEQ   txe_loop

	AND   R0, #0xFF
	STR   R0, [R2]

rxne_loop
	LDR   R3, [R1]
	AND   R3, #0xFF
	ANDS  R3, #SPI_SR_RXNE
	BEQ   rxne_loop

bsy_loop
	LDR   R3, [R1]
	AND   R3, #0xFF
	ANDS  R3, #SPI_SR_BSY
	BNE   bsy_loop

	LDR   R0, [R2]
	AND   R0, #0xFF
	BX    LR

; --- LCD WRITE PARAM ---
lcd_write_param
	PUSH  { LR, R4 }
	MOV   R4, R0

	BL    lcd_dc_1
	BL    lcd_cs_0

	LSR   R0, R4, #8
	BL    lcd_write_byte

	AND   R0, R4, #0xFF
	BL    lcd_write_byte

	BL    lcd_cs_1
	POP   { R4, PC }


; --- LCD WRITE CMD ---
lcd_write_cmd
	PUSH  { LR, R4 }
	MOV   R4, R0

	BL    lcd_dc_0
	BL    lcd_cs_0

	MOV   R0, R4
	BL    lcd_write_byte

	BL    lcd_cs_1
	POP   { R4, PC }



; --- LCD DIGIT ---
; R0: X, R1: Y, R2: Digit
lcd_digit
	PUSH { LR, R4-R8 }

	MOV   R4, R0
	MOV   R5, R1
	MOV   R6, R2

	; X
	MOV   R0, #0x2A
	BL    lcd_write_cmd

	LSR   R0, R4, #8
	AND   R0, #0xFF
	BL    lcd_write_param

	AND   R0, R4, #0xFF
	BL    lcd_write_param

	; X + W - 1
	ADD   R8, R4, #(16 - 1)

	LSR   R0, R8, #8
	AND   R0, #0xFF
	BL    lcd_write_param

	AND   R0, R8, #0xFF
	BL    lcd_write_param

	; Y
	MOV   R0, #0x2B
	BL    lcd_write_cmd

	LSR   R0, R5, #8
	AND   R0, #0xFF
	BL    lcd_write_param

	AND   R0, R5, #0xFF
	BL    lcd_write_param

	; Y + Height - 1
	ADD   R8, R5, #(32 - 1)

	LSR   R0, R8, #8
	AND   R0, #0xFF
	BL    lcd_write_param

	AND   R0, R8, #0xFF
	BL    lcd_write_param

	; Data
	MOV   R0, #0x2C
	BL    lcd_write_cmd

	BL    lcd_dc_1
	BL    lcd_cs_0

	LDR   R7, =digits
	LSL   R6, #6
	ADD   R7, R6

	MOV   R5, #32
	MOV   R4, #0

lcd_digit_loop
	LDRH  R8, [R7, R4]
	MOV   R6, #16

digit_row_loop
	ANDS  R0, R8, #0x8000
	MOVNE R0, #0xFF
	BL    lcd_write_byte
	ANDS  R0, R8, #0x8000
	MOVNE R0, #0xFF
	BL    lcd_write_byte

	LSL   R8, #1
	SUBS  R6, #1
	BNE   digit_row_loop

	ADD   R4, #2
	SUBS  R5, #1
	BNE   lcd_digit_loop

	BL    lcd_cs_1

	POP   { R4-R8, PC }



; --- LCD RECT ---
; R0: X, R1: Y, R2: Width, R3: Height
lcd_rect
	PUSH { LR, R4-R8 }

	MOV   R4, R0
	MOV   R5, R1
	MOV   R6, R2
	MOV   R7, R3

	; X
	MOV   R0, #0x2A
	BL    lcd_write_cmd

	LSR   R0, R4, #8
	AND   R0, #0xFF
	BL    lcd_write_param

	AND   R0, R4, #0xFF
	BL    lcd_write_param

	; X + W - 1
	ADD   R8, R4, R6
	SUB   R8, #1

	LSR   R0, R8, #8
	AND   R0, #0xFF
	BL    lcd_write_param

	AND   R0, R8, #0xFF
	BL    lcd_write_param

	; Y
	MOV   R0, #0x2B
	BL    lcd_write_cmd

	LSR   R0, R5, #8
	AND   R0, #0xFF
	BL    lcd_write_param

	AND   R0, R5, #0xFF
	BL    lcd_write_param

	; Y + Height - 1
	ADD   R8, R5, R7
	SUB   R8, #1

	LSR   R0, R8, #8
	AND   R0, #0xFF
	BL    lcd_write_param

	AND   R0, R8, #0xFF
	BL    lcd_write_param

	; Data
	MOV   R0, #0x2C
	BL    lcd_write_cmd

	BL    lcd_dc_1
	BL    lcd_cs_0

	; Load Color
	LDR   R8, =color_rgb565
	LDRB  R4, [R8]
	LDRB  R5, [R8, #1]

	MUL   R8, R6, R7 ; Width * Height

lcd_rect_loop
	MOV   R0, R5
	BL    lcd_write_byte
	MOV   R0, R4
	BL    lcd_write_byte

	SUBS  R8, #1
	BNE   lcd_rect_loop

	BL    lcd_cs_1

	POP   { R4-R8, PC }

	ALIGN
	END
