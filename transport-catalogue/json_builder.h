#pragma once

#include <cassert>
#include "json.h"

namespace json {

class Builder;

class Builder {
	class Context;
	class KeyContext;
	class DictItemContext;
	class ArrayItemContext;
	class ValueInKeyContext;
	class ValueInArrayContext;

	enum class LastMethodCalled {
		UNKNOWN,
		KEY,
		VALUE,
		START_DICT,
		END_DICT,
		START_ARRAY,
		END_ARRAY,
		BUILD
	};

	enum class ObjStatus {
		NEW,
		IN_PROGRESS,
		COPMLETED
	};

public:
	explicit Builder() = default;
	KeyContext Key(std::string key);
	Builder& Value(Node::Value value);
	ValueInKeyContext ValueAfterKey(Node::Value value);
	ValueInArrayContext ValueAfterStartArray(Node::Value value);
	DictItemContext StartDict();
	ArrayItemContext StartArray();
	Builder& EndDict();
	Builder& EndArray();
	Node Build();
	~Builder();

private:
	Node root_;
	std::vector<Node*> nodes_stack_;
	std::vector<std::string> key_stack_;
	LastMethodCalled last_method_ = LastMethodCalled::UNKNOWN;
	ObjStatus status_ = ObjStatus::NEW;

	Array& GetArrayRefFromStack();
	Dict& GetDictRefFromStack();

	void CheckCallKey();
	void CheckCallValue();
	void CheckCallStartDict();
	void CheckCallEndDict();
	void CheckCallStartArray();
	void CheckCallEndArray();
	void CheckCallBuild();
	void CheckValueMethodCalled(const std::string_view name);

	Builder& AddValueInKeyContext(Node::Value value);
	Builder& AddValueInArrayContext(Node::Value value);

	//Контексты вызова методов билдера
	class Context {
	public:
		Context(Builder& builder);
		KeyContext Key(std::string key);
		ArrayItemContext StartArray();
		DictItemContext StartDict();
		Builder& EndArray();
		Builder& EndDict();

	protected:
		~Context() = default;
		Builder& builder_;
	};

	class KeyContext : public Context {
	public:
		KeyContext(Builder& builder)
			: Context(builder) {}
		ValueInKeyContext Value(Node::Value value);
		KeyContext Key() = delete;
		Builder& EndArray() = delete;
		Builder& EndDict() = delete;
	};

	class DictItemContext : public Context {
	public:
		DictItemContext(Builder& builder)
			: Context(builder) {}
		ArrayItemContext StartArray() = delete;
		DictItemContext StartDict() = delete;
		Builder& EndArray() = delete;
	};

	class ArrayItemContext : public Context {
	public:
		ArrayItemContext(Builder& builder)
			: Context(builder) {}
		ValueInArrayContext Value(Node::Value value);
		KeyContext Key() = delete;
		Builder& EndDict() = delete;
	};

	class ValueInKeyContext : public Context {
	public:
		ValueInKeyContext(Builder& builder)
			: Context(builder) {}
		ArrayItemContext StartArray() = delete;
		DictItemContext StartDict() = delete;
		Builder& EndArray() = delete;
	};

	class ValueInArrayContext : public Context {
	public:
		ValueInArrayContext(Builder& builder)
			: Context(builder) {}
		ValueInArrayContext Value(Node::Value value);
		KeyContext Key() = delete;
		Builder& EndDict() = delete;
	};
};

} // end namespace json