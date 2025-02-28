#ifndef GDEXAMPLE_H
#define GDEXAMPLE_H

#include "md4c.h"

#include <godot_cpp/classes/rich_text_label.hpp>

namespace godot {

enum MD2BBError {
	MD_OK=0, BAD_BLOCK=1, BAD_SPAN=2, BAD_HEADER_SIZE=3
};

class MD2BBHeaderFormat : public Resource {
	GDCLASS(MD2BBHeaderFormat, Resource);

public:
	float font_size;
	bool bold;
	bool italic;
	bool underlined;
	bool has_color;
	Color font_color;

	float get_font_size() const { return font_size; }
	void set_font_size(float value) { font_size = value; }
	bool get_bold() const { return bold; }
	void set_bold(bool value) { bold = value; }
	bool get_italic() const { return italic; }
	void set_italic(bool value) { italic = value; }
	bool get_underlined() const { return underlined; }
	void set_underlined(bool value) { underlined = value; }
	bool get_has_color() const { return has_color; }
	void set_has_color(bool value) { has_color = value; }
	Color get_font_color() const { return font_color; }
	void set_font_color(Color value) { font_color = value; }

protected:
	static void _bind_methods();
};

class MD2BBCellFormat : public Resource {
	GDCLASS(MD2BBCellFormat, Resource);

public:
	Color border_color;
	Rect2 padding;
	Color row_bg_odd;
	Color row_bg_even;
	bool size_override;
	Vector2 min_size_override;
	Vector2 max_size_override;

protected:
	static void _bind_methods() {}
};

/**
 * Options for how specific markdown formats are represented in bbcode
 */
class MD2BBFormat : public Resource {
	GDCLASS(MD2BBFormat, Resource);

public:
	Ref<MD2BBHeaderFormat> h1_format;
	Ref<MD2BBHeaderFormat> h2_format;
	Ref<MD2BBHeaderFormat> h3_format;
	Ref<MD2BBHeaderFormat> h4_format;
	Ref<MD2BBHeaderFormat> h5_format;
	Ref<MD2BBHeaderFormat> h6_format;

	Ref<MD2BBCellFormat> table_head_format;
	Ref<MD2BBCellFormat> table_body_format;

	Ref<MD2BBHeaderFormat> get_h1_format () const { return h1_format; }
	void set_h1_format (Ref<MD2BBHeaderFormat> value) { h1_format = value; }
	Ref<MD2BBHeaderFormat> get_h2_format () const { return h2_format; }
	void set_h2_format (Ref<MD2BBHeaderFormat> value) { h2_format = value; }
	Ref<MD2BBHeaderFormat> get_h3_format () const { return h3_format; }
	void set_h3_format (Ref<MD2BBHeaderFormat> value) { h3_format = value; }
	Ref<MD2BBHeaderFormat> get_h4_format () const { return h4_format; }
	void set_h4_format (Ref<MD2BBHeaderFormat> value) { h4_format = value; }
	Ref<MD2BBHeaderFormat> get_h5_format () const { return h5_format; }
	void set_h5_format (Ref<MD2BBHeaderFormat> value) { h5_format = value; }
	Ref<MD2BBHeaderFormat> get_h6_format () const { return h6_format; }
	void set_h6_format (Ref<MD2BBHeaderFormat> value) { h6_format = value; }

	Ref<MD2BBCellFormat> get_table_head_format () const { return table_head_format; }
	void set_table_head_format (Ref<MD2BBCellFormat> value) { table_head_format = value; }
	Ref<MD2BBCellFormat> get_table_body_format () const { return table_body_format; }
	void set_table_body_format (Ref<MD2BBCellFormat> value) { table_body_format = value; }

protected:
	static void _bind_methods();

};

/**
 * RichTextLabel but with Markdown support, internally converts Markdown text into a bbcode tag stack
 */
class MDTextLabel : public RichTextLabel {
	GDCLASS(MDTextLabel, RichTextLabel)

public:
	String markdown;
	Ref<MD2BBFormat> format;

private:
	MD_PARSER* _md_parser;
    
    // Callbacks for md_parse()
    // Static so callback closures can access
    int _handle_md_block(MD_BLOCKTYPE block_type, void* detail, MD2BBFormat* md_data, bool exiting);
    int _handle_md_span(MD_SPANTYPE span_type, void* detail, MD2BBFormat* md_data, bool exiting);
    int _handle_md_text(MD_TEXTTYPE text_type, const MD_CHAR* text, MD_SIZE size, MD2BBFormat* md_data);

    // Tags with more complex behaviour than just tag replacement
    int _handle_md_header(void* detail, MD2BBFormat* md_data, bool exiting);
    int _handle_md_link(void* detail, MD2BBFormat* md_data, bool exiting);
    int _handle_md_wikilink(void* detail, MD2BBFormat* md_data, bool exiting);

	// BBCode format state helper functions
	void _set_md_cell_format(Ref<MD2BBCellFormat> format);

    // Utility functions
    static String _mdchar_to_string (const MD_CHAR* text, MD_SIZE size);
    static String _mdattr_to_string (MD_ATTRIBUTE attr);

protected:
	static void _bind_methods();
	int _parse_markdown(String md_text);
	void _validate_property(PropertyInfo& property);

public:
	void set_markdown(String p_text);
	void append_markdown(String p_text);
	String get_markdown() const;

	void set_format(const Ref<MD2BBFormat> format);
	Ref<MD2BBFormat> get_format() const;

	MDTextLabel();
	~MDTextLabel();
};

}

#endif
