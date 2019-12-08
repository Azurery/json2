#ifndef   _READ_STREAM_H_
#define _READ_STREAM_H_ 

#include <cstdio>
#include <vector>
#include <cassert>
#include <string>

namespace json2 {

/*
 * file_read_stream 类：主要是将文件中的内容读入由 std::vector<char> 组成的 buffer_ 中，
 *      相当于形成一个 stream，即流的形式
 *
 */
class file_read_stream {
public:
  using iterator = std::vector<char>::iterator;

public:
  file_read_stream(const file_read_stream&) = delete;
  file_read_stream& operator=(const file_read_stream&) = delete;

  // 初始化一个 file_read_stream 对象时:
  //  1. 将 input 文件中的内容全部读入 buffer_ 缓存中
  //  2. 将 iter_ 指向 buffer_ 的开头
  explicit file_read_stream(FILE* input) {
    read_stream(input);
    iter_ = buffer_.begin();
  }

  bool has_next() const {
    return iter_ != buffer_.end(); 
  }

  // next() 函数的作用：将 iter 向后移动一个，并返回该 iter_ 指向的字符
  char next() {
    if(has_next()) {
      char ch = *iter_;
      iter_++;
      return ch; 
    }
    return '\0';
  }

  // peek() 函数的作用：主要是返回 iter_, 但并不返回其所指向的字符
  char peek() {
    return has_next() ? *iter_ : '\0'; 
  }

  const iterator get_iterator() const {
    return iter_; 
  }

  // assert_next() 的作用：首先判断 buffer_ 中的下一个字符是否为指定的字符
  //          如果下一个字符就是 ch，便返回下一个字符，并将 iter_ 步数加 1
  void assert_next(char ch) {
    assert(peek() == ch);
    next();
  }
private:
  std::vector<char> buffer_;
  iterator iter_;

private:
  // 此函数的作用是：将 input 文件中的内容先读入 buffer，然后再将 buffer 中的
  //      中的字符压入 vector 组成的 buffer_中
  void read_stream(FILE* input) {
    char buffer[65535];
    while(true) {
      size_t read_bytes = fread(buffer, 1, sizeof(buffer), input);
      if(read_bytes)
        break;
      buffer_.insert(buffer_.end(), buffer, buffer + read_bytes);    
    }
  }

};


class string_read_stream {
public:
  using iterator = std::string::iterator;

public:
  string_read_stream(const string_read_stream&) = delete;
  string_read_stream& operator=(const string_read_stream&) = delete;

  explicit string_read_stream(std::string data):
    data_(data),
    iter_(data_.begin()) {}

  bool has_next() const {
    return iter_ != data_.end(); 
  }

  char peek() {
    return has_next() ? *iter_ : '\0'; 
  }

  iterator get_iterator() const {
    return iter_; 
  }

  char next() {
    if(has_next()) {
      char ch = *iter_;
      iter_++;
      return ch;
    }
    return '\0';
  }

  void assert_next(char ch) {
    assert(peek() == ch);
    next();
  }

private:
  std::string data_;
  iterator iter_;
};
}


#endif
