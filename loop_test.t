for (i32 j : 0..10) {
    print_i32(j);
    u8[] str = "... k = [";
    print_str(str);
    for (i32 k : 0..5) {
        print_i32(k);
    }
    print_str("]\n");
}