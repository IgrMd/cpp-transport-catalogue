#include "json_builder.h"

namespace json {

// ---------- Builder ------------------
Builder::~Builder() {
	for (Node* node : nodes_stack_) {
		if (node) { delete node; }
	}
}

Builder::KeyContext Builder::Key(std::string key) {
	CheckCallKey();
	key_stack_.push_back(std::move(key));
	return KeyContext(*this);
}

Builder& Builder::AddValueInKeyContext(Node::Value value) {
	GetDictRefFromStack().insert(
		std::pair{ std::move(key_stack_.back()), std::move(value) }
	);
	key_stack_.pop_back();
	return *this;
}

Builder& Builder::AddValueInArrayContext(Node::Value value) {
	GetArrayRefFromStack().emplace_back(value);
	return *this;
}

Builder& Builder::Value(Node::Value value) {
	CheckCallValue();
	if (nodes_stack_.empty()) {
		root_.GetValue() = std::move(value);
		status_ = ObjStatus::COPMLETED;
		return *this;
	}
	if (nodes_stack_.back()->IsArray()) {
		return AddValueInArrayContext(std::move(value));
	}
	if (nodes_stack_.back()->IsMap()) {
		return AddValueInKeyContext(std::move(value));
	}
	return *this;
}

Builder::ValueInKeyContext Builder::ValueAfterKey(Node::Value value) {
	CheckCallValue();
	return ValueInKeyContext(AddValueInKeyContext(std::move(value)));
}

Builder::ValueInArrayContext Builder::ValueAfterStartArray(Node::Value value) {
	CheckCallValue();
	return ValueInArrayContext(AddValueInArrayContext(std::move(value)));
}

Builder::DictItemContext Builder::StartDict() {
	CheckCallStartDict();
	nodes_stack_.push_back(new Node(Dict{}));
	return DictItemContext(*this);
}

Builder& Builder::EndDict() {
	CheckCallEndDict();
	Node* node = nodes_stack_.back();
	nodes_stack_.pop_back();
	if (!nodes_stack_.empty()) {
		if (nodes_stack_.back()->IsArray()) {
			GetArrayRefFromStack().push_back(std::move(*node));
			return *this;
		}
		if (nodes_stack_.back()->IsMap()) {
			GetDictRefFromStack().insert(
				std::pair{ std::move(key_stack_.back()), std::move(*node) }
			);
			key_stack_.pop_back();
			return *this;
		}
	}
	root_ = std::move(*node);
	status_ = ObjStatus::COPMLETED;
	return *this;
}

Builder::ArrayItemContext Builder::StartArray() {
	CheckCallStartArray();
	nodes_stack_.push_back(new Node(Array{}));
	return ArrayItemContext(*this);
}

Builder& Builder::EndArray() {
	CheckCallEndArray();
	Node* node = nodes_stack_.back();
	nodes_stack_.pop_back();
	if (!nodes_stack_.empty()) {
		if (nodes_stack_.back()->IsArray()) {
			GetArrayRefFromStack().push_back(std::move(*node));
			return *this;
		}
		if (nodes_stack_.back()->IsMap()) {
			GetDictRefFromStack().insert(
				std::pair{ std::move(key_stack_.back()), std::move(*node) }
			);
			key_stack_.pop_back();
			return *this;
		}
	}
	root_ = std::move(*node);
	status_ = ObjStatus::COPMLETED;
	return *this;
}

Node Builder::Build() {
	CheckCallBuild();
	return root_;
}

Array& Builder::GetArrayRefFromStack() {
	return std::get<Array>(nodes_stack_.back()->GetValue());
}

Dict& Builder::GetDictRefFromStack() {
	return std::get<Dict>(nodes_stack_.back()->GetValue());
}

void Builder::CheckCallKey() {
	if (status_ == ObjStatus::COPMLETED) {
		throw std::logic_error(".Key() method called for completed Object");
	}
	if (!nodes_stack_.back()->IsMap()) {
		throw std::logic_error("Dict not started");
	}
	if (last_method_ == LastMethodCalled::KEY) {
		throw std::logic_error("Double .Key() method called");
	}
	last_method_ = LastMethodCalled::KEY;
}

void Builder::CheckCallValue() {
	CheckValueMethodCalled(".Value()");
	last_method_ = LastMethodCalled::VALUE;
}

void Builder::CheckCallStartDict() {
	CheckValueMethodCalled(".StartDict()");
	last_method_ = LastMethodCalled::START_DICT;
}

void Builder::CheckCallEndDict() {
	if (status_ == ObjStatus::COPMLETED) {
		throw std::logic_error(".EndDict() method called for completed Object");
	}
	if (!nodes_stack_.back()->IsMap()) {
		throw std::logic_error("Can't end non-Dict Object");
	}
	last_method_ = LastMethodCalled::END_DICT;
}

void Builder::CheckCallStartArray() {
	CheckValueMethodCalled(".StartArray()");
	last_method_ = LastMethodCalled::START_ARRAY;
}

void Builder::CheckCallEndArray() {
	if (status_ == ObjStatus::COPMLETED) {
		throw std::logic_error(".EndArray() method called for completed Object");
	}
	if (!nodes_stack_.back()->IsArray()) {
		throw std::logic_error("Can't end non-array Object");
	}
	last_method_ = LastMethodCalled::END_ARRAY;
}

void Builder::CheckCallBuild() {
	if (status_ != ObjStatus::COPMLETED) {
		throw std::logic_error("Object construction hasn't been completed");
	}
}

void Builder::CheckValueMethodCalled(const std::string_view name) {
	if (status_ == ObjStatus::COPMLETED) {
		throw std::logic_error(std::string{ name } + " method called for completed Object");
	}
	if (!(last_method_ == LastMethodCalled::UNKNOWN ||
		last_method_ == LastMethodCalled::KEY ||
		nodes_stack_.back()->IsArray())) {
		throw std::logic_error(std::string{ name } + " method called in incorrect way");
	}
	status_ = ObjStatus::IN_PROGRESS;
}

// ---------- Context ------------------
Builder::Context::Context(Builder& builder)
	:builder_(builder) {}

Builder::KeyContext Builder::Context::Key(std::string key) {
	return builder_.Key(std::move(key));
}
Builder::ArrayItemContext Builder::Context::StartArray() {
	return builder_.StartArray();
}
Builder::DictItemContext Builder::Context::StartDict() {
	return builder_.StartDict();
}
Builder& Builder::Context::EndArray() {
	return builder_.EndArray();
}
Builder& Builder::Context::EndDict() {
	return builder_.EndDict();
}

// ---------- KeyContext ------------------
Builder::ValueInKeyContext Builder::KeyContext::Value(Node::Value value) {
	return builder_.ValueAfterKey(std::move(value));
}

// ---------- ArrayItemContext ------------------
Builder::ValueInArrayContext Builder::ArrayItemContext::Value(Node::Value value) {
	return builder_.ValueAfterStartArray(std::move(value));
}

// ---------- ValueInArrayContext ------------------
Builder::ValueInArrayContext Builder::ValueInArrayContext::Value(Node::Value value) {
	return builder_.ValueAfterStartArray(std::move(value));
}

} // end namespace json