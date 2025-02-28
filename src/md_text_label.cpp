#include "md_text_label.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#define __POP_IF_EXIT if (exiting) { pop(); return MD_OK; }

using namespace godot;

const PackedStringArray HIDDEN_PROPERTIES = {"bbcode_enabled", "text"};

void MDTextLabel::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_markdown", "p_markdown"), &MDTextLabel::set_markdown);
	ClassDB::bind_method(D_METHOD("append_markdown", "p_markdown"), &MDTextLabel::append_markdown);
	ClassDB::bind_method(D_METHOD("get_markdown"), &MDTextLabel::get_markdown);
	ClassDB::bind_method(D_METHOD("get_format"), &MDTextLabel::get_format);
	ClassDB::bind_method(D_METHOD("set_format", "p_format"), &MDTextLabel::set_format);
	
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "markdown", PROPERTY_HINT_MULTILINE_TEXT), "set_markdown", "get_markdown");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "format", PROPERTY_HINT_RESOURCE_TYPE, "MD2BBFormat"), "set_format", "get_format");
}

MDTextLabel::MDTextLabel() {

	_md_parser = new MD_PARSER();
    // Need to set to 0
    // Not sure why docs for md4c just say so ¯\_(ツ)_/¯
    _md_parser->abi_version = 0;
    // See md4c.h:306
    _md_parser->flags = MD_FLAG_TABLES | MD_FLAG_STRIKETHROUGH | MD_FLAG_WIKILINKS | MD_FLAG_UNDERLINE | MD_FLAG_NOHTMLBLOCKS | MD_FLAG_NOHTMLSPANS;

    // Block callbacks
    _md_parser->enter_block = [](MD_BLOCKTYPE block_type, void* detail, void* user_data) {
		MDTextLabel* md_label = (MDTextLabel*)user_data;
        return md_label->_handle_md_block(block_type, detail, md_label->format.ptr(), false);
    };
    _md_parser->leave_block = [](MD_BLOCKTYPE block_type, void* detail, void* user_data) {
		MDTextLabel* md_label = (MDTextLabel*)user_data;
        return md_label->_handle_md_block(block_type, detail, md_label->format.ptr(), true);
    };
    // Span callbacks
    _md_parser->enter_span = [](MD_SPANTYPE span_type, void* detail, void* user_data) {
		MDTextLabel* md_label = (MDTextLabel*)user_data;
        return md_label->_handle_md_span(span_type, detail, md_label->format.ptr(), false);
    };
    _md_parser->leave_span = [](MD_SPANTYPE span_type, void* detail, void* user_data) {
		MDTextLabel* md_label = (MDTextLabel*)user_data;
        return md_label->_handle_md_span(span_type, detail, md_label->format.ptr(), true);
    };
    // Text callback
    _md_parser->text = [](MD_TEXTTYPE text_type, const MD_CHAR* text, MD_SIZE size, void* user_data) {
		MDTextLabel* md_label = (MDTextLabel*)user_data;
        return md_label->_handle_md_text(text_type, text, size, md_label->format.ptr());
    };
}

MDTextLabel::~MDTextLabel() {
	delete _md_parser;
}

void MDTextLabel::_validate_property(PropertyInfo& property) {
    // Hide properties that this class modifies
	if (HIDDEN_PROPERTIES.has(property.name)) {
        property.usage = PROPERTY_USAGE_NO_EDITOR;
    }
}

void MDTextLabel::set_markdown(const String p_markdown) {
	ERR_FAIL_COND(format.is_null());
	if (!is_using_bbcode())
		set_use_bbcode(true);
	markdown = p_markdown;
    int err = _parse_markdown(markdown);
	ERR_FAIL_COND_MSG(err != MD_OK, "Failed to parse markdown, error code " + err);
}

void MDTextLabel::append_markdown(const String p_markdown) {
	ERR_FAIL_COND(format.is_null());
	if (!is_using_bbcode())
		set_use_bbcode(true);
	markdown += p_markdown;
    int err = _parse_markdown(markdown);
	ERR_FAIL_COND_MSG(err != MD_OK, "Failed to parse markdown, error code " + err);
}


int MDTextLabel::_parse_markdown(String md_text) {
    set_text("");
    return md_parse(md_text.utf8().get_data(), md_text.length(), _md_parser, this);
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


int MDTextLabel::_handle_md_block(MD_BLOCKTYPE block_type, void* detail, MD2BBFormat* md_data, bool exiting) {
    switch (block_type) {
		case MD_BLOCK_DOC:
			// No BBCode equivalent
			return MD_OK;
		case MD_BLOCK_QUOTE:
			// No BBCode equivalent
			break;
		case MD_BLOCK_UL:
			__POP_IF_EXIT
			{
				MD_BLOCK_UL_DETAIL* ul_detail = (MD_BLOCK_UL_DETAIL*)detail;
				push_list(0, ListType::LIST_DOTS, false, _mdchar_to_string(&ul_detail->mark, 1));
			}
			break;
		case MD_BLOCK_OL:
			__POP_IF_EXIT
			{
				MD_BLOCK_OL_DETAIL* ol_detail = (MD_BLOCK_OL_DETAIL*)detail;
				push_list(0, ListType::LIST_NUMBERS, false, _mdchar_to_string(&ol_detail->mark_delimiter, 1));
			}
			break;
		case MD_BLOCK_LI:
			// BBCode list items are just separated by newlines
			if (exiting) {
				add_text("\n");
			}
			break;
		case MD_BLOCK_HR:
			add_text("\n");
			break;
		case MD_BLOCK_H:
			return _handle_md_header(detail, md_data, exiting);
			break;
		case MD_BLOCK_CODE:
			// TODO: potentially differentiate fenced codeblocks from indented codeblocks?
			__POP_IF_EXIT
			push_mono();
			break;
		case MD_BLOCK_HTML:
			WARN_PRINT("[MDTextLabel] HTML rendering is not supported by Godot. HTML will be rendered in a code block instead.");
			push_mono();
			break;
		case MD_BLOCK_P:
			push_paragraph(HORIZONTAL_ALIGNMENT_LEFT);
			break;
		case MD_BLOCK_TABLE:
			// Formatting for table cells is set at table head and table body
			__POP_IF_EXIT
			{
				MD_BLOCK_TABLE_DETAIL* table_detail = (MD_BLOCK_TABLE_DETAIL*)detail;
				push_table(table_detail->col_count);
			}
			break;
		case MD_BLOCK_THEAD:
			__POP_IF_EXIT
			{
				Ref<MD2BBCellFormat> head_format = md_data->table_head_format;
				_set_md_cell_format(head_format);
			}
			break;
		case MD_BLOCK_TBODY:
			__POP_IF_EXIT
			{
				Ref<MD2BBCellFormat> body_format = md_data->table_body_format;
				_set_md_cell_format(body_format);
			}
			break;
		case MD_BLOCK_TR:
			// No BBCode equivalent
			break;
		case MD_BLOCK_TH:
			__POP_IF_EXIT
			push_cell();
			break;
		case MD_BLOCK_TD:
			__POP_IF_EXIT
			push_cell();
			break;
		default:
			ERR_PRINT("[MDTextLabel] Unrecognized markdown block type: " + block_type);
			return BAD_BLOCK;
	}

	return MD_OK;
}

int MDTextLabel::_handle_md_span(MD_SPANTYPE span_type, void* detail, MD2BBFormat* md_data, bool exiting) {
    switch (span_type) {
        case MD_SPAN_EM:
            __POP_IF_EXIT
			push_italics();
            break;
        case MD_SPAN_STRONG:
			__POP_IF_EXIT
			push_bold();
            break;
        case MD_SPAN_A:
            return _handle_md_link(detail, md_data, exiting);
            break;
        case MD_SPAN_IMG:
			// Image push and pop is done in one step - so don't pop on exit again
			if (exiting)
				break;
			{
				MD_SPAN_IMG_DETAIL* img_detail = (MD_SPAN_IMG_DETAIL*)detail;
				// Appending the image bbcode here as a string is easier than trying to re-write the image fetching code ourselves
				append_text("[img]" + _mdattr_to_string(img_detail->src) + "[/img]");
			}
            break;
        case MD_SPAN_CODE:
			__POP_IF_EXIT
            push_mono();
            break;
        case MD_SPAN_DEL:
        	__POP_IF_EXIT
			push_strikethrough();
            break;
        case MD_SPAN_LATEXMATH:
            ERR_PRINT("[MDTextLabel] LATEX rendering is not supported by Godot.");
            return BAD_SPAN;
            break;
        case MD_SPAN_LATEXMATH_DISPLAY:
            ERR_PRINT("[MDTextLabel] LATEX rendering is not supported by Godot.");
            return BAD_SPAN;
        case MD_SPAN_WIKILINK:
            return _handle_md_wikilink(detail, md_data, exiting);
            break;
        case MD_SPAN_U:
			__POP_IF_EXIT
			push_underline();
            break;
        default:
            ERR_PRINT("[MDTextLabel] Unrecognized markdown span type: " + span_type);
            return BAD_BLOCK;
	}
	return MD_OK;
}

int MDTextLabel::_handle_md_text(MD_TEXTTYPE text_type, const MD_CHAR* text, MD_SIZE size, MD2BBFormat* md_data) {
    switch (text_type) {
        case MD_TEXT_NORMAL:
            add_text(_mdchar_to_string(text, size));
            break;
        case MD_TEXT_CODE:
            add_text(_mdchar_to_string(text, size));
            break;
        case MD_TEXT_NULLCHAR:
            break;
        case MD_TEXT_BR:
            add_text("\n");
            break;
        case MD_TEXT_SOFTBR:
			add_text("\n");
            break;
        case MD_TEXT_ENTITY:
            add_text(_mdchar_to_string(text, size));
            break;
        case MD_TEXT_HTML:
            // Not supported
            break;
        case MD_TEXT_LATEXMATH:
            // Not supported
            break;
        default:
            ERR_PRINT("[MDTextLabel] Unrecognized markdown text type: " + text_type);
            break;
    }  
    
    return MD_OK;
}

/**
 * Handle a header block, using header format settings in MD2BBHeaderFormat
 */
int MDTextLabel::_handle_md_header(void* detail, MD2BBFormat* md_data, bool exiting) {
    unsigned level = ((MD_BLOCK_H_DETAIL*)detail)->level;
    Ref<MD2BBHeaderFormat> header_format;
    switch (level) {
        case 1:
            header_format = md_data->h1_format;
            break;
        case 2:
            header_format = md_data->h2_format;
            break;
        case 3:
            header_format = md_data->h3_format;
            break;
        case 4:
            header_format = md_data->h4_format;
            break;
        case 5:
            header_format = md_data->h5_format;
            break;
        case 6:
            header_format = md_data->h6_format;
            break;
        default:
            return BAD_HEADER_SIZE;
    }
    if (!exiting) {
        float size = header_format->font_size;
        push_font_size(size);
        if (header_format->bold) {
            push_bold();
        }
        if (header_format->italic) {
            push_italics();
        }
        if (header_format->underlined) {
            push_underline();
        }
        if (header_format->has_color) {
            push_color(header_format->font_color);
        }
    } else {
        if (header_format->has_color)
            pop();
		if (header_format->underlined)
			pop();
		if (header_format->italic)
			pop();
		if (header_format->bold)
			pop();
		// font size tag
		pop();
        add_text("\n");
    }
    return MD_OK;
}

/**
 * Handles a spans - and whether to include text label or not
 */
int MDTextLabel::_handle_md_link(void* detail, MD2BBFormat* md_data, bool exiting) {
    /*MD_SPAN_A_DETAIL* link_detail = (MD_SPAN_A_DETAIL*)detail;
    if (!exiting) {
        if (link_detail->title.size > 0) {
            md_data->output += "[hint=" + _mdattr_to_string(link_detail->title) + "]";
        }
        md_data->output += "[url=" + _mdattr_to_string(link_detail->href) + "]" + _mdattr_to_string(link_detail->title) + "[/url]";
    } else {
        md_data->output += "[/url]";
        if (link_detail->title.size > 0) {
            md_data->output += "[/hint]";
        }
    }*/
    return MD_OK;
}

int MDTextLabel::_handle_md_wikilink(void* detail, MD2BBFormat* md_data, bool exiting) {
    ERR_PRINT("[MDTextLabel] Wikilinks are not yet supported. Use normal links with filepaths instead.");
    return BAD_SPAN;
}

/**
 * Apply a table cell format, affecting all consequent cells
 */
void MDTextLabel::_set_md_cell_format(Ref<MD2BBCellFormat> format) {
	set_cell_border_color(format->border_color);
	set_cell_padding(format->padding);
	set_cell_row_background_color(format->row_bg_odd, format->row_bg_even);
	if (format->size_override) {
		set_cell_size_override(format->min_size_override, format->max_size_override);
	} else {
		set_cell_size_override(Vector2(), Vector2());
	}
}


/**
 * Convert a md4c char array to a godot String.
 * md4c does not guarantee to provide null-terminated strings, so needs extra work to convert
 */
String MDTextLabel::_mdchar_to_string(const MD_CHAR* text, MD_SIZE size) {
    // Double check char array isnt already null-terminated
    if (text[size - 1] == '\0')
        return String(text);
    // Create new char array for null termination
    // Avoid realloc as md4c handles freeing it's own char pointers
    MD_CHAR* null_terminated_text = (MD_CHAR*)memalloc(size + 1);
    strncpy(null_terminated_text, text, size);
    null_terminated_text[size] = '\0';
    String str(null_terminated_text);
    memfree(null_terminated_text);
    return str;
}

/**
 * Shortcut function for MD_ATTRIBUTE
 */
String MDTextLabel::_mdattr_to_string(MD_ATTRIBUTE attr) {
    return _mdchar_to_string(attr.text, attr.size);
}



// =============== RESOURCE DEFINITIONS ====================

void MD2BBFormat::_bind_methods() {
    
    ClassDB::bind_method(D_METHOD("get_h1_format"), &MD2BBFormat::get_h1_format);
    ClassDB::bind_method(D_METHOD("set_h1_format", "value"), &MD2BBFormat::set_h1_format);
    ClassDB::bind_method(D_METHOD("get_h2_format"), &MD2BBFormat::get_h2_format);
    ClassDB::bind_method(D_METHOD("set_h2_format", "value"), &MD2BBFormat::set_h2_format);
    ClassDB::bind_method(D_METHOD("get_h3_format"), &MD2BBFormat::get_h3_format);
    ClassDB::bind_method(D_METHOD("set_h3_format", "value"), &MD2BBFormat::set_h3_format);
    ClassDB::bind_method(D_METHOD("get_h4_format"), &MD2BBFormat::get_h4_format);
    ClassDB::bind_method(D_METHOD("set_h4_format", "value"), &MD2BBFormat::set_h4_format);
    ClassDB::bind_method(D_METHOD("get_h5_format"), &MD2BBFormat::get_h5_format);
    ClassDB::bind_method(D_METHOD("set_h5_format", "value"), &MD2BBFormat::set_h5_format);
    ClassDB::bind_method(D_METHOD("get_h6_format"), &MD2BBFormat::get_h6_format);
    ClassDB::bind_method(D_METHOD("set_h6_format", "value"), &MD2BBFormat::set_h6_format);
    ClassDB::bind_method(D_METHOD("get_table_head_format"), &MD2BBFormat::get_table_head_format);
    ClassDB::bind_method(D_METHOD("set_table_head_format", "value"), &MD2BBFormat::set_table_head_format);
    ClassDB::bind_method(D_METHOD("get_table_body_format"), &MD2BBFormat::get_table_body_format);
    ClassDB::bind_method(D_METHOD("set_table_body_format", "value"), &MD2BBFormat::set_table_body_format);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "h1_format", PROPERTY_HINT_RESOURCE_TYPE, "MD2BBHeaderFormat"), "set_h1_format", "get_h1_format");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "h2_format", PROPERTY_HINT_RESOURCE_TYPE, "MD2BBHeaderFormat"), "set_h2_format", "get_h2_format");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "h3_format", PROPERTY_HINT_RESOURCE_TYPE, "MD2BBHeaderFormat"), "set_h3_format", "get_h3_format");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "h4_format", PROPERTY_HINT_RESOURCE_TYPE, "MD2BBHeaderFormat"), "set_h4_format", "get_h4_format");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "h5_format", PROPERTY_HINT_RESOURCE_TYPE, "MD2BBHeaderFormat"), "set_h5_format", "get_h5_format");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "h6_format", PROPERTY_HINT_RESOURCE_TYPE, "MD2BBHeaderFormat"), "set_h6_format", "get_h6_format");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "table_head_format", PROPERTY_HINT_RESOURCE_TYPE, "MD2BBCellFormat"), "set_table_head_format", "get_table_head_format");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "table_body_format", PROPERTY_HINT_RESOURCE_TYPE, "MD2BBCellFormat"), "set_table_body_format", "get_table_body_format");

}

void MD2BBHeaderFormat::_bind_methods() {

    ClassDB::bind_method(D_METHOD("get_font_size"), &MD2BBHeaderFormat::get_font_size);
	ClassDB::bind_method(D_METHOD("set_font_size", "value"), &MD2BBHeaderFormat::set_font_size);
    ClassDB::bind_method(D_METHOD("get_bold"), &MD2BBHeaderFormat::get_bold);
	ClassDB::bind_method(D_METHOD("set_bold", "value"), &MD2BBHeaderFormat::set_bold);
    ClassDB::bind_method(D_METHOD("get_italic"), &MD2BBHeaderFormat::get_italic);
	ClassDB::bind_method(D_METHOD("set_italic", "value"), &MD2BBHeaderFormat::set_italic);
    ClassDB::bind_method(D_METHOD("get_underlined"), &MD2BBHeaderFormat::get_underlined);
	ClassDB::bind_method(D_METHOD("set_underlined", "value"), &MD2BBHeaderFormat::set_underlined);
    ClassDB::bind_method(D_METHOD("get_has_color"), &MD2BBHeaderFormat::get_has_color);
	ClassDB::bind_method(D_METHOD("set_has_color", "value"), &MD2BBHeaderFormat::set_has_color);
    ClassDB::bind_method(D_METHOD("get_font_color"), &MD2BBHeaderFormat::get_font_color);
	ClassDB::bind_method(D_METHOD("set_font_color", "value"), &MD2BBHeaderFormat::set_font_color);
	
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "font_size", PROPERTY_HINT_RANGE, "1,100"), "set_font_size", "get_font_size");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "bold"), "set_bold", "get_bold");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "italic"), "set_italic", "get_italic");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "underlined"), "set_underlined", "get_underlined");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "has_color"), "set_has_color", "get_has_color");
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "font_color"), "set_font_color", "get_font_color");

}
