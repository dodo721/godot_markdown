#include "md2bb.h"

#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

MD2BBConverter::MD2BBConverter() {

    _md_parser = new MD_PARSER();
    // Need to set to 0
    // Not sure why docs just say so ¯\_(ツ)_/¯
    _md_parser->abi_version = 0;
    // See md4c.h:306
    _md_parser->flags = MD_FLAG_TABLES | MD_FLAG_STRIKETHROUGH | MD_FLAG_WIKILINKS | MD_FLAG_UNDERLINE | MD_FLAG_NOHTMLBLOCKS | MD_FLAG_NOHTMLSPANS;

    // Block callbacks
    _md_parser->enter_block = [](MD_BLOCKTYPE block_type, void* detail, void* user_data) {
        return MD2BBConverter::_handle_block(block_type, detail, (BBConstructionData*)user_data, false);
    };
    _md_parser->leave_block = [](MD_BLOCKTYPE block_type, void* detail, void* user_data) {
        return MD2BBConverter::_handle_block(block_type, detail, (BBConstructionData*)user_data, true);
    };
    // Span callbacks
    _md_parser->enter_span = [](MD_SPANTYPE span_type, void* detail, void* user_data) {
        return MD2BBConverter::_handle_span(span_type, detail, (BBConstructionData*)user_data, false);
    };
    _md_parser->leave_span = [](MD_SPANTYPE span_type, void* detail, void* user_data) {
        return MD2BBConverter::_handle_span(span_type, detail, (BBConstructionData*)user_data, true);
    };
    // Text callback
    _md_parser->text = [](MD_TEXTTYPE text_type, const MD_CHAR* text, MD_SIZE size, void* user_data) {
        return MD2BBConverter::_handle_text(text_type, text, size, (BBConstructionData*)user_data);
    };

}

MD2BBConverter::~MD2BBConverter() {
    delete _md_parser;
}

int MD2BBConverter::_handle_block(MD_BLOCKTYPE block_type, void* detail, BBConstructionData* md_data, bool exiting) {
    String tag_name;
    switch (block_type) {
        case MD_BLOCK_DOC:
            break;
        case MD_BLOCK_QUOTE:
            tag_name = "quote";
            break;
        case MD_BLOCK_UL:
            tag_name = "ul";
            break;
        case MD_BLOCK_OL:
            tag_name = "ol";
            break;
        case MD_BLOCK_LI:
            tag_name = "li";
            break;
        case MD_BLOCK_HR:
            tag_name = "hr";
            break;
        case MD_BLOCK_H:
            return _handle_header(detail, md_data, exiting);
            break;
        case MD_BLOCK_CODE:
        // TODO: potentially differentiate fenced codeblocks from indented codeblocks?
            tag_name = "code";
            break;
        case MD_BLOCK_HTML:
            WARN_PRINT("[MD2BBConverter] HTML rendering is not supported by Godot. HTML will be rendered in a code block instead.");
            tag_name = "code";
            break;
        case MD_BLOCK_P:
            tag_name = "p";
            break;
        case MD_BLOCK_TABLE:
            tag_name = "table";
            break;
        case MD_BLOCK_THEAD:
            // No BBCode equivalent
            return MD_OK;
            break;
        case MD_BLOCK_TBODY:
            // No BBCode equivalent
            return MD_OK;
            break;
        case MD_BLOCK_TR:
            tag_name = "tr";
            break;
        case MD_BLOCK_TH:
            tag_name = "th";
            break;
        case MD_BLOCK_TD:
            tag_name = "td";
            break;
        default:
            ERR_PRINT("[MD2BBConverter] Unrecognized markdown block type: " + block_type);
            return BAD_BLOCK;
	}
    if (!exiting) {
        md_data->output += "[" + tag_name + "]";
    } else {
        md_data->output += "[/" + tag_name + "]";
    }
	return MD_OK;
}

int MD2BBConverter::_handle_span(MD_SPANTYPE span_type, void* detail, BBConstructionData* md_data, bool exiting) {
    String tag_name;
    switch (span_type) {
        case MD_SPAN_EM:
            tag_name = "i";
            break;
        case MD_SPAN_STRONG:
            tag_name = "b";
            break;
        case MD_SPAN_A:
            return _handle_link(detail, md_data, exiting);
            break;
        case MD_SPAN_IMG:
            tag_name = "img";
            break;
        case MD_SPAN_CODE:
            tag_name = "code";
            break;
        case MD_SPAN_DEL:
            tag_name = "s";
            break;
        case MD_SPAN_LATEXMATH:
            ERR_PRINT("[MD2BBConverter] LATEX rendering is not supported by Godot.");
            return BAD_SPAN;
            break;
        case MD_SPAN_LATEXMATH_DISPLAY:
            ERR_PRINT("[MD2BBConverter] LATEX rendering is not supported by Godot.");
            return BAD_SPAN;
        case MD_SPAN_WIKILINK:
            return _handle_wikilink(detail, md_data, exiting);
            break;
        case MD_SPAN_U:
            tag_name = "u";
            break;
        default:
            ERR_PRINT("[MD2BBConverter] Unrecognized markdown span type: " + span_type);
            return BAD_BLOCK;
	}
    if (!exiting) {
        md_data->output += "[" + tag_name + "]";
    } else {
        md_data->output += "[/" + tag_name + "]";
    }
	return MD_OK;
}

int MD2BBConverter::_handle_text(MD_TEXTTYPE text_type, const MD_CHAR* text, MD_SIZE size, BBConstructionData* md_data) {
    switch (text_type) {
        case MD_TEXT_NORMAL:
            md_data->output += _mdchar_to_string(text, size);
            break;
        case MD_TEXT_CODE:
            md_data->output += _mdchar_to_string(text, size);
            break;
        case MD_TEXT_NULLCHAR:
            break;
        case MD_TEXT_BR:
            md_data->output += "\n";
            break;
        case MD_TEXT_SOFTBR:
            md_data->output += "\n";
            break;
        case MD_TEXT_ENTITY:
            md_data->output += _mdchar_to_string(text, size);
            break;
        case MD_TEXT_HTML:
            // Not supported
            break;
        case MD_TEXT_LATEXMATH:
            // Not supported
            break;
        default:
            break;
    }  
    
    return MD_OK;
}

/**
 * Handle a header block, using header format settings in MD2BBHeaderFormat
 */
int MD2BBConverter::_handle_header(void* detail, BBConstructionData* md_data, bool exiting) {
    unsigned level = ((MD_BLOCK_H_DETAIL*)detail)->level;
    MD2BBHeaderFormat header_format;
    switch (level) {
        case 1:
            header_format = md_data->format->h1_format;
        case 2:
            header_format = md_data->format->h2_format;
        case 3:
            header_format = md_data->format->h3_format;
        case 4:
            header_format = md_data->format->h4_format;
        case 5:
            header_format = md_data->format->h5_format;
        case 6:
            header_format = md_data->format->h6_format;
        default:
            return BAD_HEADER_SIZE;
    }
    if (!exiting) {
        float size = header_format.font_size;
        md_data->output += "[size=";
        md_data->output += size;
        md_data->output += "]";
        if (header_format.bold)
            md_data->output += "[b]";
        if (header_format.italic)
            md_data->output += "[i]";
        if (header_format.underlined)
            md_data->output += "[u]";
        if (header_format.has_color) {
            md_data->output += "[color=#" + header_format.font_color.to_html(false) + "]";
        }
    } else {
        md_data->output += "[/size]";
        if (header_format.bold)
            md_data->output += "[/b]";
        if (header_format.italic)
            md_data->output += "[/i]";
        if (header_format.underlined)
            md_data->output += "[/u]";
        if (header_format.has_color)
            md_data->output += "[/color]";
        md_data->output += "\n";
    }
    return MD_OK;
}

/**
 * Handles a spans - and whether to include text label or not
 */
int MD2BBConverter::_handle_link(void* detail, BBConstructionData* md_data, bool exiting) {
    MD_SPAN_A_DETAIL* link_detail = (MD_SPAN_A_DETAIL*)detail;
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
    }
    return MD_OK;
}

int MD2BBConverter::_handle_wikilink(void* detail, BBConstructionData* md_data, bool exiting) {
    ERR_PRINT("[MD2BBConverter] Wikilinks are not yet supported. Use normal links with filepaths instead.");
    return BAD_SPAN;
}


/**
 * Convert a md4c char array to a godot String.
 * md4c does not guarantee to provide null-terminated strings, so needs extra work to convert
 */
String MD2BBConverter::_mdchar_to_string(const MD_CHAR* text, MD_SIZE size) {
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
String MD2BBConverter::_mdattr_to_string(MD_ATTRIBUTE attr) {
    return _mdchar_to_string(attr.text, attr.size);
}

/**
 * Convert provided markdown-formatted text to bbcode-formatted text
 */
String MD2BBConverter::convert(String md_text, int& error) {
    BBConstructionData md_data;
    error = md_parse(md_text.utf8().get_data(), md_text.length(), _md_parser, &md_data);
    String bbcode = md_data.output;
    return bbcode;
}


void MD2BBFormat::_bind_methods() {
    
}