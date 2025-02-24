#ifndef MD2BB_H
#define MD2BB_H

#include "md4c.h"

#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/classes/resource.hpp>

namespace godot {

enum MD2BBError {
    MD_OK=0, BAD_BLOCK=1, BAD_SPAN=2, BAD_HEADER_SIZE=3
};

struct MD2BBHeaderFormat {
    float font_size;
    bool bold;
    bool italic;
    bool underlined;
    bool has_color;
    Color font_color;
};

/**
 * Options for how specific markdown formats are represented in bbcode
 */
class MD2BBFormat : public Resource {
    GDCLASS(MD2BBFormat, Resource);

public:
    MD2BBHeaderFormat h1_format = {
        2.285f, false, false, false, false
    };
    MD2BBHeaderFormat h2_format = {
        1.714f, false, false, false, false
    };
    MD2BBHeaderFormat h3_format = {
        1.428f, false, false, false, false
    };
    MD2BBHeaderFormat h4_format = {
        1.142f, false, false, false, false
    };
    MD2BBHeaderFormat h5_format = {
        1.0f, false, false, false, false
    };
    MD2BBHeaderFormat h6_format = {
        0.857f, false, false, false, false
    };

protected:
    static void _bind_methods();

};

/**
 * Persistent data during bbcode construction
 */
struct BBConstructionData {
    Ref<MD2BBFormat> format;
    String output;
};

/**
 * The powerhouse for markdown -> bbcode conversion!
 * Uses md4c parsing engine to build bbcode string.
 * Settings for how resulting bbcode should be formatted can be customized by setting 'format'.
 */
class MD2BBConverter : public RefCounted {
    GDCLASS(MD2BBConverter, RefCounted);

private:
	MD_PARSER* _md_parser;
    
    // Callbacks for md_parse()
    // Static so callback closures can access
    static int _handle_block(MD_BLOCKTYPE block_type, void* detail, BBConstructionData* md_data, bool exiting);
    static int _handle_span(MD_SPANTYPE span_type, void* detail, BBConstructionData* md_data, bool exiting);
    static int _handle_text(MD_TEXTTYPE text_type, const MD_CHAR* text, MD_SIZE size, BBConstructionData* md_data);

    // Tags with more complex behaviour than just tag replacement
    static int _handle_header(void* detail, BBConstructionData* md_data, bool exiting);
    static int _handle_link(void* detail, BBConstructionData* md_data, bool exiting);
    static int _handle_wikilink(void* detail, BBConstructionData* md_data, bool exiting);

    // Utility functions
    static String _mdchar_to_string (const MD_CHAR* text, MD_SIZE size);
    static String _mdattr_to_string (MD_ATTRIBUTE attr);

public:
    String convert(String md_text, int& error);

    MD2BBConverter();
    ~MD2BBConverter();

protected:
    static void _bind_methods() {}

};

}

#endif