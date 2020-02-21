#include "Xor.hpp"

string Xor::simplifyOne(string exp)
{
    if (exp.length() < 3)
        return exp;
    string out = "";
    int i;
    for (i = 0; i < exp.length() - 2; i++)
    {
        string temp = "";
        temp += exp.at(i);
        temp += exp.at(i + 1);
        temp += exp.at(i + 2);
        if (Xor::truthTable(temp) == SPACE)
            out += exp.at(i);
        else
        {
            out += Xor::truthTable(temp);
            i += 2;
        }
    }
    if (i == exp.length() - 2)
    {
        out += exp.at(exp.length() - 2);
        i++;
    }
    if (i == exp.length() - 1)
        out += exp.at(exp.length() - 1);
    return out;
}
string Xor::simplify(string exp)
{
    if (exp.length() < 3)
        return exp;
    int len = exp.length();
    exp = Xor::simplifyOne(exp);
    if (exp.length() == 1)
        return exp;
    if (exp.length() != len)
        return Xor::simplify(exp);
    return exp;
}

char Xor::truthTable(string exp)
{
    if (exp.at(1) == XOR)
        if (exp.at(0) == ZERO && exp.at(2) == ZERO)
            return ZERO;
    if (exp.at(0) == ZERO && exp.at(2) == ONE)
        return ONE;
    if (exp.at(0) == ONE && exp.at(2) == ZERO)
        return ONE;
    if (exp.at(0) == ONE && exp.at(2) == ONE)
        return ZERO;
    return SPACE;
}