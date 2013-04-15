all: b.i b.ii b.iii

b.i: ex0.s ex0.out ex0.map
ex0.s: ex0.c
	mcc ex0.c -O7 -S -o ex0.s
ex0.out ex0.map: ex0.s
	mcc ex0.s -o ex0.out -m > ex0.map

b.ii: ex0.dbg.s ex0.dbg.out
ex0.dbg.s: ex0.c
	mcc ex0.c -g -S -o ex0.dbg.s
ex0.dbg.out: ex0.dbg.s
	mcc ex0.dbg.s -o ex0.dbg.out

b.iii: only_text.out text_rodata.out text_rodata_data.out all_sections.out 
only_text.out: only_text.c
	gcc only_text.c -o only_text.out -nostdlib
text_rodata.out: text_rodata.c
	gcc text_rodata.c -o text_rodata.out -nostdlib
text_rodata_data.out: text_rodata_data.c
	gcc text_rodata_data.c -o text_rodata_data.out -nostdlib
all_sections.out: all_sections.c
	gcc all_sections.c -o all_sections.out -nostdlib

tar:
	tar cvf fedidat_waller.tar ex0.c makefile README only_text.c text_rodata.c text_rodata_data.c all_sections.c
clean:
	rm -f ex0.s ex0.out ex0.map ex0.dbg.s ex0.dbg.out only_text.out text_rodata.out text_rodata_data.out all_sections.out
