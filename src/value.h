#ifndef _VALUE_H_
#define _VALUE_H_

#include <cassert>
#include <cstring>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <atomic>

namespace json2 {

/* json2 所支持的 8 种类型
 * 
 * 1.JSON 名称/值对
 *    JSON 数据的书写格式是：名称/值对。
 *    名称/值对包括字段名称（在双引号中），后面写一个冒号，然后是值：
 *    
 *    "name" : "菜鸟教程"
 * 
 * 2. JSON 值
 *     JSON 值可以是：
 *        - 数字（整数或浮点数）
 *        - 字符串（在双引号中）
 *        - 逻辑值（true 或 false）
 *        - 数组（在中括号中）
 *        - 对象（在大括号中）
 *        - null
 * 
 * 2.1 JSON 数字
 *    JSON 数字可以是整型或者浮点型：
 * 
 * { "age":30 }
 * 
 * 2.2 JSON 对象
 *    JSON 对象在大括号（{}）中书写：
 * 
 *    对象可以包含多个名称/值对：
 * 
 *    { "name":"菜鸟教程" , "url":"www.runoob.com" }
 *  
 *  这一点也容易理解，与这条 JavaScript 语句等价：
 * 
 *  name = "菜鸟教程"
 *  url = "www.runoob.com"
 * 
 * 2.3 JSON 数组
 *    JSON 数组在中括号中书写：
 * 
 *    数组可包含多个对象：
 * 
 * {
 *  "sites": [
 *            { "name":"菜鸟教程" , "url":"www.runoob.com" }, 
 *            { "name":"google" , "url":"www.google.com" }, 
 *            { "name":"微博" , "url":"www.weibo.com" }
 *          ]
 * }
 *
 * 在上面的例子中，对象 "sites" 是包含三个对象的数组。每个对象代表一条关于某个网站（name、url）的记录。
 * 
 * 2.4 JSON 布尔值
 *    JSON 布尔值可以是 true 或者 false：
 * 
 * { "flag":true }
 * 
 * 2.5 JSON null
 *    JSON 可以设置 null 值：
 * 
 *  { "runoob":null }
 * 
 *
 *  array 类型：
 *  e.g.
 *  {
 *    "name":"网站",
 *    "num":3,
 *    "sites":[ "Google", "Amazon", "Microsoft" ]
 *  }
 * json 对象：json 对象使用在 {} 中书写。对象可以包含多个 key/value 对
 * key 必须是字符串，value 可以是合法的 json 数据类型（字符串，数字，对象，数组，布尔值或 null）
 * key 和 value 中使用 : 分割。每个 key/value 对使用 , 分割
 *  e.g.
 *  { 
 *    "name":"runoob", 
 *    "alexa":10000, 
 *    "site":null 
 *  }
 *
 */

enum value_type {
  TYPE_NULL,
  TYPE_BOOL,
  TYPE_INT32,
  TYPE_INT64,
  TYPE_DOUBLE,
  TYPE_STRING,
  TYPE_ARRAY,
  TYPE_OBJECT,
};;

struct element;
class document;


template <typename T>
struct refcount {
public:  
 
  // TODO:
  // 对这个完美转发还不是很理解？
    // 在初始化 refcount 这个类时，将引用计数 refcount_ 设置为 1
    template <typename... Args>
    refcount(Args&&... args) :
      refcount_(1),
      data_(std::forward<Args>(args)...) {} 
    
    ~refcount() { 
      assert(refcount_ == 0); 
    }

    int increment_and_get() {
      assert(refcount_ > 0);
      return ++refcount_;
    }

    int decrement_and_get() {
      assert(refcount_ > 0);
      return --refcount_;
    }

public:
    // refcount_ 表示引用计数
    std::atomic_int refcount_;
    T data_; 
};
  

/* 在 json2 的上下文中，一个 value 的实例可以包含 6 中 json数据类型之一
 */
class value {
  friend class document;

public:
  using element_iterator = std::vector<element>::iterator;
  using const_element_iterator = std::vector<element>::const_iterator;

public:
  explicit value(value_type type = TYPE_NULL);
  
  explicit value(bool bool_value) :
    type_(TYPE_BOOL),
    bool_value_(bool_value) {}

  explicit value(int32_t int32_value) :
    type_(TYPE_INT32),
    int32_value_(int32_value) {}

  explicit value(int64_t int64_value) :
    type_(TYPE_INT64),
    int64_value_(int64_value) {}

  explicit value(double double_value) :
    type_(TYPE_DOUBLE),
    double_value_(double_value) {}
 
  // 以下三种为 value_type 为 TYPE_STRING 的情况
  //   1. 直接以 std::string 进行构造
  //   2. 以字符串进行构造（char*)
  //   3. 以字符串和字符串的长度进行构造
  explicit value(std::string str) :
    type_(TYPE_STRING),
    string_value_(new string_with_refcount(str.begin(), str.end())) {}

  explicit value(const char* str) :
    type_(TYPE_STRING),
    string_value_(new string_with_refcount(str, str + strlen(str))) {}

  explicit value(const char* str, size_t len) :
    value(std::string(str, len)) {}
 
  value(const value& rhs);
  value(value&& rhs);

  value& operator=(const value& rhs);
  value& operator=(value&& rhs);

  ~value();

  value_type get_type() const {
    return type_; 
  }

  size_t get_size() const {
    if(type_ == TYPE_ARRAY) 
      return array_value_->data_.size();
    else if(type_ == TYPE_OBJECT)
      return object_value_->data_.size();
    return 1;
  }

  bool is_null() const {
    return type_ == TYPE_NULL;  
  }

  bool is_bool() const {
    return type_ == TYPE_BOOL; 
  }

  bool is_int32() const {
    return type_ == TYPE_INT32; 
  }

  bool is_int64() const {
    return type_ == TYPE_INT64; 
  }

  bool is_double() const {
    return type_ == TYPE_DOUBLE; 
  }

  bool is_string() const {
    return type_ == TYPE_STRING; 
  }

  bool is_array() const {
    return type_ == TYPE_ARRAY; 
  }

  bool is_object() const {
    return type_ == TYPE_OBJECT; 
  }

  // 以下是 getter && setter
  value& set_null() {
    // 在设置 NULL 时，需要先调用析构函数，将当前对象 value 对象析构
    // 然后再利用 placement new 操作符，在原 value 对象所留下的内存区域构造一个
    // type 为 TYPE_NULL 的对象
    this->~value();
    return *new (this) value(TYPE_NULL);
  }
  
  bool get_bool_value() const {
    assert(type_ == TYPE_BOOL);
    return bool_value_;
  }
  
  value& set_bool(bool bool_value) {
    this->~value();
    return *new(this) value(bool_value);
  }

  int32_t get_int32_value() const {
    assert(type_ == TYPE_INT32);
    return int32_value_;
  }

  value& set_int32_value(int32_t int32_value) {
    this->~value();
    return *new(this) value(int32_value);
  }
  
  int64_t get_int64_value() const {
    // 对于 int32_t 类型的值也可以在此处返回
    assert(type_ == TYPE_INT64 || type_ == TYPE_INT32);
    return type_ == TYPE_INT64 ? int64_value_ : int32_value_;
  }
  
  value& set_int64(int64_t int64_value) {
    this->~value();
    return *new(this) value(int64_value);
  }

  double get_double_value() const {
    assert(type_ == TYPE_DOUBLE);
    return double_value_;
  }

  value& set_double(double double_value) {
    this->~value();
    return *new(this) value(double_value);
  }

  // TODO: 此处的实现不对
  std::string get_string_value() const {
    assert(type_ == TYPE_STRING);
    auto ret = std::string(string_value_->data_.begin(), string_value_->data_.end());
    return ret;
  }
  
  value& set_string(std::string str) {
    this->~value();
    return *new(this) value(str);
  }

  const auto& get_array_value() const {
    assert(type_ == TYPE_ARRAY);
    return array_value_->data_;
  }

  value& set_array() {
    this->~value();
    return *new(this) value(TYPE_ARRAY);
  }
  
  const auto& get_object_value() const {
    assert(type_ == TYPE_OBJECT);
    return object_value_->data_;
  }

  value& set_object() {
    this->~value();
    return *new(this) value(TYPE_OBJECT);
  }

  // 由于 object 是一个 vector，即由众多 element 组成，调用 begin() 和 end()
  // 可以获得指向 object 头部和尾部的迭代器
  element_iterator element_begin() {
    assert(type_ == TYPE_OBJECT);
    return (object_value_->data_).begin();
  }

  const_element_iterator element_begin() const {
    return const_cast<value&>(*this).element_begin(); 
  }

  // TODO:存在bug 
  element_iterator element_end() {
    assert(type_ == TYPE_OBJECT);
    return object_value_->data_.end();
  }

  const_element_iterator element_end() const {
    return const_cast<value&>(*this).element_end(); 
  }
  
  element_iterator find_element(std::string key);
  const_element_iterator find_element(std::string key) const;

  value& add_element(value&& key, value&& val);

  // TODO:?
  template <typename Value>
  value& add_element(const char* key, Value& val) {
    return add_element(value(key), value(std::forward<Value>(val))); 
  }

  template <typename Value>
  value& add_value(Value&& val) {
    assert(type_ == TYPE_ARRAY);
    array_value_->data_.emplace_back(std::forward<Value>(val));
    return array_value_->data_.back();
  }

  value& operator[] (std::string key);
  const value& operator[] (std::string key) const;
  
  value& operator[] (size_t idx);
  const value& operator[] (size_t idx) const;

private:
  value_type type_;
  
  using string_with_refcount = refcount<std::vector<char>>; 
  using array_with_refcount = refcount<std::vector<value>>;
  using object_with_refcount = refcount<std::vector<element>>;

  union {
    bool bool_value_;
    int32_t int32_value_;
    int64_t int64_value_;
    double double_value_;
    string_with_refcount* string_value_;
    array_with_refcount* array_value_;
    object_with_refcount* object_value_;
  
  };

};

// element 这个类用于表示一个 key-value 键值对，即一个 json 结点
struct element {
  friend class value;
public:
  element(value&& key, value&& value) :
    key_(std::move(key)),
    value_(std::move(value)) {}

  element(std::string key, value&& value) :
    key_(key),
    value_(std::move(value)) {}

public:
    value key_;
    value value_;
};

}


#endif
