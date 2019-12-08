/*
 * @Author: your name
 * @Date: 2019-12-06 09:21:02
 * @LastEditTime: 2019-12-08 15:38:09
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \json2\exception.h
 */
#ifndef _EXCEPTION_H_
#define _EXCEPTION_H_

#include <exception>
#include <cassert>

namespace json2 {

// 定义了一个 ERROR_MAP，即错误 map
#define ERROR_MAP(XX)                                             \    
  XX(OK, "ok")                                                    \ 
  XX(ROOT_NOT_SINGULAR, "root not singular")                      \
  XX(BAD_VALUE, "bad value")                                      \
  XX(EXPECT_VALUE, "except value")                                \  
  XX(NUMBER_TOO_BIG, "number too big")                            \
  XX(BAD_STRING_CHAR, "bad character")                            \
  XX(BAD_STRING_ESCAPE, "bad escape")                             \
  XX(BAD_UNICODE_HEX, "bad unicode hex")                          \
  XX(BAD_UNICODE_SURROGATE, "bad unicode surrogate")              \
  XX(MISS_QUOTATION_MARK, "miss quotation mark")                  \
  XX(MISS_COMMA_OR_SQUARE_BRACKET, "miss comma or square bracket")\
  XX(MISS_KEY, "miss key")                                        \
  XX(MISS_COLON, "miss colon")                                    \
  XX(MISS_COMMA_OR_CURLY_BRACKET, "miss comma or curly bracket")  \
  XX(USER_STOPPED, "user stopped parse")                          \

// parse_error 这个 enum 用于表示在 parse json 过程中的各种错误
// 错误形式例如：PARSE_OK, PARSE_ROOT_NET_SINGULAR
// 会跟根据 ERROR_MAP 中的错误类型生成一个 parse 错误 enum 
enum parse_error {
#define GENERATE_ERRNO(e, s) PARSE_##e,
  ERROR_MAP(GENERATE_ERRNO)
#undef GENERATE_ERRNO
};

inline const char* parse_error_str(parse_error err) {
  const static char* tab[] = {
#define GENERATE_STRERR(e, n) n,
    ERROR_MAP(GENERATE_STRERR)
#undef GENERATE_STRERR
  };
  assert(err >= 0 && err < sizeof(tab) / sizeof(tab[0]));
  return tab[err];
}


class json_exception : public std::exception {
public:
  explicit json_exception(parse_error err) :
    err_(err) {}

  parse_error error() const {
    return err_; 
  }
  
  const char* what() const noexcept {
    return parse_error_str(err_); 
  }

private:
  parse_error err_;
};

#undef ERROR_MAP
}

#endif
