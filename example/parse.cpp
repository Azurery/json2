/*
 * @Author: your name
 * @Date: 2019-12-08 14:46:22
 * @LastEditTime: 2019-12-08 15:44:48
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \json2\example\parse.cpp
 */
#include <cstdio>
#include "../src/reader.h"
#include "../src/writer.h"
#include "../src/read_stream.h"
#include "../src/write_stream.h"

using namespace json2;

int main() {
    file_read_stream in(stdin);
    file_write_stream out(stdout);
    writer<file_write_stream> write(out);

    parse_error err = reader::parse(in, write);
    if(err != PARSE_OK) {
       printf("%s\n", parse_error_str(err)); 
    } 
    
    return 0;
}