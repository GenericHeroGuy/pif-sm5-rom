architecture n64.pif

////////////////////////////////////////
// compile-time constants (see Makefile)

// PIF region
constant regionNTSC = region == 0
constant regionPAL = region == 1
assert(regionNTSC || regionPAL)

////////////////////////////////////////
// page 0 (reset vector)
origin $000

ResetVector: // 00:00
	lbmx 2
	lblx 2
	ex
	lblx 2
	lax 1
	out
	tpb 2
	tr L00_0c
	trs L01_32
	tc
L00_0a:
	tr L00_0a
	tr L00_0f
L00_0c:
	lax 0
	out
	trs L01_12

L00_0f:
	lax region
	trs L01_06
	lax 0
	trs L01_06
	lax 1
	trs L01_06
	call L03_00
	call L02_2f
	lblx 10
L00_1a:
	call L02_0f
	incb
	tr L00_1a
	call L03_06
	call L06_22
	call L02_20
	lbmx 0
	lblx 0
	trs L01_04
L00_27:
	call L02_0f
	incb
	tr L00_27
	tl L06_00

////////////////////////////////////////
// page 1 (TRS vectors)
origin $040

TRS00:
	tl L05_00

L01_02:
	tr L01_02
	tr L01_02

L01_04:
	lax 0
	tr L01_06

L01_06:
	ex
L01_07:
	tpb 1
	tr L01_0a
	tr L01_07
L01_0a:
	out
L01_0b:
	tpb 1
	tr L01_0b
	lax 1
	out
	ex
	rtn
	nop

L01_12:
	ex
	sc
	lblx 15
	lax 0
	out
	lblx 2
L01_18:
	tpb 1
	tr L01_1b
	tr L01_18
L01_1b:
	tpb 0
	rc
	lblx 15
	lax 1
	out
	lblx 2
L01_21:
	tpb 1
	tr L01_21
	ex
	rtn
	nop

L01_26:
	ex
	sc
	tsf
	rc
	incb
	tr L01_30
	exbm
	adx 1
	exbm
L01_30:
	ex
	rtn

L01_32:
	ex
	sc
	lblx 15
	lax 0
	out
	lblx 2
L01_38:
	tpb 1
	tr L01_3b
	tr L01_38
L01_3b:
	lax 13
L01_3c:
	adx 1
	tr L01_3c
	tr L01_1b
	nop

////////////////////////////////////////
// page 2 (interrupt vectors)
// NOTE: CIC does not use interrupts!
origin $080

L02_00:
	lax 15
	exc 0
	trs L01_12
	tc
	rm 3
	trs L01_12
	tc
	rm 2
	trs L01_12
	tc
	rm 1
	trs L01_12
	tc
	rm 0
	rtn

L02_0f:
	lax 1
	tm 3
	lax 0
	trs L01_06
	lax 1
	tm 2
	lax 0
	trs L01_06
	lax 1
	tm 1
	lax 0
	trs L01_06
	lax 1
	tm 0
	lax 0
	tl L01_06

L02_20:
	call L02_2b
	call L02_2b
	call L02_2b
	tr L02_2b
L02_27:
	adx 1
	nop
	add
	exc 0
L02_2b:
	lda 0
	incb
	tr L02_27
	rtn

L02_2f:
	lbmx 0
	lblx 10
	lax 11
	exci 0
	lax 5
	excd 0
	call L02_2b
	lblx 10
	tr L02_2b

////////////////////////////////////////
// page 3 (standby exit vector)
// NOTE: CIC does not use halt/stop!
origin $0c0

L03_00:
	lbmx 4
	lblx 0
	ex
	lbmx 0
	lblx 12
	tr L03_0b

L03_06:
	lbmx 5
	lblx 0
	ex
	lbmx 0
	lblx 4
L03_0b:
	lax 15
	exc 0
	trs L01_26
	tc
	rm 3
	trs L01_26
	tc
	rm 2
	trs L01_26
	tc
	rm 1
	trs L01_26
	tc
	rm 0
	incb
	tr L03_0b
	lbmx 2
	lblx 2
	ex
	rtn

L03_1f:
	lbmx 0
	lblx 0
	lax 0
	exc 1
	lax 0
	exc 1
	lax 0
	atx
	tr L03_2c

L03_28:
	exc 1
	tr L03_2b
L03_2a:
	exc 0
L03_2b:
	exax
L03_2c:
	call L06_37
	adx 1
	tr L03_2c
	exax
	adx 1
	tr L03_2b
	exc 0
	adx 1
	tr L03_2a
	exc 1
	exc 0
	adx 1
	tr L03_28
	trs L01_04
	tl L04_0e

////////////////////////////////////////
// page 4 (PAT data)
origin $100

// PAT table data, used to initialize CIC compare data
if regionNTSC {
	db $19, $4a, $f1, $88, $b5, $5a, $71, $c3, $de, $61, $10, $ed, $9e, $8c
} else if regionPAL {
	db $14, $2f, $35, $f1, $82, $21, $77, $11, $99, $88, $15, $17, $55, $ca
}

L04_0e:
	trs L01_12
	tc
	tr L04_18
	trs L01_12
	tc
	tl L07_00
	rc
	tl L03_1f
L04_18:
	trs L01_12
	tc
	tr L04_1c
	trs L01_02
L04_1c:
	lbmx 0
	trs TRS00
	trs TRS00
	trs TRS00
	lbmx 1
	trs TRS00
	trs TRS00
	trs TRS00
	lbmx 1
	lblx 7
	lda 0
	adx 15
	lax 0
	adx 1
	exbl
L04_2b:
	trs L01_12
	lbmx 1
	lax 1
	tm 0
	lax 0
	trs L01_06
	lbmx 0
	tc
	tr L04_39
	tm 0
L04_35:
	trs L01_02
L04_36:
if regionNTSC {
	incb
} else if regionPAL {
	decb
	lax 0
	tabl
}
	tr L04_2b
	tr L04_0e
L04_39:
	tm 0
	tr L04_36
	tr L04_35

////////////////////////////////////////
// page 5
origin $140

L05_00:
	lblx 15
	lda 0
L05_02:
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
L05_19:
	adx 1
	nop
	add
	exc 0
	lda 0
	incb
	tr L05_19
	exax
	adx 15
	rtn
	tr L05_02

////////////////////////////////////////
// page 6
origin $180

L06_00:
	lbmx 0
	lblx 0
	lax 0
	exc 0
	ex
	lbmx 1
	lblx 1
	lax 11
	exci 1
L06_09:
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
	tr L06_09
	lbmx 2
	lblx 2
	ex
	lbmx 0
	lblx 1
	call L02_00
	lbmx 1
	call L02_00
	tl L04_0e

L06_22:
	lbmx 0
	lblx 2
L06_24:
	exax
L06_25:
	tpb 1
	tr L06_28
	tr L06_2e
L06_28:
	add
	lblx 0
	exci 0
	exax
	excd 0
	rtn

L06_2e:
	adx 1
	tr L06_25
	exax
	adx 1
	tr L06_24
	exc 0
	adx 1
	exc 0
	tr L06_25

L06_37:
	nop
	nop
	nop
	rtn

////////////////////////////////////////
// page 7
origin $1c0

L07_00:
	lbmx 2
	lblx 0
	lax 10
	exc 0
	call L02_0f
	call L02_0f
	lax 15
	exax
L07_0a:
	exax
	adx 15
	tr L07_17
	exax
	call L02_00
	incb
	call L02_00
	incb
	tr L07_0a
	lbmx 3
	tr L07_0a
L07_17:
	call L07_2c
	lbmx 2
	lblx 0
	lax 15
	exax
	trs L01_04
L07_1e:
	exax
	adx 15
	tl L04_0e
	exax
	call L02_0f
	incb
	call L02_0f
	incb
	tr L07_1e
	lbmx 3
	tr L07_1e
L07_2c:
	lbmx 2
	lblx 0
	lax 15
	exax
L07_30:
	exax
	adx 15
	rtn
	exax
	exc 0
	coma
	exci 0
	exc 0
	coma
	exci 0
	tr L07_30
	lbmx 3
	tr L07_30

////////////////////////////////////////
// page 8
origin $200

// some unused code snippet?
L08_00:
	nop
	in
	nop
	tpb 0
	rm 0
	tpb 1
	rm 1
	tpb 2
	rm 2
	tpb 3
	rm 3
	nop
	nop
	tr +
	nop
+;	nop

////////////////////////////////////////
// pages 9-F are unused
origin $240
origin $280
origin $2c0
origin $300
origin $340
origin $380
origin $3c0

origin $400
