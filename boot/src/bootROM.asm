SECTION "Boot", ROM0[$0000]
	LD SP,$fffe		
	XOR A			
	LD HL,$9fff		
clearVRAM:
	LD [HL-],A		
	BIT 7,H		
	JR NZ, clearVRAM	
	LD HL,$ff26		
	LD C,$11	
	LD A,$80		
	LD [HL-],A		
	LD [$FF00+C],A	
	INC C			
	LD A,$f3		
	LD [$FF00+C],A	
	LD [HL-],A		
	LD A,$77		
	LD [HL],A		
	LD A,$fc		
	LDH [$FF47],A	
	LD DE,$0104		
	LD HL,$8010		
decomp:
	LD A,[DE]		
	CALL $0095		
	CALL $0096		
	INC DE		
	LD A,E		
	CP $34		
	JR NZ, decomp	
	LD DE,$00d8		
	LD B,$08		
copyR:
	LD A,[DE]		
	INC DE		
	LD [HL+],A		
	INC HL		
	DEC B			
	JR NZ, copyR	
	LD A,$19		
	LD [$9910],A	
	LD HL,$992f		
tileMapRow:
	LD C,$0c		
tileMapWrite:
	DEC A			
	JR Z, logoScroll	
	LD [HL-],A		
	DEC C			
	JR NZ, tileMapWrite	
	LD L,$0f		
	JR tileMapRow	
logoScroll:
	LD H,A		
	LD A,$7A
	LD D,A		
	LDH [$FF43],A	
	LD A,$91		
	LDH [$FF40],A	
	INC B			
scrollFrames:
	LD E,$02		
scrollFrameDelay:
	LD C,$0c		
vblankWait:
	LDH A,[$FF44]	
	CP $90		
	JR NZ, vblankWait	
	DEC C			
	JR NZ, vblankWait	
	DEC E			
	JR NZ, scrollFrameDelay	
	LD C,$13		
	INC H			
	LD A,H		
	LD E,$83		
	CP $62		
	JR Z, bootSound
	LD E,$c1		
	CP $64		
	JR NZ, updateScrollPos	
bootSound:
	LD A,E	
	LD [$FF00+C],A	
	INC C			
	LD A,$87		
	LD [$FF00+C],A	
updateScrollPos:
	LDH A,[$FF43]	
	SUB B			
	LDH [$FF43],A	
	DEC D			 
	JR NZ, scrollFrames	
	DEC B			
	JR NZ, checkLogo	
	LD D,$20		
	JR scrollFrames	
	LD C,A		
	LD B,$04		
decompNibble:
	PUSH BC		
	RL C			
	RLA			
	POP BC		
	RL C			
	RLA			
	DEC B		
	JR NZ, decompNibble	
	LD [HL+],A		
	INC HL		
	LD [HL+],A		
	INC HL		
	RET			
logoData:
	DB $CE,$ED,$66,$66,$CC,$0D,$00,$0B,$03,$73,$00,$83,$00,$0C,$00,$0D 
	DB $00,$08,$11,$1F,$88,$89,$00,$0E,$DC,$CC,$6E,$E6,$DD,$DD,$D9,$99 
	DB $BB,$BB,$67,$63,$6E,$0E,$EC,$CC,$DD,$DC,$99,$9F,$BB,$B9,$33,$3E 
regSymbol:
	DB $3C,$42,$B9,$A5,$B9,$A5,$42,$3C
checkLogo:	
	LD HL,$0104		
	LD DE,$00a8		
cmpLogo:
	LD A,[DE]		
	INC DE		
	CP [HL]		
	JR NZ,$fe		
	INC HL		
	LD A,L		
	CP $34		
	JR NZ, cmpLogo	
	LD B,$19		
	LD A,B		
checksum:
	ADD [HL]		
	INC HL		
	DEC B			
	JR NZ, checksum	
	ADD [HL]		
	JR NZ,$fe		
	LD A,$01		
	LDH [$FF50],a
ds $100 - @
ASSERT @ == $100