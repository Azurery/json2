/*
 * @Author: your name
 * @Date: 2019-12-08 14:46:22
 * @LastEditTime: 2019-12-08 17:09:51
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \json2\example\parse.cpp
 */
#include <cstdio>
#include <iostream>
#include "../src/reader.h"
#include "../src/writer.h"
#include "../src/read_stream.h"
#include "../src/write_stream.h"

using namespace json2;

int main() {
    FILE* fp;
    // TODO:相对路径有问题 
    if((fp = fopen("C:\\Users\\Magicmanoooo\\Desktop\\json2\\example\\demo.txt", "rb")) == NULL) {
        printf("failed to open file\n");
        exit(0);
    }

    file_read_stream in(fp);

    file_write_stream out(stdout);
    writer<file_write_stream> write(out);
    //std::cout << "-----------" << std::endl;
    parse_error err = reader::parse(in, write);
    if(err != PARSE_OK) {
       printf("%s\n", parse_error_str(err)); 
    } 
    
    return 0;
}