/*
 * @Author: your name
 * @Date: 2019-12-08 11:43:10
 * @LastEditTime: 2019-12-08 14:39:45
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \json2\pretty_writter.h
 */
#ifndef _PRETTY_WRITTER_H_
#define _PRETTY_WRITTER_H_

#include "writer.h"

namespace json2 {

/**
 * @description: writer 所输出的是没有空格字符的最紧凑 JSON，适合网络传输或储存，但不适合人类阅读。
 *      因此，json2 提供了一个 pretty_writer，它在输出中加入缩进及换行* 
 *      pretty_writer 的用法与 writer 几乎一样，不同之处是 pretty_writer 提供了一个 
 *      SetIndent(Ch indentChar, unsigned indentCharCount) 函数。缺省的缩进是 4 个空格。 
 */

template <typename WriteStream>
class pretty_writter {
public:
  pretty_writter(const pretty_writter&) = delete;
  pretty_writter& operator=(const pretty_writter&) = delete;

//    explicit pretty_writter(Write_Stream& stream, std::string indent = "    ") :

  bool handle_null() {
    writer_.handle_null();
    keep_indent();
    return true;
  }

  bool handle_bool(bool val) {
    writer_.handle_bool(val);
    keep_indent();
    return true;
  }

  bool handle_int32(int32_t val) {
    writer_.handle_int32(val);
    keep_indent();
    return true;
  }

  bool handle_int64(int64_t val) {
    writer_.handle_int64(val);
    keep_indent();
    return true;
  }

  bool handle_double(double val) {
    writer_.handle_double(val);
    keep_indent();
    return true;
  }

  bool handle_string(std::string val) {
    writer_.handle_string(val);
    keep_indent();
    return true;
  }
  
  // 对于 object 类型，由于可能存在嵌套，所以需要根据嵌套的 object 所处的 depth，
  // 来进行相应的调整
  // 
  //  e.g.  
  //  {"name":"cxk","age":25,"sites": {"site":"www.cxk.com"}}
  //
  //  {
  //    "name":"cxk",
  //    "age":25,
  //    "sites": {
  //        "site":"www.cxk.com",
  //    }
  //  }
  bool handle_start_obejct() {
    writer_.handle_start_obejct();
    increment_indent();
    return true;
  }

  bool handle_end_array() {
    decrement_indent();
    writer_.handle_end_array();
    return true;
  }

private:
  void add_newline() {
    stream_.put('\n');
  }

  void add_indent() {
    for(int i = 0; i < indent_depth_; i++) {
      stream_.put(indent_);
    }
  }

  // 进行对齐时，需要加 '\n' + indent_
  void keep_indent() {
    if(indent_depth_ > 0) {
      add_newline();
      add_indent();
    }

    if(expect_object_value_) 
      expect_object_value_ = false;
  }

  void increment_indent() {
    indent_depth_++;
    add_new_line();
    add_indent();
    if(expect_object_value_) 
      expect_object_value_ = false;
  }

  void decrement_indent() {
    assert(indent_depth_ > 0);
    indent_depth_--;
    add_newline();
    add_indent();
    if(expect_object_value_)
      expect_object_value_ = false;
  }

private:
  std::string indent_; // 缩进的字符串
  int indent_depth_; // 需要进行缩进的位置所处的 depth 
  bool expect_object_value_ = false;

private:
  writer<WriteStream> writer_;
  WriteStream& stream_;
};


}



#endif