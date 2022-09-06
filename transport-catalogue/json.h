#pragma once

#include <algorithm>
#include <cassert>
#include <exception>
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace json {

class Document;

class Node;

using Dict = std::map<std::string, Node>;

using Array = std::vector<Node>;

namespace detail {

struct NodePrinter;

struct PrintContext;

}// end namespace detail

// Эта ошибка должна выбрасываться при ошибках парсинга JSON
class ParsingError : public std::runtime_error {
public:
	using runtime_error::runtime_error;
};

class Node final
	: private std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string> {
public:
	using variant::variant;
	using Value = variant;

	Node(Value value)
		:variant(std::move(value)) {}

	bool IsArray() const;
	bool IsBool() const;
	bool IsDouble() const;
	bool IsInt() const;
	bool IsNull() const;
	bool IsDict() const;
	bool IsPureDouble() const;
	bool IsString() const;
	bool IsEqual(const Node& other) const;
	const Array& AsArray() const;
	bool AsBool() const;
	int AsInt() const;
	double AsDouble() const;
	const Dict& AsMap() const;
	const std::string& AsString() const;
	const Value& GetValue() const;
	Value& GetValue();
	void PrintValue(detail::PrintContext context) const;
};

bool operator==(const Node& lhs, const Node& rhs);

bool operator!=(const Node& lhs, const Node& rhs);

//Хранит json::Node, возвращает на него ссылку при вызове .GetRoot()
class Document {
public:
	explicit Document(Node root);

	const Node& GetRoot() const;

private:
	Node root_;
};

//Загружает корневой узел из потока ввода
Document Load(std::istream& input);

void Print(const Document& doc, std::ostream& output);

namespace detail {

// Контекст вывода, хранит ссылку на поток вывода и текущий отсуп
struct PrintContext {
	std::ostream& out;
	int indent_step = 2;
	int indent = 0;

	// Выводит текущий отступ
	void PrintIndent() const;

	// Возвращает новый контекст вывода с увеличенным смещением
	PrintContext Indented() const;
};

//Посетитель для вывода контента
struct NodePrinter {
	PrintContext context;
	void operator()(std::nullptr_t) const;
	void operator()(const Array& array) const;
	void operator()(const Dict& dict) const;
	void operator()(bool value) const;
	void operator()(int value) const;
	void operator()(double value) const;
	void operator()(const std::string& str) const;
};

//Выводит строку, заключенную в кавычки и с заданным отступом
void PrintString(const std::string& str, PrintContext context);

//Выводит строку с экранированием сиволов Escape - последовательностей
void PrintWithEscapeSeq(const std::string& str, std::ostream& out);

}// end namespace detail

//Функции парсинга строки и формирования Node
namespace parsing{

Node LoadNode(std::string_view str);

Node LoadDict(std::string_view str);

Node LoadArray(std::string_view str);

Node LoadString(std::string_view str);

Node LoadInt(std::string_view str);

Node LoadDouble(std::string_view str);

//Фунцкии-помощники парсинга
namespace helpers {

char UnshildedChar(char c);

char ShieldedChar(char c);

size_t FindNextUnshildedQuote(std::string_view sv, size_t from = 0);

size_t FindValueBegin(std::string_view sv, size_t from = 0);

size_t FindValueEnd(std::string_view sv, size_t from = 0);

size_t FindKeyBegin(std::string_view sv, size_t from = 0);

size_t FindkeyEnd(std::string_view sv, size_t key_begin = 0);

}//end namespace parsing

}//end namespace helpers

}//end namespace json