#ifndef GDEXAMPLE_H
#define GDEXAMPLE_H

#include "md2bb.h"

#include <godot_cpp/classes/rich_text_label.hpp>

namespace godot {

class MDTextLabel : public RichTextLabel {
	GDCLASS(MDTextLabel, RichTextLabel)

public:
	String markdown;
	Ref<MD2BBFormat> format;

protected:
	static void _bind_methods();

public:
	void set_markdown(String p_text);
	void append_markdown(String p_text);
	String get_markdown() const;

	void set_format(const Ref<MD2BBFormat> format);
	Ref<MD2BBFormat> get_format() const;
	
	static String md2bb(String p_text, Ref<MD2BBFormat> format);

	MDTextLabel();
	~MDTextLabel();
};

}

#endif
