STACK_SIZE = 5120;
HEAP_SIZE = 5120;

MEMORY {
       ROM: ORIGIN = 0x00000000 LENGTH = 16K
       RAM: ORIGIN = 0x00006000 LENGTH = 48K

}

SECTIONS {
         GROUP: {
                * (TEXT) : {}
                * (LIT) : {}
         } > ROM


         GROUP: {
                * (DATA) : {}
                * (BSS) : {}
                .stack SIZE(STACK_SIZE) : {}
                .heap SIZE(HEAP_SIZE) : {}   
         } > RAM
}

