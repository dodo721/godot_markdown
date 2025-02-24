#include "md_text_label.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void MDTextLabel::_bind_methods() {
	ClassDB::bind_static_method("MDTextLabel", D_METHOD("md2bb", "text"), &MDTextLabel::md2bb);
	ClassDB::bind_method(D_METHOD("set_markdown", "p_markdown"), &MDTextLabel::set_markdown);
	ClassDB::bind_method(D_METHOD("append_markdown", "p_markdown"), &MDTextLabel::append_markdown);
	ClassDB::bind_method(D_METHOD("get_markdown"), &MDTextLabel::get_markdown);
	ClassDB::bind_method(D_METHOD("get_format"), &MDTextLabel::get_format);
	ClassDB::bind_method(D_METHOD("set_format", "p_format"), &MDTextLabel::set_format);
	
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "markdown", PROPERTY_HINT_MULTILINE_TEXT), "set_markdown", "get_markdown");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "format", PROPERTY_HINT_RESOURCE_TYPE, "MD2BBFormat"), "set_format", "get_format");
}

MDTextLabel::MDTextLabel() {
}

MDTextLabel::~MDTextLabel() {
}

void MDTextLabel::set_markdown(const String p_markdown) {
	UtilityFunctions::print("Converting markdown...");
	if (!is_using_bbcode())
		set_use_bbcode(true);
	markdown = p_markdown;
	//set_text(markdown);
	String bbcode = md2bb(p_markdown, format);
	set_text(bbcode);
}

void MDTextLabel::append_markdown(const String p_markdown) {
	if (!is_using_bbcode())
		set_use_bbcode(true);
	markdown += p_markdown;
	String bbcode = md2bb(p_markdown, format);
	append_text(bbcode);
}

String MDTextLabel::get_markdown() const {
	return markdown;
}

void MDTextLabel::set_format(const Ref<MD2BBFormat> format) {
	this->format = format;
}

Ref<MD2BBFormat> MDTextLabel::get_format() const {
	return format;
}

String MDTextLabel::md2bb(String p_text, Ref<MD2BBFormat> format) {
	Ref<MD2BBConverter> converter;
	converter.instantiate();
	int error;
	String bbcode = converter->convert(p_text, error);
	if (error) {
		UtilityFunctions::printerr("Error converting markdown text to bbcode: ", error);
	}
	return bbcode;
}
