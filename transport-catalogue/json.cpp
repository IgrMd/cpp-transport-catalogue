#include "json.h"

using namespace std::literals;

namespace json {

static const std::string_view SPACES = " \n\r\t"sv; //Spaces
static const std::string_view CHARS_TO_SHIELD = "\r\n\"\\"sv; //Escape sequences to shield
static const std::string_view DIGITS = "-0123456789xX"sv;

using Variant = std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string>;

// ---------- Node ------------------
//// Check Type
bool Node::IsArray() const {
	return std::holds_alternative<Array>(*this);
}

bool Node::IsBool() const {
	return std::holds_alternative<bool>(*this);
}

bool Node::IsDouble() const { //Возвращает true, если в Node хранится int либо double.
	return std::holds_alternative<double>(*this) || IsInt();
}

bool Node::IsInt() const {
	return std::holds_alternative<int>(*this);
}

bool Node::IsPureDouble() const { //Возвращает true, если в Node хранится double.
	return std::holds_alternative<double>(*this);
}

bool Node::IsString() const {
	return std::holds_alternative<std::string>(*this);
}

bool Node::IsNull() const {
	return std::holds_alternative<std::nullptr_t>(*this);
}

bool Node::IsDict() const {
	return std::holds_alternative<Dict>(*this);
}

bool Node::IsEqual(const Node& other) const {
	return static_cast<Variant>(*this) == static_cast<Variant>(other);
}

//Return value
const Array& Node::AsArray() const {
	if (!IsArray()) { throw std::logic_error("Not an array"s); }
	return std::get<Array>(*this);
}

bool Node::AsBool() const {
	if (!IsBool()) { throw std::logic_error("Not a bool"s); }
	return std::get<bool>(*this);
}

const Dict& Node::AsMap() const {
	if (!IsDict()) { throw std::logic_error("Not a map"s); }
	return std::get<Dict>(*this);
}

int Node::AsInt() const {
	if (!IsInt()) { throw std::logic_error("Not an int"s); }
	return std::get<int>(*this);
}

double Node::AsDouble() const {
	if (!IsDouble()) { throw std::logic_error("Not a double"s); }
	return IsPureDouble() ? std::get<double>(*this) : static_cast<double>(AsInt());
}

const std::string& Node::AsString() const {
	if (!IsString()) { throw std::logic_error("Not a string"s); }
	return std::get<std::string>(*this);
}

const Node::Value& Node::GetValue() const {
	return *this;
}

Node::Value& Node::GetValue() {
	return *this;
}

//Other
void Node::PrintValue(detail::PrintContext context) const {
	std::visit(detail::NodePrinter{ context }, static_cast<Variant>(*this));
}
bool operator==(const Node& lhs, const Node& rhs) {
	return lhs.IsEqual(rhs);
}
bool operator!=(const Node& lhs, const Node& rhs) {
	return !lhs.IsEqual(rhs);
}

// ---------- Document ------------------
Document::Document(Node root)
	: root_(std::move(root)) {
}

const Node& Document::GetRoot() const {
	return root_;
}

Document Load(std::istream& input) {
	std::vector<std::string> inp_data;
	std::string line;
	while (!input.eof()) {
		std::getline(input, line);
		inp_data.push_back(std::move(line));
	}
	if (inp_data.empty()) {
		throw json::ParsingError("Empty File");
	}
	for (const auto& str : inp_data) {
		line.append(str);
	}
	return Document{ parsing::LoadNode(line) };
}

void Print(const Document& doc, std::ostream& output) {
	detail::PrintContext context{ output };
	doc.GetRoot().PrintValue(context);
}

namespace detail {

// ---------- PrintContext ------------------
void PrintContext::PrintIndent() const {
	for (int i = 0; i < indent; ++i) {
		out.put(' ');
	}
}

PrintContext PrintContext::Indented() const {
	return { out, indent_step, indent_step + indent };
}

// ---------- NodePrinter ------------------
void NodePrinter::operator()(std::nullptr_t) const {
	context.PrintIndent();
	context.out << "null";
}

void NodePrinter::operator()(const Array& array) const {
	context.out << '[' << '\n';
	bool is_first = true;
	for (const Node& node : array) {
		if (is_first) {
			if (node.IsDict() || node.IsArray()) {
				context.Indented().PrintIndent();
				node.PrintValue(context.Indented());
			} else {
				node.PrintValue(context.Indented());
			}
			is_first = false;
		} else {
			context.out << ',' << '\n';
			if (node.IsDict() || node.IsArray()) {
				context.Indented().PrintIndent();
				node.PrintValue(context.Indented());
			} else {
				node.PrintValue(context.Indented());
			}
		}
	}
	context.out << '\n';
	context.PrintIndent();
	context.out << "]";
}

void NodePrinter::operator()(const Dict& dict) const {
	context.out << '{' << '\n';
	bool is_first = true;
	for (const auto& [key, value] : dict) {
		if (is_first) {
			PrintString(key, context.Indented());
			if (value.IsDict() || value.IsArray()) {
				value.PrintValue(context.Indented());
			} else {
				value.PrintValue({ context.out });
			}
			is_first = false;
		} else {
			context.out << ',' << '\n';
			PrintString(key, context.Indented());
			if (value.IsDict() || value.IsArray()) {
				value.PrintValue(context.Indented());
			} else {
				value.PrintValue({ context.out });
			}
		}
	}
	context.out << '\n';
	context.PrintIndent();
	context.out << "}";
}

void NodePrinter::operator()(bool value) const {
	context.PrintIndent();
	context.out << std::boolalpha << value;
}

void NodePrinter::operator()(int value) const {
	context.PrintIndent();
	context.out << value;
}

void NodePrinter::operator()(double value) const {
	context.PrintIndent();
	context.out << value;
}

void NodePrinter::operator()(const std::string& str) const {
	context.PrintIndent();
	context.out << '\"';
	if (str.find_first_of(SPACES) == std::string::npos) {
		context.out << str;
	} else {
		PrintWithEscapeSeq(str, context.out);
	}
	context.out << '\"';
}

void PrintString(const std::string& str, PrintContext context) {
	context.PrintIndent();
	context.out << '\"' << str << "\": "s;
}

void PrintWithEscapeSeq(const std::string& str, std::ostream& out) {
	for (char c : str) {
		if (CHARS_TO_SHIELD.find_first_of(c) != std::string::npos) {
			out << '\\';
			out << parsing::helpers::ShieldedChar(c);
		} else {
			out << c;
		}
	}
}

}// end namespace detail

namespace parsing {

using namespace helpers;
Node LoadNode(std::string_view str) {
	size_t begin = str.find_first_not_of(SPACES);
	if (begin == std::string::npos) {
		throw json::ParsingError("Node is empty");
	}
	size_t end = str.find_last_not_of(SPACES);
	str = str.substr(begin, end - begin + 1);
	if (str.front() == '{' && str.back() == '}') {
		return LoadDict(str);
	}
	if (str.front() == '[' && str.back() == ']') {
		return LoadArray(str);
	}
	if (str.front() == '\"' && str.back() == '\"') {
		return LoadString(str);
	}
	if (str == "null") {
		return Node();
	}
	if (str == "false") {
		return Node(false);
	}
	if (str == "true") {
		return Node(true);
	}
	if (str.find_first_not_of(DIGITS) == std::string::npos) {
		return LoadInt(str);
	}
	return LoadDouble(str);
}

Node LoadDict(std::string_view sv) {
	Dict result;
	if (sv.size() == 2) {
		return Node(std::move(result));
	}
	sv.remove_prefix(1);
	sv.remove_suffix(1);
	size_t key_begin = FindKeyBegin(sv);
	if (key_begin == std::string::npos) {
		return Node(std::move(result));
	}
	size_t key_end = FindkeyEnd(sv, key_begin);
	size_t value_begin = FindValueBegin(sv, key_end + 1);
	size_t value_end = FindValueEnd(sv, value_begin);
	while (key_begin != std::string::npos) {
		std::string key{ sv.substr(key_begin, key_end - key_begin) };
		Node value = LoadNode(sv.substr(value_begin, value_end - value_begin));
		result.insert({ key, value });
		key_begin = FindKeyBegin(sv, value_end);
		key_end = FindkeyEnd(sv, key_begin);
		value_begin = FindValueBegin(sv, key_end + 1);
		value_end = FindValueEnd(sv, value_begin);
	}
	return Node(std::move(result));
}

Node LoadArray(std::string_view sv) {
	Array result;
	if (sv.size() == 2) {
		return Node(std::move(result));
	}
	sv.remove_prefix(1);
	sv.remove_suffix(1);
	size_t begin = FindValueBegin(sv);
	if (begin == std::string::npos) {
		return Node(std::move(result));
	}
	size_t end = FindValueEnd(sv, begin);
	while (begin != std::string::npos) {
		result.push_back(LoadNode(sv.substr(begin, end - begin)));
		begin = FindValueBegin(sv, end);
		end = FindValueEnd(sv, begin);
	}
	return Node(std::move(result));
}

Node LoadString(std::string_view sv) {
	sv.remove_prefix(1);
	sv.remove_suffix(1);
	std::string str;
	size_t i = 0;
	for (; i + 1 < sv.size(); ++i) {
		if (sv[i] == '\\') {
			str += UnshildedChar(sv[i + 1]);
			++i;
			continue;
		} else {
			str += sv[i];
		}
	}
	if (i == sv.size() - 1) {
		str += sv[i];
	}
	return Node(std::move(str));
}

Node LoadInt(std::string_view sv) {
	size_t count = 0;
	int value = 0;
	try {
		value = std::stoi(std::string{ sv }, &count);
	} catch (std::invalid_argument& e) {
		throw json::ParsingError(e.what());
	} catch (std::out_of_range& e) {
		LoadDouble(sv);
	}
	if (count != sv.size()) {
		throw json::ParsingError("Undefined Node type");
	}
	return Node(value);
}

Node LoadDouble(std::string_view sv) {
	size_t count = 0;
	double value = 0;
	try {
		value = std::stod(std::string{ sv }, &count);
	} catch (std::invalid_argument& e) {
		throw json::ParsingError(e.what());
	} catch (std::out_of_range& e) {
		throw json::ParsingError(e.what());
	}
	if (count != sv.size()) {
		throw json::ParsingError("Undefined Node type");
	}
	return Node(value);
}

namespace helpers {

char UnshildedChar(char c) {
	switch (c) {
	case 'r':
		return '\r';
	case 't':
		return '\t';
	case 'n':
		return '\n';
	case '\\':
		return '\\';
	case '\"':
		return '\"';
	default:
		return c;
		break;
	}
}

char ShieldedChar(char c) {
	switch (c) {
	case '\r':
		return 'r';
	case '\n':
		return 'n';
	case '\\':
		return '\\';
	case '\"':
		return '\"';
	default:
		return c;
		break;
	}
}

size_t FindNextUnshildedQuote(std::string_view sv, size_t from) {
	size_t end = from + 1;
	while (end != sv.size() && !(sv[end] == '\"' && sv[end - 1] != '\\')) {
		++end;
	}
	return end == sv.size() ? std::string::npos : end;
}

size_t FindValueBegin(std::string_view sv, size_t from) {
	if (from >= sv.size()) {
		return std::string::npos;
	}
	size_t begin = sv.find_first_not_of(SPACES, from);
	if (begin == std::string::npos) {
		return std::string::npos;
	}
	if (sv[begin] == ':' || sv[begin] == ',') {
		return sv.find_first_not_of(SPACES, begin + 1);
	}
	return begin;
}

size_t FindValueEnd(std::string_view sv, size_t from) {
	if (from >= sv.size()) {
		return std::string::npos;
	}
	const char start_char = sv[from];
	size_t end = from;
	if (start_char == '{' || start_char == '[') {
		const char end_char = start_char == '{' ? '}' : ']';
		const std::string_view braces = (start_char == '{') ? "{}\""sv : "[]\""sv;
		size_t open_brace_count = 1, close_brace_count = 0;
		bool is_quote_open = false;
		while (open_brace_count != close_brace_count) {
			end = sv.find_first_of(braces, end + 1);
			if (sv[end] == start_char && !is_quote_open) {
				++open_brace_count;
			} else if (sv[end] == end_char && !is_quote_open) {
				++close_brace_count;
			} else if (sv[end] == '\"' && sv[end - 1] != '\\') {
				is_quote_open = !is_quote_open;
			}
			if (end == std::string::npos) {
				throw json::ParsingError("Incorrect Array/Dictionary");
			}
		}
		return end + 1;
	}
	if (start_char == '\"') {
		end = FindNextUnshildedQuote(sv, from);
	}
	if (end == std::string::npos) {
		throw json::ParsingError("Incorrect Array");
	}
	end = sv.find_first_of(',', end);
	return end == std::string::npos ? sv.size() : end;
}

size_t FindKeyBegin(std::string_view sv, size_t from) {
	if (from >= sv.size()) {
		return std::string::npos;
	}
	if (sv[from] == ',') {
		++from;
	}
	size_t key_begin = sv.find_first_not_of(SPACES, from);
	if (key_begin == std::string::npos) {
		return std::string::npos;
	}
	if (sv[key_begin] != '\"') {
		throw json::ParsingError("Incorrect Dictionary");
	}
	return key_begin + 1;
}

size_t FindkeyEnd(std::string_view sv, size_t key_begin) {
	if (key_begin == std::string::npos) {
		return  key_begin;
	}
	size_t key_end = FindNextUnshildedQuote(sv, key_begin);
	if (key_end == std::string::npos) {
		throw json::ParsingError("Incorrect Dictionary");
	}
	return key_end;
}

}//end namespace helpers

}//end namespace parsing

}  // namespace json