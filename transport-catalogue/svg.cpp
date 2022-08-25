#include "svg.h"

namespace svg {

using namespace std::literals;

void Object::Render(const RenderContext& context) const {
	context.RenderIndent();

	// Делегируем вывод тега своим подклассам
	RenderObject(context);

	context.out << std::endl;
}

// ---------- Circle ------------------

Circle& Circle::SetCenter(Point center) {
	center_ = center;
	return *this;
}

Circle& Circle::SetRadius(double radius) {
	radius_ = radius;
	return *this;
}

void Circle::RenderObject(const RenderContext& context) const {
	auto& out = context.out;
	out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
	out << "r=\""sv << radius_ << "\""sv;
	// Выводим атрибуты, унаследованные от PathProps
	RenderAttrs(context.out);
	out << "/>"sv;
}

// ---------- Polyline ------------------
Polyline& Polyline::AddPoint(Point point) {
	points_.push_back(point);
	return *this;
}

void Polyline::RenderObject(const RenderContext& context) const {
	auto& out = context.out;
	out << "<polyline points=\""sv;
	bool is_first = true;
	for (auto& point : points_){
		if (is_first) {
			out << point.x << ',' << point.y;
			is_first = false;
		} else {
			out << ' ' << point.x << ',' << point.y;
		}
	}
	out << "\""sv;
	// Выводим атрибуты, унаследованные от PathProps
	RenderAttrs(context.out);
	out << "/>"sv;
}

// ---------- Text ------------------
Text& Text::SetPosition(Point pos) {
	pos_ = pos;
	return *this;
}

Text& Text::SetOffset(Point offset) {
	offset_ = offset;
	return *this;
}

Text& Text::SetFontSize(uint32_t size) {
	font_size_ = size;
	return *this;
}

Text& Text::SetFontFamily(std::string font_family) {
	font_family_ = std::move(font_family);
	return *this;
}

Text& Text::SetFontWeight(std::string font_weight) {
	font_weight_ = std::move(font_weight);
	return *this;
}

Text& Text::SetData(std::string data) {
	data_ = std::move(data);
	return *this;
}

void Text::RenderObject(const RenderContext& context) const {
	auto& out = context.out;
	out << "<text";
	// Выводим атрибуты, унаследованные от PathProps
	RenderAttrs(context.out);
	out << " x=\""sv << pos_.x << "\" y=\""sv << pos_.y << "\" "sv;
	out << "dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y << "\" "sv;
	out << "font-size=\""sv << font_size_ << "\""sv;
	if (!font_family_.empty()) {
		out << " font-family=\""sv << font_family_ << "\""sv;
	}
	if (!font_weight_.empty()) {
		out << " font-weight=\""sv << font_weight_ << "\""sv;
	}
	out << '>';
	detail::PrintShieldedString(data_, out);
	out << "</text>"sv;
}

// ---------- Document ------------------ 

void Document::AddPtr(std::unique_ptr<Object>&& obj) {
	objects_.push_back(std::move(obj));
}

	// Выводит в ostream svg-представление документа
void Document::Render(std::ostream& out) const {
	out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
	out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;
	RenderContext ctx(out, 2, 2);
	for (auto& obj : objects_) {
		obj->Render(ctx);
	}
	out << "</svg>"sv;
}

std::ostream& operator<<(std::ostream& out, StrokeLineCap stroke_line_cap) {
	switch (stroke_line_cap) {
	case svg::StrokeLineCap::BUTT:
		return out << "butt";
	case svg::StrokeLineCap::ROUND:
		return out << "round";
	case svg::StrokeLineCap::SQUARE:
		return out << "square";
	default:
		assert(false);
		return out;
	}

}

std::ostream& operator<<(std::ostream& out, StrokeLineJoin stroke_line_join) {
	switch (stroke_line_join) {
	case svg::StrokeLineJoin::ARCS:
		return out << "arcs";
	case svg::StrokeLineJoin::BEVEL:
		return out << "bevel";
	case svg::StrokeLineJoin::MITER:
		return out << "miter";
	case svg::StrokeLineJoin::MITER_CLIP:
		return out << "miter-clip";
	case svg::StrokeLineJoin::ROUND:
		return out << "round";
	default:
		assert(false);
		return out;
	}

}

// ---------- ColorPrinter ------------------ 
void ColorPrinter::operator()(std::monostate) const {
	out << "none";
}
void ColorPrinter::operator()(std::string color) const {
	out << color;
}
void ColorPrinter::operator()(svg::Rgb color) const {
	out << "rgb(" << color.red << ',' << color.green << ',' << color.blue << ')';
}
void ColorPrinter::operator()(svg::Rgba color) const {
	out << "rgba(" << color.red << ','
		<< color.green << ','
		<< color.blue << ','
		<< color.opacity << ')';
}

std::ostream& operator<<(std::ostream& out, Color color) {
	std::visit(ColorPrinter{ out }, color);
	return out;
}

namespace detail {

static const std::string_view SPECIAL_SYMBOLS = "\"<>'&"sv;
static const std::string_view QUOT = "&quot;"sv;
static const std::string_view APOS = "&apos;"sv;
static const std::string_view LT = "&lt;"sv;
static const std::string_view GT = "&gt;"sv;
static const std::string_view AMP = "&amp;"sv;

std::string_view ConvertSpecialSimbol(char simbol) {
	switch (simbol) {
	case '"':
		return QUOT;
	case '\'':
		return APOS;
	case '<':
		return LT;
	case '>':
		return GT;
	case '&':
		return AMP;
	default:
		assert(false);
		return "";
	}
}

void PrintShieldedString(std::string_view str, std::ostream& out) {
	size_t special_simbol_pos = str.find_first_of(SPECIAL_SYMBOLS);
	if (special_simbol_pos == std::string::npos) {
		out << str;
		return;
	}
	size_t begin = 0;
	while (special_simbol_pos != std::string::npos) {
		out << str.substr(begin, special_simbol_pos - begin);
		out << ConvertSpecialSimbol(str[special_simbol_pos]);
		begin = special_simbol_pos + 1;
		special_simbol_pos = str.find_first_of(SPECIAL_SYMBOLS, begin);
	}
	out << str.substr(begin, str.size() - begin);
}

} // namespace detail

}  // namespace svg