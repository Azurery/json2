/*
 * @Author: your name
 * @Date: 2019-12-08 15:58:34
 * @LastEditTime: 2019-12-08 16:10:34
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \json2\src\utils.h
 */
#ifndef _UTILS_H_
#define _UTILS_H_

#include <cstdint>

namespace json2 {

unsigned fast_itoa(int32_t val, char* buf);
unsigned fast_itoa(int64_t val, char* buf);   

}

#endif