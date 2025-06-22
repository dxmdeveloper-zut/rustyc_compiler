i32 i = 10.0 * 5 + 25;
f32 a = i + 25.0;
print_f32(a);
i32 c = a;
print_str("\n");
print_i32(c);

if (c == 75) {
    print_str("\nc == 75\n");
    if (c == 75) {
        u8[] str = "c == 75 once again\n";
        print_str(str);
    }
}

print_str("\n");
