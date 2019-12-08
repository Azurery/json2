#ifndef _READER_H_
#define _READER_H_

#include <cassert>
#include <cmath>
#include <string>
#include <vector>
#include <memory>
#include "exception.h"
#include "value.h"

namespace json2 {

// json2 定义了 3 个概念：read_stream、write_stream、handler
// - read_stream：用于读取字节流，目前实现了 file_read_stream 和 string_read_stream，
//              分别从文件和内存中读取
// - write_stream：用于输出字节流，目前实现了 file_writer_stream 和 string_writer_string，
//              分别输出至文件和内存
// - handler：其为解析时的事件处理器，是实现 SAX 风格的 api 的关键。目前实现了 write、pretty_writer、document
//          前两者接收事件后，输出字符串至 write_stream，而 document 接收事件后则构建树形存储结构
//
//
//  用户可以自定义并自行组合这 3 个概念。例如，将多个 handler 串联起来，完成复杂的任务（pretty_writer 内部就串联 writer）
//  这种灵活性


/**
 * @description: Reader 从输入流解析一个 JSON。
 * 当它从流中读取字符时，它会基于 JSON 的语法去分析字符，并向处理器发送事件。

 *  例如，以下是一个 JSON。
 * 
 *  {
 *      "hello": "world",
 *      "t": true ,
 *      "f": false,
 *      "n": null,
 *      "i": 123,
 *      "pi": 3.1416,
 *      "a": [1, 2, 3, 4]
 *  }
 *  
 * 当一个 Reader 解析此 JSON 时，它会顺序地向处理器发送以下的事件：
 * ``` 
 *  StartObject()
 *  Key("hello", 5, true)
 *  String("world", 5, true)
 *  Key("t", 1, true)
 *  Bool(true)
 *  Key("f", 1, true)
 *  Bool(false)
 *  Key("n", 1, true)
 *  Null()
 *  Key("i")
 *  Uint(123)
 *  Key("pi")
 *  Double(3.1416)
 *  Key("a")
 *  StartArray()
 *  Uint(1)
 *  Uint(2)
 *  Uint(3)
 *  Uint(4)
 *  EndArray(4)
 *  EndObject(7)  
 * ```   
 * */
class reader {
public:
  reader(const reader&) = delete;
  reader& operator=(const reader&) = delete;

public:

  template <typename ReadStream, typename Handler>
  static parse_error parse(ReadStream& stream, Handler& handler) {
    try {
      parse_whitespace(stream);
      parse_value(stream, handler);
      parse_whitespace(stream);
      if(stream.has_next()) {
        throw json_exception(PARSE_ROOT_NOT_SINGULAR);
      }
      return PARSE_OK;
    } catch(json_exception& e) {
      return e.error();
    }
  }

private:
#define CALL(expr) \
  if(!(expr)) throw json_exception(PARSE_USER_STOPPED)

  // parse_hex_aux() 函数主要用于 parse Unicode 的辅助函数
  // 每次 parse 4位
  template <typename ReadStream>
  static unsigned parse_hex_aux(ReadStream& stream) {
    unsigned ret = 0;
    for(int i = 0; i < 4; i++) {
      // 将结果左移 4 位，将结果存储在之前的 4 位上
      ret <<= 4;
      // 遍历输出流的 4 位，然后依次翻译
      // 由于是按位修改 ret，而且不能改变之前得到的 ret 的值，所以需要使用
      // 或操作符（|）
      switch(char ch = stream.next()) {
        case '0' ... '9':
          ret |= ch - '0';
          break;
        case 'a' ... 'f':
          ret |= ch - 'a' + 10;
          break;
        case 'A' ... 'F':
          ret |= ch - 'A' + 10;
          break;
        default:
          throw json_exception(PARSE_BAD_UNICODE_HEX);
      } 
    }
    return ret;
  }

  template <typename ReadStream>
  static void parse_whitespace(ReadStream& stream) {
    // 遍历整个 stream，遇到空格(whitespace) 
    while(stream.has_next()) {
      // peek() 主要是将 iter_ 所指向的字符弹出
      char ch = stream.peek(); 
      if(ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
        stream.next();
      } else break;

    }  
  }
 
  // parse_literal_aux() 函数用于解析字面常量值，如 null、NaN，Inf、true、false 等
  template <typename ReadStream, typename Handler>
  static void parse_literal_aux(ReadStream& stream, Handler& handler, 
                            const char* literal, value_type type) {
    char ch = *literal;
    // 用于判断 stream 的下一个字符是否与输入的字面值的首字母相同
    // 如果相同，stream 中的接下来的几个字符有可能是字面常量值
    // 接着，将指针向后移动一个单位 
    stream.assert_next(*literal++);
   
    // 如果没有达到输入字符串的末尾，并且流 stream 接下来的几个字符与输入字符串 literal 相等才截止
    while(*literal != '\0' && *literal == stream.peek()) {
      literal++;
      stream.next();
    }
 
    // 已处理完输入的字符串 literal
    if(*literal == '\0') {
      switch(type) {
        case TYPE_NULL:
          CALL(handler.handle_null());
          return;
        case TYPE_BOOL:
          CALL(handler.handle_bool(ch == 't'));
          return;
        case TYPE_DOUBLE:
          CALL(handler.handle_double(ch == 'N' ? NAN : INFINITY));
          return;
        default:
          assert(false && "incorrect type");
      
      }  
    }
 
    throw json_exception(PARSE_BAD_VALUE);
  }

  template <typename ReadStream, typename Handler>
  static void parse_number(ReadStream& stream, Handler& handler) {
    // parse 'NaN' && 'Infinity'
    // float 或者 double 类型都有 NaN
    if(stream.peek() == 'N') {
      parse_literal_aux(stream, handler, "NaN", TYPE_DOUBLE);
      return;
    } else if(stream.peek() == 'I') {
      parse_literal_aux(stream, handler, "Infinity", TYPE_DOUBLE);
      return;
    }

    auto start = stream.get_iterator();
    // 开始处理数字部分
    // 首先将负号(-)处理掉
    if(stream.peek() == '-')
      stream.next();
    
    // 如果一个数字以 lead-zero 开头，
    if(stream.peek() == '0') {
      stream.next();
      if(isdigit(stream.peek())) 
        throw json_exception(PARSE_BAD_VALUE);
    } else if(is_digit129(stream.peek())) {
      stream.next();
      while(is_digit(stream.peek()))
        stream.next();
    } else 
      throw json_exception(PARSE_BAD_VALUE);

    auto expect_type = TYPE_NULL;
  
    // 主要是处理以下这种形式的浮点数：123.45
    // 处理小数点之后的数字
    if(stream.peek() == '.') {
      expect_type = TYPE_DOUBLE;
      stream.next();
      if(!is_digit(stream.peek()))
        stream.next(); 
    }
    
    // 主要处理以下这种形式的浮点数:123e-2 或者 123.45e-4
    if(stream.peek() == 'e' || stream.peek() == 'E') {
      expect_type = TYPE_DOUBLE;
      stream.next();
      // 这种浮点数的形式为：124E(e)+(-)23 
      if(stream.peek() == '+' || stream.peek() == '-') 
        stream.next();
      // 如果下一个字符不是数字，便是错误的数字形式，直接报错
      if(!is_digit(stream.peek()))
        throw json_exception(PARSE_BAD_VALUE);
    
      stream.next();
      while(is_digit(stream.peek()))
        stream.next();
    }

    // int64 或 int32
    if(stream.peek() == 'i') {
      stream.next();
      if(expect_type == TYPE_DOUBLE)
        throw json_exception(PARSE_BAD_VALUE);
      switch(stream.next()) {
        case '3':
          if(stream.next() != '2') 
            throw json_exception(PARSE_BAD_VALUE);
          expect_type = TYPE_INT32;
          break;
        case '6':
          if(stream.next() != '4')
            throw json_exception(PARSE_BAD_VALUE);
          expect_type = TYPE_INT64;
          break;
        default:
          throw json_exception(PARSE_BAD_VALUE); 
      } 
    } 
    
    auto end = stream.get_iterator();
    if(start == end)
      throw json_exception(PARSE_BAD_VALUE);

    // 上面的的判断过程结束后，就需要将字符串形式的数字转换为数字形式
    try {
      std::size_t idx;
      if(expect_type == TYPE_DOUBLE) {
        double val = __gnu_cxx::__stoa(&std::strtod, "stod", &*start, &idx);
        assert(start + idx == end);
        CALL(handler.handle_double(val));
      } else {
        int64_t val = __gnu_cxx::__stoa(&std::strtol, "stol", &*start, &idx, 10);
        if(expect_type == TYPE_INT64) {
          CALL(handler.handle_int64(val)); 
        } else if (expect_type == TYPE_INT32) {
          // 排除超出 int32_t 范围数字
          if(val > std::numeric_limits<int32_t>::max() ||
             val < std::numeric_limits<int32_t>::min()) {
            throw std::out_of_range("int32_t overflow");
          }
          CALL(handler.handle_int32(static_cast<int32_t>(val)));
        } else if(val > std::numeric_limits<int32_t>::max() ||
                  val < std::numeric_limits<int32_t>::min()) {
          // 这么做的原因：也许在上面的步骤中，有不满足上述要求的数值，即 expect_type 可以为 TYPE_NULL，但
          // 其却是数字
          CALL(handler.handle_int32(static_cast<int32_t>(val)));
        } else {
          CALL(handler.handle_int64(val)); 
        }
      }
    } catch(std::out_of_range& e) {
      throw json_exception(PARSE_NUMBER_TOO_BIG); 
    }
  }

  template <typename ReadStream, typename Handler>
  static void parse_string(ReadStream& stream, Handler& handler, bool is_key) {
    // 如果为 string 形式，则一定以 "" 开始和结尾 
    // 故现在此处进行对 string 的起始进行一个预判断
    // 如果确实是以 " 开头，可初步判断为 string，并将指针后移 
    stream.assert_next('"');
    std::string buffer;
    while(stream.has_next()) {
      switch(char ch = stream.next()) {
        case '"':
          // 有可能是 "" 这种形式的string，其甚至可能是一个 key
          if(is_key) {
            CALL(handler.handle_key(std::move(buffer)));
          } else {
            CALL(handler.handle_string(std::move(buffer)));
          }
          return;
        /*
         * 0x 和 \u 区别，unicode编码
         * - \u 则代表 unicode 编码，是一个字符；
         * - 0x 开头代表十六进制，实际上就是一个整数；
         * - \x 对应的是 UTF-8 编码的数据，通过转化规则可以转换为 Unicode 编码，就能得到对应的汉字，转换规则很简单，先将\x去掉，转换为数字; 
         */ 
        case '\x01'...'\x1f':
          // 由于 0~31 这些都是控制字符，为不可见字符，所以不应该出现在 string 之中
          throw json_exception(PARSE_BAD_STRING_CHAR);
        case '\\':
          // 如果为正确的 string，其形式应该为：\uD1ef
          switch(stream.next()) {
            case '"':
              buffer.push_back('"');  break;
            case '\\': 
              buffer.push_back('\\');  break;
            case '/': 
              buffer.push_back('\\');  break;
            case 'b': 
              buffer.push_back('\b');  break;
            case 'f': 
              buffer.push_back('\f');  break;
            case 'n': 
              buffer.push_back('\n');  break;
            case 'r': 
              buffer.push_back('\r');  break;
            case 't': 
              buffer.push_back('\t');  break;
            case 'u': {
              // 如果是类似 \u123d 这种形式，便是 Unicode 形式
              unsigned high_surrogate = parse_hex_aux(stream);
              if(high_surrogate >= 0xD800 && high_surrogate <= 0xDBFF) {
                /* unicode 理解
                 *  1. Unicode
                 *    我们知道 ASCII，它是一种字符编码，把 128 个字符映射至整数 0 ~ 127。
                 *    例如，1 -> 49，A -> 65，B -> 66 等等。这种 7-bit 字符编码系统非常简单，
                 *    在计算机中以一个字节存储一个字符。然而，它仅适合美国英语，甚至一些英语中常用的标点符号、重音符号都不能表示，
                 *    无法表示各国语言，特别是中日韩语等表意文字。
                 *
                 *    在 Unicode 出现之前，各地区制定了不同的编码系统，如中文主要用 GB 2312 和大五码、日文主要用 JIS 等。
                 *    这样会造成很多不便，例如一个文本信息很难混合各种语言的文字。
                 *    
                 *    后来，多个机构成立了 Unicode 联盟，在 1991 年释出 Unicode 1.0，收录了 24 种语言共 7161 个字符。
                 *    在 2016年，Unicode 已释出 9.0 版本，收录 135 种语言共 128237 个字符。
                 * 
                 *    这些字符被收录为统一字符集（Universal Coded Character Set, UCS），每个字符映射至一个整数码点（code point），
                 *    码点的范围是 0 至 0x10FFFF，码点又通常记作 U+XXXX，当中 XXXX 为 16 进位数字。
                 *    例如 蔡 --> \u8521 徐 --> \u5f90 坤 --> \u5764。很明显，UCS 中的字符无法像 ASCII 般以一个字节存储。
                 * 
                 *    因此，Unicode 还制定了各种储存码点的方式，这些方式称为 Unicode 转换格式（Uniform Transformation Format, UTF）。
                 *    现时流行的 UTF 为 UTF-8、UTF-16 和 UTF-32。每种 UTF 会把一个码点储存为一至多个编码单元（code unit）。
                 *    例如 UTF-8 的编码单元是 8 位的字节、UTF-16 为 16 位、UTF-32 为 32 位。除 UTF-32 外，
                 *    UTF-8 和 UTF-16 都是可变长度编码。
                 * 
                 *    UTF-8 成为现时互联网上最流行的格式，有几个原因：
                 *    - 它采用字节为编码单元，不会有字节序（endianness）的问题。
                 *    - 每个 ASCII 字符只需一个字节去储存。
                 *    - 如果程序原来是以字节方式储存字符，理论上不需要特别改动就能处理 UTF-8 的数据。
                 *  2. 需求
                 *  由于 UTF-8 的普及性，大部分的 JSON 也通常会以 UTF-8 存储。我们的 JSON 库也会只支持 UTF-8。
                 *  C 标准库没有关于 Unicode 的处理功能（C++11 有），我们会实现 JSON 库所需的字符编码处理功能。
                 *
                 *  对于非转义（unescaped）的字符，只要它们不少于 32（0 ~ 31 是不合法的编码单元），我们可以直接复制至结果。
                 *  我们假设输入是以合法 UTF-8 编码。
                 * 
                 *  而对于 JSON字符串中的 \uXXXX 是以 16 进制表示码点 U+0000 至 U+FFFF，我们需要：
                 *  - 解析 4 位十六进制整数为码点；
                 *  - 由于字符串是以 UTF-8 存储，我们要把这个码点编码成 UTF-8。
                 *
                 *  我们可能会发现，4 位的 16 进制数字只能表示 0 至 0xFFFF，但 UCS 的码点是从 0 至 0x10FFFF，
                 *  那怎么能表示多出来的码点？
                 *  
                 *  其实，U+0000 至 U+FFFF 这组 Unicode 字符称为基本多文种平面（basic multilingual plane, BMP），
                 *  还有另外 16 个平面。那么 BMP 以外的字符，JSON 会使用代理对（surrogate pair）表示 \uXXXX\uYYYY。
                 *  在 BMP 中，保留了 2048 个代理码点。
                 *  - 如果第一个码点是 U+D800 至 U+DBFF，我们便知道它的代码对的高代理项（high surrogate），
                 *    之后应该伴随一个 U+DC00 至 U+DFFF 的低代理项（low surrogate）。
                 *    然后，我们用下列公式把代理对 (H, L) 变换成真实的码点：
                 *
                 *    ```
                 *      codepoint = 0x10000 + (H − 0xD800) × 0x400 + (L − 0xDC00)
                 *    ``` 
                 *  举个例子，高音谱号字符 ? -> U+1D11E 不是 BMP 之内的字符。在 JSON 中可写成转义序列 \uD834\uDD1E，
                 *  我们解析第一个 \uD834 得到码点 U+D834，我们发现它是 U+D800 至 U+DBFF 内的码点，所以它是高代理项。
                 *  然后我们解析下一个转义序列 \uDD1E 得到码点 U+DD1E，它在 U+DC00 至 U+DFFF 之内，是合法的低代理项。
                 *  我们计算其码点：
                 *    ```
                 *      H = 0xD834, L = 0xDD1E
                 *      codepoint = 0x10000 + (H − 0xD800) × 0x400 + (L − 0xDC00)
                 *                = 0x10000 + (0xD834 - 0xD800) × 0x400 + (0xDD1E − 0xDC00)
                 *                = 0x10000 + 0x34 × 0x400 + 0x11E
                 *                = 0x10000 + 0xD000 + 0x11E
                 *                = 0x1D11E
                 *    ``` 
                 *  这样就得出这转义序列的码点，然后我们再把它编码成 UTF-8。如果只有高代理项而欠缺低代理项，
                 *  或是低代理项不在合法码点范围，我们都返回 PARSE_INVALID_UNICODE_SURROGATE 错误。
                 *  如果 \u 后不是 4 位十六进位数字，则返回 PARSE_INVALID_UNICODE_HEX 错误。 
                 * 
                 *  UTF-8 的编码单元是 8 位字节，每个码点编码成 1 至 4 个字节。
                 *  它的编码方式很简单，按照码点的范围，把码点的二进位分拆成 1 至最多 4 个字节  
                 *
                 * // 	码点范围			    码点位数		 字节1		    字节2        字节3       字节4
                 * // 	U+0000 ~ U+0007F   7		    0xxxxxxx
                 * // 	U+0080 ~ U+07FF	   11	    	110xxxxx    10xxxxxx
                 * // 	U+0800 ~ U+FFFF	   16	    	1110xxxx    10xxxxxx     10xxxxxx
                 * // 	U+10000 ~ U+10FFFF 21	    	11110xxx    10xxxxxx     10xxxxxx    10xxxxxx  
                 *
                 *  这个编码方法的好处之一是，码点范围 U+0000 ~ U+007F 编码为一个字节，与 ASCII 编码兼容。
                 *  这范围的 Unicode 码点也是和 ASCII 字符相同的。因此，一个 ASCII 文本也是一个 UTF-8 文本。
                 * 
                 *  我们举一个例子解析多字节的情况，欧元符号 € -> U+20AC：
                 *  - U+20AC 在 U+0800 ~ U+FFFF 的范围内，应编码成 3 个字节。
                 *  - U+20AC 的二进位为 10000010101100
                 *  - 3 个字节的情况我们要 16 位的码点，所以在前面补两个 0，成为 0010000010101100
                 *  - 按上表把二进位分成 3 组：0010, 000010, 101100
                 *  - 加上每个字节的前缀：11100010, 10000010, 10101100
                 *  - 用十六进位表示即：0xE2, 0x82, 0xAC
                 *   
                 * 对于这例子的范围，对应的 C 代码是这样的：
                 *  ```c  
                 *    if (u >= 0x0800 && u <= 0xFFFF) {
                 *        OutputByte(0xE0 | ((u >> 12) & 0xFF)); // 0xE0 = 11100000 
                 *        OutputByte(0x80 | ((u >>  6) & 0x3F)); // 0x80 = 10000000 
                 *        OutputByte(0x80 | ( u        & 0x3F)); // 0x3F = 00111111 
                 *    }
                 *  ```
                 */
                  int val = 0;
                  if(stream.next() != '\\')
                    // 因为对于高代理项而言，应是这种形式:\uXXXX\uYYYY  
                    throw json_exception(PARSE_BAD_UNICODE_SURROGATE);
                  if(stream.next() != 'u')
                    throw json_exception(PARSE_BAD_UNICODE_SURROGATE);
                  
                  unsigned high_surrogate = parse_hex_aux(stream);
                  if(high_surrogate >= 0xDC00 && high_surrogate <= 0xDFFF) {
                    val = 0x10000 + (high_surrogate - 0xD800) * 0x400 + (high_surrogate - 0xDC00);
                  } else {
                    throw json_exception(PARSE_BAD_UNICODE_SURROGATE);
                  }
                  
                  encode_utf8(buffer, val);
                  break;
              } 
            } default:
                throw json_exception(PARSE_BAD_STRING_ESCAPE);
          }
          break;
        default:
          buffer.push_back(ch);
      }
    } 
    throw json_exception(PARSE_MISS_QUOTATION_MARK);
  }

  template <typename ReadStream, typename Handler>
  static void parse_value(ReadStream& stream, Handler& handler) {
    if(!stream.has_next())  
      throw json_exception(PARSE_EXPECT_VALUE);
    switch(stream.peek()) {
      case 'n': 
        return parse_literal_aux(stream, handler, "null", TYPE_NULL);
      case 't': 
        return parse_literal_aux(stream, handler, "true", TYPE_BOOL);
      case 'f': 
        return parse_literal_aux(stream, handler, "false", TYPE_BOOL);
      case '"': 
        return parse_string(stream, handler, false);
      case '[': 
        return parse_array(stream, handler);
      case '{': 
        return parse_object(stream, handler);
      default:
        return parse_number(stream, handler); 
    }
  }

  template <typename ReadStream, typename Handler>
  static void parse_array(ReadStream& stream, Handler& handler) {
    CALL(handler.handle_start_array());
    stream.assert_next('['); 
    parse_whitespace(stream);
    // 处理空 array 
    if(stream.peek() == ']') {
      stream.next();
      CALL(handler.handle_end_array());
      return;
    }

    while(true) {
      parse_value(stream, handler);
      parse_whitespace(stream);
      switch(stream.next()) {
        case ',':
          // 如果遇到 , 说明 array 中还有其他项，需要继续往后 parse 
          parse_whitespace(stream);
          break;
        case ']':
          CALL(handler.handle_end_array());
          return;
        default:
          throw json_exception(PARSE_MISS_COMMA_OR_SQUARE_BRACKET);
      }
    }
  }

  template <typename ReadStream, typename Handler>
  static void parse_object(ReadStream& stream, Handler& handler) {
    CALL(handler.handle_start_object());
    stream.assert_next('{');
    parse_whitespace(stream);
    // 用于处理空 object 
    if(stream.peek() == '}') {
      stream.next();
      CALL(handler.handle_end_object());
      return;
    }

    while(true) {
      if(stream.peek() != '"')
        // object 的 key 的类型必须为 string
        throw json_exception(PARSE_MISS_KEY);
      // parse key      
      parse_string(stream, handler, true);

      // parse ':'
      parse_whitespace(stream);
      if(stream.next() != ':')
        throw json_exception(PARSE_MISS_COLON);

      // parse value
      parse_whitespace(stream);
      parse_value(stream, handler);
      switch(stream.next()) {
        case ',':
          parse_whitespace(stream);
          break;
        case '}':
          CALL(handler.handle_end_object());
          return;
        default:
          throw json_exception(PARSE_MISS_COMMA_OR_CURLY_BRACKET);
      }
    }
  }
#undef CALL 

private:
  static bool is_digit(char ch) {
    return ch >= '0' && ch <= '9';
  }

  static bool is_digit129(char ch) {
    return ch >= '1' && ch <= '9'; 
  }

// #pragma GCC diagnostic 为 gcc 编译器警告
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
  // 	码点范围			    码点位数		 字节1		    字节2        字节3       字节4
  // 	U+0000 ~ U+0007F   7		    0xxxxxxx
  // 	U+0080 ~ U+07FF	   11	    	110xxxxx    10xxxxxx
  // 	U+0800 ~ U+FFFF	   16	    	1110xxxx    10xxxxxx     10xxxxxx
  // 	U+10000 ~ U+10FFFF 21	    	11110xxx    10xxxxxx     10xxxxxx    10xxxxxx  

  static void encode_utf8(std::string& buffer, unsigned val) {
    switch(val) {
      case 0x00 ... 0x7F:
        buffer.push_back(val & 0xFF); 
        break;
     
         //0xC0 ===> 1100 0000
			   //0x80 ===> 1000 0000
			   //0x3F ===> 0011 1111
           
			   //U+0080 ~ U+07FF 
      case 0x080 ... 0x7FF:
        buffer.push_back(0xC0 | ((val >> 6) & 0xFF));
        buffer.push_back(0x80 | (val & 0x3F));
        break;
      case  0x0800 ... 0xFFFF:
        buffer.push_back(0xE0 | ((val >> 12) & 0xFF));
        buffer.push_back(0x80 | ((val >> 6) & 0x3F));
        buffer.push_back(0x80 | (val & 0x3F));
        break;
      case 0x010000 ... 0x10FFFF:
        buffer.push_back(0xF0 | ((val >> 18) & 0xFF));
        buffer.push_back(0x80 | ((val >> 12) & 0x3F));
        buffer.push_back(0x80 | ((val >> 6) & 0x3F));
        buffer.push_back(0x80 | (val & 0x3F));
        break;
      default:
        assert(false && "out of range"); 
    }
  }
#pragma GCC diagnostic pop
};

}
#endif
