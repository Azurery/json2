/*
 * @Author: your name
 * @Date: 2019-12-06 14:51:01
 * @LastEditTime: 2019-12-08 16:13:04
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \json2\writer.h
 */
#ifndef _WRITER_H_
#define _WRITER_H_ 

#include <string>
#include <cstdint>
#include <vector>
#include <cassert>
#include <cmath>
#include "value.h"
#include "utils.h"

namespace json2 {

/**
 * @description: depth 类主要是提供一些 nested depth 的信息
 */
struct depth {
public:
  explicit depth(bool in_array) :
    in_array_(in_array),
    value_count_(0) {}

public:
  bool in_array_; // 如果为 true，则表示 in array；否则就是 in object
  int value_count_; // 在此 depth 中 value 的数量
};

template <typename WriteStream>
/**
 * @description:处理器
 *  使用者需要实现一个处理器（handler），用于处理来自 Reader 的事件（函数调用）。
 *  处理器必须包含以下的成员函数。
 * ``` 
 *  class Handler {
 *      bool handle_null();
 *      bool hand_bool(bool b);
 *      bool handle_int32(int i);
 *      bool handle_int64(int64_t i);
 *      bool handle_double(double d);
 *      bool handle_string(const Ch* str, SizeType length, bool copy);
 *      bool handle_start_object();
 *      bool handle_key(const Ch* str, SizeType length, bool copy);
 *      bool handle_end_object();
 *      bool handle_start_array();
 *      bool handle_end_array();
 *  };
 *  
 *  当 Reader 遇到 JSON null 值时会调用 handle_null()。
 *  当 Reader 遇到 JSON true 或 false 值时会调用 handle_bool(bool)。
 *  当 Reader 遇到 JSON number，它会选择一个合适的 C++ 类型映射，然后调用 handle_int(int)、
 *    、handle_int64(int64_t) 及 handle_double(double) 的其中之一个。 
 *    
 *  当 Reader 遇到 JSON string，它会调用 handle_string(const char* str)。
 *  第一个参数是字符串的指针。第二个参数是字符串的长度（不包含空终止符号）。
 *  
 *  当 Reader 遇到 JSON object 的开始之时，它会调用 handle_start_object()。
 *  JSON 的 object 是一个键值对（成员）的集合。若 object 包含成员，它会先为成员的
 *  名字调用 handle_key()，然后再按值的类型调用函数。它不断调用这些键值对，
 *  直至最终调用 handle_end_object。
 *  
 *  JSON array 与 object 相似，但更简单。在 array 开始时，Reader 会调用 handle_begin_arary()。
 *  若 array 含有元素，它会按元素的类型来读用函数。相似地，最后它会调用 handle_end_array()
 *     
 *  每个处理器函数都返回一个 bool。正常它们应返回 true。若处理器遇到错误，它可以返回 false 
 *  去通知事件发送方停止继续处理。
 *  
 *  例如，当我们用 Reader 解析一个 JSON 时，处理器检测到该 JSON 并不符合所需的 schema，
 *  那么处理器可以返回 false，令 Reader 停止之后的解析工作。
 *  而 Reader 会进入一个错误状态。  
 *   */

// 简而言之，writer 就是一个将 json 转换为字符串，即序列化的过程
class writer {
public:
  writer(const writer&) = delete;
  writer& operator=(const writer&) = delete;

  explicit writer(WriteStream& stream) :
    stream_(stream), 
    see_value_(false) {}

  bool handle_null() {
    handle_nested_aux(TYPE_NULL);
    stream_.dump("null");
    return true;
  }

  bool handle_bool(bool val) {
    handle_nested_aux(TYPE_BOOL);
    stream_.dump(val ? "true" : "false");
    return true;    
  }

  bool handle_int32(int32_t val) {
    handle_nested_aux(TYPE_INT32);
    // TODO:这个数值是否可变 
    char buf[11];
    unsigned count = fast_itoa(val, buf);
    stream_.dump(std::string(buf, count));
    return true;
  }

  bool handle_int64(int64_t val) {
    handle_nested_aux(TYPE_INT64);
    // TODO:这个数值是否可变 
    char buf[20];
    unsigned count = fast_itoa(val, buf);
    stream_.dump(std::string(buf, count));
    return true;
  }

  bool handle_double(double val) {
    handle_nested_aux(TYPE_DOUBLE);
    char buf[32];
    if(std::isinf(val)) {
      strcpy(buf, "Infinity");
    } else if(std::isnan(val)) {
      strcpy(buf, "NaN");
    } else {
      int num = sprintf(buf, "%.17g", val);
      assert(num > 0 && num < 32);
      // 如果不加 ".0" 的话，会损失类型信息
      // e.g. "1.0" --> double 1 --> "1"
      auto iter = std::find_if_not(buf, buf + num, isdigit);
      if(iter == buf + num) {
        strcat(buf, ".0;");
      }
    }
    stream_.dump(buf);
    return true;
  }

  bool handle_string(std::string str) {
    handle_nested_aux(TYPE_STRING);
    stream_.dump('"');
    for(auto ch : str) {
      auto val = static_cast<unsigned>(ch);
      switch(val) {
        case '\"':
          // TODO:stream_.dump("\\"");
          stream_.dump("\\\"");
        case '\b':
          stream_.dump("\\b");
        case '\f':
          stream_.dump("\\f");
        case '\n':
          stream_.dump("\\n");
        case '\r':
          stream_.dump("\\r");
        case '\t':
          stream_.dump("\\t");
        case '\\':
          stream_.dump("\\\\");
        default:
          if(val < 0x20) {
            char buf[7];
            snprintf(buf, 7, "\\u%04X", val);
            stream_.dump(buf);
          } else {
            stream_.dump(val);
          }
          break; 
      }
    }
    stream_.dump('"');
    return true;
  }

  bool handle_start_object() {
    handle_nested_aux(TYPE_OBJECT);
    //  由于处理的是 object，所以需要把 in_array_ 设置为 false
    stack_.emplace_back(false);
    stream_.dump('{');
    return true;
  }

  bool handle_key(std::string key) {
    handle_nested_aux(TYPE_STRING);
    stream_.dump('"');
    stream_.dump(key);
    stream_.dump('"');
    return true;
  }

  bool handle_end_object() {
    assert(!stack_.empty());
    assert(!stack_.back().in_array_);
    stack_.pop_back();
    stream_.dump('}');
    return true;
  }

  bool handle_start_array() {
    handle_nested_aux(TYPE_ARRAY);
    stack_.emplace_back(true);
    stream_.dump('[');
    return true;
  }

  bool handle_end_array() {
    assert(!stack_.empty());
    assert(stack_.back().in_array_);
    stack_.pop_back();
    stream_.dump(']');
    return true;
  }
  
private:

  /**
   * @description: 用于处理嵌套，根据输入类型 type，来进行不同的嵌套处理
   *                - 对于 array 和 object 类型，才有嵌套的可能
   *                - 对于 primitive 类型，则不做处理，只是递增同一 depth 下 value_count_ 
   * @param {type} 输入类型
   * @return: 
   */  
  void handle_nested_aux(value_type type) {
    if(see_value_)
      assert(!stack_.empty() && "root not singular");
    else
      see_value_ = true;
    
    if(stack_.empty())
      return;
    auto& top_depth = stack_.back();

    // 以下是判断是否有 array 或 object
    if(top_depth.in_array_) { // array
      if(top_depth.value_count_ > 0) 
        stream_.dump(','); // 如果不是 array 中的第一个 element，便添加 ,
    } else { // object
      if(top_depth.value_count_ % 2 == 1) // 奇数个 object 
        stream_.dump(':');
      else {   
        assert(type == TYPE_STRING && "miss quotation mark");
        if(top_depth.value_count_ > 0)
          stream_.dump(',');
      }
    }
    // 如果为普通类型，则说明不会有嵌套，还是在同一 depth 之中，
    // 将 value_count_ 递增即可
    top_depth.value_count_++;
  }

private:
  std::vector<depth> stack_;
  WriteStream& stream_;
  bool see_value_;
};


}


#endif