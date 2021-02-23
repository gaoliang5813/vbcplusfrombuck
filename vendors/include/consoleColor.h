//=============================================================================
/**
 *  @file   console_color.h
 *
 *  @author Jackie <zhaosb@crearo.com>
 *
 *  @create 2020/04/27
 *
 *  @brief  提供支持Windows和Linux平台终端彩色输出接口。
 *  主要包括：彩色显示，下划线，文本隐藏等功能
 */
//=============================================================================

#ifndef __CONSOLE_COLOR_H__
#define __CONSOLE_COLOR_H__

#include <iostream>

namespace concol
{
#ifdef WIN32
    enum Color
    {
        black = 0, dark_blue, dark_green, dark_aqua, dark_cyan = 3,
        dark_red, dark_purple, dark_pink = 5, dark_magenta = 5,
        dark_yellow, dark_white, gray, blue, green, aqua, cyan = 11,
        red, purple, pink = 13, magenta = 13, yellow, white,
    };
#else
    enum Color
    {
        black = 0, dark_blue, dark_green, dark_aqua, dark_cyan,
        dark_red, dark_purple, dark_pink, dark_magenta,
        dark_yellow, dark_white, gray, blue, green, aqua, cyan,
        red, purple, pink, magenta, yellow, white, default_color,
    };
#endif /* WIN32 */

    const int kLineLen = 100; // 默认打印长度
    const char kLineChar = '-'; // 默认打印字符

    void init();
    void set_text_color(Color color);
    void set_back_color(Color color);
    void set_text_back_color(Color textColor, Color backColor);
    void reset_color();
    void set_title(const char *title);
    void set_blink();
    void set_underline();
    void set_reverse();
    void set_hidden();
    void reset_blink();
    void reset_underline();
    void reset_reverse();
    void reset_hidden();

    Color default_text_color();
    Color default_back_color();

    std::ostream& operator<< (std::ostream& os, Color color);
    std::istream& operator>> (std::istream& is, Color color);

    void print_red_line(char c = kLineChar, int len = kLineLen);
    void print_green_line(char c = kLineChar, int len = kLineLen);
    void print_blue_line(char c = kLineChar, int len = kLineLen);
    void print_cyan_line(char c = kLineChar, int len = kLineLen);
    void print_pink_line(char c = kLineChar, int len = kLineLen);

    // 以下两个函数通常用于程序结束时，接受用户按任意键结束程序
    int get_char();
    int get_char_echo();
}

#endif /*__CONSOLE_COLOR_H__*/
