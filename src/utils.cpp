/*
 * @Author: your name
 * @Date: 2019-12-07 11:53:47
 * @LastEditTime: 2019-12-08 16:44:52
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \json2\writer.cpp
 */
#include <type_traits>
#include "utils.h"

namespace json2 {
 
/**
 * @description: 此函数用于计算 num 的位数
 *                      e.g. num = 123, ret = 3
 *                           num = 2345666, ret = 7
 * @param {uint32_t num} 输入的数字
 * @return: unsigned value 
 */
unsigned count_digits_aux(uint32_t num) {
    static const uint32_t powers_of_10[] = {
        0,
        10,
        100,
        1000,
        10000,
        100000,
        1000000,
        10000000,
        100000000,
        1000000000
    };
    // reference :Bit Twiddling Hacks  http://graphics.stanford.edu/~seander/bithacks.html#IntegerLog10
    //
    // Find integer log base 10 of an integer
    // 
    // unsigned int v; // non-zero 32-bit integer value to compute the log base 10 of 
    // int r;          // result goes here
    // int t;          // temporar// 
    // static unsigned int const PowersOf10[] = 
    //     {1, 10, 100, 1000, 10000, 100000,
    //      1000000, 10000000, 100000000, 1000000000}// 
    // t = (IntegerLogBase2(v) + 1) * 1233 >> 12; // (use a lg2 method from above)
    // r = t - (v < PowersOf10[t]);
    // 
    // The integer log base 10 is computed by first using one of the techniques above for finding the log base 2. 
    // By the relationship log10(v) = log2(v) / log2(10), we need to multiply it by 1/log2(10), 
    // which is approximately 1233/4096, or 1233 followed by a right shift of 12. 
    // Adding one is needed because the IntegerLogBase2 rounds down. 
    // Finally, since the value t is only an approximation that may be off by one, the exact value is found by 
    // subtracting the result of v < PowersOf10[t].
    // 
    // This method takes 6 more operations than IntegerLogBase2. 
    // It may be sped up (on machines with fast memory access) by modifying the log base 2 table-lookup method 
    // above so that the entries hold what is computed for t (that is, pre-add, -mulitply, and -shift). 
    // Doing so would require a total of only 9 operations to find the log base 10, 
    // assuming 4 tables were used (one for each byte of v).
    
    // __builtin_clz() 用于返回 num 的 leading 0 的数量 
    uint32_t val = (32 - __builtin_clz(num | 1)) * 1233 >> 12;
    return val - (num < powers_of_10[val]) + 1;
}

    unsigned count_digits_aux(uint64_t num) {
        static const uint64_t powers_of_10[] = {
           0,
           10,
           100,
           1000,
           10000,
           100000,
           1000000,
           10000000,
           100000000,
           1000000000,
           10000000000,
           100000000000,
           1000000000000,
           10000000000000,
           100000000000000,
           1000000000000000,
           10000000000000000,
           100000000000000000,
           1000000000000000000,
           10000000000000000000U 
        };
        uint32_t val = (64 - __builtin_clzll(num | 1)) * 1233 >> 12;
        return val - (num < powers_of_10[val]) + 1;
    }

#pragma GCC diagnostic push 
#pragma GCC diagnostic ignored "-Wconversion"

template <typename T>
unsigned itoa_aux(T val, char* buf) {
    static_assert(std::is_unsigned<T>::value, "Type must be unsigned integer");
    static const char digits_table[201] = 
            "00010203040506070809"
            "10111213141516171819"
            "20212223242526272829"
            "30313233343536373839"
            "40414243444546474849"
            "50515253545556575859"
            "60616263646566676869"
            "70717273747576777879"
            "80818283848586878889"
            "90919293949596979899"; 

    // 首先计算 val 的位数，得到其总长度 
    unsigned count = count_digits_aux(val);
    unsigned next = count - 1;

    // 如果输入值大于 100，就开始循环处理
    while(val >= 100) {
        // 两位两位地进行处理
        // e.g. val = 12372
        //   count = 5
        //   next = 4
        //   idx = (val % 100) * 2 = (12372 % 100) * 2 = 72 * 2 = 144
        //   val = 12372 / 100 = 123
        //   buf[4] = digits_table[145] = 2
        //   buf[3] = digits_table[144] = 7
        //   next = 2
        unsigned idx = (val % 100) * 2;
        val /= 100;
        buf[next] = digits_table[idx + 1];
        buf[next -1] = digits_table[idx];
        next -= 2;
    } 

    // 处理最后 1~2 个数字
    if(val < 10) {
        buf[next] = '0' + val;
    } else {
        unsigned idx = val * 2;
        buf[next] = digits_table[idx +1];
        buf[next - 1] = digits_table[idx];
    }

    return count;
}
#pragma GCC diagnostic pop

    // 如果 val 为负数，则需要首先在 buf 中加入符号 '-'
    // 接着需要对 val 进行取反操作(~)，再 +1，使得 val 变为正数 
   
    /* 
    * ~（取反操作符）
    * 
    * 例1： ~5 = -6
    * 计算公式： -（x + 1）
    * 计算过程：
    * ``` 
    *     5
    *   原码: 0 000 0101
    *   反码: 0 000 0101
    *   补码: 0 000 0101
    *   ~5(按照5的补码取反)
    *   补码: 1 111 1010
    *   反码: 1 111 1001
    *   原码: 1 000 0110
    *   得到结果：-5
    * ``` 
    * 例2： ~-5 = 4
    * 计算公式： -（x + 1）
    * 计算过程：
    * ``` 
    *   -5
    *   原码: 1 000 0101
    *   反码: 1 111 1010
    *   补码: 1 111 1011
    *   ~-5(按照-5的补码按位取反，正数的原码，反码，补码是一样的)
    *   补码: 0 000 0100
    *   反码: 0 000 0100
    *   原码: 0 000 0100
    *   得到结果：4
    * ```
    * */
// #define FAST_ITOA_AUX(val, buf)              \                 
//     if(val < 0) {                            \ 
//         *buf++ = '-';                        \ 
//         num = ~num + 1;                      \
//     }                                        \
//     return (val < 0) + itoa_aux(num, buf);   \

unsigned fast_itoa(int32_t val, char* buf) {
    uint32_t num = static_cast<uint32_t>(val);
    // FAST_ITOA_AUX(val, buf);
    if(val < 0) {                            
        *buf++ = '-';                        
        num = ~num + 1;                      
    }                                        
    return (val < 0) + itoa_aux(num, buf);   
}

unsigned fast_itoa(int64_t val, char* buf) {
    uint64_t num = static_cast<uint64_t>(val);
    // FAST_ITOA_AUX(val, buf); 
    if(val < 0) {                            
        *buf++ = '-';                        
        num = ~num + 1;                      
    }                                        
    return (val < 0) + itoa_aux(num, buf);   
}

}