void kernel_main(void) {
    char *video = (char*)0xb8000;
    video[0] = '4';
    video[1] = 0x07;
    video[2] = '2';
    video[3] = 0x07;
}