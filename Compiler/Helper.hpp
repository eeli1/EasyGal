#pragma once
#include <string>
#include <iostream>
#include <cmath>
#include <vector>
using namespace std;
void error(string);
namespace Helper
{
int char2Int(char C);
int str2Int(string Str);
vector<bool> int2Bool(int);
vector<vector<bool>> generateTable2D(int);
int bool2Int(vector<bool>);
} // namespace Helper