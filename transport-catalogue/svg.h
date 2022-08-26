#pragma once

#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>
#include <utility>

namespace svg {

struct Rgb {
	Rgb() = default;
	Rgb(int red, int green, int blue)
		:red(red), green(green), blue(blue) {}
	uint16_t red = 0;
	uint16_t green = 0;
	uint16_t blue = 0;
};

struct Rgba : public Rgb {
	Rgba() = default;
	Rgba(int red, int green, int blue, double opacity)
		:Rgb(red, green, blue), opacity(opacity) {}
	double opacity = 1.0;
};

using Color = std::variant<std::monostate, std::string, svg::Rgb, svg::Rgba>;

inline const Color NoneColor{ std::monostate{} };

std::ostream& operator<<(std::ostream& out, Color color);

struct ColorPrinter {
	std::ostream& out;
	void operator()(std::monostate) const;
	void operator()(std::string color) const;
	void operator()(svg::Rgb color) const;
	void operator()(svg::Rgba color) const;
};

struct Point {
	Point() = default;
	Point(double x, double y)
		: x(x)
		, y(y) {}
	double x = 0;
	double y = 0;
	Point& operator+=(const Point& other) {
		this->x += other.x;
		this->y += other.y;
		return *this;
	}
};

enum class StrokeLineCap {
	BUTT,
	ROUND,
	SQUARE,
};

enum class StrokeLineJoin {
	ARCS,
	BEVEL,
	MITER,
	MITER_CLIP,
	ROUND,
};

std::ostream& operator<<(std::ostream& out, StrokeLineCap stroke_line_cap);

std::ostream& operator<<(std::ostream& out, StrokeLineJoin stroke_line_join);

// Вспомогательная структура, хранящая контекст для вывода SVG-документа с отступами.
// Хранит ссылку на поток вывода, текущее значение и шаг отступа при выводе элемента
struct RenderContext {
	RenderContext(std::ostream& out)
		: out(out) {}

	RenderContext(std::ostream& out, int indent_step, int indent = 0)
		: out(out)
		, indent_step(indent_step)
		, indent(indent) {}

	RenderContext Indented() const {
		return { out, indent_step, indent + indent_step };
	}

	void RenderIndent() const {
		for (int i = 0; i < indent; ++i) {
			out.put(' ');
		}
	}

	std::ostream& out;
	int indent_step = 0;
	int indent = 0;
};

template <typename Owner>
class PathProps {
public:
	Owner& SetFillColor(Color color) {
		fill_color_ = std::move(color);
		return AsOwner();
	}
	Owner& SetStrokeColor(Color color) {
		stroke_color_ = std::move(color);
		return AsOwner();
	}
	Owner& SetStrokeWidth(double width) {
		stroke_width_ = width;
		return AsOwner();
	}
	Owner& SetStrokeLineCap(StrokeLineCap line_cap) {
		stroke_line_cap_ = line_cap;
		return AsOwner();
	}
	Owner& SetStrokeLineJoin(StrokeLineJoin line_join) {
		stroke_line_join_ = line_join;
		return AsOwner();
	}

protected:
	~PathProps() = default;

	void RenderAttrs(std::ostream& out) const {
		using namespace std::literals;

		if (fill_color_) {
			out << " fill=\""sv << *fill_color_ << "\""sv;
		}
		if (stroke_color_) {
			out << " stroke=\""sv << *stroke_color_ << "\""sv;
		}
		if (stroke_width_) {
			out << " stroke-width=\""sv << *stroke_width_ << "\""sv;
		}
		if (stroke_line_cap_) {
			out << " stroke-linecap=\""sv << *stroke_line_cap_ << "\""sv;
		}
		if (stroke_line_join_) {
			out << " stroke-linejoin=\""sv << *stroke_line_join_ << "\""sv;
		}
	}

private:
	Owner& AsOwner() {
		return static_cast<Owner&>(*this);
	}

	std::optional<Color> fill_color_;
	std::optional<Color> stroke_color_;
	std::optional<double> stroke_width_;
	std::optional<StrokeLineCap> stroke_line_cap_;
	std::optional<StrokeLineJoin> stroke_line_join_;
};

//Абстрактный базовый класс Object служит для унифицированного хранения
//конкретных тегов SVG-документа
//Реализует паттерн "Шаблонный метод" для вывода содержимого тега
class Object {
public:
	void Render(const RenderContext& context) const;
	virtual ~Object() = default;

private:
	virtual void RenderObject(const RenderContext& context) const = 0;
};


//Класс Circle моделирует элемент <circle> для отображения круга
class Circle final : public Object, public PathProps<Circle> {
public:
	Circle& SetCenter(Point center);
	Circle& SetRadius(double radius);

private:
	void RenderObject(const RenderContext& context) const override;

	Point center_;
	double radius_ = 1.0;
};

//Класс Polyline моделирует элемент <polyline> для отображения ломаных линий
class Polyline final : public Object, public PathProps<Polyline> {
public:
	// Добавляет очередную вершину к ломаной линии
	Polyline& AddPoint(Point point);

private:
	void RenderObject(const RenderContext& context) const override;

	std::vector<Point> points_;
};

//Класс Text моделирует элемент <text> для отображения текста
class Text final : public Object, public PathProps<Text> {
public:
	// Задаёт координаты опорной точки (атрибуты x и y)
	Text& SetPosition(Point pos);

	// Задаёт смещение относительно опорной точки (атрибуты dx, dy)
	Text& SetOffset(Point offset);

	// Задаёт размеры шрифта (атрибут font-size)
	Text& SetFontSize(uint32_t size);

	// Задаёт название шрифта (атрибут font-family)
	Text& SetFontFamily(std::string font_family);

	// Задаёт толщину шрифта (атрибут font-weight)
	Text& SetFontWeight(std::string font_weight);

	// Задаёт текстовое содержимое объекта (отображается внутри тега text)
	Text& SetData(std::string data);
private:
	void RenderObject(const RenderContext& context) const override;

	Point pos_;
	Point offset_;
	uint32_t font_size_ = 1;
	std::string font_family_;
	std::string font_weight_;
	std::string data_;
};

//<<Interface>>
class ObjectContainer {
public:
	//Метод Add добавляет в контейнер любой объект-наследник svg::Object.
	template <typename Obj>
	void Add(Obj obj);
	virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;
protected:
	~ObjectContainer() = default;
	std::vector<std::unique_ptr<Object>> objects_;
};

class Document : public ObjectContainer {
public:
	Document() = default;
	Document(Document&& other) = default;
	// Добавляет в svg-документ объект-наследник svg::Object
	void AddPtr(std::unique_ptr<Object>&& obj);

	// Выводит в ostream svg-представление документа
	void Render(std::ostream& out) const;

};

template <typename Obj>
void ObjectContainer::Add(Obj obj) {
	objects_.emplace_back(std::make_unique<Obj>(std::move(obj)));
}

//<<interface>>
class Drawable {
public:
	virtual void Draw(ObjectContainer& container) const = 0;
	virtual ~Drawable() = default;
};

namespace detail {

void PrintShieldedString(std::string_view str, std::ostream& out);

std::string_view ConvertSpecialSimbol(char simbol);

} // namespace detail

}  // namespace svg