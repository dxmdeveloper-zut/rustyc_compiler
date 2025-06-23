i32 a = 10 * 8;
i32 b = 5;
i32 c = a - 6;
i32 aq[20, 13];

aq[0, 2] = 1;
aq[1, 3] = 2;

for(i32 i : 0..5){
    for(i32 ii: 0..5){
        print_str("aq[");
        print_i32(ii);
        print_str(", ");
        print_i32(i);
        print_str("] = ");
        print_i32(aq[ii, i]);
        print_str("\n");
    }
}

// if (c == 100) {
//     print_str("\nc == 100\n");
//     if (c == 75) {
//         u8[] str = "c == 75\n";
//         print_str(str);
//     } else {
//         print_str("\nc != 75\n");
//     }
// }

print_str("\n");
