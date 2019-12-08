#ifndef _WRTIE_STREAM_H_
#define _WRTIE_STREAM_H_

#include <cstdio>
#include <string>
#include <vector>

namespace json2 {

class file_write_stream {
public:
  file_write_stream(const file_write_stream&) = delete;
  file_write_stream& operator=(const file_write_stream&) = delete;

  explicit file_write_stream(FILE* output) :
    output_(output) {}

  void dump(char ch) {
    putc(ch, output_); 
  }

  void dump(const char* str) {
    fputs(str, output_);
  }

  // 此重载的 put() 函数将格式化的内容输出到流 stream 中
  // 注：%.*s 后面跟的两个参数表示：
  //        1. str.length() 表示输出数据所占的位置大小
  //        2. str.data() 表示要输出的内容
  //  e.g.  
  //    printf("%.*s\n", 1, "abc");        // 输出a
  //    printf("%.*s\n", 2, "abc");        // 输出ab
  //    printf("%.*s\n", 3, "abc");        // 输出abc >3是一样的效果 因为输出类型type = s，遇到'\0'会结束 
  //  
  //  简而言之，"%.*s" 的作用就是输出字符串中指定大小的内容
  void dump(std::string str) {
    fprintf(output_, "%.*s", static_cast<int>(str.length()), str.data());
  }

private:
  FILE* output_;
};


class string_write_stream {
public:
  string_write_stream(const string_write_stream&) = delete;
  string_write_stream& operator=(const string_write_stream&) = delete;

  void dump(char ch) {
    buffer_.push_back(ch); 
  }
  
  void dump(std::string str) {
    buffer_.insert(buffer_.end(), str.begin(), str.end());
  }

  std::string get() const {
    return std::string(&*buffer_.begin(), buffer_.size());
  }

private:
  std::vector<char> buffer_;
};

}


#endif
