/*
 * Parser.cpp
 *
 *  Created on: May 29, 2020
 *      Author: elias
 */

#include "Parser.h"

Parser::Parser(string path)
{
    // initialise variables
    lexer = Lexer(path);
    isFill = false;
    fill = false;
    isCount = false;

    // initialise extra parser
    functionParser = FunctionParser();
    tableParser = TableParser();
}

vector<TableData> Parser::parse()
{
    nextToken();
    while (!lexer.isFinished())
        parseNext();

    return tables;
}

// ------------------------------------ main parser function ------------------------------------
void Parser::parseNext()
{
    if (isToken(PIN))
        return parsePin();

    if (isToken(TABLE))
        return parseTable();

    if (isToken(Token::Type::identifier))
        return parseIdentifier();

    syntaxError(Token::Type::identifier);
}

void Parser::parsePin()
{
    string str;
    pair<string, uint32_t> temp;

    expect(PIN);
    str = currentToken.value;
    if (isToken(Token::Type::boolean))
        nextToken();
    else
        expect(Token::Type::number);
    temp.second = str2Int(str);
    expect("=");
    str = currentToken.value;
    expect(Token::Type::identifier);
    temp.first = str;
#ifdef WITH_SEMICOLON
    expect(";");
#endif
    alias.push_back(temp);
}

void Parser::parseTable()
{
    isFill = false;
    fill = false;
    isCount = false;

    tableParser.setLineIndex(lexer.getLineIndex());

    expect(TABLE);
    expect("(");
    vector<uint32_t> inPins = getIdentifier();
    expect("=>");
    vector<uint32_t> outPins = getIdentifier();
    expect(")");
    extraFunction();
    expect("{");
    vector<bool> boolTable = getBooltable();
    expect("}");
#ifdef WITH_SEMICOLON
#ifdef WITH_SEMICOLON_TABLE
    expect(";");
#endif
#endif

    vector<TableData> temp;
    if (isCount)
        temp = tableParser.getTableDataCount(boolTable, inPins, outPins);
    else if (isFill)
        temp = tableParser.getTableDataFill(boolTable, inPins, outPins, fill);
    else
        temp = tableParser.getTableData(boolTable, inPins, outPins);

    for (TableData t : temp)
        tables.push_back(t);
}

void Parser::parseIdentifier()
{
    string pinName = currentToken.value;
    expect(Token::Type::identifier);
    if (isToken("."))
    {
        insertDFF(str2Pin(pinName));
        expect(".");
        expect(DFF);
#ifdef WITH_SEMICOLON
        expect(";");
#endif
        return;
    }

    expect("=");
    vector<Token> expression = getExpression();
#ifdef WITH_SEMICOLON
    expect(";");
#endif
    tables.push_back(assembleTableFromFunc(pinName, expression));
}

// ------------------------------------ helper for parsePin() ------------------------------------
string Parser::pin2Str(uint32_t pin)
{
    for (pair<string, uint32_t> p : alias)
        if (p.second == pin)
            return p.first;

#ifdef LANG_DE
    parsingError("Pin " + to_string(pin) + " ist nicht diffident");
#else
    parsingError("pin " + to_string(pin) + " is not diffident");
#endif
    return "";
}

uint32_t Parser::str2Pin(string pinName)
{
    for (pair<string, uint32_t> p : alias)
        if (p.first == pinName)
            return p.second;
#ifdef LANG_DE
    parsingError("Pin " + pinName + " ist nicht diffident");
#else
    parsingError("pin " + pinName + " is not diffident");
#endif
    return 0;
}

// ------------------------------------ helper for tableParse() ------------------------------------
vector<uint32_t> Parser::getIdentifier()
{
    Token t;
    vector<uint32_t> pins;

    t = currentToken;
    expect(Token::Type::identifier);
    pins.push_back(str2Pin(t.value));

    while (isToken(","))
    {
        expect(",");
        t = currentToken;
        expect(Token::Type::identifier);
        pins.push_back(str2Pin(t.value));
    }

    return pins;
}

void Parser::extraFunction()
{
    if (!isToken("."))
        return;

    expect(".");
    if (isToken(COUNT))
    {
        expect(COUNT);
        isCount = true;
        return;
    }

    expect(FILL);
    isFill = true;
    expect("(");
    string temp = currentToken.value;
    expect(Token::Type::boolean);
    expect(")");

    if (temp.size() != 1)
        return syntaxError(ZERO + " or " + ONE);

    if (temp.at(0) == ONE)
        fill = true;
}

vector<bool> Parser::getBooltable()
{
    vector<bool> boolTable;
    insertBooltable(boolTable, currentToken.value);
    expect(Token::Type::boolean);
    while (isToken(Token::Type::boolean))
    {
        insertBooltable(boolTable, currentToken.value);
        expect(Token::Type::boolean);
    }
    return boolTable;
}

void Parser::insertBooltable(vector<bool> &boolTable, string str)
{
    for (uint32_t i = 0; i < str.size(); i++)
        if (str.at(i) == ONE)
            boolTable.push_back(true);
        else
            boolTable.push_back(false);
}

// ------------------------------------ helper for parseIdentifier() ------------------------------------
void Parser::insertDFF(uint32_t pin)
{
    for (uint32_t i = 0; i < tables.size(); i++)
        if (tables.at(i).m_OutputPin == pin)
        {
            tables.at(i).m_EnableFlipFlop = true;
            return;
        }
#ifdef LANG_DE
    parsingError(pin2Str(pin) + " ist kein Ausgangspin oder ist noch nicht definiert");
#else
    parsingError(pin2Str(pin) + " is not an output pin or not yet defined");
#endif
}

vector<Token> Parser::getExpression()
{
    vector<Token> tokens;
    if (!validExpression())
        syntaxError(Token::Type::identifier);

#ifdef WITH_SEMICOLON
    while (validExpression())
    {
        tokens.push_back(currentToken);
        nextToken();
    }
#else
    uint32_t lineNum = lexer.getLineIndex();
    while (validExpression() && lineNum == lexer.getLineIndex())
    {
        tokens.push_back(currentToken);
        nextToken();
    }
#endif

    return tokens;
}

bool Parser::validExpression()
{
    if (isToken(Token::Type::symbol) && currentToken.value.size() == 1)
        switch (currentToken.value.at(0))
        {
        case AND:
        case OR:
        case XOR:
        case NOT:
        case '(':
        case ')':
            return true;
        default:
            return false;
        }

    if (isToken(Token::Type::identifier))
        return true;
    return false;
}

TableData Parser::assembleTableFromFunc(string outName, vector<Token> expression)
{
    TableData table;
    table.m_EnableFlipFlop = false;
    table.m_OutputPin = str2Pin(outName);

    table.m_Table = functionParser.parse(expression, lexer.getLineIndex());

    vector<uint32_t> inPins;
    for (string inName : functionParser.getNames())
        inPins.push_back(str2Pin(inName));
    table.m_InputPins = inPins;

    return table;
}

// ------------------------------------ helpful functions ------------------------------------
void Parser::nextToken() { currentToken = lexer.next(); }

bool Parser::isToken(char expected) { return currentToken.value == "" + expected; }
bool Parser::isToken(string expected) { return currentToken.value == expected; }
bool Parser::isToken(Token::Type expected) { return currentToken.type == expected; }

void Parser::expect(char expected) { expect("" + expected); }
void Parser::expect(string expected)
{
    if (currentToken.value != expected)
        syntaxError(expected);
    nextToken();
}
void Parser::expect(Token::Type expected)
{
    if (currentToken.type != expected)
        syntaxError(expected);
    nextToken();
}

uint32_t Parser::str2Int(string str)
{
    uint32_t result = 0;
    for (uint32_t i = 0; i < str.size(); i++)
        result += getInt(str.at(i)) * pow(10, str.size() - i - 1);
    return result;
}

uint32_t Parser::getInt(char c)
{
    switch (c)
    {
    case '0':
        return 0;
    case '1':
        return 1;
    case '2':
        return 2;
    case '3':
        return 3;
    case '4':
        return 4;
    case '5':
        return 5;
    case '6':
        return 6;
    case '7':
        return 7;
    case '8':
        return 8;
    case '9':
        return 9;
    default:
#ifdef LANG_DE
        parsingError(c + " ist keine Zahl");
#else
        parsingError(c + " is not a number");
#endif
        return 0;
    }
}

// ------------------------------------ error handling ------------------------------------
void Parser::parsingError(string input) { Error::makeError(Error::Type::parsing, lexer.getLineIndex(), input); }
void Parser::syntaxError(string expected) { Error::makeError(Error::Type::syntax, lexer.getLineIndex(), currentToken, expected); }
void Parser::syntaxError(Token::Type expected) { Error::makeError(Error::Type::syntax, lexer.getLineIndex(), currentToken, expected); }