#include "value.h"

namespace json2 {

// TODO：感觉没必要特意对 string_value_ 进行置空
value::value(value_type type) :
  type_(type),
  string_value_(nullptr) {
  
  switch(type_) { 
    case TYPE_NULL:
    case TYPE_BOOL:
    case TYPE_INT32:
    case TYPE_INT64:
    case TYPE_DOUBLE:
      break;
    case TYPE_STRING:
      string_value_ = new string_with_refcount(); 
    case TYPE_ARRAY:
      array_value_ = new array_with_refcount(); 
    case TYPE_OBJECT:
      object_value_ = new object_with_refcount();
    default:
      assert(false && "incorrect value type!");
  } 
}

value::value(const value& rhs) :
  type_(rhs.type_),
  array_value_(rhs.array_value_) {
  
  switch(type_) {
    case TYPE_NULL:
    case TYPE_BOOL:
    case TYPE_INT32:
    case TYPE_INT64:
    case TYPE_DOUBLE:
      break;
    // 对于以下三种复杂类型，由于它们本身带有 refcount，
    // 所以以别的 json 对象直接拷贝构造当前对象时，需要将 refcount 的值加 1
    case TYPE_STRING:
      string_value_->increment_and_get();
      break;
    case TYPE_ARRAY:
      array_value_->increment_and_get();
      break;
    case TYPE_OBJECT:
      object_value_->increment_and_get();
      break;
    default:
      assert(false && "incorrect value type!"); 
  }
  
}

// 当以一个 json 右值的形式构造当前对象之后，原先的 json 被转移到当前对象，
// 所以需要对其已亡值进行置空
value::value(value&& rhs) :
  type_(rhs.type_),
  array_value_(rhs.array_value_) {
  rhs.type_ = TYPE_NULL;
  rhs.array_value_ = nullptr;
}

value& value::operator=(const value& rhs) {
  assert(this != &rhs);
  this->~value();
  type_ = rhs.type_;
  array_value_ = rhs.array_value_;
  switch(type_) {
    case TYPE_NULL:
    case TYPE_BOOL:
    case TYPE_INT32:
    case TYPE_INT64: 
    case TYPE_DOUBLE:
      break;

    case TYPE_STRING:
      string_value_->increment_and_get();
      break;
    case TYPE_ARRAY:
      array_value_->increment_and_get();
      break;
    case TYPE_OBJECT:
      object_value_->increment_and_get();
      break;
    default:
      assert(false && "incorrect value type");
  }
  return *this;  
}

value& value::operator=(value&& rhs) {
  assert(this != &rhs);
  this->~value();
  type_ = rhs.type_;
  array_value_ = rhs.array_value_;
  rhs.type_ = TYPE_NULL;
  rhs.array_value_ = nullptr;
  return *this;
} 

value::~value() {
    
  switch(type_) {
    case TYPE_NULL:
    case TYPE_BOOL:
    case TYPE_INT32:
    case TYPE_INT64:
    case TYPE_DOUBLE:
      break;
    case TYPE_STRING:
      if(string_value_->decrement_and_get() == 0)
        delete string_value_;
      break;
    case TYPE_ARRAY:
      if(array_value_->decrement_and_get())
        delete array_value_;
      break;
    case TYPE_OBJECT:
      if(object_value_->decrement_and_get())
        delete object_value_;
      break;
    default:
      assert(false && "incorrect value type!");
  }
}

value::element_iterator value::find_element(std::string key) {
  // 只有 object 对象才会有多个 key/vale 对 
  assert(type_ == TYPE_OBJECT);
  // 主要是通过在 object 中查找是否存在传入的 key 
  return std::find_if(
      object_value_->data_.begin(),
      object_value_->data_.end(),
      // 个人觉得这样写也可以
      // [key](const element& elem) {
      //    return elem.key_.get_string_value() == key;
      [key](const element& elem) -> bool {
        return elem.key_.get_string_value() == key; 
      });
}

// const 版本其实就是在先将 *this 指针强转为 cosnt 类型，再调用非 const
// 版本即可
value::const_element_iterator value::find_element(std::string key) const {
  return const_cast<value&>(*this).find_element(key);
}

value& value::add_element(value&& key, value&& val) {
  // 1. 由于 json 对象的 key 的类型必须是字符串，所以在先检查
  // 2. 只可以在 json 对象中加入 key/value 对，故需要满足值类型必须是 object
  // 3. 由于在 json 对象中，key 的值是唯一的，需提前检查确认 
  assert(type_ == TYPE_OBJECT);
  assert(key.type_ == TYPE_STRING);
  assert(find_element(key.get_string_value()) == element_end());
  // 只需要在 object_value_ 这个 vector 插入即可，只是此处没有使用 push_back，而是使用
  // emplace_back，这是出于直接 push_back 操作会存在拷贝赋值，而利用 c++11 的右值特性，可以
  // 省去拷贝带来的消耗
  object_value_->data_.emplace_back(std::move(key), std::move(val));
  return object_value_->data_.back().value_;
}

value& value::operator[] (std::string key) {
  assert(type_ == TYPE_OBJECT);
  auto iter = find_element(key);
  if(iter != object_value_->data_.end()) 
    return iter->value_;
  // TODO: 构造函数有问题
  // else
  //   return value(TYPE_NULL);
}

const value& value::operator[] (std::string key) const {
  return const_cast<value&>(*this)[key]; 
}

value& value::operator[] (size_t idx) {
  assert(type_ == TYPE_ARRAY);
  return array_value_->data_[idx];
}

const value& value::operator[] (size_t idx) const {
  assert(type_ == TYPE_ARRAY);
  return array_value_->data_[idx];
}


}

