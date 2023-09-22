/// DO NOT EDIT THIS FILE. 

// This header file contains signatures for some functions/classes


#pragma once
#include <string>
#include <unordered_map>
#include <vector>

/// splits string into tokens (words)
/// separators are sequences of white spaces
std::vector<std::string> split(const std::string & str);

/// HIDDEN HINT: this "may" help you get a bit more performance
/// in your cycle finding algorithm, since arrays are faster
/// than hash tables... if you don't know what that means,
/// safely ignore this hint :)
///
/// utility class you can use to convert words to unique integers
/// get(word) returns the same number given the same word
///           the numbers will start at 0, then 1, 2, ...
///
/// example:
///   Word2Int w2i;
///   w2i.get("hello") = 0
///   w2i.get("world") = 1
///   w2i.get("hello") = 0
///
class Word2Int {
    int counter = 0;
    std::unordered_map<std::string, int> myset;

public:
    int get(const std::string & w);
};
