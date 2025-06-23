u8 nl[] = "\n";
i32 a = 10 * 8;
i32 b = 5;
i32 c = a - 6 * 7.0;
i32 aq[6, 5];
f32 farr[3, 3, 3];

farr[1, 1, 1] = 5.0;
print_f32(farr[1, 1, 1]);
print_str(nl);

aq[0, 2] = 1;
aq[1, 3] = 5;

for(i32 i : 0..aq[1, 3]){
    for(i32 ii: 0..5){
        print_str("aq[");
        print_i32(ii);
        print_str(", ");
        print_i32(i);
        print_str("] = ");
        print_i32(aq[ii, i]);

        if(i < 3.1) {
            print_str(" i < 3.1");
            if(i < 2){
                print_str(" i < 2");
            }
        } else {
            print_str(" i >= 3.1");
        }
        print_str(nl);
    }
}

print_str(nl);
