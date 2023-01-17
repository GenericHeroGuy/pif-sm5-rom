architecture n64.pif

// page 0 (reset vector)
origin $000

	lax 1
	lblx 5
	out        //write 1 to P5
	lblx 14
	out        //write 1 to RE (enable interrupt A)
	trs TRS10  // L0D_31
	lbmx 3
	lblx 4
	trs L01_16 // [$34..$3f] = 0
	lbmx 5
	lblx 14
// [$5e] = CIC region ID
	trs TRS0E  // L0C_00

	decb
	lax 1
	tam
	tr L00_12
// ID == 1
	lax 4
	tr L00_16

L00_12:
	lax 9
	tam
	trs TRS0C // L03_39
// ID == 9
	lax 12

L00_16:
	exc 0
	lbmx 8
	lblx 0
	tr L00_1B

L00_1A:
	exbm
L00_1B:
	trs L01_16 // [$80..$ff] = 0
	exbm
	adx 1
	tr L00_1A

	lbmx 1
	lblx 10
// [$1a..$1f] = CIC seed
L00_21:
// read nibble of CIC seed
	trs TRS0E // L0C_00
	tr L00_21

// encode seed
	lblx 10
	trs TRS0A // L0E_15
	lblx 10
	trs TRS0A // L0E_15
	lbmx 5
	lblx 14
	lda 0
	rm 1
	rm 3
	lbmx 1
	lblx 11
	excd 0
	rc

L00_30:
// set bit 7 of PIF status byte
// (response to checksum verification?)
	lbmx 15
	lblx 14
	sm 3
	ie
	tl L05_00

// page 1 (TRS vectors)
origin $040

TRS00:
	tl L06_29
TRS02:
	tl L04_0E
TRS04:
	tl L0C_10
TRS06:
	tl L09_0D
TRS08:
	tl L0E_1B
TRS0A:
	tl L0E_15
TRS0C:
	tl L03_39
TRS0E:
	tl L0C_00
TRS10:
	tl L0D_31

// write $fb to [B] and zero rest of segment
L01_12:
	lax 15
	exci 0
	lax 11
	exci 0

// zero memory from B to end of segment
L01_16:
	lax 0
	exci 0
	tr L01_16
	rtn

// fill [$40..$45] with 8
L01_1A:
	lbmx 4
	lblx 5
L01_1C:
	lax 8
	excd 0
	tr L01_1C
	rtn

L01_20:
// set Bl to 5, trashing X
	exax
	lax 5
	exbl

	exax
	out // P5 <- A

// short delay
	lax 11
-;	adx 1
	tr -

	tr L01_35
	nop

// read bit from CIC into C
L01_2A:
// set Bl to 5, save old Bl in X
	lax 5
	exbl
	exax

	lax 3
	out   // P5 <- 3

// short delay
	lax 11
-;	adx 1
	tr -

// if bit 3 of P5 is set, set carry, else clear carry
	sc
	tpb 3
	rc

L01_35:
	lax 1
	out   // P5 <- 1

// another delay
	lax 12
-;	adx 1
	tr -

// restore Bl from X
	exax
	exbl
	rtn

// page 2 (interrupt vectors)
origin $080

ifa_int:
	ex
	exci 0
	tr L02_0E
	nop

ift_int:
	ex
	exc 0
	lblx 14
	rm 3
	lax 1
	out
	lblx 8
	lax 2
	out
	tr L02_3B

L02_0E:
	tpb 3
	tr L02_1D
	tpb 2
	tr L02_39
	lbmx 15
	lblx 15
	tm 1
	tl L07_13
	lbmx 5
	lblx 14
	tm 3
	tr L02_39
	tl L07_09
L02_1D:
	call L03_06

// if bit 0 of PIF status byte is set, run joybus protocol
	lbmx 15
	lblx 15
	tm 0
	tr SkipJoybus

	rm 0
	call L0F_2F
	lbmx 1
	lblx 0
	ex
	trs L01_1A
	call L09_2D
L02_2C:
	lbmx 5
	lblx 9
	sc
	tm 0
	rc
	decb
	excd 0
	exax
	call L0C_32
	tr L02_3B
SkipJoybus:
	lbmx 5
	tr L02_3B
L02_39:
	call L03_06
L02_3B:
	lblx 6
	exc 0
	ex
	rtni

// page 3 (standby exit vector)
origin $0C0

halt_exit:
	nop
	tm 3
	rtn
	lax 5
	out
	rtn

L03_06:
	lblx 14
	lax 1
	out
	halt
	tr halt_exit

L03_0B:
	ie
L03_0C:
	lbmx 5
	lblx 14
	tm 3
	tl L06_00
	tm 1
	tr L03_16
	rm 1
	tl L0D_00

L03_16:
	lax 2
	trs L01_20
	lax 2
	trs L01_20
	lbmx 6
	trs TRS08 // L0E_1B
	trs TRS08 // L0E_1B
	trs TRS08 // L0E_1B
	lbmx 7
	trs TRS08 // L0E_1B
	trs TRS08 // L0E_1B
	trs TRS08 // L0E_1B
	lbmx 7
	lblx 7
	lda 0
	adx 15
	lax 0
	adx 1
	exbl
L03_29:
	lbmx 6
	lax 3
	tm 0
	lax 2
	trs L01_20
	lbmx 7
	trs L01_2A
	tc
	tr L03_37
	tm 0
	tr L03_39
L03_34:
	incb
	tr L03_29
	tr L03_0C
L03_37:
	tm 0
	tr L03_34
L03_39:
	id
	lblx 8
L03_3B:
	out
	coma
	tr L03_3B

// page 4 (PAT data)
origin $100

	db $19
	db $4a
	db $f1
	db $88
	db $b5
	db $5a
	db $71
	db $c3
	db $de
	db $61
	db $10
	db $ed
	db $9e
	db $8c

// swap internal and external memory
// [$1b..$1f] <-> [$cb..$cf]
// [$34..$3f] <-> [$e4..$ef]
L04_0E:
	lbmx 12
	lblx 11
	call L04_14
	lbmx 14
	lblx 4
L04_14:
	exc 0
	exbm
	adx 5
	nop
	exbm
	exc 0
	exbm
	adx 11
	exbm
	exc 0
	incb
	tr L04_14
	lbmx 15
	lblx 14
	rtn

L04_23:
	lblx 2
	lax 0
	out   // P2 <- 0
	lax 1
	out
	lblx 10
	in
	exbl
	lbmx 1
	call L0C_32
	lblx 4
	tpb 3
	tr L04_33
	lax 8
	tr L04_34
L04_33:
	lax 4
L04_34:
	ex
	incb
	call L0D_35
	add
	exc 0
	ex
	lax 0
	out
	tl L07_1B

// page 5
origin $140

L05_00:
	id
	trs TRS02 // L04_0E
	trs L01_16
	lblx 14
	ie

// wait for rom lockout bit of PIF status byte to become set
L05_05:
	tm 0
	tr L05_05

	id
	lax 1
	lblx 6
	out
	lblx 2
	lax 1
	out   // P2 <- 1
	trs L01_1A

	lbmx 15
	lblx 14
	ie

// wait for checksum verification bit
-;	tm 1
	tr -

	id
	trs TRS02 // L04_0E
	sm 3
	ie

// wait for clear pif ram bit
L05_18:
	tm 2
	tr L05_18
	id
	lax 0
	exc 0
	tc
	call L0E_00
	lbmx 3
	lblx 4

// compare checksum
L05_22:
	lax 0
	exc 1
	tam
	trs TRS0C // L03_39
	lbmx 3
	incb
	tr L05_22
	lbmx 4
	lblx 10
	trs L01_12
	ie
L05_2D:
// test bit 3 of PIF status byte
// (must be set by CPU within 5 seconds after booting, else system locks up)
	lbmx 15
	lblx 15
	tm 3
	tl L07_00

	id
	lblx 14
	trs L01_16
	lbmx 5
	lblx 14
	lax 5
	out
	sm 3
	tb
	nop
	tl L03_0B

// page 6
origin $180

L06_00:
	lax 3
	trs L01_20
	lax 3
	trs L01_20
	lblx 5
	lax 3
	out
	lbmx 0
	lblx 12
	trs L01_16
L06_0A:
// set bit 7 of PIF status byte
// (response to checksum verification?)
	lbmx 15
	lblx 14
	sm 3

	lbmx 0
	lblx 5
	tpb 3
	tr L06_18
	lblx 15
	trs TRS00 // L06_29
	tr L06_0A
	trs TRS00 // L06_29
	tr L06_0A
	trs TRS0C // L03_39
	lblx 5
L06_18:
	lax 1
	out
	lblx 8
	lax 0
L06_1C:
	tpb 3
	tr L06_1C

	id
	lblx 6
	out
	lax 9
	lblx 8
	out
	lax 8
	out
	sc
	tl L00_30

L06_29:
	exc 0
	adx 1
	tr L06_32
	excd 0
	exc 0
	adx 1
	tr L06_34
	excd 0
	rtns
L06_32:
	exc 0
	rtn
L06_34:
	exci 0
	rtn

// page 7
origin $1C0

L07_00:
	lbmx 4
	trs TRS00 // L06_29
	tr L07_06
	trs TRS00 // L06_29
	tr L07_06
	trs TRS00 // L06_29

L07_06:
	tl L05_2D
	trs TRS0C // L03_39
L07_09:
	lbmx 15
	lblx 15
	rm 1
	lbmx 5
	lblx 14
	sm 1
	lblx 6
	exc 0
	ex
	rtn

L07_13:
	call L0F_2F
	lblx 10
	lax 4
	tr L07_1F

L07_18:
	lblx 2
	lax 1
	out   // P2 <- 1
L07_1B:
	lblx 10
	in
	adx 15
	tr L07_38
L07_1F:
	out
	exbl
	lbmx 4
	tm 0
	tr L07_27
	call L0C_26
	tr L07_1B
L07_27:
	tm 3
	tr L07_2A
	tr L07_1B
L07_2A:
	lbmx 1
	call L0C_32
	lbmx 2
	lblx 2
	call L09_16
	tr L07_33
	tr L07_1B
L07_33:
	call L09_1F
	lblx 3
	tl L08_18
L07_38:
	lbmx 5
	call L03_06
	tl L02_2C

// page 8
origin $200

L08_00:
	excd 0
	exc 0
	adx 15
	tr L08_1D
	exci 0
L08_05:
	tpb 2
	tl L04_23
	tpb 3
	tr L08_05

	ex
	lda 0
	incb
	outl
	lda 0
	outl
	incb
	tr L08_17
	exbm
	adx 1
	tr L08_16
	lax 8
L08_16:
	exbm
L08_17:
	ex
L08_18:
	exc 0
	adx 15
	tr L08_00
	exc 0
	tr L08_05

L08_1D:
	call L09_00
	lbmx 3
	lblx 3
	tr L08_38
L08_22:
	excd 0
	exc 0
	adx 15
	tl L07_18
	exci 0
L08_28:
	tpb 2
	tl L04_23
	tpb 3
	tr L08_28

	inl
	ex
	exci 0
	inl
	exci 0
	tr L08_37
	exbm
	lbmx 8
	adx 1
	exbm
L08_37:
	ex
L08_38:
	exc 0
	adx 15
	tr L08_22
	exc 0
	tr L08_28

L08_3D:
	nop
	nop
	tr L08_3D

// page 9
origin $240

L09_00:
	lbmx 15
	lblx 15
	tm 2
	tr L09_09
	tm 3
	call L09_09
	trs TRS06 // L09_0D
	rtn

L09_09:
	lblx 2
	lax 2
	out   // P2 <- 2
	rtn

L09_0D:
	lax 0
	atx

L09_0F:
	exax
L09_10:
	adx 1
	tr L09_10
	exax
	adx 1
	tr L09_0F
	rtn

L09_16:
	ex
	tm 3
	tr L09_1A
	rtns

L09_1A:
	tm 2
	tr L09_22
	call L0C_26
	rtns

L09_1F:
	ex
	rm 3
	rm 2
L09_22:
	lda 0
	ex
	exci 0
	ex
	incb
	lda 0
	call L0D_35
	ex
	excd 1
	rtn

L09_2D:
	lbmx 8
	lblx 0
L09_2F:
	lax 15
	tam
	tl L0A_0E
	incb
	lda 0
	adx 1
	tl L0A_03
	incb
	tr L09_2F
	exbm
	adx 1
	tl L0A_00
	rtn

// page A
origin $280

L0A_00:
	exbm
	tl L09_2F
L0A_03:
	adx 1
	tr L0A_06
	rtn
L0A_06:
	adx 1
	tr L0A_1F
	ex
	lbmx 4
	sm 0
	lbmx 1
	ex
	tr L0A_14
L0A_0E:
	lax 0
L0A_0F:
	tam
	tr L0A_20
	incb
	tam
	tr L0A_1F
L0A_14:
	incb
	tr L0A_1B
	exbm
	adx 1
	tr L0A_1A
	rtn

L0A_1A:
	exbm
L0A_1B:
	ex
	incb
	tl L0B_33
L0A_1F:
	decb
L0A_20:
	exbm
	atx
	exbm
	exax
	ex
	exc 1
	ex
	exbl
	atx
	exbl
	exax
	ex
	exci 1
	ex
	lda 0
	rm 2
	rm 3
	exci 0
	exax
	lda 0
	ex
	lbmx 4
	decb
	rm 3
	incb
	lbmx 1
	exax
	tl L0B_00

// page B
origin $2C0

L0B_00:
	exc 1
	exax
	exc 1
	ex
	incb
	tr L0B_0B
	exbm
	adx 1
	tr L0B_0A
	tr L0B_3A

L0B_0A:
	exbm
L0B_0B:
	lda 0
	rm 3
	rm 2
	exci 0
	exax
	lda 0
	ex
	rc
	lbmx 0
	adc
	nop
	exc 1
	exax
	adc
	exc 1
	rc
	lda 0
	adc
	nop
	exc 1
	lda 0
	adc
	exc 1
	ex
	exbl
	ex
	sc
	adc
	nop
	lbmx 1
	ex
	exbl
	exbm
	ex
	adc
	tr L0B_30
	tr L0B_3B

L0B_30:
	ex
	exbm
	ex
L0B_33:
	lax 5
	tabl
	tr L0B_37
	rtn

L0B_37:
	ex
	tl L09_2F
L0B_3A:
	ex
L0B_3B:
	decb
	lbmx 4
	rm 0
	sm 3
	rtn

// page C
origin $300

// read nibble from CIC into [B]
// increments BL, returns with skip on carry
L0C_00:
	lax 15
	exc 0
	trs L01_2A
	tc
	rm 3
	trs L01_2A
	tc
	rm 2
	trs L01_2A
	tc
	rm 1
	trs L01_2A
	tc
	rm 0
	rc
	tr L0C_20

L0C_10:
	lax 3
	tm 3
	lax 2
	trs L01_20
	lax 3
	tm 2
	lax 2
	trs L01_20
	lax 3
	tm 1
	lax 2
	trs L01_20
	lax 3
	tm 0
	lax 2
	trs L01_20
L0C_20:
	incb
	rtn
	rtns

L0C_23:
	lblx 4
	lax 0
	out
L0C_26:
	lblx 3
	tpb 3
	tr L0C_23
	lblx 2
	lax 3
	out   // P2 <- 3
	lax 1
	out
	lblx 3
L0C_2F:
	tpb 3
	tr L0C_2F
	rtn

L0C_32:
	lda 1
	ex
	exbm
	ex
	lda 1
	ex
	exbl
	ex
	rtn

// page D
origin $340

L0D_00:
	lax 3
	trs L01_20
	lax 2
	trs L01_20
	lbmx 0
	lblx 10
	trs TRS0E // L0C_00
	trs TRS0E // L0C_00
	lbmx 13
	lblx 13
	call L0D_1B
	lbmx 0
	lblx 11
L0D_0E:
	trs TRS00 // L06_29
	tr L0D_0E

	trs L01_2A
	lbmx 13
	lblx 15
	call L0D_1B
	lbmx 5
	call L03_06
	trs TRS10 // L0D_31
	tl L03_0B

L0D_1B:
	ex
	lbmx 14
	lblx 0
L0D_1E:
	ex
	lda 0
	adx 15
	rtn

L0D_22:
	exc 0
	lax 13
	tabl
	tr L0D_2B
	ex
	trs TRS04 // L0C_10
	trs TRS04 // L0C_10
	tr L0D_1E
	tr L0D_2F

L0D_2B:
	ex
	trs TRS0E // L0C_00
	trs TRS0E // L0C_00
	tr L0D_1E
L0D_2F:
	lbmx 15
	tr L0D_1E

// set SB = $56
L0D_31:
	lbmx 5
	lblx 6
	ex
	rtn

// increment B and wrap to $80 on overflow
L0D_35:
	incb
	rtn
	exbm
	adx 1
	tr L0D_3B
	lax 8
L0D_3B:
	exbm
	rtn

// page E
origin $380

L0E_00:
	lbmx 1
	lblx 11
	sm 1
	call L0F_1B
	lblx 5
	lax 3
	out
	trs TRS06 // L09_0D
	lax 1
	out
	lbmx 2
	lblx 0
L0E_0D:
	trs TRS0E // L0C_00
	tr L0E_0D

	trs TRS0A // L0E_15
	trs TRS0A // L0E_15
	trs TRS0A // L0E_15
	trs TRS0A // L0E_15
	tl L0F_00

// encode CIC seed or checksum (one round)
L0E_15:
	lax 15
L0E_16:
	coma
	add
	exci 0
	tr L0E_16
	rtn

L0E_1B:
	lblx 15
	lda 0
L0E_1D:
	atx
	sc
	lblx 1
	adc
	sc
	exc 0
	lda 0
	incb
	adc
	sc
	coma
	exci 0
	adc
	exci 0
	add
	exc 0
	lda 0
	incb
	add
	exci 0
	adx 8
	add
	exci 0

L0E_34:
	adx 1
	nop
	add
	exc 0
	lda 0
	incb
	tr L0E_34

	exax
	adx 15
	rtn
	tr L0E_1D

// page F
origin $3C0

L0F_00:
	lbmx 6
	lblx 0
	lax 0
	exc 0
	ex
	lbmx 6
	lblx 2
L0F_07:
	ex
	lax 4
	atx
	lda 0
	adx 1
	exc 0
	ex
	pat
	nop
	exc 1
	exax
	exci 1
	tr L0F_07
	lblx 1
	trs TRS04 // L0C_10
	lbmx 7
	lblx 1
	trs TRS04 // L0C_10
	tl L0D_31

L0F_1B:
	lbmx 6
	lblx 9
	lax 1
	out

L0F_1F:
	call L06_29
	nop
	lblx 9
	tpb 3
	tr L0F_1F

	lax 0
	out
	excd 0
	exax
	exc 0
	lblx 1
	exc 1
	exax
	exc 1
	rtn

L0F_2F:
	lbmx 5
	lblx 7
	ex
	exbm
	ex
	exc 1
	ex
	exbl
	ex
	exci 1
	exax
	exci 0
	sm 0
	tc
	rm 0
	rc
	rtn
